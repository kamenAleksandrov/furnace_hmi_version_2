# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

include(FetchContent)

set(
  FURNACE_HMI_LVGL_REVISION
  "85aa60d18b3d5e5588d7b247abf90198f07c8a63"
  CACHE STRING
  "LVGL revision used by Zephyr 4.4.0"
)
set(
  FURNACE_HMI_LVGL_SOURCE_DIR
  ""
  CACHE PATH
  "Existing LVGL checkout; leave empty to reuse the west module or fetch the pinned revision"
)
option(
  FURNACE_HMI_FETCH_LVGL
  "Fetch the pinned LVGL revision when no matching local checkout is available"
  ON
)

if(NOT FURNACE_HMI_LVGL_REVISION STREQUAL "85aa60d18b3d5e5588d7b247abf90198f07c8a63")
  message(
    FATAL_ERROR
    "The desktop proof must use LVGL revision 85aa60d18b3d5e5588d7b247abf90198f07c8a63, "
    "which is the revision pinned by Zephyr 4.4.0."
  )
endif()

set(_furnace_hmi_lvgl_source "${FURNACE_HMI_LVGL_SOURCE_DIR}")

if(NOT _furnace_hmi_lvgl_source AND DEFINED ENV{ZEPHYR_BASE})
  get_filename_component(
    _furnace_hmi_zephyr_workspace
    "$ENV{ZEPHYR_BASE}/.."
    ABSOLUTE
  )
  set(
    _furnace_hmi_zephyr_lvgl
    "${_furnace_hmi_zephyr_workspace}/modules/lib/gui/lvgl"
  )
  if(EXISTS "${_furnace_hmi_zephyr_lvgl}/CMakeLists.txt")
    set(_furnace_hmi_lvgl_source "${_furnace_hmi_zephyr_lvgl}")
  endif()
endif()

if(NOT _furnace_hmi_lvgl_source)
  find_program(_furnace_hmi_west_executable NAMES west)
  if(_furnace_hmi_west_executable)
    execute_process(
      COMMAND "${_furnace_hmi_west_executable}" list lvgl -f "{abspath}"
      WORKING_DIRECTORY "${FURNACE_HMI_REPOSITORY_ROOT}"
      RESULT_VARIABLE _furnace_hmi_west_result
      OUTPUT_VARIABLE _furnace_hmi_west_lvgl
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(
      _furnace_hmi_west_result EQUAL 0
      AND EXISTS "${_furnace_hmi_west_lvgl}/CMakeLists.txt"
    )
      set(_furnace_hmi_lvgl_source "${_furnace_hmi_west_lvgl}")
    endif()
  endif()
endif()

if(NOT _furnace_hmi_lvgl_source)
  foreach(
    _furnace_hmi_lvgl_candidate
    IN ITEMS
      "${FURNACE_HMI_REPOSITORY_ROOT}/.deps/zephyr-workspace/modules/lib/gui/lvgl"
      "${FURNACE_HMI_REPOSITORY_ROOT}/modules/lib/gui/lvgl"
      "${FURNACE_HMI_REPOSITORY_ROOT}/../modules/lib/gui/lvgl"
  )
    if(EXISTS "${_furnace_hmi_lvgl_candidate}/CMakeLists.txt")
      set(_furnace_hmi_lvgl_source "${_furnace_hmi_lvgl_candidate}")
      break()
    endif()
  endforeach()
endif()

get_filename_component(
  _furnace_hmi_lv_conf_path
  "${CMAKE_CURRENT_LIST_DIR}/../lv_conf.h"
  ABSOLUTE
)
set(LV_BUILD_CONF_PATH "${_furnace_hmi_lv_conf_path}" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

if(_furnace_hmi_lvgl_source)
  get_filename_component(_furnace_hmi_lvgl_source "${_furnace_hmi_lvgl_source}" ABSOLUTE)
  if(NOT EXISTS "${_furnace_hmi_lvgl_source}/CMakeLists.txt")
    message(
      FATAL_ERROR
      "FURNACE_HMI_LVGL_SOURCE_DIR does not contain an LVGL CMake project: "
      "${_furnace_hmi_lvgl_source}"
    )
  endif()

  find_package(Git REQUIRED)
  execute_process(
    COMMAND
      "${GIT_EXECUTABLE}"
      -c "safe.directory=${_furnace_hmi_lvgl_source}"
      -C "${_furnace_hmi_lvgl_source}"
      rev-parse HEAD
    RESULT_VARIABLE _furnace_hmi_lvgl_git_result
    OUTPUT_VARIABLE _furnace_hmi_lvgl_head
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND
      "${GIT_EXECUTABLE}"
      -c "safe.directory=${_furnace_hmi_lvgl_source}"
      -C "${_furnace_hmi_lvgl_source}"
      status --porcelain --untracked-files=all
    RESULT_VARIABLE _furnace_hmi_lvgl_status_result
    OUTPUT_VARIABLE _furnace_hmi_lvgl_status
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(
    NOT _furnace_hmi_lvgl_git_result EQUAL 0
    OR NOT _furnace_hmi_lvgl_head STREQUAL FURNACE_HMI_LVGL_REVISION
    OR NOT _furnace_hmi_lvgl_status_result EQUAL 0
    OR _furnace_hmi_lvgl_status
  )
    message(
      FATAL_ERROR
      "The local LVGL checkout is not clean and at the Zephyr 4.4.0 revision "
      "${FURNACE_HMI_LVGL_REVISION}. Set FURNACE_HMI_LVGL_SOURCE_DIR to the "
      "matching west module or enable the pinned FetchContent fallback."
    )
  endif()

  message(STATUS "Using pinned LVGL checkout: ${_furnace_hmi_lvgl_source}")
  add_subdirectory(
    "${_furnace_hmi_lvgl_source}"
    "${CMAKE_CURRENT_BINARY_DIR}/lvgl"
    EXCLUDE_FROM_ALL
  )
elseif(FURNACE_HMI_FETCH_LVGL)
  message(STATUS "Fetching pinned LVGL revision ${FURNACE_HMI_LVGL_REVISION}")
  FetchContent_Declare(
    furnace_hmi_lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG "${FURNACE_HMI_LVGL_REVISION}"
    GIT_SHALLOW FALSE
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(furnace_hmi_lvgl)
else()
  message(
    FATAL_ERROR
    "No matching LVGL checkout was found and FURNACE_HMI_FETCH_LVGL is OFF. "
    "Run west update for the lvgl module or set FURNACE_HMI_LVGL_SOURCE_DIR."
  )
endif()

if(NOT TARGET lvgl::lvgl)
  message(FATAL_ERROR "The pinned LVGL source did not define target lvgl::lvgl.")
endif()

# LVGL's SDL driver is selected in lv_conf.h. Supplying SDL2 publicly here
# gives the driver both its include directory and link dependency.
target_link_libraries(lvgl PUBLIC SDL2::SDL2)
