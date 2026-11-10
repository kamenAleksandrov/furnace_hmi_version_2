# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The byte port to the controller, and an in-memory loopback for tests.
add_library(furnace_hmi_transport STATIC src/loopback_port.c)
add_library(furnace_hmi::transport ALIAS furnace_hmi_transport)

target_include_directories(furnace_hmi_transport
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(furnace_hmi_transport PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_transport PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_transport PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
