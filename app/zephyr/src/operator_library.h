/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_ZEPHYR_OPERATOR_LIBRARY_H
#define FURNACE_HMI_ZEPHYR_OPERATOR_LIBRARY_H

#include <furnace_hmi/controller_library.h>
#include <furnace_hmi/controller_observation.h>
#include <furnace_hmi/operator_screens.h>

/*
 * The controller's library as the operator screens show it (Phase 21 M21-7b):
 * stage kinds and units of the screens, the controller's identity and name. A
 * value the screens cannot show exactly marks the program inexact (not edited
 * there), never rounded silently into a later save.
 */
void furnace_hmi_operator_library_from_controller(const furnace_hmi_controller_library_t *library,
                                                  const furnace_hmi_operator_program_limits_t *limits,
                                                  const furnace_hmi_operator_planning_reference_t *reference,
                                                  furnace_hmi_operator_library_t *out);

/*
 * The controller's program limits and planning reference for the screens
 * (M21-7d): targets rounded inward to whole degrees, nothing known without a
 * configuration or before the controller reported it.
 */
void furnace_hmi_operator_limits_from_controller(const furnace_hmi_controller_observation_t *observation,
                                                 furnace_hmi_operator_program_limits_t *limits,
                                                 furnace_hmi_operator_planning_reference_t *reference);

#endif /* FURNACE_HMI_ZEPHYR_OPERATOR_LIBRARY_H */
