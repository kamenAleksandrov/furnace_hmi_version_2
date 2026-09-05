# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

add_library(furnace_hmi_dashboard_view STATIC src/dashboard_view.c)
add_library(furnace_hmi::dashboard_view ALIAS furnace_hmi_dashboard_view)

target_include_directories(furnace_hmi_dashboard_view
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(furnace_hmi_dashboard_view
  PUBLIC
    furnace_hmi::model
    furnace_hmi::ui_strings
    lvgl::lvgl
)
target_compile_features(furnace_hmi_dashboard_view PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_dashboard_view PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_dashboard_view PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
