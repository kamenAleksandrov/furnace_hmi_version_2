# SPDX-License-Identifier: Apache-2.0

add_library(
  furnace_hmi_state_availability_view
  STATIC
  src/state_availability_view.c
)
add_library(
  furnace_hmi::state_availability_view
  ALIAS furnace_hmi_state_availability_view
)

target_include_directories(
  furnace_hmi_state_availability_view
  PUBLIC include
)
target_link_libraries(
  furnace_hmi_state_availability_view
  PUBLIC lvgl::lvgl
  PRIVATE furnace_hmi::ui_strings
)
target_compile_features(
  furnace_hmi_state_availability_view
  PUBLIC c_std_17
)

if(MSVC)
  target_compile_options(
    furnace_hmi_state_availability_view
    PRIVATE /W4 /WX /permissive-
  )
else()
  target_compile_options(
    furnace_hmi_state_availability_view
    PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()
