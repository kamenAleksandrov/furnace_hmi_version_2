# SPDX-License-Identifier: Apache-2.0

add_library(furnace_hmi_foundation STATIC
  src/foundation.c
)

add_library(furnace_hmi::foundation ALIAS furnace_hmi_foundation)

target_include_directories(furnace_hmi_foundation
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(furnace_hmi_foundation PUBLIC c_std_17)

if(MSVC)
  target_compile_options(furnace_hmi_foundation PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_foundation PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
