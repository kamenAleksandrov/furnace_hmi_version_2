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
    /* A quality fault awaits the operator's decision (Phase 21 M21-5c). */
    FURNACE_HMI_SIM_SCENARIO_DECISION,
    FURNACE_HMI_SIM_SCENARIO_COUNT,
} furnace_hmi_sim_scenario_t;

#define FURNACE_HMI_SIM_MAX_STAGES 18U

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
    /* The run starts from start_c (a Start at a later stage of a hot furnace); otherwise from
     * the simulation's 25 C. */
    bool hot_start;
    int32_t start_c;
} furnace_hmi_sim_program_t;

/*
 * The controller's Start rules the labs simulate (furnace core/run start selection and start
 * transaction; M21-7d-2). Test values, as the furnace firmware's fixtures (T-08).
 */
typedef struct {
    /** A Hold may start within this of its target (fixture profile A: 5.0 C). */
    int32_t hold_entry_tolerance_decicelsius;
    /** The advisory duration (0: the maximum) and the maximum, in minutes. */
    uint32_t recommended_curing_minutes;
    uint32_t curing_max_minutes;
} furnace_hmi_sim_start_rules_t;

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
    /* Latched fault entries outlive the run that raised them (M21-5c). */
    uint8_t kept_fault_count;
    furnace_hmi_fault_entry_t kept_faults[FURNACE_HMI_DASHBOARD_MAX_FAULTS];
    /* The open start transaction's program (state.start_proposal is its proposal; M21-7d-2). */
    furnace_hmi_sim_program_t proposed_program;
    uint32_t next_transaction_id;
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

/*
 * A program Start, answered as the controller answers START_REQUEST (simulation only,
 * M21-7d-2). The stage the run starts at is the first one the measured temperature allows (a
 * Heating stage below its target, a Hold within the tolerance of its target); the estimate runs
 * from there. A Start at the first stage within the advisory duration runs at once (ACCEPTED);
 * one at a later stage, or longer than the advisory duration, is proposed (PROPOSED, with
 * state.start_proposal open); otherwise it is refused (REJECTED with the controller's reason:
 * a run active, no valid temperature, no eligible stage, longer than the maximum). The answer
 * is state.command_feedback under `submission`. The proposal is not re-evaluated while open.
 */
void furnace_hmi_simulator_request_start(furnace_hmi_simulator_t *simulator, uint32_t submission,
                                         const furnace_hmi_sim_program_t *program,
                                         const furnace_hmi_sim_start_rules_t *rules);

/*
 * Confirm exactly the open proposal (its transaction, revision and stage, and when it asks for
 * one an accepted duration from its estimate to the maximum): the run starts at that stage
 * from the measured temperature. Cancel closes the transaction. Answered as the controller does.
 */
void furnace_hmi_simulator_confirm_start(furnace_hmi_simulator_t *simulator, uint32_t submission,
                                         uint32_t transaction_id, uint32_t proposal_revision,
                                         uint8_t proposed_stage, uint32_t accepted_duration_s);
void furnace_hmi_simulator_cancel_start(furnace_hmi_simulator_t *simulator, uint32_t submission,
                                        uint32_t transaction_id);

/** Pause or resume the active run; false when there is none to change. */
bool furnace_hmi_simulator_set_paused(furnace_hmi_simulator_t *simulator, bool paused);

/**
 * Operator Stop. A started program is cut where it stands and cools
 * automatically (already cooling: unchanged). A scenario run returns to idle.
 */
bool furnace_hmi_simulator_stop(furnace_hmi_simulator_t *simulator);

/** Close a run (stopped, cooling, completed or faulted) and return to idle. */
bool furnace_hmi_simulator_close_run(furnace_hmi_simulator_t *simulator);

/*
 * The fault commands and the decision, as the controller would answer them
 * (simulation only, M21-5c). Acknowledge marks the entry seen; Recover removes
 * an acknowledged latched entry once no run is active; Continue releases the
 * decision and the run goes on. False when nothing changed.
 */
bool furnace_hmi_simulator_fault_acknowledge(furnace_hmi_simulator_t *simulator, uint8_t fault_id, uint8_t source);
bool furnace_hmi_simulator_fault_recover(furnace_hmi_simulator_t *simulator, uint8_t fault_id, uint8_t source);
bool furnace_hmi_simulator_continue(furnace_hmi_simulator_t *simulator);

const furnace_hmi_dashboard_state_t *furnace_hmi_simulator_state(
    const furnace_hmi_simulator_t *simulator
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_SIMULATED_CONTROLLER_H */
