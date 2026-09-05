/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_STATE_AVAILABILITY_H
#define FURNACE_HMI_STATE_AVAILABILITY_H

#include <furnace_hmi/hmi_state_availability.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Immutable plain-data input consumed by the GUI owner while rendering. */
typedef struct {
    furnace_hmi_state_availability_t availability;
} furnace_hmi_state_availability_view_state_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_STATE_AVAILABILITY_H */
