/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_LIBRARY_H
#define FURNACE_HMI_CONTROLLER_LIBRARY_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/controller_command.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Programs the HMI holds; a larger library is reported as too large (plan, M21-7 design). */
#define FURNACE_HMI_CONTROLLER_MAX_PROGRAMS 12U
/** A name is at most this many code points (controller interface: PROGRAM_DATA). */
#define FURNACE_HMI_CONTROLLER_NAME_MAX_CODE_POINTS 31U

/** Where the HMI's copy of the controller's library stands. */
typedef enum {
    /** No complete listing yet in this controller session. */
    FURNACE_HMI_LIBRARY_UNKNOWN = 0,
    /** A complete listing, every program read, at library_revision. */
    FURNACE_HMI_LIBRARY_CURRENT,
    /** The controller serves no library (PROGRAM_LIST or PROGRAM_DATA STORAGE_UNAVAILABLE). */
    FURNACE_HMI_LIBRARY_UNAVAILABLE,
    /** More programs than FURNACE_HMI_CONTROLLER_MAX_PROGRAMS: none are shown. */
    FURNACE_HMI_LIBRARY_TOO_LARGE,
    FURNACE_HMI_LIBRARY_COUNT,
} furnace_hmi_library_state_t;

/** One library program as the controller holds it; codes and units as received. */
typedef struct {
    uint32_t program_id;
    uint32_t program_revision;
    /** The controller's UTF-8 bytes, NUL-terminated. */
    char name[FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U];
    /** Well-formed UTF-8 of at most FURNACE_HMI_CONTROLLER_NAME_MAX_CODE_POINTS code points. */
    bool name_valid;
    /** Both copies are damaged: no stages, and it cannot be read or started. */
    bool damaged;
    int32_t max_target_decicelsius;
    uint32_t curing_estimate_s;
    int32_t planning_reference_decicelsius;
    uint8_t stage_count;
    furnace_hmi_controller_stage_t stages[FURNACE_HMI_CONTROLLER_MAX_STAGES];
} furnace_hmi_controller_program_t;

/**
 * The HMI's copy of the controller's program library, as plain data
 * (ADR-0003). Only a complete listing is ever shown; while a newer revision is
 * being read, the previous one stays with `updating` set. It belongs to the
 * controller session it was read from and is discarded when that changes.
 */
typedef struct {
    furnace_hmi_library_state_t state;
    /** The revision of the listing shown (CURRENT), or the one refused (UNAVAILABLE, TOO_LARGE). */
    uint32_t library_revision;
    /** The controller reports another revision: a new listing is being read. */
    bool updating;
    /** In listing order. */
    uint8_t count;
    furnace_hmi_controller_program_t programs[FURNACE_HMI_CONTROLLER_MAX_PROGRAMS];
    /** Listings started again because the library changed while it was read; saturating. */
    uint32_t restarts;
    /** Requests asked again after no answer; saturating. */
    uint32_t retries;
} furnace_hmi_controller_library_t;

/** True when bytes[0..length) are well-formed UTF-8 of at most max_code_points code points. */
bool furnace_hmi_controller_name_valid(const char *bytes, uint32_t length,
                                       uint32_t max_code_points);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_LIBRARY_H */
