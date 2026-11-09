# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_wire_conformance LANGUAGES C)

set(FURNACE_HMI_WIRE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../app/hmi/protocol/wire")
set(FURNACE_HMI_CONFORMANCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../conformance/wire")

target_sources(app PRIVATE
  src/main.c
  "${FURNACE_HMI_WIRE_DIR}/src/crc32.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_frame.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_messages.c"
  "${FURNACE_HMI_CONFORMANCE_DIR}/wire_conformance.c"
  "${FURNACE_HMI_CONFORMANCE_DIR}/wire_vectors_gen.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_WIRE_DIR}/include"
  "${FURNACE_HMI_CONFORMANCE_DIR}"
)
