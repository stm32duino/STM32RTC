/**
  ******************************************************************************
  * @file    rtc.c
  * @author  Frederic Pillon
  * @brief   Provides a RTC driver
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "rtc.h"
#include "stm32yyxx_ll_rcc.h"
#include "stm32yyxx_ll_rtc.h"
#include <string.h>

#if !defined(HAL_RTC_MODULE_ONLY) && \
    (defined(HAL_RTC_MODULE_ENABLED) || (defined(USE_HAL_RTC_MODULE) && (USE_HAL_RTC_MODULE == 1)))
#if defined(STM32MP1xx)
  /**
  * Currently there is no RTC driver for STM32MP1xx. If RTC is used in the future
  * the function call HAL_RCCEx_PeriphCLKConfig() shall be done under
  * if(IS_ENGINEERING_BOOT_MODE()), since clock source selection is done by
  * First Stage Boot Loader on Cortex-A.
  */
  #error "RTC shall not be handled by Arduino in STM32MP1xx."
#endif /* STM32MP1xx */

#ifdef __cplusplus
extern "C" {
#endif

/* Private define ------------------------------------------------------------*/
#if defined(USE_HALV2_DRIVER)
#define IS_RTC_ASYNCH_PREDIV(PREDIV)   ((PREDIV) <= 0x7FU)
/* ToDo: check if mode is needed */
/**
  * Test synchronous prescaler value if mode is HAL_RTC_MODE_BCD.
  */
/* #define IS_RTC_SYNCH_PREDIV(prediv, mode) (((mode) != HAL_RTC_MODE_BCD) || ((prediv) <= 0x7FFFU))*/
#define IS_RTC_SYNCH_PREDIV(PREDIV)    ((PREDIV) <= 0x7FFFU)
#endif
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if defined(USE_HALV2_DRIVER)
static hal_rtc_config_t rtc_config = {
  .mode = HAL_RTC_MODE_BCD,
  .asynch_prediv = 0x7F,
  .synch_prediv = 0x00FF,
  .bcd_update = HAL_RTC_BCD_UPDATE_8BITS
};
hal_rtc_calendar_config_t calendar_config = {
  .hour_format = HAL_RTC_CALENDAR_HOUR_FORMAT_24,
  .bypass_shadow_register = HAL_RTC_CALENDAR_SHADOW_REG_BYPASS
};
#else
static RTC_HandleTypeDef RtcHandle = {.Instance = RTC};
#endif
static voidCallbackPtr RTCUserCallback = NULL;
static void *callbackUserData = NULL;
#if defined(ALARM_B_AVAILABLE)
static voidCallbackPtr RTCUserCallbackB = NULL;
static void *callbackUserDataB = NULL;
#endif
#ifdef ONESECOND_IRQn
static voidCallbackPtr RTCSecondsIrqCallback = NULL;
#endif
#ifdef STM32WLxx
static voidCallbackPtr RTCSubSecondsUnderflowIrqCallback = NULL;
#endif
static sourceClock_t clkSrc = LSI_CLOCK;
static uint32_t clkVal = LSI_VALUE;
#if !defined(LL_RCC_LSCO_CLKSOURCE_HSI64M_DIV2048)
static uint8_t HSEDiv = 0;
#endif
#if !defined(STM32F1xx)
/* predividers values */
static uint8_t predivSync_bits = 0xFF;
static uint32_t predivAsync = (PREDIVA_MAX + 1);
static uint32_t predivSync = (PREDIVS_MAX + 1);
static uint32_t fqce_apre;
#else
/* Default, let HAL calculate the prescaler*/
static uint32_t predivAsync = RTC_AUTO_1_SECOND;
#endif /* !STM32F1xx */

static hourFormat_t initFormat = HOUR_FORMAT_12;
static binaryMode_t initMode = MODE_BINARY_NONE;

/* Private function prototypes -----------------------------------------------*/
static void RTC_initClock(sourceClock_t source);
#if !defined(STM32F1xx)
static void RTC_computePrediv(uint32_t *asynch, uint32_t *synch);
#endif /* !STM32F1xx */
#if defined(LL_RTC_BINARY_NONE)
static void RTC_BinaryConf(binaryMode_t mode);
static void RTC_SetBinaryConf(void);
#endif

static inline int _log2(int x)
{
  return (x > 0) ? (sizeof(int) * 8 - __builtin_clz(x) - 1) : 0;
}

/* Exported functions --------------------------------------------------------*/
#if !defined(USE_HALV2_DRIVER)
/**
  * @brief Get pointer to RTC_HandleTypeDef
  * @param None
  * @retval pointer to RTC_HandleTypeDef
  */
RTC_HandleTypeDef *RTC_GetHandle(void)
{
  return &RtcHandle;
}
#endif /* !USE_HALV2_DRIVER */

/**
  * @brief Set RTC clock source
  * @param source: RTC clock source: LSE, LSI or HSE
  * @retval None
  */
void RTC_SetClockSource(sourceClock_t source)
{
  clkSrc = source;
  if (source == LSE_CLOCK) {
    clkVal = LSE_VALUE;
#if defined(LL_RCC_LSCO_CLKSOURCE_HSI64M_DIV2048)
  } else if (source == HSI_CLOCK) {
    /* HSI division factor for RTC clock must be define to ensure that
     * the clock supplied to the RTC is less than or equal to 1 MHz
     */
    clkVal = 32000; /* HSI64M divided by 64 --> 1 MHz */
#else
  } else if (source == HSE_CLOCK) {
    /* HSE division factor for RTC clock must be define to ensure that
     * the clock supplied to the RTC is less than or equal to 1 MHz
     */
#if defined(STM32F1xx)
    /* HSE max is 16 MHZ divided by 128 --> 125 KHz */
    HSEDiv = 128;
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV32) && !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    HSEDiv = 32;
#elif !defined(RCC_RTCCLKSOURCE_HSE_DIV31) && !defined(LL_RCC_RTC_CLKSOURCE_HSE_DIV)
    if ((HSE_VALUE / 2) <= HSE_RTC_MAX) {
      HSEDiv = 2;
    } else if ((HSE_VALUE / 4) <= HSE_RTC_MAX) {
      HSEDiv = 4;
    } else if ((HSE_VALUE / 8) <= HSE_RTC_MAX) {
      HSEDiv = 8;
    } else if ((HSE_VALUE / 16) <= HSE_RTC_MAX) {
      HSEDiv = 16;
    }
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV31) || defined(LL_RCC_RTC_CLKSOURCE_HSE_DIV)
    /* Not defined for STM32F2xx */
#ifndef RCC_RTCCLKSOURCE_HSE_DIVX
#define RCC_RTCCLKSOURCE_HSE_DIVX 0x00000300U
#endif /* RCC_RTCCLKSOURCE_HSE_DIVX */
#if defined(RCC_RTCCLKSOURCE_HSE_DIV63)
#define HSEDIV_MAX 64
#else
#define HSEDIV_MAX 32
#endif
    for (HSEDiv = 2; HSEDiv < HSEDIV_MAX; HSEDiv++) {
      if ((HSE_VALUE / HSEDiv) <= HSE_RTC_MAX) {
        break;
      }
    }
#else
#error "Could not define HSE div"
#endif /* STM32F1xx */
    if ((HSE_VALUE / HSEDiv) > HSE_RTC_MAX) {
      Error_Handler();
    }
    clkVal = HSE_VALUE / HSEDiv;
#endif
  } else if (source == LSI_CLOCK) {
    clkVal = LSI_VALUE;
  } else {
    Error_Handler();
  }
}

/**
  * @brief RTC clock initialization
  *        This function configures the hardware resources used.
  * @param source: RTC clock source: LSE, LSI or HSE
  * @note  Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to select
  *        the RTC clock source; in this case the Backup domain will be reset in
  *        order to modify the RTC Clock source, as consequence RTC registers (including
  *        the backup registers) and RCC_CSR register are set to their reset values.
  * @retval None
  */
static void RTC_initClock(sourceClock_t source)
{
#if defined(USE_HALV2_DRIVER)
  hal_status_t status = HAL_ERROR;
  RTC_SetClockSource(source);
  if (source == LSE_CLOCK) {
    enableClock(LSE_CLOCK);
    status = HAL_RCC_RTC_SetKernelClkSource(HAL_RCC_RTC_CLK_SRC_LSE);
  } else if (source == LSI_CLOCK) {
    enableClock(LSI_CLOCK);
    status = HAL_RCC_RTC_SetKernelClkSource(HAL_RCC_RTC_CLK_SRC_LSI);
  } else if (source == HSE_CLOCK) {
    enableClock(HSE_CLOCK);
    status = HAL_RCC_RTC_SetKernelClkSource(HAL_RCC_RTC_CLK_SRC_HSE_DIV);
    HAL_RCC_RTC_SetHSEPrescaler(HSEDiv);
  }
  if (status != HAL_OK) {
    Error_Handler();
  }
  /* Enable the RTC peripheral */
  LL_RCC_EnableRTC();
#else
  RCC_PeriphCLKInitTypeDef PeriphClkInit;
  RTC_SetClockSource(source);
  if (source == LSE_CLOCK) {
    /* Enable the clock if not already set by user */
    enableClock(LSE_CLOCK);
#if defined(RCC_PERIPHCLK_RTC_WDG_BLEWKUP)
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_BLEWKUP;
    PeriphClkInit.RTCWDGBLEWKUPClockSelection = RCC_RTC_WDG_BLEWKUP_CLKSOURCE_LSE;
  } else if (source == HSI_CLOCK) {
    /* Enable the clock if not already set by user */
    enableClock(HSI_CLOCK);
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_BLEWKUP;
    PeriphClkInit.RTCWDGBLEWKUPClockSelection = RCC_RTC_WDG_BLEWKUP_CLKSOURCE_HSI64M_DIV2048;
#elif defined(RCC_PERIPHCLK_RTC_WDG_SUBG_LPAWUR_LCD_LCSC)
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_SUBG_LPAWUR_LCD_LCSC;
    PeriphClkInit.RTCWDGSUBGLPAWURLCDLCSCClockSelection = RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_LSE;
  } else if (source == HSI_CLOCK) {
    /* Enable the clock if not already set by user */
    enableClock(HSI_CLOCK);
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_SUBG_LPAWUR_LCD_LCSC;
    PeriphClkInit.RTCWDGSUBGLPAWURLCDLCSCClockSelection = RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_DIV512;
#else
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  } else if (source == HSE_CLOCK) {
    /* Enable the clock if not already set by user */
    enableClock(HSE_CLOCK);
    /* HSE division factor for RTC clock must be set to ensure that
     * the clock supplied to the RTC is less than or equal to 1 MHz
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
#if defined(STM32F1xx)
    /* HSE max is 16 MHZ divided by 128 --> 125 KHz */
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV128;
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV32) && !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV32;
#elif !defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    if ((HSE_VALUE / 2) <= HSE_RTC_MAX) {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV2;
    } else if ((HSE_VALUE / 4) <= HSE_RTC_MAX) {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV4;
    } else if ((HSE_VALUE / 8) <= HSE_RTC_MAX) {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV8;
    } else if ((HSE_VALUE / 16) <= HSE_RTC_MAX) {
      PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV16;
    }
