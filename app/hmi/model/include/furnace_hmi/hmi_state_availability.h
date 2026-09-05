/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_HMI_STATE_AVAILABILITY_H
#define FURNACE_HMI_HMI_STATE_AVAILABILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/** Validity of controller-observed state, independent of transport status. */
typedef enum {
    FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE = 0,
    FURNACE_HMI_STATE_AVAILABILITY_STALE,
    FURNACE_HMI_STATE_AVAILABILITY_CURRENT,
    FURNACE_HMI_STATE_AVAILABILITY_COUNT
} furnace_hmi_state_availability_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_HMI_STATE_AVAILABILITY_H */
