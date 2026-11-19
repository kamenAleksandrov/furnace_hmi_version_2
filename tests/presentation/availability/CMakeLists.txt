# SPDX-License-Identifier: Apache-2.0

# Checks of the state-availability presentation, shared by the desktop
# program's --presentation-smoke-test and the Zephyr test
# (tests/zephyr/presentation), which compiles the same source.
add_library(
  furnace_hmi_availability_presentation_checks
  STATIC
  src/availability_presentation_checks.c
)
add_library(
  furnace_hmi::availability_presentation_checks
  ALIAS furnace_hmi_availability_presentation_checks
)

target_include_directories(
  furnace_hmi_availability_presentation_checks
  PUBLIC include
)
target_link_libraries(
  furnace_hmi_availability_presentation_checks
  PUBLIC lvgl::lvgl
  PRIVATE furnace_hmi::state_availability_view furnace_hmi::ui_strings
)
target_compile_features(
  furnace_hmi_availability_presentation_checks
  PUBLIC c_std_17
)

if(MSVC)
  target_compile_options(
    furnace_hmi_availability_presentation_checks
    PRIVATE /W4 /WX /permissive-
  )
else()
  target_compile_options(
    furnace_hmi_availability_presentation_checks
    PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()