#elif defined(RCC_RTCCLKSOURCE_HSE_DIV31)
    /* Not defined for STM32F2xx */
#ifndef RCC_RTCCLKSOURCE_HSE_DIVX
#define RCC_RTCCLKSOURCE_HSE_DIVX 0x00000300U
#endif /* RCC_RTCCLKSOURCE_HSE_DIVX */
#if defined(RCC_RTCCLKSOURCE_HSE_DIV63)
#define HSESHIFT 12
#else
#define HSESHIFT 16
#endif
    PeriphClkInit.RTCClockSelection = (HSEDiv << HSESHIFT) | RCC_RTCCLKSOURCE_HSE_DIVX;
#else
#error "Could not define RTCClockSelection"
#endif /* STM32F1xx */
#endif /* RCC_PERIPHCLK_RTC_WDG_BLEWKUP */
  } else if (source == LSI_CLOCK) {
    /* Enable the clock if not already set by user */
    enableClock(LSI_CLOCK);
#if defined(RCC_PERIPHCLK_RTC_WDG_BLEWKUP)
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_BLEWKUP;
    PeriphClkInit.RTCWDGBLEWKUPClockSelection = RCC_RTC_WDG_BLEWKUP_CLKSOURCE_LSI;
#elif defined(RCC_PERIPHCLK_RTC_WDG_SUBG_LPAWUR_LCD_LCSC)
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_SUBG_LPAWUR_LCD_LCSC;
    PeriphClkInit.RTCWDGSUBGLPAWURLCDLCSCClockSelection = RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_LSI;
#else
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif /* RCC_PERIPHCLK_RTC_WDG_BLEWKUP */
  } else {
    /* Invalid clock source */
    Error_Handler();
  }
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
#if defined(__HAL_RCC_RTC_CLK_ENABLE)
  __HAL_RCC_RTC_CLK_ENABLE();
#endif
#endif /* USE_HALV2_DRIVER */
}

/**
  * @brief set user (a)synchronous prescaler values.
  * @param asynch: asynchronous prescaler value in range 0 - PREDIVA_MAX
  * @note   Reset value: RTC_AUTO_1_SECOND for STM32F1xx series, else (PREDIVA_MAX + 1)
  * @param synch: synchronous prescaler value in range 0 - PREDIVS_MAX
  * @note   Reset value: (PREDIVS_MAX + 1), not used for STM32F1xx series.
  * @retval None
  */
void RTC_setPrediv(uint32_t asynch, uint32_t synch)
{
#if defined(STM32F1xx)
  (void)synch;
  /* set the prescaler for a stm32F1 (value is hold by one param) */
  predivAsync = asynch;
  if (!IS_RTC_ASYNCH_PREDIV(predivAsync)) {
    predivAsync = RTC_AUTO_1_SECOND;
  }
  LL_RTC_SetAsynchPrescaler(RTC, predivAsync);
#else
  if ((asynch <= PREDIVA_MAX) && (synch <= PREDIVS_MAX)) {
    predivAsync = asynch;
    predivSync = synch;
  } else {
    RTC_computePrediv(&predivAsync, &predivSync);
  }
  predivSync_bits = (uint8_t)_log2(predivSync) + 1;
#endif /* STM32F1xx */
}


/**
  * @brief get user (a)synchronous prescaler values if set else computed ones
  *        for the current clock source.
  * @param asynch: pointer where return asynchronous prescaler value.
  * @param synch: pointer where return synchronous prescaler value,
  *         not used for STM32F1xx series.
  * @retval None
  */
void RTC_getPrediv(uint32_t *asynch, uint32_t *synch)
{
#if defined(STM32F1xx)
  (void)synch;
  /* get the prescaler for a stm32F1 (value is hold by one param) */
  predivAsync = LL_RTC_GetDivider(RTC);
  *asynch = predivAsync;
#else
  if ((!IS_RTC_SYNCH_PREDIV(predivSync)) || (!IS_RTC_ASYNCH_PREDIV(predivAsync))) {
#if defined(USE_HALV2_DRIVER)
    if (!LL_RTC_IsActiveFlag_INITS()) {
      RTC_computePrediv(&predivAsync, &predivSync);
    } else {
      predivAsync = LL_RTC_GetAsynchPrescaler();
      predivSync = LL_RTC_GetSynchPrescaler();
    }
#else
    if (!LL_RTC_IsActiveFlag_INITS(RTC)) {
      RTC_computePrediv(&predivAsync, &predivSync);
    } else {
      predivAsync = LL_RTC_GetAsynchPrescaler(RTC);
      predivSync = LL_RTC_GetSynchPrescaler(RTC);
    }
#endif
  }
  if ((asynch != NULL) && (synch != NULL)) {
    *asynch = predivAsync;
    *synch = predivSync;
  }
  predivSync_bits = (uint8_t)_log2(predivSync) + 1;
#endif /* STM32F1xx */
}

#if !defined(STM32F1xx)
/**
  * @brief Compute (a)synchronous prescaler
  *        RTC prescalers are compute to obtain the RTC clock to 1Hz. See AN4759.
  * @param asynch: pointer where return asynchronous prescaler value.
  * @param synch: pointer where return synchronous prescaler value.
  * @retval None
  */
static void RTC_computePrediv(uint32_t *asynch, uint32_t *synch)
{
  uint32_t predivS = PREDIVS_MAX + 1;
  *asynch = PREDIVA_MAX + 1;

  /* Get user predividers if manually configured */
  if ((asynch == NULL) || (synch == NULL)) {
    return;
  }

  /* Find (a)synchronous prescalers to obtain the 1Hz calendar clock */
  do {
    (*asynch)--;
    predivS = (clkVal / (*asynch + 1)) - 1;

    if (((predivS + 1) * (*asynch + 1)) == clkVal) {
      break;
    }
  } while (*asynch != 0);

  /*
   * Can't find a 1Hz, so give priority to RTC power consumption
   * by choosing the higher possible value for predivA
   */
  if ((!IS_RTC_SYNCH_PREDIV(predivS)) || (!IS_RTC_ASYNCH_PREDIV(*asynch))) {
    *asynch = PREDIVA_MAX;
    predivS = (clkVal / (*asynch + 1)) - 1;
  }

  if (!IS_RTC_SYNCH_PREDIV(predivS)) {
    Error_Handler();
  }
  *synch = predivS;

  fqce_apre = clkVal / (*asynch + 1);
}
#endif /* !STM32F1xx */

#if defined(LL_RTC_BINARY_NONE)
static void RTC_BinaryConf(binaryMode_t mode)
{
#if defined(USE_HALV2_DRIVER)
  rtc_config.mode = (mode == MODE_BINARY_MIX) ? HAL_RTC_MODE_MIX : ((mode == MODE_BINARY_ONLY) ? HAL_RTC_MODE_BINARY : HAL_RTC_MODE_BCD);
  if (rtc_config.mode  == HAL_RTC_MODE_MIX) {
    /* Configure the 1s BCD calendar increment */
    uint32_t inc = 1 / (1.0 / ((float)clkVal / (float)(predivAsync + 1.0)));
    if (inc <= 256) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_8BITS;
    } else if (inc < (256 << 1)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_9BITS;
    } else if (inc < (256 << 2)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_10BITS;
    } else if (inc < (256 << 3)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_11BITS;
    } else if (inc < (256 << 4)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_12BITS;
    } else if (inc < (256 << 5)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_13BITS;
    } else if (inc < (256 << 6)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_14BITS;
    } else if (inc < (256 << 7)) {
      rtc_config.bcd_update = HAL_RTC_BCD_UPDATE_15BITS;
    } else {
      Error_Handler();
    }
  }
#else
  RtcHandle.Init.BinMode = (mode == MODE_BINARY_MIX) ? RTC_BINARY_MIX : ((mode == MODE_BINARY_ONLY) ? RTC_BINARY_ONLY : RTC_BINARY_NONE);
  if (RtcHandle.Init.BinMode == RTC_BINARY_MIX) {
    /* Configure the 1s BCD calendar increment */

    uint32_t inc = 1 / (1.0 / ((float)clkVal / (float)(predivAsync + 1.0)));
    if (inc <= 256) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_0;
    } else if (inc < (256 << 1)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_1;
    } else if (inc < (256 << 2)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_2;
    } else if (inc < (256 << 3)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_3;
    } else if (inc < (256 << 4)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_4;
    } else if (inc < (256 << 5)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_5;
    } else if (inc < (256 << 6)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_6;
    } else if (inc < (256 << 7)) {
      RtcHandle.Init.BinMixBcdU = RTC_BINARY_MIX_BCDU_7;
    } else {
      Error_Handler();
    }
  }
#endif
}

