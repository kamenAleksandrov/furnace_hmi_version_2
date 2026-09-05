# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

add_library(furnace_hmi_simulated_controller STATIC src/simulated_controller.c)
add_library(furnace_hmi::simulated_controller ALIAS furnace_hmi_simulated_controller)

target_include_directories(furnace_hmi_simulated_controller
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(furnace_hmi_simulated_controller PUBLIC furnace_hmi::model)
target_compile_features(furnace_hmi_simulated_controller PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_simulated_controller PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_simulated_controller PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
