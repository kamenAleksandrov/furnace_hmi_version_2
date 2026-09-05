/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_SIMULATED_CONTROLLER_H
#define FURNACE_HMI_SIMULATED_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/dashboard_model.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FURNACE_HMI_SIM_SCENARIO_DISCONNECTED = 0,
    FURNACE_HMI_SIM_SCENARIO_IDLE,
    FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL,
    FURNACE_HMI_SIM_SCENARIO_MANUAL,
    FURNACE_HMI_SIM_SCENARIO_PAUSED,
    FURNACE_HMI_SIM_SCENARIO_FAULT,
    FURNACE_HMI_SIM_SCENARIO_STALE,
    FURNACE_HMI_SIM_SCENARIO_RUNNING_OVERRUN,
    FURNACE_HMI_SIM_SCENARIO_COUNT,
} furnace_hmi_sim_scenario_t;

typedef struct {
    furnace_hmi_sim_scenario_t scenario;
    uint32_t virtual_elapsed_seconds;
    furnace_hmi_dashboard_state_t state;
} furnace_hmi_simulator_t;

const char *furnace_hmi_sim_scenario_name(furnace_hmi_sim_scenario_t scenario);

bool furnace_hmi_sim_scenario_from_name(
    const char *name,
    furnace_hmi_sim_scenario_t *scenario
);

void furnace_hmi_simulator_initialize(
    furnace_hmi_simulator_t *simulator,
    furnace_hmi_sim_scenario_t scenario
);

/** Advance deterministic virtual time; no wall-clock or platform APIs. */
void furnace_hmi_simulator_tick(
    furnace_hmi_simulator_t *simulator,
    uint32_t elapsed_seconds
);

void furnace_hmi_simulator_set_scenario(
    furnace_hmi_simulator_t *simulator,
    furnace_hmi_sim_scenario_t scenario
);

const furnace_hmi_dashboard_state_t *furnace_hmi_simulator_state(
    const furnace_hmi_simulator_t *simulator
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_SIMULATED_CONTROLLER_H */
