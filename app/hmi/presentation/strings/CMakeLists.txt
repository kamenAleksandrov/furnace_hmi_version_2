# SPDX-License-Identifier: Apache-2.0

add_library(furnace_hmi_ui_strings STATIC
  src/ui_strings.c
)

add_library(furnace_hmi::ui_strings ALIAS furnace_hmi_ui_strings)

target_include_directories(furnace_hmi_ui_strings
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(furnace_hmi_ui_strings PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_ui_strings PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_ui_strings PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
