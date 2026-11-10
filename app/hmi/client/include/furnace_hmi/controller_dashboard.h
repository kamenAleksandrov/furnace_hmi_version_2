/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_DASHBOARD_H
#define FURNACE_HMI_CONTROLLER_DASHBOARD_H

#include <furnace_hmi/controller_observation.h>
#include <furnace_hmi/dashboard_model.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The operator screens' status model from the controller observation (Phase 21
 * M21-2b). Only what the controller reported is mapped; everything else stays
 * unavailable (furnace_hmi_dashboard_state_initialize), and no history, no
 * graph sample and no fault category is made up.
 *
 * - availability: the observation's. Values carry CURRENT only while it is
 *   CURRENT, STALE while it is STALE; the temperature also takes the
 *   controller's own validity (furnace_hmi_controller_temperature_validity).
 * - mode: by session_kind (PROGRAM 1, MANUAL 2) for any session that is not
 *   IDLE; otherwise IDLE.
 * - run_state: RUNNING, PAUSED, COMPLETED -> COMPLETE, STOPPED -> COMPLETE with
 *   run_stopped, FAULTED -> FAULT; AWAITING_DECISION -> PAUSED (the model has
 *   no decision state; the decision workflow is M21-5). Unknown codes -> IDLE.
 *   Whenever the controller reports execution active, the result is RUNNING or
 *   PAUSED, so the screens keep Running authoritative (interface: Lifecycle).
 * - run_phase: COOLING while cooling is ACTIVE; CURING while executing.
 * - Temperatures in whole degrees Celsius, rounded from 0.1 C.
 * - door and fan from physical feedback only; NOT_AVAILABLE stays unavailable.
 */
void furnace_hmi_controller_dashboard_state(
    const furnace_hmi_controller_observation_t *observation,
    furnace_hmi_dashboard_state_t *state
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_DASHBOARD_H */