/*
* Function to check if the RTC ICSR register must be updated with new bit
* field BIN and BCDU. To be called just after the RTC_BinaryConf
* Map the LL RTC bin mode to the corresponding RtcHandle.Init.BinMode values
* assuming the LL_RTC_BINARY_xxx is identical to RTC_BINARY_xxx (RTC_ICSR_BIN_xxx)
* Idem for the LL_RTC_BINARY_MIX_BCDU_n and RTC_BINARY_MIX_BCDU_n
*/
#if !defined(USE_HALV2_DRIVER)
#if (RTC_BINARY_MIX != LL_RTC_BINARY_MIX)
#error "RTC_BINARY_MIX and LL_RTC_BINARY_MIX do not match"
#endif
#if (RTC_BINARY_MIX_BCDU_7 != LL_RTC_BINARY_MIX_BCDU_7)
#error "RTC_BINARY_MIX_BCDU_n and LL_RTC_BINARY_MIX_BCDU_n do not match"
#endif
#endif /* !USE_HALV2_DRIVER */
static void RTC_SetBinaryConf(void)
{
#if defined(USE_HALV2_DRIVER)
  if (LL_RTC_GetBinaryMode() != rtc_config.mode) {
    LL_RTC_DisableWriteProtection();
    LL_RTC_EnableInitMode();

    LL_RTC_SetBinaryMode(rtc_config.mode);
    if (rtc_config.mode == HAL_RTC_MODE_MIX) {
      LL_RTC_SetBinMixBCDU(rtc_config.bcd_update);
    }
    LL_RTC_DisableInitMode();
    LL_RTC_EnableWriteProtection();
  }
#else
  if (LL_RTC_GetBinaryMode(RTC) != RtcHandle.Init.BinMode) {
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_EnableInitMode(RTC);

    LL_RTC_SetBinaryMode(RTC, RtcHandle.Init.BinMode);
    if (RtcHandle.Init.BinMode == RTC_BINARY_MIX) {
      LL_RTC_SetBinMixBCDU(RTC, RtcHandle.Init.BinMixBcdU);
    }
    LL_RTC_ExitInitMode(RTC);
    LL_RTC_EnableWriteProtection(RTC);
  }
#endif /* USE_HALV2_DRIVER */
}
#endif /* LL_RTC_BINARY_NONE */

/**
  * @brief RTC Initialization
  *        This function configures the RTC time and calendar. By default, the
  *        RTC is set to the 1st January 2001
  *        Note: year 2000 is invalid as it is the hardware reset value and doesn't raise INITS flag
  * @param format: enable the RTC in 12 or 24 hours mode
  * @param mode: enable the RTC in BCD or Mix or Binary mode
  * @param source: RTC clock source: LSE, LSI or HSE
  * @param reset: force RTC reset, even if previously configured
  * @retval True if RTC is reinitialized, else false
  */
