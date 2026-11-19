# SPDX-License-Identifier: Apache-2.0

# The operator screens (Home, Programs, Running, Manual, Settings, Device): one
# implementation for every LVGL build (Phase 21 M21-2b). Moved from the host UI
# lab; the lab, the desktop program and the Zephyr application compose it.
add_library(
  furnace_hmi_operator_screens
  STATIC
  src/operator_screens.c
  src/operator_theme.c
)
add_library(
  furnace_hmi::operator_screens
  ALIAS furnace_hmi_operator_screens
)

target_include_directories(
  furnace_hmi_operator_screens
  PUBLIC include
)
target_link_libraries(
  furnace_hmi_operator_screens
  PUBLIC lvgl::lvgl furnace_hmi::model furnace_hmi::theme
  PRIVATE furnace_hmi::ui_strings furnace_hmi::fonts furnace_hmi::icons
)
target_compile_features(
  furnace_hmi_operator_screens
  PUBLIC c_std_17
)

if(MSVC)
  target_compile_options(
    furnace_hmi_operator_screens
    PRIVATE /W4 /WX /permissive-
  )
else()
  target_compile_options(
    furnace_hmi_operator_screens
    PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()
