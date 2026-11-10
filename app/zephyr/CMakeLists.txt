# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The LVGL presentation is built for boards with a display: for now
# native_sim/native/64, whose SDL display opens a window on the host. Other
# boards (qemu_x86) build the console probe.
if(BOARD MATCHES "^native_sim/native/64")
  list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_LIST_DIR}/gui.conf")
endif()

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_foundation LANGUAGES C)

set(FURNACE_HMI_FOUNDATION_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/foundation")
set(FURNACE_HMI_MODEL_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/model")
set(FURNACE_HMI_STRINGS_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/presentation/strings")
set(FURNACE_HMI_AVAILABILITY_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/presentation/state_availability")

target_sources(app PRIVATE
  "${FURNACE_HMI_FOUNDATION_DIR}/src/foundation.c"
  "${FURNACE_HMI_MODEL_DIR}/src/dashboard_model.c"
  "${FURNACE_HMI_STRINGS_DIR}/src/ui_strings.c"
  "${FURNACE_HMI_STRINGS_DIR}/generated/ui_strings_catalogue.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_FOUNDATION_DIR}/include"
  "${FURNACE_HMI_MODEL_DIR}/include"
  "${FURNACE_HMI_STRINGS_DIR}/include"
  "${FURNACE_HMI_STRINGS_DIR}/src"
)

if(CONFIG_LVGL)
  target_sources(app PRIVATE
    src/gui_main.c
    "${FURNACE_HMI_AVAILABILITY_DIR}/src/state_availability_view.c"
  )
  target_include_directories(app PRIVATE "${FURNACE_HMI_AVAILABILITY_DIR}/include")
else()
  target_sources(app PRIVATE src/main.c)
endif()
