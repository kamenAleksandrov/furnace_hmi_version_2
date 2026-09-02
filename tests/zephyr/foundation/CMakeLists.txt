# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_foundation_test LANGUAGES C)

set(FURNACE_HMI_FOUNDATION_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../app/hmi/foundation")
set(FURNACE_HMI_STRINGS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../app/hmi/presentation/strings")

target_sources(app PRIVATE
  src/main.c
  "${FURNACE_HMI_FOUNDATION_DIR}/src/foundation.c"
  "${FURNACE_HMI_STRINGS_DIR}/src/ui_strings.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_FOUNDATION_DIR}/include"
  "${FURNACE_HMI_STRINGS_DIR}/include"
)
