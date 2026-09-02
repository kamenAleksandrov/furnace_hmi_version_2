# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.24)

project(furnace_hmi_host_tests LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

get_filename_component(FURNACE_HMI_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(NOT TARGET furnace_hmi_foundation)
  add_subdirectory(
    "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/foundation"
    "${CMAKE_CURRENT_BINARY_DIR}/foundation"
  )
endif()

if(NOT TARGET furnace_hmi_ui_strings)
  add_subdirectory(
    "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/presentation/strings"
    "${CMAKE_CURRENT_BINARY_DIR}/ui_strings"
  )
endif()

add_executable(furnace_hmi_foundation_smoke foundation_smoke.c)
target_link_libraries(furnace_hmi_foundation_smoke PRIVATE furnace_hmi::foundation)

add_executable(furnace_hmi_ui_strings_smoke ui_strings_smoke.c)
target_link_libraries(furnace_hmi_ui_strings_smoke PRIVATE furnace_hmi::ui_strings)

if(MSVC)
  target_compile_options(furnace_hmi_foundation_smoke PRIVATE /W4 /WX /permissive-)
  target_compile_options(furnace_hmi_ui_strings_smoke PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(
    furnace_hmi_foundation_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
  target_compile_options(
    furnace_hmi_ui_strings_smoke PRIVATE -Wall -Wextra -Wpedantic -Werror
  )
endif()

add_test(NAME host.foundation_probe COMMAND furnace_hmi_foundation_smoke)
add_test(NAME host.ui_strings_catalog COMMAND furnace_hmi_ui_strings_smoke)
add_test(
  NAME tools.codebase_index
  COMMAND
    "${Python3_EXECUTABLE}"
    -m unittest discover
    -s tools/tests
    -p test_codebase_index.py
    -v
)
add_test(
  NAME tools.bootstrap_zephyr
  COMMAND
    "${Python3_EXECUTABLE}"
    -m unittest discover
    -s tools/tests
    -p test_bootstrap_zephyr.py
    -v
)
add_test(
  NAME tools.desktop_ui
  COMMAND
    "${Python3_EXECUTABLE}"
    -m unittest discover
    -s tools/tests
    -p test_run_desktop_ui.py
    -v
)
set_tests_properties(
  tools.codebase_index
  tools.bootstrap_zephyr
  tools.desktop_ui
  PROPERTIES
    WORKING_DIRECTORY "${FURNACE_HMI_REPOSITORY_ROOT}"
    TIMEOUT 30
)
