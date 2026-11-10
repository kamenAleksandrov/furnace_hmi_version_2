# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_controller_client LANGUAGES C)

set(FURNACE_HMI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(FURNACE_HMI_WIRE_DIR "${FURNACE_HMI_ROOT}/app/hmi/protocol/wire")
set(FURNACE_HMI_TESTS_DIR "${FURNACE_HMI_ROOT}/tests/client")

target_sources(app PRIVATE
  src/main.c
  "${FURNACE_HMI_TESTS_DIR}/controller_client_tests.c"
  "${FURNACE_HMI_TESTS_DIR}/controller_dashboard_tests.c"
  "${FURNACE_HMI_ROOT}/app/hmi/client/src/controller_client.c"
  "${FURNACE_HMI_ROOT}/app/hmi/client/src/controller_dashboard.c"
  "${FURNACE_HMI_ROOT}/app/hmi/model/src/dashboard_model.c"
  "${FURNACE_HMI_ROOT}/app/hmi/transport/src/loopback_port.c"
  "${FURNACE_HMI_WIRE_DIR}/src/crc32.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_frame.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_messages.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_TESTS_DIR}"
  "${FURNACE_HMI_ROOT}/app/hmi/client/include"
  "${FURNACE_HMI_ROOT}/app/hmi/transport/include"
  "${FURNACE_HMI_ROOT}/app/hmi/model/include"
  "${FURNACE_HMI_WIRE_DIR}/include"
)
