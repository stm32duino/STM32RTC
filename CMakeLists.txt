# v3.21 implemented semantic changes regarding $<TARGET_OBJECTS:...>
# See https://cmake.org/cmake/help/v3.21/command/target_link_libraries.html#linking-object-libraries-via-target-objects
cmake_minimum_required(VERSION 3.21)

add_library(STM32RTC INTERFACE)
add_library(STM32RTC_usage INTERFACE)

target_include_directories(STM32RTC_usage INTERFACE
  src
)


target_link_libraries(STM32RTC_usage INTERFACE
  base_config
)

target_link_libraries(STM32RTC INTERFACE STM32RTC_usage)



add_library(STM32RTC_bin OBJECT EXCLUDE_FROM_ALL
  src/rtc.c
  src/STM32RTC.cpp
)
target_link_libraries(STM32RTC_bin PUBLIC STM32RTC_usage)

target_link_libraries(STM32RTC INTERFACE
  STM32RTC_bin
  $<TARGET_OBJECTS:STM32RTC_bin>
)

