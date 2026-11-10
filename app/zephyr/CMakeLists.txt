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

if(BOARD STREQUAL "native_sim" AND BOARD_QUALIFIERS MATCHES "^/?native/64$" AND NOT CONFIG_LVGL)
  message(FATAL_ERROR "native_sim/native/64 builds the LVGL application: pass the board with -b "
                      "(or -DBOARD) so that gui.conf is applied")
endif()

if(CONFIG_LVGL)
  # The GUI build is the HMI: the screen, and the controller link (Phase 21 M21-3).
  set(FURNACE_HMI_MARKER_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/presentation/development_marker")
  set(FURNACE_HMI_CLIENT_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/client")
  set(FURNACE_HMI_TRANSPORT_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/transport")
  set(FURNACE_HMI_WIRE_DIR "${CMAKE_CURRENT_LIST_DIR}/../hmi/protocol/wire")
  target_sources(app PRIVATE
    src/gui_main.c
    src/controller_link.c
    "${FURNACE_HMI_AVAILABILITY_DIR}/src/state_availability_view.c"
    "${FURNACE_HMI_MARKER_DIR}/src/development_marker.c"
    "${FURNACE_HMI_CLIENT_DIR}/src/controller_client.c"
    "${FURNACE_HMI_WIRE_DIR}/src/crc32.c"
    "${FURNACE_HMI_WIRE_DIR}/src/wire_frame.c"
    "${FURNACE_HMI_WIRE_DIR}/src/wire_messages.c"
  )
  target_include_directories(app PRIVATE
    "${FURNACE_HMI_AVAILABILITY_DIR}/include"
    "${FURNACE_HMI_MARKER_DIR}/include"
    "${FURNACE_HMI_CLIENT_DIR}/include"
    "${FURNACE_HMI_TRANSPORT_DIR}/include"
    "${FURNACE_HMI_WIRE_DIR}/include"
  )
else()
  target_sources(app PRIVATE src/main.c)
endif()