bool RTC_init(hourFormat_t format, binaryMode_t mode, sourceClock_t source, bool reset)
{
  bool reinit = false;
  hourAM_PM_t period = HOUR_AM, alarmPeriod = HOUR_AM;
  uint32_t subSeconds = 0, alarmSubseconds = 0;
  uint8_t seconds = 0, minutes = 0, hours = 0, weekDay = 0, days = 0, month = 0, years = 0;
  uint8_t alarmMask = 0, alarmDay = 0, alarmHours = 0, alarmMinutes = 0, alarmSeconds = 0;
  bool isAlarmASet = false;
#if defined(ALARM_B_AVAILABLE)
  hourAM_PM_t alarmBPeriod = HOUR_AM;
  uint8_t alarmBMask = 0, alarmBDay = 0, alarmBHours = 0, alarmBMinutes = 0, alarmBSeconds = 0;
  uint32_t alarmBSubseconds = 0;
  bool isAlarmBSet = false;
#endif

  initFormat = format;
  initMode = mode;

#if defined(USE_HALV2_DRIVER)
  /* Ensure backup domain is enabled before we init the RTC so we can use the backup registers for date retention on stm32f1xx boards */
  enableBackupDomain();

  if (reset) {
    resetBackupDomain();
  }
#else
  /* Ensure all RtcHandle properly set */
  RtcHandle.Instance = RTC;
#if defined(STM32F1xx)
  RtcHandle.Init.AsynchPrediv = predivAsync;
  RtcHandle.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
#else
  RtcHandle.Init.HourFormat = (format == HOUR_FORMAT_12) ? RTC_HOURFORMAT_12 : RTC_HOURFORMAT_24;
  RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
  RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
#if defined(RTC_OUTPUT_TYPE_OPENDRAIN)
  RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
#endif
#if defined(RTC_OUTPUT_PULLUP_NONE)
  RtcHandle.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
#endif
#if defined(RTC_OUTPUT_REMAP_NONE)
  RtcHandle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
#endif /* RTC_OUTPUT_REMAP_NONE */
#endif /* STM32F1xx */

  /* Ensure backup domain is enabled before we init the RTC so we can use the backup registers for date retention on stm32f1xx boards */
  enableBackupDomain();

  if (reset) {
    resetBackupDomain();
  }

#ifdef __HAL_RCC_RTCAPB_CLK_ENABLE
  __HAL_RCC_RTCAPB_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_RTC_ENABLE
  __HAL_RCC_RTC_ENABLE();
#endif
#endif /* USE_HALV2_DRIVER */
  isAlarmASet = RTC_IsAlarmSet(ALARM_A);
#if defined(ALARM_B_AVAILABLE)
  isAlarmBSet = RTC_IsAlarmSet(ALARM_B);
#endif
#if defined(STM32F1xx)
  uint32_t BackupDate;
  BackupDate = getBackupRegister(RTC_BKP_DATE) << 16;
  BackupDate |= getBackupRegister(RTC_BKP_DATE + 1) & 0xFFFF;
  if ((BackupDate == 0) || reset) {
    // RTC needs initialization
    // Init RTC clock
    RTC_initClock(source);
#else
#if defined(USE_HALV2_DRIVER)
  if (!LL_RTC_IsActiveFlag_INITS() || reset) {
#else
  if (!LL_RTC_IsActiveFlag_INITS(RTC) || reset) {
#endif

    // RTC needs initialization
    // Init RTC clock
    RTC_initClock(source);
#if defined(USE_HALV2_DRIVER)
    RTC_getPrediv(&(rtc_config.asynch_prediv), &(rtc_config.synch_prediv));
#else
    RTC_getPrediv(&(RtcHandle.Init.AsynchPrediv), &(RtcHandle.Init.SynchPrediv));
#endif
#if defined(LL_RTC_BINARY_NONE)
    /*
     * If RTC BIN mode changed, calling the HAL_RTC_Init will
     * force the update of the BIN register in the RTC_ICSR
     */
    RTC_BinaryConf(mode);
#endif /* LL_RTC_BINARY_NONE */
#endif  // STM32F1xx

#if defined(USE_HALV2_DRIVER)
    rtc_config.mode = (initMode == MODE_BINARY_MIX) ? HAL_RTC_MODE_MIX : (initMode == MODE_BINARY_ONLY ? HAL_RTC_MODE_BINARY : HAL_RTC_MODE_BCD);
    calendar_config.hour_format = (format == HOUR_FORMAT_24) ? HAL_RTC_CALENDAR_HOUR_FORMAT_24 : HAL_RTC_CALENDAR_HOUR_FORMAT_AMPM;
    HAL_RTC_DisableWriteProtection();
    if ((HAL_RTC_EnterInitMode() != HAL_OK) || (HAL_RTC_SetConfig(&rtc_config) != HAL_OK) ||
        (HAL_RTC_CALENDAR_SetConfig(&calendar_config) != HAL_OK) ||
        (HAL_RTC_ExitInitMode() != HAL_OK) || (HAL_RTC_EnableWriteProtection() != HAL_OK)) {
      Error_Handler();
    }
#else
    HAL_RTC_Init(&RtcHandle);
#endif
    // Default: saturday 1st of January 2001
    // Note: year 2000 is invalid as it is the hardware reset value and doesn't raise INITS flag
    RTC_SetDate(1, 1, 1, 6);
    reinit = true;
  } else {
    // RTC is already initialized
#if defined(__HAL_RCC_GET_RTC_WDG_BLEWKUP_CLK_CONFIG)
    uint32_t oldRtcClockSource = __HAL_RCC_GET_RTC_WDG_BLEWKUP_CLK_CONFIG();
    oldRtcClockSource = ((oldRtcClockSource == RCC_RTC_WDG_BLEWKUP_CLKSOURCE_LSE) ? LSE_CLOCK :
                         (oldRtcClockSource == RCC_RTC_WDG_BLEWKUP_CLKSOURCE_LSI) ? LSI_CLOCK :
                         (oldRtcClockSource == RCC_RTC_WDG_BLEWKUP_CLKSOURCE_HSI64M_DIV2048) ? HSI_CLOCK :
                         // default case corresponding to no clock source
                         0xFFFFFFFF);
#elif defined(__HAL_RCC_GET_RTC_SUBG_LPAWUR_LCD_LCSC_CLK_CONFIG)
    uint32_t oldRtcClockSource = __HAL_RCC_GET_RTC_SUBG_LPAWUR_LCD_LCSC_CLK_CONFIG();
    oldRtcClockSource = ((oldRtcClockSource == RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_LSE) ? LSE_CLOCK :
                         (oldRtcClockSource == RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_LSI) ? LSI_CLOCK :
                         (oldRtcClockSource == RCC_RTC_WDG_SUBG_LPAWUR_LCD_LCSC_CLKSOURCE_DIV512) ? HSI_CLOCK :
                         // default case corresponding to no clock source
                         0xFFFFFFFF);
#else
#if defined(STM32U3xx)
    uint32_t oldRtcClockSource = LL_RCC_GetRTCClockSource(0);
#else
    uint32_t oldRtcClockSource = LL_RCC_GetRTCClockSource();
#endif
    oldRtcClockSource = ((oldRtcClockSource == LL_RCC_RTC_CLKSOURCE_LSE) ? LSE_CLOCK :
                         (oldRtcClockSource == LL_RCC_RTC_CLKSOURCE_LSI) ? LSI_CLOCK :
#if defined(LL_RCC_RTC_CLKSOURCE_HSE)
                         (oldRtcClockSource == LL_RCC_RTC_CLKSOURCE_HSE) ? HSE_CLOCK :
#elif defined(LL_RCC_RTC_CLKSOURCE_HSE_DIV32)
                         (oldRtcClockSource == LL_RCC_RTC_CLKSOURCE_HSE_DIV32) ? HSE_CLOCK :
#elif defined(LL_RCC_RTC_CLKSOURCE_HSE_DIV128)
                         (oldRtcClockSource == LL_RCC_RTC_CLKSOURCE_HSE_DIV128) ? HSE_CLOCK :
#endif
                         // default case corresponding to no clock source
                         0xFFFFFFFF);
#endif
#if defined(STM32F1xx)
    if ((RtcHandle.DateToUpdate.WeekDay == 0)
        && (RtcHandle.DateToUpdate.Month == 0)
        && (RtcHandle.DateToUpdate.Date == 0)
        && (RtcHandle.DateToUpdate.Year == 0)) {
      // After a reset for example, restore HAL handle date with values from BackupRegister date
      memcpy(&RtcHandle.DateToUpdate, &BackupDate, 4);
    }
#endif  // STM32F1xx
    if (source != oldRtcClockSource) {
      // RTC is already initialized, but RTC clock source is changed
      // In case of RTC source clock change, Backup Domain is reset by RTC_initClock()
      // Save current config before reinit
      RTC_GetDate(&years, &month, &days, &weekDay);
      RTC_GetTime(&hours, &minutes, &seconds, &subSeconds, &period);
      // As clock source changed, force update prediv with user or computef ones
#if defined(STM32F1xx)
      RTC_setPrediv(predivAsync, 0);
#else
      RTC_setPrediv(predivAsync, predivSync);
#endif
      if (isAlarmASet) {
        RTC_GetAlarm(ALARM_A, &alarmDay, &alarmHours, &alarmMinutes, &alarmSeconds, &alarmSubseconds, &alarmPeriod, &alarmMask);
      }
#if defined(ALARM_B_AVAILABLE)
      if (isAlarmBSet) {
        RTC_GetAlarm(ALARM_B, &alarmBDay, &alarmBHours, &alarmBMinutes, &alarmBSeconds, &alarmBSubseconds, &alarmBPeriod, &alarmBMask);
      }
#endif
      RTC_DeInit(false);
      // Init RTC clock
      RTC_initClock(source);
#if defined(STM32F1xx)
      RTC_getPrediv(&(RtcHandle.Init.AsynchPrediv), NULL);
#elif defined(USE_HALV2_DRIVER)
      RTC_getPrediv(&(rtc_config.asynch_prediv), &(rtc_config.synch_prediv));
#else
      RTC_getPrediv(&(RtcHandle.Init.AsynchPrediv), &(RtcHandle.Init.SynchPrediv));
#endif
#if defined(LL_RTC_BINARY_NONE)
      /*
       * If RTC BIN mode changed, calling the HAL_RTC_Init will
       * force the update of the BIN register in the RTC_ICSR
       */
      RTC_BinaryConf(mode);
#endif /* LL_RTC_BINARY_NONE */
#if defined(USE_HALV2_DRIVER)
      rtc_config.mode = (initMode == MODE_BINARY_MIX) ? HAL_RTC_MODE_MIX : (initMode == MODE_BINARY_ONLY ? HAL_RTC_MODE_BINARY : HAL_RTC_MODE_BCD);
      calendar_config.hour_format = (format == HOUR_FORMAT_24) ? HAL_RTC_CALENDAR_HOUR_FORMAT_24 : HAL_RTC_CALENDAR_HOUR_FORMAT_AMPM;

      HAL_RTC_DisableWriteProtection();
      if ((HAL_RTC_EnterInitMode() != HAL_OK) || (HAL_RTC_SetConfig(&rtc_config) != HAL_OK) ||
          (HAL_RTC_CALENDAR_SetConfig(&calendar_config) != HAL_OK) ||
          (HAL_RTC_ExitInitMode() != HAL_OK) || (HAL_RTC_EnableWriteProtection() != HAL_OK)) {
        Error_Handler();
      }
#else
      HAL_RTC_Init(&RtcHandle);
#endif /* USE_HALV2_DRIVER */
      // Restore config
      RTC_SetTime(hours, minutes, seconds, subSeconds, period);
      RTC_SetDate(years, month, days, weekDay);
      if (isAlarmASet) {
        RTC_StartAlarm(ALARM_A, alarmDay, alarmHours, alarmMinutes, alarmSeconds, alarmSubseconds, alarmPeriod, alarmMask);
      }
#if defined(ALARM_B_AVAILABLE)
      if (isAlarmBSet) {
        RTC_StartAlarm(ALARM_B, alarmBDay, alarmBHours, alarmBMinutes, alarmBSeconds, alarmBSubseconds, alarmBPeriod, alarmBMask);
      }
#endif
    } else {
      // RTC is already initialized, and RTC stays on the same clock source
      // Init RTC clock
      RTC_initClock(source);
      // This initialize variables: predivAsync, predivSync and predivSync_bits
#if defined(STM32F1xx)
      RTC_getPrediv(&(RtcHandle.Init.AsynchPrediv), NULL);
#elif defined(USE_HALV2_DRIVER)
      RTC_getPrediv(&(rtc_config.asynch_prediv), &(rtc_config.synch_prediv));
#else
      RTC_getPrediv(&(RtcHandle.Init.AsynchPrediv), &(RtcHandle.Init.SynchPrediv));
#endif
#if defined(LL_RTC_BINARY_NONE)
      RTC_BinaryConf(mode);
      /*
       * RTC is already initialized, but RTC BIN mode is changed :
       * force the update of the BIN register and BCDu in the RTC_ICSR
       * by the RTC_SetBinaryConf function
       */
      RTC_SetBinaryConf();

#endif /* LL_RTC_BINARY_NONE */
#if defined(STM32F1xx)
      memcpy(&RtcHandle.DateToUpdate, &BackupDate, 4);
      /* Update date automatically by calling HAL_RTC_GetDate */
      RTC_GetDate(&years, &month, &days, &weekDay);
      /* and fill the new RTC Date value */
      RTC_SetDate(RtcHandle.DateToUpdate.Year, RtcHandle.DateToUpdate.Month,
                  RtcHandle.DateToUpdate.Date, RtcHandle.DateToUpdate.WeekDay);
#endif // STM32F1xx
    }
  }

#if defined(RTC_CR_BYPSHAD)
  /* Enable Direct Read of the calendar registers (not through Shadow) */
#if defined(USE_HALV2_DRIVER)
  LL_RTC_EnableBypassShadowReg();
#else
  HAL_RTCEx_EnableBypassShadow(&RtcHandle);
#endif
#endif /* RTC_CR_BYPSHAD */

  /*
   * NOTE: freezing the RTC during stop mode (lowPower deepSleep)
   * could inhibit the alarm interrupt and prevent the system to wakeUp
   * from stop mode even if the RTC alarm flag is set.
   */
  return reinit;
}

/**
  * @brief RTC deinitialization. Stop the RTC.
  * @param reset_cb: reset user callback
  * @retval None
  */
void RTC_DeInit(bool reset_cb)
{
#if defined(USE_HALV2_DRIVER)
  resetBackupDomain();
  LL_RCC_DisableRTC();
  HAL_RCC_RTCAPB_DisableClock();
  HAL_CORTEX_NVIC_DisableIRQ(RTC_Alarm_IRQn);
#ifdef ONESECOND_IRQn
  HAL_CORTEX_NVIC_DisableIRQ(ONESECOND_IRQn);
#endif
#else
  HAL_RTC_DeInit(&RtcHandle);
  /* Peripheral clock disable */
#ifdef __HAL_RCC_RTC_DISABLE
  __HAL_RCC_RTC_DISABLE();
#endif
#ifdef __HAL_RCC_RTCAPB_CLK_DISABLE
  __HAL_RCC_RTCAPB_CLK_DISABLE();
#endif
  HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
#ifdef ONESECOND_IRQn
  HAL_NVIC_DisableIRQ(ONESECOND_IRQn);
#endif
#ifdef STM32WLxx
  HAL_NVIC_DisableIRQ(TAMP_STAMP_LSECSS_SSRU_IRQn);
#endif
#endif /* USE_HALV2_DRIVER */
  if (reset_cb) {
    RTCUserCallback = NULL;
    callbackUserData = NULL;
#if defined(ALARM_B_AVAILABLE)
    RTCUserCallbackB = NULL;
    callbackUserDataB = NULL;
#endif
#ifdef ONESECOND_IRQn
    RTCSecondsIrqCallback = NULL;
#endif
#ifdef STM32WLxx
    RTCSubSecondsUnderflowIrqCallback = NULL;
#endif
  }
}

/**
  * @brief Check if time is already set
  * @retval True if set else false
  */
bool RTC_IsConfigured(void)
{
#if defined(STM32F1xx)
  uint32_t BackupDate;
  BackupDate = getBackupRegister(RTC_BKP_DATE) << 16;
  BackupDate |= getBackupRegister(RTC_BKP_DATE + 1) & 0xFFFF;
  return (BackupDate != 0);
#else
#if defined(USE_HALV2_DRIVER)
  return LL_RTC_IsActiveFlag_INITS();
#else
  return LL_RTC_IsActiveFlag_INITS(RTC);
#endif
#endif
}

/**
  * @brief Set RTC time
  * @param hours: 0-12 or 0-23. Depends on the format used.
  * @param minutes: 0-59
  * @param seconds: 0-59
  * @param subSeconds: 0-999 (not used)
  * @param period: select HOUR_AM or HOUR_PM period in case RTC is set in 12 hours mode. Else ignored.
  * @retval None
  */
void RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, hourAM_PM_t period)
{
#if defined(USE_HALV2_DRIVER)
  hal_rtc_time_t rtc_time;
  (void)subSeconds; /* not used (read-only register) */
  /* Ignore time AM PM configuration if in 24 hours format */
  if (initFormat == HOUR_FORMAT_24) {
    period = HOUR_AM;
  }
  rtc_time.am_pm = (period == HOUR_PM) ? HAL_RTC_TIME_FORMAT_PM : HAL_RTC_TIME_FORMAT_AM_24H;
  rtc_time.hour = hours;
  rtc_time.min = minutes;
  rtc_time.sec = seconds;
  HAL_RTC_DisableWriteProtection();
  if (HAL_RTC_EnterInitMode() != HAL_OK || HAL_RTC_CALENDAR_SetTime(&rtc_time) != HAL_OK ||
      HAL_RTC_ExitInitMode() != HAL_OK || HAL_RTC_EnableWriteProtection() != HAL_OK) {
    Error_Handler();
  }
#else

  RTC_TimeTypeDef RTC_TimeStruct;
  (void)subSeconds; /* not used (read-only register) */
  /* Ignore time AM PM configuration if in 24 hours format */
  if (initFormat == HOUR_FORMAT_24) {
    period = HOUR_AM;
  }

  if ((((initFormat == HOUR_FORMAT_24) && IS_RTC_HOUR24(hours)) || IS_RTC_HOUR12(hours))
      && IS_RTC_MINUTES(minutes) && IS_RTC_SECONDS(seconds)) {
    RTC_TimeStruct.Hours = hours;
    RTC_TimeStruct.Minutes = minutes;
    RTC_TimeStruct.Seconds = seconds;
#if !defined(STM32F1xx)
    if (period == HOUR_PM) {
      RTC_TimeStruct.TimeFormat = RTC_HOURFORMAT12_PM;
    } else {
      RTC_TimeStruct.TimeFormat = RTC_HOURFORMAT12_AM;
    }
#if defined(RTC_SSR_SS)
    /* subSeconds is read only, so no need to set it */
    /*RTC_TimeStruct.SubSeconds = subSeconds;*/
    /*RTC_TimeStruct.SecondFraction = 0;*/
#endif /* RTC_SSR_SS */
    RTC_TimeStruct.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    RTC_TimeStruct.StoreOperation = RTC_STOREOPERATION_RESET;
#else
    (void)period;
#endif /* !STM32F1xx */

    HAL_RTC_SetTime(&RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN);
  }
#endif /* USE_HALV2_DRIVER */
}

/**
  * @brief Get RTC time
  * @param hours: 0-12 or 0-23. Depends on the format used.
  * @param minutes: 0-59
  * @param seconds: 0-59
  * @param subSeconds: 0-999 (optional could be NULL)
  * @param period: HOUR_AM or HOUR_PM period in case RTC is set in 12 hours mode (optional could be NULL).
  * @retval None
  */
void RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, hourAM_PM_t *period)
{
#if defined(USE_HALV2_DRIVER)
  if ((hours != NULL) && (minutes != NULL) && (seconds != NULL)) {
    hal_rtc_time_t rtc_time;
    if (HAL_RTC_CALENDAR_GetTime(&rtc_time) == HAL_OK) { /* in BIN mode, only the subsecond is used */
      *hours = rtc_time.hour;
      *minutes = rtc_time.min;
      *seconds = rtc_time.sec;
      if (period != NULL) {
        if (rtc_time.am_pm == HAL_RTC_TIME_FORMAT_PM) {
          *period = HOUR_PM;
        } else {
          *period = HOUR_AM;
        }
      }
      if (subSeconds != NULL) {
        /*
        * The subsecond is the free-running downcounter, to be converted in milliseconds.
        */
        if (initMode == MODE_BINARY_ONLY) {
          *subSeconds = (((UINT32_MAX - rtc_time.subsec + 1) & UINT32_MAX)
                         * 1000) / fqce_apre;
        } else if (initMode == MODE_BINARY_MIX) {
          *subSeconds = (((UINT32_MAX - rtc_time.subsec) & predivSync)
                         * 1000) / fqce_apre;
        } else {
          /* the subsecond register value is converted in millisec on 32bit */
          *subSeconds = ((predivSync - rtc_time.subsec) * 1000) / (predivSync + 1);
        }
      }
    }
  }
#else
  RTC_TimeTypeDef RTC_TimeStruct = {0}; /* in BIN mode, only the subsecond is used */

  if ((hours != NULL) && (minutes != NULL) && (seconds != NULL)) {
#if defined(STM32F1xx)
    /* Store the date prior to checking the time, this may roll over to the next day as part of the time check,
       we need to the new date details in the backup registers if it changes */
    uint8_t current_date = RtcHandle.DateToUpdate.Date;
#endif

    HAL_RTC_GetTime(&RtcHandle, &RTC_TimeStruct, RTC_FORMAT_BIN);
    *hours = RTC_TimeStruct.Hours;
    *minutes = RTC_TimeStruct.Minutes;
    *seconds = RTC_TimeStruct.Seconds;
#if !defined(STM32F1xx)
    if (period != NULL) {
      if (RTC_TimeStruct.TimeFormat == RTC_HOURFORMAT12_PM) {
        *period = HOUR_PM;
      } else {
        *period = HOUR_AM;
      }
    }
#if defined(RTC_SSR_SS)
    if (subSeconds != NULL) {
      /*
       * The subsecond is the free-running downcounter, to be converted in milliseconds.
       */
      if (initMode == MODE_BINARY_ONLY) {
        *subSeconds = (((UINT32_MAX - RTC_TimeStruct.SubSeconds + 1) & UINT32_MAX)
                       * 1000) / fqce_apre;
      } else if (initMode == MODE_BINARY_MIX) {
        *subSeconds = (((UINT32_MAX - RTC_TimeStruct.SubSeconds) & predivSync)
                       * 1000) / fqce_apre;
      } else {
        /* the subsecond register value is converted in millisec on 32bit */
        *subSeconds = ((predivSync - RTC_TimeStruct.SubSeconds) * 1000) / (predivSync + 1);
      }
    }
#else
    (void)subSeconds;
#endif /* RTC_SSR_SS */
#else
    (void)period;
    (void)subSeconds;

    if (current_date != RtcHandle.DateToUpdate.Date) {
      RTC_StoreDate();
    }
#endif /* !STM32F1xx */
  }
#endif /* USE_HALV2_DRIVER */
}

