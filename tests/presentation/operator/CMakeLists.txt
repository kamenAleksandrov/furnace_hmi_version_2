# SPDX-License-Identifier: Apache-2.0

# Checks of the operator screens without controller-owned content, shared by
# the desktop program's --operator-content-smoke-test and the Zephyr test
# (tests/zephyr/operator), which compiles the same source.
add_library(
  furnace_hmi_operator_content_checks
  STATIC
  src/operator_content_checks.c
)
add_library(
  furnace_hmi::operator_content_checks
  ALIAS furnace_hmi_operator_content_checks
)

target_include_directories(
  furnace_hmi_operator_content_checks
  PUBLIC include
)
target_link_libraries(
  furnace_hmi_operator_content_checks
  PUBLIC lvgl::lvgl
  PRIVATE furnace_hmi::operator_screens furnace_hmi::ui_strings furnace_hmi::model
)
target_compile_features(
  furnace_hmi_operator_content_checks
  PUBLIC c_std_17
)

if(MSVC)
  target_compile_options(
    furnace_hmi_operator_content_checks
    PRIVATE /W4 /WX /permissive-
  )
else()
  target_compile_options(
    furnace_hmi_operator_content_checks
    PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()
