# CMakeList.txt : CMake project for micro_rtc, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (micro_rtc "micro_rtc.c"   "dst_dates.h" "iso_8601_date.h"    "micro_rtc.h" "test.c" "build.c" "build.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET micro_rtc PROPERTY CXX_STANDARD 20)
endif()

# for testing
add_compile_definitions(RTC_ENABLE_INTERRUPT="do{}while(0)")
add_compile_definitions(RTC_DISABLE_INTERRUPT="do{}while(0)")

# TODO: Add tests and install targets if needed.
