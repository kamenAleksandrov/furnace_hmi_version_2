# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

set(FURNACE_HMI_SDL2_MIN_VERSION "2.32.10")

# Prefer a toolchain-provided package (for example vcpkg with MSVC).
find_package(SDL2 ${FURNACE_HMI_SDL2_MIN_VERSION} CONFIG QUIET)

# An MSYS2/MinGW installation commonly keeps SDL2 beside the selected
# compiler. Deriving this hint avoids embedding a developer-specific path.
if(NOT TARGET SDL2::SDL2 AND MINGW)
  get_filename_component(_furnace_hmi_compiler_bin "${CMAKE_C_COMPILER}" DIRECTORY)
  get_filename_component(_furnace_hmi_compiler_prefix "${_furnace_hmi_compiler_bin}" DIRECTORY)
  find_package(
    SDL2
    ${FURNACE_HMI_SDL2_MIN_VERSION}
    CONFIG
    QUIET
    HINTS
      "${_furnace_hmi_compiler_prefix}"
      "${_furnace_hmi_compiler_prefix}/lib/cmake/SDL2"
  )
endif()

# On Unix-like hosts, distributions frequently expose SDL2 through pkg-config.
if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(
      FURNACE_HMI_SDL2
      QUIET
      IMPORTED_TARGET
      "sdl2>=${FURNACE_HMI_SDL2_MIN_VERSION}"
    )
    if(TARGET PkgConfig::FURNACE_HMI_SDL2)
      add_library(SDL2::SDL2 ALIAS PkgConfig::FURNACE_HMI_SDL2)
    endif()
  endif()
endif()

if(NOT TARGET SDL2::SDL2)
  message(
    FATAL_ERROR
    "SDL2 development files were not found. Set SDL2_DIR to an SDL2 CMake "
    "package, use a toolchain that supplies SDL2 (such as vcpkg for MSVC), "
    "or install SDL2 ${FURNACE_HMI_SDL2_MIN_VERSION} or newer for the selected "
    "compiler."
  )
endif()
