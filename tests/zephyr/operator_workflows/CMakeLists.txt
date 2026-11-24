# SPDX-License-Identifier: Apache-2.0

# SIMULATION ONLY (Phase 21 M21-8): the HMI application's screens, GUI pass and
# controller link, driven by taps, against the controller's link_pty program
# (tools/controller_workflow_check.py --suite operator).

cmake_minimum_required(VERSION 3.28.0)

set(FURNACE_HMI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")

# The application's LVGL configuration, so the test draws what the application draws.
list(APPEND EXTRA_CONF_FILE "${FURNACE_HMI_ROOT}/app/zephyr/gui.conf")

find_package(Zephyr 4.4.0 EXACT REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(furnace_hmi_zephyr_operator_workflows LANGUAGES C)

set(FURNACE_HMI_APP_DIR "${FURNACE_HMI_ROOT}/app/zephyr/src")
set(FURNACE_HMI_PRESENTATION_DIR "${FURNACE_HMI_ROOT}/app/hmi/presentation")
set(FURNACE_HMI_CLIENT_DIR "${FURNACE_HMI_ROOT}/app/hmi/client")
set(FURNACE_HMI_WIRE_DIR "${FURNACE_HMI_ROOT}/app/hmi/protocol/wire")
set(FURNACE_HMI_CHECKS_DIR "${FURNACE_HMI_ROOT}/tests/presentation/operator")
file(GLOB FURNACE_HMI_FONT_SOURCES "${FURNACE_HMI_PRESENTATION_DIR}/fonts/generated/*.c")

target_sources(app PRIVATE
  src/main.c
  "${FURNACE_HMI_CHECKS_DIR}/src/screen_probe.c"
  "${FURNACE_HMI_APP_DIR}/gui_host.c"
  "${FURNACE_HMI_APP_DIR}/controller_link.c"
  "${FURNACE_HMI_APP_DIR}/operator_commands.c"
  "${FURNACE_HMI_APP_DIR}/operator_library.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/operator/src/operator_diagnostics.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/operator/src/operator_screens.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/operator/src/operator_theme.c"
  ${FURNACE_HMI_FONT_SOURCES}
  "${FURNACE_HMI_PRESENTATION_DIR}/icons/generated/hmi_icons.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/icons/generated/support_qr.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/theme/generated/hmi_theme.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/strings/src/ui_strings.c"
  "${FURNACE_HMI_PRESENTATION_DIR}/strings/generated/ui_strings_catalogue.c"
  "${FURNACE_HMI_ROOT}/app/hmi/model/src/dashboard_model.c"
  "${FURNACE_HMI_CLIENT_DIR}/src/controller_client.c"
  "${FURNACE_HMI_CLIENT_DIR}/src/controller_diagnostics.c"
  "${FURNACE_HMI_CLIENT_DIR}/src/controller_library.c"
  "${FURNACE_HMI_CLIENT_DIR}/src/controller_dashboard.c"
  "${FURNACE_HMI_WIRE_DIR}/src/crc32.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_frame.c"
  "${FURNACE_HMI_WIRE_DIR}/src/wire_messages.c"
)

target_include_directories(app PRIVATE
  "${FURNACE_HMI_APP_DIR}"
  "${FURNACE_HMI_CHECKS_DIR}/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/operator/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/fonts/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/icons/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/theme/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/strings/include"
  "${FURNACE_HMI_PRESENTATION_DIR}/strings/src"
  "${FURNACE_HMI_ROOT}/app/hmi/model/include"
  "${FURNACE_HMI_CLIENT_DIR}/include"
  "${FURNACE_HMI_ROOT}/app/hmi/transport/include"
  "${FURNACE_HMI_WIRE_DIR}/include"
)
