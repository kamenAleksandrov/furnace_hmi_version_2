/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_DASHBOARD_MODEL_H
#define FURNACE_HMI_DASHBOARD_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <furnace_hmi/hmi_state_availability.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FURNACE_HMI_PROGRAM_NAME_CAPACITY 40U
#define FURNACE_HMI_STAGE_NAME_CAPACITY 32U
#define FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS 32U

/** Meaning of a controller-owned engineering value. */
typedef enum {
    FURNACE_HMI_VALUE_PROVENANCE_MEASURED = 0,
    FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
    FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED,
    FURNACE_HMI_VALUE_PROVENANCE_CALCULATED,
    FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
    FURNACE_HMI_VALUE_PROVENANCE_COUNT,
} furnace_hmi_value_provenance_t;

/** Validity of one value, independent of transport connectivity. */
typedef enum {
    FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE = 0,
    FURNACE_HMI_VALUE_VALIDITY_STALE,
    FURNACE_HMI_VALUE_VALIDITY_CURRENT,
    FURNACE_HMI_VALUE_VALIDITY_COUNT,
} furnace_hmi_value_validity_t;

typedef struct {
    int32_t value;
    furnace_hmi_value_provenance_t provenance;
    furnace_hmi_value_validity_t validity;
} furnace_hmi_i32_value_t;

typedef struct {
    bool value;
    furnace_hmi_value_provenance_t provenance;
    furnace_hmi_value_validity_t validity;
} furnace_hmi_bool_value_t;

typedef enum {
    FURNACE_HMI_MACHINE_MODE_IDLE = 0,
    FURNACE_HMI_MACHINE_MODE_PROGRAM,
    FURNACE_HMI_MACHINE_MODE_MANUAL,
    FURNACE_HMI_MACHINE_MODE_COUNT,
} furnace_hmi_machine_mode_t;

typedef enum {
    FURNACE_HMI_RUN_STATE_IDLE = 0,
    FURNACE_HMI_RUN_STATE_RUNNING,
    FURNACE_HMI_RUN_STATE_PAUSED,
    FURNACE_HMI_RUN_STATE_COMPLETE,
    FURNACE_HMI_RUN_STATE_FAULT,
    FURNACE_HMI_RUN_STATE_COUNT,
} furnace_hmi_run_state_t;

/**
 * Controller-reported result of evaluating a proposed run against its
 * maintenance/service policy. The HMI can explain this observation and can
 * later submit an acknowledgement request, but it never authorizes a run.
 */
typedef enum {
    FURNACE_HMI_SERVICE_RUN_GATE_UNKNOWN = 0,
    FURNACE_HMI_SERVICE_RUN_GATE_CLEAR,
    FURNACE_HMI_SERVICE_RUN_GATE_ACKNOWLEDGEMENT_REQUIRED,
    FURNACE_HMI_SERVICE_RUN_GATE_BLOCKED,
    FURNACE_HMI_SERVICE_RUN_GATE_COUNT,
} furnace_hmi_service_run_gate_t;

typedef struct {
    uint32_t elapsed_seconds;
    int32_t planned_temperature_c;
    int32_t measured_temperature_c;
    furnace_hmi_value_validity_t measured_validity;
} furnace_hmi_graph_sample_t;

/**
 * Bounded, immutable-at-the-view-boundary observation used by the dashboard.
 *
 * The controller/session/revision fields are included so that a future client
 * can reject stale observations before they reach presentation. The current
 * host simulator fills them with deterministic values; it does not claim to
 * implement the production protocol.
 */
typedef struct {
    furnace_hmi_state_availability_t availability;
    uint32_t controller_session_epoch;
    uint32_t state_revision;
    furnace_hmi_machine_mode_t mode;
    furnace_hmi_run_state_t run_state;

    uint32_t program_id;
    uint32_t program_revision;
    char program_name[FURNACE_HMI_PROGRAM_NAME_CAPACITY];
    uint8_t stage_index;
    uint8_t stage_count;
    char stage_name[FURNACE_HMI_STAGE_NAME_CAPACITY];

    furnace_hmi_i32_value_t current_temperature_c;
    furnace_hmi_i32_value_t target_temperature_c;
    furnace_hmi_i32_value_t target_delta_c_per_minute;
    furnace_hmi_i32_value_t elapsed_seconds;
    furnace_hmi_i32_value_t remaining_seconds;
    furnace_hmi_i32_value_t heater_demand_percent;
    furnace_hmi_i32_value_t estimated_power_kw_x100;
    furnace_hmi_i32_value_t fan_rpm;
    furnace_hmi_i32_value_t service_time_hours;
    furnace_hmi_i32_value_t service_due_at_hours;
    furnace_hmi_i32_value_t service_forecast_completion_hours;
    furnace_hmi_service_run_gate_t service_run_gate;
    furnace_hmi_bool_value_t door_closed;
    furnace_hmi_bool_value_t fan_running;

    size_t graph_sample_count;
    furnace_hmi_graph_sample_t graph_samples[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
} furnace_hmi_dashboard_state_t;

/** Clear a state to a safe unavailable baseline. */
void furnace_hmi_dashboard_state_initialize(furnace_hmi_dashboard_state_t *state);

/** Validate bounded strings, enumerations, and graph capacity. */
bool furnace_hmi_dashboard_state_is_well_formed(
    const furnace_hmi_dashboard_state_t *state
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_DASHBOARD_MODEL_H */
