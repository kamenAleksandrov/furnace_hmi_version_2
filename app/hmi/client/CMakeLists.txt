# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The controller client (WI-H06): portable, single-threaded, no platform calls.
add_library(furnace_hmi_client STATIC src/controller_client.c)
add_library(furnace_hmi::client ALIAS furnace_hmi_client)

target_include_directories(furnace_hmi_client
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(furnace_hmi_client
  PUBLIC furnace_hmi::wire furnace_hmi::transport furnace_hmi::model
)

target_compile_features(furnace_hmi_client PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_client PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_client PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
