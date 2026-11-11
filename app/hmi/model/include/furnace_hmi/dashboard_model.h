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
 * Controller-reported phase of an active Program run. Automatic cooling follows
 * curing and is controller-owned; NONE is the safe default when unknown.
 */
typedef enum {
    FURNACE_HMI_RUN_PHASE_NONE = 0,
    FURNACE_HMI_RUN_PHASE_CURING,
    FURNACE_HMI_RUN_PHASE_COOLING,
    FURNACE_HMI_RUN_PHASE_COUNT,
} furnace_hmi_run_phase_t;

/**
 * Category of the fault the controller reports for the current or last run.
 * HMI representation only: the controller owns detection, the run outcome,
 * acknowledgement and recovery, and no wire schema is implied. NONE is the
 * safe default and also covers faults the HMI cannot classify.
 */
typedef enum {
    FURNACE_HMI_FAULT_KIND_NONE = 0,
    FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE,
    FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED,
    FURNACE_HMI_FAULT_KIND_DOOR_OPEN,
    FURNACE_HMI_FAULT_KIND_SENSOR,
    FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED,
    FURNACE_HMI_FAULT_KIND_COUNT,
} furnace_hmi_fault_kind_t;

/**
 * Kind of the current program stage, as the controller reports it. HMI
 * representation only (no wire schema); UNKNOWN is the safe default, and the
 * presentation then shows the controller's stage name instead.
 */
typedef enum {
    FURNACE_HMI_STAGE_KIND_UNKNOWN = 0,
    FURNACE_HMI_STAGE_KIND_HEATING,
    FURNACE_HMI_STAGE_KIND_HOLD,
    FURNACE_HMI_STAGE_KIND_COUNT,
} furnace_hmi_stage_kind_t;

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

/**
 * What became of an operator command (Phase 21 M21-5). Only the controller's
 * result makes it ACCEPTED or REJECTED; sent is never done.
 */
typedef enum {
    FURNACE_HMI_FEEDBACK_NONE = 0,
    /** Sent, no result yet. */
    FURNACE_HMI_FEEDBACK_PENDING,
    FURNACE_HMI_FEEDBACK_ACCEPTED,
    /** The controller refused it; reason is its run-command or fault reply code. */
    FURNACE_HMI_FEEDBACK_REJECTED,
    /** The controller's service refused it before the controller saw it; reason is its code. */
    FURNACE_HMI_FEEDBACK_REFUSED,
    /** Not known whether it ran; the state is the controller's status. reason: the cause. */
    FURNACE_HMI_FEEDBACK_UNRESOLVED,
    /** The HMI did not send it (link not current, busy, superseded by Stop); reason: why. */
    FURNACE_HMI_FEEDBACK_NOT_SENT,
    FURNACE_HMI_FEEDBACK_COUNT,
} furnace_hmi_feedback_state_t;

/** Why the HMI did not send a command (reason of FURNACE_HMI_FEEDBACK_NOT_SENT). */
typedef enum {
    FURNACE_HMI_NOT_SENT_LINK = 1,
    FURNACE_HMI_NOT_SENT_BUSY,
    FURNACE_HMI_NOT_SENT_INVALID,
    FURNACE_HMI_NOT_SENT_SUPERSEDED_BY_STOP,
} furnace_hmi_not_sent_t;

typedef struct {
    /** The number the host gave the command when it was handed over (0: none). */
    uint32_t submission;
    furnace_hmi_feedback_state_t state;
    uint8_t reason;
} furnace_hmi_command_feedback_t;

/** How a fault entry may end, as the controller reports it (ADR-0018). */
typedef enum {
    FURNACE_HMI_FAULT_RECOVERY_UNKNOWN = 0,
    /** A warning: nothing to recover. */
    FURNACE_HMI_FAULT_RECOVERY_NOT_REQUIRED,
    /** Recoverable, its condition still active. */
    FURNACE_HMI_FAULT_RECOVERY_AWAIT_CLEAR,
    /** Recoverable and cleared: Resume after revalidation. */
    FURNACE_HMI_FAULT_RECOVERY_RESUME,
    /** A latched fault the operator may ask the controller to recover. */
    FURNACE_HMI_FAULT_RECOVERY_OPERATOR,
    /** A protective trip: reset outside software. */
    FURNACE_HMI_FAULT_RECOVERY_PROTECTIVE_EXTERNAL,
    FURNACE_HMI_FAULT_RECOVERY_COUNT,
} furnace_hmi_fault_recovery_t;

