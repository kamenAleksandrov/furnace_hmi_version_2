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
    FURNACE_HMI_SIM_SCENARIO_COOLING,
    FURNACE_HMI_SIM_SCENARIO_OVER_TEMPERATURE,
    FURNACE_HMI_SIM_SCENARIO_HEATING_SLOW,
    FURNACE_HMI_SIM_SCENARIO_SENSOR_FAULT,
    FURNACE_HMI_SIM_SCENARIO_POWER_LOSS,
    FURNACE_HMI_SIM_SCENARIO_COUNT,
} furnace_hmi_sim_scenario_t;

#define FURNACE_HMI_SIM_MAX_STAGES 8U

/* One authored stage of a program the simulator runs: Heating ramps from the
 * previous target to `target_c` over `minutes`; Hold keeps the previous
 * target for `minutes`. */
typedef struct {
    bool hold;
    int32_t target_c;
    int32_t rate_tenths_c_per_minute;
    uint32_t minutes;
} furnace_hmi_sim_stage_t;

typedef struct {
    char name[FURNACE_HMI_PROGRAM_NAME_CAPACITY];
    uint8_t stage_count;
    furnace_hmi_sim_stage_t stages[FURNACE_HMI_SIM_MAX_STAGES];
} furnace_hmi_sim_program_t;

typedef struct {
    furnace_hmi_sim_scenario_t scenario;
    uint32_t virtual_elapsed_seconds;
    furnace_hmi_dashboard_state_t state;
    /* A program started on request (furnace_hmi_simulator_start_program):
     * the controller follows it from start through automatic cooling. */
    bool program_run;
    furnace_hmi_sim_program_t program;
    uint32_t run_seconds;
    uint32_t cooling_seconds;
    uint32_t next_program_id;
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

/**
 * Start a program, as the controller would on an accepted start request.
 * Rejected (false, state unchanged) without a current connection, while a run
 * is active, or for a program without stages or outside the furnace limits.
 */
bool furnace_hmi_simulator_start_program(
    furnace_hmi_simulator_t *simulator,
    const furnace_hmi_sim_program_t *program
);

/** Pause or resume the active run; false when there is none to change. */
bool furnace_hmi_simulator_set_paused(furnace_hmi_simulator_t *simulator, bool paused);

/**
 * Operator Stop. A started program is cut where it stands and cools
 * automatically (already cooling: unchanged). A scenario run returns to idle.
 */
bool furnace_hmi_simulator_stop(furnace_hmi_simulator_t *simulator);

/** Close a run (stopped, cooling, completed or faulted) and return to idle. */
bool furnace_hmi_simulator_close_run(furnace_hmi_simulator_t *simulator);

const furnace_hmi_dashboard_state_t *furnace_hmi_simulator_state(
    const furnace_hmi_simulator_t *simulator
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_SIMULATED_CONTROLLER_H */
