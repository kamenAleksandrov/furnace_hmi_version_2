# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.24)

project(furnace_hmi_desktop_foundation VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

set(
  FURNACE_HMI_DESKTOP_WIDTH
  "800"
  CACHE STRING
  "Desktop UI development width; this is not a production display assumption"
)
set(
  FURNACE_HMI_DESKTOP_HEIGHT
  "480"
  CACHE STRING
  "Desktop UI development height; this is not a production display assumption"
)
if(NOT FURNACE_HMI_DESKTOP_WIDTH MATCHES "^[1-9][0-9]*$"
   OR NOT FURNACE_HMI_DESKTOP_HEIGHT MATCHES "^[1-9][0-9]*$")
  message(FATAL_ERROR "Desktop proof width and height must be positive integers.")
endif()
if(FURNACE_HMI_DESKTOP_WIDTH LESS 640 OR FURNACE_HMI_DESKTOP_HEIGHT LESS 360)
  message(
    FATAL_ERROR
    "The current desktop UI development viewport requires at least 640x360."
  )
endif()

get_filename_component(FURNACE_HMI_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ResolveSDL2)
include(ResolveLvgl)

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

if(NOT TARGET furnace_hmi_state_availability_view)
  add_subdirectory(
    "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/presentation/state_availability"
    "${CMAKE_CURRENT_BINARY_DIR}/state_availability_view"
  )
endif()

add_executable(furnace_hmi_desktop src/main.c)

target_compile_definitions(
  furnace_hmi_desktop
  PRIVATE
    SDL_MAIN_HANDLED
    FURNACE_HMI_DESKTOP_WIDTH=${FURNACE_HMI_DESKTOP_WIDTH}
    FURNACE_HMI_DESKTOP_HEIGHT=${FURNACE_HMI_DESKTOP_HEIGHT}
)
target_link_libraries(furnace_hmi_desktop
  PRIVATE
    furnace_hmi::foundation
    furnace_hmi::state_availability_view
    furnace_hmi::ui_strings
    lvgl::lvgl
    SDL2::SDL2
)

if(MSVC)
  target_compile_options(furnace_hmi_desktop PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_desktop PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
  get_target_property(_furnace_hmi_sdl2_type SDL2::SDL2 TYPE)
  if(_furnace_hmi_sdl2_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(
      TARGET furnace_hmi_desktop
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "$<TARGET_FILE:SDL2::SDL2>"
              "$<TARGET_FILE_DIR:furnace_hmi_desktop>"
      COMMENT "Copying the SDL2 runtime beside the desktop proof"
      VERBATIM
    )
  endif()
endif()

if(BUILD_TESTING)
  add_test(
    NAME desktop.lvgl_sdl_smoke
    COMMAND furnace_hmi_desktop --smoke-test
  )
  add_test(
    NAME desktop.state_availability_smoke
    COMMAND furnace_hmi_desktop --presentation-smoke-test
  )
  set_tests_properties(
    desktop.lvgl_sdl_smoke
    desktop.state_availability_smoke
    PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy"
      TIMEOUT 15
  )
endif()