#define FURNACE_HMI_DASHBOARD_MAX_FAULTS 12U

/** One controller fault entry; fault_id and source name it in commands. */
typedef struct {
    uint8_t fault_id;
    uint8_t source;
    bool active;
    bool latched;
    bool acknowledged;
    bool continuable;
    furnace_hmi_fault_recovery_t recovery;
} furnace_hmi_fault_entry_t;

typedef struct {
    uint32_t elapsed_seconds;
    int32_t planned_temperature_c;
    int32_t measured_temperature_c;
    furnace_hmi_value_validity_t measured_validity;
    /** The planned value belongs to the estimated automatic-cooling forecast. */
    bool planned_cooling;
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
    furnace_hmi_run_phase_t run_phase;
    furnace_hmi_fault_kind_t fault_kind;
    /**
     * The controller reports the run as stopped by the operator (run_state
     * STOPPED); it then cools. False when unknown. The UI lab's simulation
     * leaves it false and tracks its own Stop.
     */
    bool run_stopped;
    /** The controller awaits the operator's decision after a quality fault (AWAITING_DECISION). */
    bool awaiting_decision;
    /**
     * The controller's navigation permission (interface: Lifecycle). Known and
     * false: the screens stay on Running. UNAVAILABLE where no controller
     * reports it; the UI lab's simulation keeps its own containment.
     */
    furnace_hmi_bool_value_t navigation_permitted;

    uint32_t program_id;
    uint32_t program_revision;
    char program_name[FURNACE_HMI_PROGRAM_NAME_CAPACITY];
    uint8_t stage_index;
    uint8_t stage_count;
    char stage_name[FURNACE_HMI_STAGE_NAME_CAPACITY];
    furnace_hmi_stage_kind_t stage_kind;

    furnace_hmi_i32_value_t current_temperature_c;
    furnace_hmi_i32_value_t target_temperature_c;
    furnace_hmi_i32_value_t target_delta_c_per_minute;
    furnace_hmi_i32_value_t elapsed_seconds;
    furnace_hmi_i32_value_t remaining_seconds;
    /** Time since automatic cooling began; unavailable outside cooling. */
    furnace_hmi_i32_value_t cooling_elapsed_seconds;
    /** Estimated cooling time left; an estimate never determines completion. */
    furnace_hmi_i32_value_t cooling_remaining_seconds;
    furnace_hmi_i32_value_t heater_demand_percent;
    furnace_hmi_i32_value_t estimated_power_kw_x100;
    furnace_hmi_i32_value_t fan_rpm;
    furnace_hmi_i32_value_t service_time_hours;
    furnace_hmi_i32_value_t service_due_at_hours;
    furnace_hmi_i32_value_t service_forecast_completion_hours;
    furnace_hmi_service_run_gate_t service_run_gate;
    furnace_hmi_bool_value_t door_closed;
    furnace_hmi_bool_value_t fan_running;

    /** The controller's run or Manual session ID; commands name the run they mean. */
    uint32_t run_id;
    /** A Manual session: its requested target and rate, and its time left. */
    furnace_hmi_i32_value_t manual_target_c;
    furnace_hmi_i32_value_t manual_rate_tenths_c_per_minute;
    furnace_hmi_i32_value_t manual_remaining_seconds;
    /** The controller's longest Manual session; a Manual Start asks for it (owner 2026-11-11). */
    furnace_hmi_i32_value_t manual_max_minutes;
    /** The controller can record faults for a Start (store readiness READY, FAULT-12). */
    furnace_hmi_bool_value_t start_ready;
    /** The operator's latest ordinary command and latest Stop. */
    furnace_hmi_command_feedback_t command_feedback;
    furnace_hmi_command_feedback_t stop_feedback;

    /**
     * The controller's fault entries (M21-5c). Unknown until the controller's
     * list arrives; at most FURNACE_HMI_DASHBOARD_MAX_FAULTS of fault_total.
     */
    bool faults_known;
    uint8_t fault_count;
    uint8_t fault_total;
    furnace_hmi_fault_entry_t faults[FURNACE_HMI_DASHBOARD_MAX_FAULTS];
    /** The controller fault behind fault_kind (0 none). */
    uint8_t fault_id;
    /** Awaiting a decision: the time left, and the fault it is about (0 unknown). */
    furnace_hmi_i32_value_t decision_remaining_seconds;
    uint8_t decision_fault_id;

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
