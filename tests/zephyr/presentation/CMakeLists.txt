# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

set(FURNACE_HMI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

# The application's LVGL configuration, so the test draws what the application draws.
list(APPEND EXTRA_CONF_FILE "${FURNACE_HMI_ROOT}/app/zephyr/gui.conf")

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_presentation_test LANGUAGES C)

set(FURNACE_HMI_MODEL_DIR "${FURNACE_HMI_ROOT}/app/hmi/model")
set(FURNACE_HMI_STRINGS_DIR "${FURNACE_HMI_ROOT}/app/hmi/presentation/strings")
set(FURNACE_HMI_AVAILABILITY_DIR "${FURNACE_HMI_ROOT}/app/hmi/presentation/state_availability")
set(FURNACE_HMI_CHECKS_DIR "${FURNACE_HMI_ROOT}/tests/presentation/availability")

target_sources(app PRIVATE
  src/main.c
  "${FURNACE_HMI_CHECKS_DIR}/src/availability_presentation_checks.c"
  "${FURNACE_HMI_AVAILABILITY_DIR}/src/state_availability_view.c"
  "${FURNACE_HMI_STRINGS_DIR}/src/ui_strings.c"
  "${FURNACE_HMI_STRINGS_DIR}/generated/ui_strings_catalogue.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_CHECKS_DIR}/include"
  "${FURNACE_HMI_AVAILABILITY_DIR}/include"
  "${FURNACE_HMI_MODEL_DIR}/include"
  "${FURNACE_HMI_STRINGS_DIR}/include"
  "${FURNACE_HMI_STRINGS_DIR}/src"
)
