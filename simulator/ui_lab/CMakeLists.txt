# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

project(furnace_hmi_ui_lab VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

get_filename_component(FURNACE_HMI_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${FURNACE_HMI_REPOSITORY_ROOT}/simulator/desktop/cmake")
include(ResolveSDL2)
include(ResolveLvgl)

if(NOT TARGET furnace_hmi_foundation)
  add_subdirectory(
    "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/foundation"
    "${CMAKE_CURRENT_BINARY_DIR}/foundation"
  )
endif()
if(NOT TARGET furnace_hmi_model)
  add_subdirectory(
    "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/model"
    "${CMAKE_CURRENT_BINARY_DIR}/model"
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
add_subdirectory(
  "${FURNACE_HMI_REPOSITORY_ROOT}/app/hmi/presentation/dashboard"
  "${CMAKE_CURRENT_BINARY_DIR}/dashboard_view"
)
add_subdirectory(
  "${FURNACE_HMI_REPOSITORY_ROOT}/simulator/semantic_controller"
  "${CMAKE_CURRENT_BINARY_DIR}/semantic_controller"
)

add_executable(
  furnace_hmi_ui_lab
  src/main.c
  src/ui_lab_explorer.c
)
target_compile_definitions(
  furnace_hmi_ui_lab
  PRIVATE
    SDL_MAIN_HANDLED
    FURNACE_HMI_DESKTOP_WIDTH=800
    FURNACE_HMI_DESKTOP_HEIGHT=480
)
target_link_libraries(furnace_hmi_ui_lab
  PRIVATE
    furnace_hmi::foundation
    furnace_hmi::model
    furnace_hmi::ui_strings
    furnace_hmi::state_availability_view
    furnace_hmi::dashboard_view
    furnace_hmi::simulated_controller
    lvgl::lvgl
    SDL2::SDL2
)

if(MSVC)
  target_compile_options(furnace_hmi_ui_lab PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_ui_lab PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(WIN32)
  get_target_property(_furnace_hmi_sdl2_type SDL2::SDL2 TYPE)
  if(_furnace_hmi_sdl2_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(
      TARGET furnace_hmi_ui_lab
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "$<TARGET_FILE:SDL2::SDL2>"
              "$<TARGET_FILE_DIR:furnace_hmi_ui_lab>"
      COMMENT "Copying the SDL2 runtime beside the UI lab"
      VERBATIM
    )
  endif()
endif()

if(BUILD_TESTING)
  add_test(NAME desktop.ui_lab_smoke COMMAND furnace_hmi_ui_lab --smoke-test)
  set_tests_properties(
    desktop.ui_lab_smoke
    PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=dummy"
      TIMEOUT 15
  )
endif()
