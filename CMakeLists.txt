# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The codebase indexer consumes the CMake File API codemodel.  Keep the query
# in configured build trees so later reconfiguration refreshes the reply.  A
# first configure must prepare the query before CMake starts; the deterministic
# command is documented in docs/_generated/README.md.
set(_furnace_hmi_file_api_query_dir "${CMAKE_BINARY_DIR}/.cmake/api/v1/query")
file(MAKE_DIRECTORY "${_furnace_hmi_file_api_query_dir}")
if(NOT EXISTS "${_furnace_hmi_file_api_query_dir}/codemodel-v2")
  file(WRITE "${_furnace_hmi_file_api_query_dir}/codemodel-v2" "")
endif()
unset(_furnace_hmi_file_api_query_dir)

project(
  FurnaceHmiV2Foundation
  DESCRIPTION "Portable Furnace HMI V2 engineering foundation"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(FURNACE_HMI_BUILD_DESKTOP "Build the desktop LVGL proof application" ON)
option(FURNACE_HMI_BUILD_HOST_TESTS "Build portable host-side tests" ON)
option(FURNACE_HMI_BUILD_UI_LAB "Build the host-only semantic UI laboratory" OFF)

include(CTest)

add_subdirectory(app/hmi/foundation)
add_subdirectory(app/hmi/model)
add_subdirectory(app/hmi/presentation/strings)

if(FURNACE_HMI_BUILD_DESKTOP)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/simulator/desktop/CMakeLists.txt")
    message(FATAL_ERROR "The desktop proof target is not present in simulator/desktop")
  endif()
  add_subdirectory(simulator/desktop)
endif()

if(FURNACE_HMI_BUILD_UI_LAB)
  if(NOT WIN32)
    message(FATAL_ERROR "The UI laboratory currently requires the Windows SDL host path.")
  endif()
  add_subdirectory(simulator/ui_lab)
endif()

if(FURNACE_HMI_BUILD_HOST_TESTS AND BUILD_TESTING)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/CMakeLists.txt")
    message(FATAL_ERROR "The portable host tests are not present in tests/host")
  endif()
  add_subdirectory(tests/host)
endif()