/**
  * @brief Set RTC calendar
  * @param year: 0-99
  * @param month: 1-12
  * @param day: 1-31
  * @param wday: 1-7
  * @retval None
  */
void RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t wday)
{
#if defined(USE_HALV2_DRIVER)
  hal_rtc_date_t rtc_date;

  if (IS_RTC_YEAR(year) && IS_RTC_MONTH(month) && IS_RTC_DATE(day) && IS_RTC_WEEKDAY(wday)) {
    rtc_date.year = year;
    rtc_date.mon = month;
    rtc_date.mday = day;
    rtc_date.wday = wday;
    HAL_RTC_DisableWriteProtection();
    if (HAL_RTC_EnterInitMode() != HAL_OK || HAL_RTC_CALENDAR_SetDate(&rtc_date) != HAL_OK ||
        HAL_RTC_ExitInitMode() != HAL_OK || HAL_RTC_EnableWriteProtection() != HAL_OK) {
      Error_Handler();
    }
  }
#else
  RTC_DateTypeDef RTC_DateStruct;

  if (IS_RTC_YEAR(year) && IS_RTC_MONTH(month) && IS_RTC_DATE(day) && IS_RTC_WEEKDAY(wday)) {
    RTC_DateStruct.Year = year;
    RTC_DateStruct.Month = month;
    RTC_DateStruct.Date = day;
    RTC_DateStruct.WeekDay = wday;
    HAL_RTC_SetDate(&RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN);
#if defined(STM32F1xx)
    RTC_StoreDate();
#endif /* STM32F1xx */
  }
#endif
}

/**
  * @brief Get RTC calendar
  * @param year: 0-99
  * @param month: 1-12
  * @param day: 1-31
  * @param wday: 1-7
  * @retval None
  */
void RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *wday)
{
#if defined(USE_HALV2_DRIVER)
  if ((year != NULL) && (month != NULL) && (day != NULL) && (wday != NULL)) {
    hal_rtc_date_t rtc_date;
    if (HAL_RTC_CALENDAR_GetDate(&rtc_date) == HAL_OK) { /* in BIN mode, the date is not used */
      *year = rtc_date.year;
      *month = rtc_date.mon;
      *day = rtc_date.mday;
      *wday = rtc_date.wday;
    }
  }
#else
  RTC_DateTypeDef RTC_DateStruct = {0}; /* in BIN mode, the date is not used */

  if ((year != NULL) && (month != NULL) && (day != NULL) && (wday != NULL)) {
    HAL_RTC_GetDate(&RtcHandle, &RTC_DateStruct, RTC_FORMAT_BIN);
    *year = RTC_DateStruct.Year;
    *month = RTC_DateStruct.Month;
    *day = RTC_DateStruct.Date;
    *wday = RTC_DateStruct.WeekDay;
  }
#endif
}

/**
  * @brief Set RTC alarm and activate it with IT mode with 64bit accuracy on subsecond param
  *        Mainly used by Lorawan in RTC BIN or MIX mode
  * @param name: ALARM_A or ALARM_B if exists
  * @param day: 1-31 (day of the month)
  * @param hours: 0-12 or 0-23 depends on the hours mode.
  * @param minutes: 0-59
  * @param seconds: 0-59
  * @param subSeconds: 0-999 milliseconds or 64bit nb of milliseconds in no BCD mode
  * @param period: HOUR_AM or HOUR_PM if in 12 hours mode else ignored.
  * @param mask: configure alarm behavior using alarmMask_t combination.
  *              See AN4579 Table 5 for possible values.
  * @retval None
  */
