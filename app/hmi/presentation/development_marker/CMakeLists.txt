# SPDX-License-Identifier: Apache-2.0

add_library(
  furnace_hmi_development_marker
  STATIC
  src/development_marker.c
)
add_library(
  furnace_hmi::development_marker
  ALIAS furnace_hmi_development_marker
)

target_include_directories(
  furnace_hmi_development_marker
  PUBLIC include
)
target_link_libraries(
  furnace_hmi_development_marker
  PUBLIC lvgl::lvgl furnace_hmi::ui_strings
)
target_compile_features(
  furnace_hmi_development_marker
  PUBLIC c_std_17
)

if(MSVC)
  target_compile_options(
    furnace_hmi_development_marker
    PRIVATE /W4 /WX /permissive-
  )
else()
  target_compile_options(
    furnace_hmi_development_marker
    PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()
