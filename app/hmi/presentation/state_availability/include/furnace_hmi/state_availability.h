/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_STATE_AVAILABILITY_H
#define FURNACE_HMI_STATE_AVAILABILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/** Validity of the controller-observed state presented by the HMI. */
typedef enum {
    FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE = 0,
    FURNACE_HMI_STATE_AVAILABILITY_STALE,
    FURNACE_HMI_STATE_AVAILABILITY_CURRENT,
    FURNACE_HMI_STATE_AVAILABILITY_COUNT
} furnace_hmi_state_availability_t;

/** Immutable plain-data input consumed by the GUI owner while rendering. */
typedef struct {
    furnace_hmi_state_availability_t availability;
} furnace_hmi_state_availability_view_state_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_STATE_AVAILABILITY_H */