void RTC_StartAlarm64(alarm_t name, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds, uint64_t subSeconds, hourAM_PM_t period, uint8_t mask)
{
#if defined(USE_HALV2_DRIVER)
  hal_rtc_alarm_t rtc_alarm = (name == ALARM_A) ? HAL_RTC_ALARM_A : HAL_RTC_ALARM_B;
  hal_rtc_alarm_config_t rtc_alarm_config;
  hal_rtc_alarm_date_time_t rtc_alarm_date_time;
  /* Ignore time AM PM configuration if in 24 hours format */
  if (initFormat == HOUR_FORMAT_24) {
    period = HOUR_AM;
  }

  if ((((initFormat == HOUR_FORMAT_24) && IS_RTC_HOUR24(hours)) || IS_RTC_HOUR12(hours))
      && IS_RTC_DATE(day) && IS_RTC_MINUTES(minutes) && IS_RTC_SECONDS(seconds)) {
    rtc_alarm_date_time.time.hour = hours;
    rtc_alarm_date_time.time.min = minutes;
    rtc_alarm_date_time.time.sec = seconds;
    rtc_alarm_date_time.time.am_pm = (period == HOUR_PM) ? HAL_RTC_TIME_FORMAT_PM : HAL_RTC_TIME_FORMAT_AM_24H;
    rtc_alarm_date_time.mday_wday_selection = HAL_RTC_ALARM_DAY_TYPE_SEL_MONTHDAY;
    rtc_alarm_date_time.wday_mday.mday = day;
    /* configure AlarmMask (M_MSK and Y_MSK ignored) */
    if (mask == OFF_MSK) {
      rtc_alarm_date_time.mask = HAL_RTC_ALARM_MASK_ALL;
    } else {
      rtc_alarm_date_time.mask = HAL_RTC_ALARM_MASK_NONE;
      if (!(mask & SS_MSK)) {
        rtc_alarm_date_time.mask |= HAL_RTC_ALARM_MASK_SECONDS;
      }
      if (!(mask & MM_MSK)) {
        rtc_alarm_date_time.mask |= HAL_RTC_ALARM_MASK_MINUTES;
      }
      if (!(mask & HH_MSK)) {
        rtc_alarm_date_time.mask |= HAL_RTC_ALARM_MASK_HOURS;
      }
      if (!(mask & D_MSK)) {
        rtc_alarm_date_time.mask |= HAL_RTC_ALARM_MASK_DAY;
      }
    }
    if (subSeconds < 1000) {
      if (initMode == MODE_BINARY_ONLY) {
        rtc_alarm_date_time.subsec_mask = 63;
      } else {
        rtc_alarm_date_time.subsec_mask = 0xF;
      }
      /*
       * The subsecond param is a nb of milliseconds to be converted in a subsecond
       * downcounter value and to be compared to the SubSecond register
       */
      if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
        /* the subsecond is the millisecond to be converted in a subsecond downcounter value */
        uint64_t tmp = (subSeconds * (uint64_t)(predivSync + 1)) / (uint64_t)1000;
        rtc_alarm_date_time.time.subsec = (uint32_t)UINT32_MAX - (uint32_t)tmp;
      } else {
        rtc_alarm_date_time.time.subsec = predivSync - ((uint32_t)subSeconds * (predivSync + 1)) / 1000;
      }
    } else {
      rtc_alarm_date_time.subsec_mask = 0x0;
    }
  } else {
    /* SS have to be managed*/
    if (initMode == MODE_BINARY_ONLY) {
      rtc_alarm_date_time.subsec_mask = 63;
    } else {
      rtc_alarm_date_time.subsec_mask = 0xF;
    }
    rtc_alarm_config.subsec_auto_reload = HAL_RTC_ALARM_SUBSECONDS_AUTO_RELOAD_DISABLE;
    rtc_alarm_config.auto_clear = HAL_ALARM_AUTO_CLEAR_DISABLE;
    rtc_alarm_date_time.mask = HAL_RTC_ALARM_MASK_ALL;
    rtc_alarm_date_time.subsec_mask = 63;
    if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
      /* We have an SubSecond alarm to set in RTC_BINARY_MIX or RTC_BINARY_ONLY mode */
      /* The subsecond in ms is converted in ticks unit 1 tick is 1000 / fqce_apre
       * For the conversion, we keep the accuracy on 64 bits, since otherwise we might
       * have an overflow even though the conversion result still fits in 32 bits.
       */
      uint64_t tmp = (subSeconds * (uint64_t)(predivSync + 1)) / (uint64_t)1000;
      rtc_alarm_date_time.time.subsec = (uint32_t)UINT32_MAX - (uint32_t)tmp;
    } else {
      rtc_alarm_date_time.time.subsec = predivSync - subSeconds * (predivSync + 1) / 1000;
    }
  }
  HAL_RTC_DisableWriteProtection();
  if (LL_RTC_ALM_IsStarted((uint32_t)rtc_alarm) == 1U) {
    HAL_RTC_ALARM_Stop(rtc_alarm);
    if (rtc_alarm == HAL_RTC_ALARM_A) {
      LL_RTC_ClearFlag_ALRA();
    } else {
      LL_RTC_ClearFlag_ALRB();
    }
  }
  if (HAL_RTC_ALARM_SetConfig(rtc_alarm, &rtc_alarm_config) == HAL_OK) {
    hal_status_t status = HAL_OK;
    if (initMode == MODE_BINARY_ONLY) {
      status = HAL_RTC_ALARM_SetBinaryTime(rtc_alarm, rtc_alarm_date_time.time.subsec);
      if (status == HAL_OK) {
        status = HAL_RTC_ALARM_SetBinarySubSecondMask(rtc_alarm, rtc_alarm_date_time.subsec_mask);
      }
    } else {
      status = HAL_RTC_ALARM_SetDateTime(rtc_alarm, &rtc_alarm_date_time);
    }
    if (status == HAL_OK) {
      HAL_RTC_ALARM_Start(rtc_alarm,  HAL_RTC_ALARM_IT_ENABLE);
      HAL_CORTEX_NVIC_SetPriority(RTC_Alarm_IRQn, RTC_IRQ_PRIO, RTC_IRQ_SUBPRIO);
      HAL_CORTEX_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    }
  }
  if (HAL_RTC_EnableWriteProtection() != HAL_OK) {
    Error_Handler();
  }
#else /* !USE_HALV2_DRIVER */
#if !defined(RTC_SSR_SS)
  (void)subSeconds;
#endif
  RTC_AlarmTypeDef RTC_AlarmStructure;

  /* Ignore time AM PM configuration if in 24 hours format */
  if (initFormat == HOUR_FORMAT_24) {
    period = HOUR_AM;
  }

#if !defined(ALARM_B_AVAILABLE)
  (void)name;
#else
  if (name == ALARM_B) {
    RTC_AlarmStructure.Alarm = RTC_ALARM_B;
  } else
#endif
  {
    RTC_AlarmStructure.Alarm = RTC_ALARM_A;
  }

  if ((((initFormat == HOUR_FORMAT_24) && IS_RTC_HOUR24(hours)) || IS_RTC_HOUR12(hours))
      && IS_RTC_DATE(day) && IS_RTC_MINUTES(minutes) && IS_RTC_SECONDS(seconds)) {
    /* Set RTC_AlarmStructure with calculated values*/
    RTC_AlarmStructure.AlarmTime.Seconds = seconds;
    RTC_AlarmStructure.AlarmTime.Minutes = minutes;
    RTC_AlarmStructure.AlarmTime.Hours = hours;
#if !defined(STM32F1xx)
#if defined(RTC_SSR_SS)
    if (subSeconds < 1000) {
#if defined(ALARM_B_AVAILABLE)
      if (name == ALARM_B) {
        RTC_AlarmStructure.AlarmSubSecondMask = predivSync_bits << RTC_ALRMBSSR_MASKSS_Pos;
      } else
#endif
      {
        RTC_AlarmStructure.AlarmSubSecondMask = predivSync_bits << RTC_ALRMASSR_MASKSS_Pos;
      }
      /*
       * The subsecond param is a nb of milliseconds to be converted in a subsecond
       * downcounter value and to be compared to the SubSecond register
       */
      if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
        /* the subsecond is the millisecond to be converted in a subsecond downcounter value */
        uint64_t tmp = (subSeconds * (uint64_t)(predivSync + 1)) / (uint64_t)1000;
        RTC_AlarmStructure.AlarmTime.SubSeconds = (uint32_t)UINT32_MAX - (uint32_t)tmp;
      } else {
        RTC_AlarmStructure.AlarmTime.SubSeconds = predivSync - ((uint32_t)subSeconds * (predivSync + 1)) / 1000;
      }
    } else {
      RTC_AlarmStructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    }
#endif /* RTC_SSR_SS */
    if (period == HOUR_PM) {
      RTC_AlarmStructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_PM;
    } else {
      RTC_AlarmStructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
    }
    RTC_AlarmStructure.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    RTC_AlarmStructure.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    RTC_AlarmStructure.AlarmDateWeekDay = day;
    RTC_AlarmStructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    /* configure AlarmMask (M_MSK and Y_MSK ignored) */
    if (mask == OFF_MSK) {
      RTC_AlarmStructure.AlarmMask = RTC_ALARMMASK_ALL;
    } else {
      RTC_AlarmStructure.AlarmMask = RTC_ALARMMASK_NONE;
      if (!(mask & SS_MSK)) {
        RTC_AlarmStructure.AlarmMask |= RTC_ALARMMASK_SECONDS;
      }
      if (!(mask & MM_MSK)) {
        RTC_AlarmStructure.AlarmMask |= RTC_ALARMMASK_MINUTES;
      }
      if (!(mask & HH_MSK)) {
        RTC_AlarmStructure.AlarmMask |= RTC_ALARMMASK_HOURS;
      }
      if (!(mask & D_MSK)) {
        RTC_AlarmStructure.AlarmMask |= RTC_ALARMMASK_DATEWEEKDAY;
      }
    }
#else
    (void)period;
    (void)day;
    (void)mask;
#endif /* !STM32F1xx */

    /* Set RTC_Alarm */
    HAL_RTC_SetAlarm_IT(&RtcHandle, &RTC_AlarmStructure, RTC_FORMAT_BIN);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, RTC_IRQ_PRIO, RTC_IRQ_SUBPRIO);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  }
