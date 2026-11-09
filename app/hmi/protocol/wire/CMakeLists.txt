# SPDX-License-Identifier: Apache-2.0
#
# The controller's 0.x wire codec, a pinned copy (ADR-0009). The sources under
# include/ and src/ are copied from the furnace firmware repository and never
# edited here: CONTROLLER_SYNC.json names the revision and every file's hash,
# and tools/sync_controller_wire.py checks them. This build file is the HMI's.

add_library(furnace_hmi_wire STATIC
  src/crc32.c
  src/wire_frame.c
  src/wire_messages.c
)
add_library(furnace_hmi::wire ALIAS furnace_hmi_wire)
target_include_directories(furnace_hmi_wire PUBLIC include)
target_compile_features(furnace_hmi_wire PUBLIC c_std_17)
set_target_properties(furnace_hmi_wire PROPERTIES C_EXTENSIONS OFF)

# The same warnings as the HMI's own code, so that a portability problem shows
# here first; a fix belongs in the controller repository (ADR-0009).
if(MSVC)
  target_compile_options(furnace_hmi_wire PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(furnace_hmi_wire PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
