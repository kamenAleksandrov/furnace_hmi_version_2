# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

add_library(furnace_hmi_model STATIC src/dashboard_model.c)
add_library(furnace_hmi::model ALIAS furnace_hmi_model)

target_include_directories(furnace_hmi_model
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(furnace_hmi_model PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_model PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