#if defined(RTC_SSR_SS)
  else {
    /* SS have to be managed*/
#if defined(RTC_ALRMASSR_SSCLR)
    RTC_AlarmStructure.BinaryAutoClr = RTC_ALARMSUBSECONDBIN_AUTOCLR_NO;
#endif /* RTC_ALRMASSR_SSCLR */
    RTC_AlarmStructure.AlarmMask = RTC_ALARMMASK_ALL;
#if defined(ALARM_B_AVAILABLE)
    if (name == ALARM_B) {
      /* Expecting RTC_ALARMSUBSECONDBINMASK_NONE for the subsecond mask on ALARM B */
      RTC_AlarmStructure.AlarmSubSecondMask = mask << RTC_ALRMBSSR_MASKSS_Pos;
    } else
#endif
    {
      /* Expecting RTC_ALARMSUBSECONDBINMASK_NONE for the subsecond mask on ALARM A */
      RTC_AlarmStructure.AlarmSubSecondMask = mask << RTC_ALRMASSR_MASKSS_Pos;
    }
#if defined(RTC_ICSR_BIN)
    if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
      /* We have an SubSecond alarm to set in RTC_BINARY_MIX or RTC_BINARY_ONLY mode */
      /* The subsecond in ms is converted in ticks unit 1 tick is 1000 / fqce_apre
       * For the conversion, we keep the accuracy on 64 bits, since otherwise we might
       * have an overflow even though the conversion result still fits in 32 bits.
       */
      uint64_t tmp = (subSeconds * (uint64_t)(predivSync + 1)) / (uint64_t)1000;
      RTC_AlarmStructure.AlarmTime.SubSeconds = (uint32_t)UINT32_MAX - (uint32_t)tmp;
    } else
#endif /* RTC_ICSR_BIN */
    {
      RTC_AlarmStructure.AlarmTime.SubSeconds = predivSync - subSeconds * (predivSync + 1) / 1000;
    }
    /* Set RTC_Alarm */
    HAL_RTC_SetAlarm_IT(&RtcHandle, &RTC_AlarmStructure, RTC_FORMAT_BIN);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, RTC_IRQ_PRIO, RTC_IRQ_SUBPRIO);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
  }
#endif /* RTC_SSR_SS */
#endif /* USE_HALV2_DRIVER */
}

/**
  * @brief Set RTC alarm and activate it with IT mode
  * @param name: ALARM_A or ALARM_B if exists
  * @param day: 1-31 (day of the month)
  * @param hours: 0-12 or 0-23 depends on the hours mode.
  * @param minutes: 0-59
  * @param seconds: 0-59
  * @param subSeconds: 0-999 milliseconds
  * @param period: HOUR_AM or HOUR_PM if in 12 hours mode else ignored.
  * @param mask: configure alarm behavior using alarmMask_t combination.
  *              See AN4579 Table 5 for possible values.
  * @retval None
  */
void RTC_StartAlarm(alarm_t name, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds, uint32_t subSeconds, hourAM_PM_t period, uint8_t mask)
{
  /* Same RTC_StartAlarm where the nb of SubSeconds is lower than UINT32_MAX */
  RTC_StartAlarm64(name, day, hours, minutes, seconds, (uint64_t)subSeconds, period, mask);
}

/**
  * @brief Disable RTC alarm
  * @param name: ALARM_A or ALARM_B if exists
  * @retval None
  */
void RTC_StopAlarm(alarm_t name)
{
  /* Clear RTC Alarm Flag */
#if defined(USE_HALV2_DRIVER)
  if (name == ALARM_A) {
    LL_RTC_ClearFlag_ALRA();
    HAL_RTC_ALARM_Stop(HAL_RTC_ALARM_A);
  } else {
    LL_RTC_ClearFlag_ALRB();
    HAL_RTC_ALARM_Stop(HAL_RTC_ALARM_B);
  }
#else
#if defined(ALARM_B_AVAILABLE)
  if (name == ALARM_B) {
    __HAL_RTC_ALARM_CLEAR_FLAG(&RtcHandle, RTC_FLAG_ALRBF);
  } else
#endif
  {
    __HAL_RTC_ALARM_CLEAR_FLAG(&RtcHandle, RTC_FLAG_ALRAF);
  }
  /* Disable the Alarm A interrupt */
  HAL_RTC_DeactivateAlarm(&RtcHandle, name);
#endif
}

/**
  * @brief Check whether RTC alarm is set
  * @param ALARM_A or ALARM_B if exists
  * @retval True if Alarm is set
  */
bool RTC_IsAlarmSet(alarm_t name)
{
  bool status = false;
#if defined(STM32F1xx)
  (void)name;
  status = LL_RTC_IsEnabledIT_ALR(RTC);
#elif defined(USE_HALV2_DRIVER)
  if (name == ALARM_A) {
    status = LL_RTC_IsEnabledIT_ALRA();
  } else {
    status = LL_RTC_IsEnabledIT_ALRB();
  }
#else
#if defined(ALARM_B_AVAILABLE)
  if (name == ALARM_B) {
    status = LL_RTC_IsEnabledIT_ALRB(RTC);
  } else
#else
  (void)name;
#endif
  {
    status = LL_RTC_IsEnabledIT_ALRA(RTC);
  }
#endif
  return status;
}

/**
  * @brief Get RTC alarm
  * @param name: ALARM_A or ALARM_B if exists
  * @param day: 1-31 day of the month (optional could be NULL)
  * @param hours: 0-12 or 0-23 depends on the hours mode
  * @param minutes: 0-59
  * @param seconds: 0-59
  * @param subSeconds: 0-999 (optional could be NULL)
  * @param period: HOUR_AM or HOUR_PM (optional could be NULL)
  * @param mask: alarm behavior using alarmMask_t combination (optional could be NULL)
  *              See AN4579 Table 5 for possible values
  * @retval None
  */
void RTC_GetAlarm(alarm_t name, uint8_t *day, uint8_t *hours, uint8_t *minutes, uint8_t *seconds, uint32_t *subSeconds, hourAM_PM_t *period, uint8_t *mask)
{
#if defined(USE_HALV2_DRIVER)
  if ((hours != NULL) && (minutes != NULL) && (seconds != NULL)) {
    hal_rtc_alarm_t rtc_alarm = (name == ALARM_A) ? HAL_RTC_ALARM_A : HAL_RTC_ALARM_B;
    hal_rtc_alarm_date_time_t rtc_alarm_date_time;

    HAL_RTC_ALARM_GetDateTime(rtc_alarm, &rtc_alarm_date_time);
    *seconds = rtc_alarm_date_time.time.sec;
    *minutes = rtc_alarm_date_time.time.min;
    *hours = rtc_alarm_date_time.time.hour;
    if (day != NULL) {
      *day = rtc_alarm_date_time.wday_mday.mday;
    }
    if (period != NULL) {
      if (rtc_alarm_date_time.time.am_pm == HAL_RTC_TIME_FORMAT_PM) {
        *period = HOUR_PM;
      } else {
        *period = HOUR_AM;
      }
    }
    if (mask != NULL) {
      *mask = OFF_MSK;
      if (!(rtc_alarm_date_time.mask & HAL_RTC_ALARM_MASK_SECONDS)) {
        *mask |= SS_MSK;
      }
      if (!(rtc_alarm_date_time.mask & HAL_RTC_ALARM_MASK_MINUTES)) {
        *mask |= MM_MSK;
      }
      if (!(rtc_alarm_date_time.mask & HAL_RTC_ALARM_MASK_HOURS)) {
        *mask |= HH_MSK;
      }
      if (!(rtc_alarm_date_time.mask & HAL_RTC_ALARM_MASK_DAY)) {
        *mask |= D_MSK;
      }
    }
    if (period != NULL) {
      if (rtc_alarm_date_time.time.am_pm == HAL_RTC_TIME_FORMAT_PM) {
        *period = HOUR_PM;
      } else {
        *period = HOUR_AM;
      }
    }
    if (day != NULL) {
      *day = rtc_alarm_date_time.wday_mday.mday;
    }
    if (subSeconds != NULL) {
      /*
        * The subsecond is the bit SS[14:0] of the ALARM SSR register (not ALARMxINR)
        * to be converted in milliseconds
        */
      if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
        /* read the ALARM SSR register on SS[14:0] bits --> 0x7FFF */
        *subSeconds = (((0x7fff - HAL_RTC_ALARM_GetBinaryTime(rtc_alarm) + 1) & 0x7fff) * 1000) / fqce_apre;
      } else {
        *subSeconds = ((predivSync - rtc_alarm_date_time.time.subsec) * 1000) / (predivSync + 1);
      }
    }
#else
  RTC_AlarmTypeDef RTC_AlarmStructure;

  if ((hours != NULL) && (minutes != NULL) && (seconds != NULL)) {
    HAL_RTC_GetAlarm(&RtcHandle, &RTC_AlarmStructure, name, RTC_FORMAT_BIN);

    *seconds = RTC_AlarmStructure.AlarmTime.Seconds;
    *minutes = RTC_AlarmStructure.AlarmTime.Minutes;
    *hours = RTC_AlarmStructure.AlarmTime.Hours;

#if !defined(STM32F1xx)
    if (day != NULL) {
      *day = RTC_AlarmStructure.AlarmDateWeekDay;
    }
    if (period != NULL) {
      if (RTC_AlarmStructure.AlarmTime.TimeFormat == RTC_HOURFORMAT12_PM) {
        *period = HOUR_PM;
      } else {
        *period = HOUR_AM;
      }
    }
#if defined(RTC_SSR_SS)
    if (subSeconds != NULL) {
      /*
       * The subsecond is the bit SS[14:0] of the ALARM SSR register (not ALARMxINR)
       * to be converted in milliseconds
       */
      if ((initMode == MODE_BINARY_ONLY) || (initMode == MODE_BINARY_MIX)) {
        /* read the ALARM SSR register on SS[14:0] bits --> 0x7FFF */
        *subSeconds = (((0x7fff - RTC_AlarmStructure.AlarmTime.SubSeconds + 1) & 0x7fff) * 1000) / fqce_apre;
      } else {
        *subSeconds = ((predivSync - RTC_AlarmStructure.AlarmTime.SubSeconds) * 1000) / (predivSync + 1);
      }
    }
#else
    (void)subSeconds;
#endif /* RTC_SSR_SS */
    if (mask != NULL) {
      *mask = OFF_MSK;
      if (!(RTC_AlarmStructure.AlarmMask & RTC_ALARMMASK_SECONDS)) {
        *mask |= SS_MSK;
      }
      if (!(RTC_AlarmStructure.AlarmMask & RTC_ALARMMASK_MINUTES)) {
        *mask |= MM_MSK;
      }
      if (!(RTC_AlarmStructure.AlarmMask & RTC_ALARMMASK_HOURS)) {
        *mask |= HH_MSK;
      }
      if (!(RTC_AlarmStructure.AlarmMask & RTC_ALARMMASK_DATEWEEKDAY)) {
        *mask |= D_MSK;
      }
    }
#else
    (void)day;
    (void)period;
    (void)subSeconds;
    (void)mask;
#endif /* !STM32F1xx */
#endif
  }
}

/**
  * @brief Attach alarm callback.
  * @param func: pointer to the callback
  * @param func: pointer to callback argument
  * @param name: ALARM_A or ALARM_B if exists
  * @retval None
  */
void attachAlarmCallback(voidCallbackPtr func, void *data, alarm_t name)
{
#if defined(ALARM_B_AVAILABLE)
  if (name == ALARM_B) {
    RTCUserCallbackB = func;
    callbackUserDataB = data;
  } else
#else
  (void)name;
#endif
  {
    RTCUserCallback = func;
    callbackUserData = data;
  }
}

/**
  * @brief Detach alarm callback.
  * @param name: ALARM_A or ALARM_B if exists
  * @retval None
  */
void detachAlarmCallback(alarm_t name)
{
#if defined(ALARM_B_AVAILABLE)
  if (name == ALARM_B) {
    RTCUserCallbackB = NULL;
    callbackUserDataB = NULL;
  } else
#else
  (void)name;
#endif
  {
    RTCUserCallback = NULL;
    callbackUserData = NULL;
  }
}

#if defined(USE_HALV2_DRIVER)
/**
  * @brief  Alarm A callback.
  * @param  None
  * @retval None
  */
void HAL_RTC_AlarmAEventCallback(void)
{
  if (RTCUserCallback != NULL) {
    RTCUserCallback(callbackUserData);
  }
}

/**
  * @brief  Alarm B callback.
  * @param  None
  * @retval None
  */
void HAL_RTC_AlarmBEventCallback(void)
{
  if (RTCUserCallbackB != NULL) {
    RTCUserCallbackB(callbackUserDataB);
  }
}
#else
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  (void)hrtc;

  if (RTCUserCallback != NULL) {
    RTCUserCallback(callbackUserData);
  }
}

#if defined(ALARM_B_AVAILABLE)
/**
  * @brief  Alarm B callback.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
  (void)hrtc;

  if (RTCUserCallbackB != NULL) {
    RTCUserCallbackB(callbackUserDataB);
  }
}
#endif
#endif /* USE_HALV2_DRIVER */
/**
  * @brief  RTC Alarm IRQHandler
  * @param  None
  * @retval None
  */
void RTC_Alarm_IRQHandler(void)
{
#if defined(USE_HALV2_DRIVER)
  HAL_RTC_ALARM_IRQHandler();
  HAL_RTC_WAKEUP_IRQHandler();
#else
  HAL_RTC_AlarmIRQHandler(&RtcHandle);
#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx) || \
    defined(STM32F091xC) || defined(STM32F098xx) || defined(STM32F070xB) || \
    defined(STM32F030xC) || defined(STM32G0xx) || defined(STM32H5xx) || \
    defined(STM32L0xx) || defined(STM32L5xx) || defined(STM32U0xx) ||\
    defined(STM32U3xx) || defined(STM32U5xx) || defined(STM32WB0x) || \
    defined(STM32WBAxx) || defined(STM32WL3x)
  // In some cases, the same vector is used to manage WakeupTimer,
  // but with a dedicated HAL IRQHandler
  HAL_RTCEx_WakeUpTimerIRQHandler(&RtcHandle);
#endif
#endif
}

#ifdef ONESECOND_IRQn
/**
  * @brief Attach Seconds interrupt callback.
  * @note  stm32F1 has a second interrupt capability
  *        other MCUs map this on their WakeUp feature
  * @param func: pointer to the callback
  * @retval None
  */
void attachSecondsIrqCallback(voidCallbackPtr func)
{
  RTCSecondsIrqCallback = func;
#if defined(USE_HALV2_DRIVER)
  hal_rtc_wakeup_config_t wakeup_config;
  hal_rtc_time_t auto_reload_time = {.hour = 0, .min = 0, .sec = 1, .subsec = 0}; /* 1 second */
  hal_rtc_time_t auto_clear_time = {.hour = 0, .min = 0, .sec = 1, .subsec = 0}; /* 1 second */
  wakeup_config.clock = HAL_RTC_WAKEUP_TIMER_CLOCK_BCD_UPDATE;
  if (HAL_RTC_WAKEUP_SetConfig(&wakeup_config) == HAL_OK) {
    if (HAL_RTC_WAKEUP_SetPeriod(&auto_reload_time, &auto_clear_time) == HAL_OK) {
      HAL_RTC_WAKEUP_Start(HAL_RTC_WAKEUP_IT_ENABLE);
    }
  }
  /* Enable the IRQ that will trig the one-second interrupt */
  HAL_CORTEX_NVIC_EnableIRQ(ONESECOND_IRQn);
#else
#if defined(STM32F1xx)
  /* callback called on Seconds interrupt */
  HAL_RTCEx_SetSecond_IT(&RtcHandle);
  __HAL_RTC_SECOND_CLEAR_FLAG(&RtcHandle, RTC_FLAG_SEC);
#else
  /* callback called on wakeUp interrupt for One-Second purpose*/
  /* for MCUs using the wakeup feature : irq each second */
#if defined(RTC_WUTR_WUTOCLR)
  HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);
#else
  HAL_RTCEx_SetWakeUpTimer_IT(&RtcHandle, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
#endif /* RTC_WUTR_WUTOCLR */
#endif /* STM32F1xx */
  /* enable the IRQ that will trig the one-second interrupt */
  HAL_NVIC_EnableIRQ(ONESECOND_IRQn);
#endif /* USE_HALV2_DRIVER */
}

/**
  * @brief Detach Seconds interrupt callback.
  * @param None
  * @retval None
  */
void detachSecondsIrqCallback(void)
{
#if defined(STM32F1xx)
  HAL_RTCEx_DeactivateSecond(&RtcHandle);
#else
  /* for MCUs using the wakeup feature : do not deactivate the WakeUp
     as it might be used for another reason than the One-Second purpose */
  // HAL_RTCEx_DeactivateWakeUpTimer(&RtcHandle);
#endif /* STM32F1xx */
  RTCSecondsIrqCallback = NULL;
}

#if defined(STM32F1xx)
/**
  * @brief  Seconds interrupt callback.
  * @param  hrtc RTC handle
  * @retval None
  */
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
  (void)hrtc;

  if (RTCSecondsIrqCallback != NULL) {
    RTCSecondsIrqCallback(NULL);
  }
}

/**
  * @brief  This function handles RTC Seconds interrupt request.
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void)
{
  HAL_RTCEx_RTCIRQHandler(&RtcHandle);
}

#else
/**
  * @brief  WakeUp event mapping the Seconds interrupt callback.
  * @param  hrtc RTC handle
  * @retval None
  */
#if defined(USE_HALV2_DRIVER)
void HAL_RTC_WakeUpTimerEventCallback()
{
  if (RTCSecondsIrqCallback != NULL) {
    RTCSecondsIrqCallback(NULL);
  }
}
#else
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  (void)hrtc;

  if (RTCSecondsIrqCallback != NULL) {
    RTCSecondsIrqCallback(NULL);
  }
}
#endif

/**
  * @brief  This function handles RTC Seconds through wakeup interrupt request.
  * @param  None
  * @retval None
  */
void RTC_WKUP_IRQHandler(void)
{
#if defined(USE_HALV2_DRIVER)
  HAL_RTC_WAKEUP_IRQHandler();
#else
  HAL_RTCEx_WakeUpTimerIRQHandler(&RtcHandle);
#endif
}
#endif /* STM32F1xx */
#endif /* ONESECOND_IRQn */

#ifdef STM32WLxx
/**
  * @brief Attach SubSeconds underflow interrupt callback.
  * @param func: pointer to the callback
  * @retval None
  */
void attachSubSecondsUnderflowIrqCallback(voidCallbackPtr func)
{
  /* Callback called on SSRU interrupt */
  RTCSubSecondsUnderflowIrqCallback = func;

  /* Enable the IRQ that will trig the one-second interrupt */
  if (HAL_RTCEx_SetSSRU_IT(&RtcHandle) != HAL_OK) {
    Error_Handler();
  }
  HAL_NVIC_SetPriority(TAMP_STAMP_LSECSS_SSRU_IRQn, RTC_IRQ_SSRU_PRIO, RTC_IRQ_SSRU_SUBPRIO);
  HAL_NVIC_EnableIRQ(TAMP_STAMP_LSECSS_SSRU_IRQn);
}

/**
  * @brief Detach  SubSeconds underflow interrupt callback.
  * @param None
  * @retval None
  */
void detachSubSecondsUnderflowIrqCallback(void)
{
  RTCSubSecondsUnderflowIrqCallback = NULL;
  if (HAL_RTCEx_DeactivateSSRU(&RtcHandle) != HAL_OK) {
    Error_Handler();
  }
  HAL_NVIC_DisableIRQ(TAMP_STAMP_LSECSS_SSRU_IRQn);
}

void HAL_RTCEx_SSRUEventCallback(RTC_HandleTypeDef *hrtc)
{
  (void)hrtc;
  if (RTCSubSecondsUnderflowIrqCallback != NULL) {
    RTCSubSecondsUnderflowIrqCallback(NULL);
  }
}
#endif /* STM32WLxx */

#if defined(STM32F1xx)
void RTC_StoreDate(void)
{
  /* Store the date in the backup registers */
  uint32_t dateToStore;
  memcpy(&dateToStore, &RtcHandle.DateToUpdate, 4);
  setBackupRegister(RTC_BKP_DATE, dateToStore >> 16);
  setBackupRegister(RTC_BKP_DATE + 1, dateToStore & 0xffff);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* !HAL_RTC_MODULE_ONLY && (HAL_RTC_MODULE_ENABLED || (USE_HAL_RTC_MODULE...) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
