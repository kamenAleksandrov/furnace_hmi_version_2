/* SPDX-License-Identifier: Apache-2.0 */

/* The controller observation as the operator screens' status model (Phase 21 M21-2b). */

#include <stddef.h>
#include <string.h>

#include <furnace_hmi/controller_dashboard.h>

/* Controller codes (controller interface; wire format §7.5). */
enum {
    RUN_IDLE = 1,
    RUN_RUNNING = 2,
    RUN_PAUSED = 3,
    RUN_COMPLETED = 4,
    RUN_STOPPED = 5,
    RUN_FAULTED = 6,
    RUN_AWAITING_DECISION = 7,
};
#define SESSION_PROGRAM 1U
#define SESSION_MANUAL 2U
#define COOLING_ACTIVE 1U
#define COOLING_COMPLETE 2U
#define STAGE_HEATING 1U
#define STAGE_HOLD 2U
#define DOOR_CLOSED 1U
#define DOOR_OPEN 2U
#define FAN_NOT_SPINNING 1U
#define FAN_SPINNING 2U
#define STORE_READY 3U
/* Fault IDs and recovery codes (controller interface: Fault management). */
#define FAULT_ID_LIMIT 32U
#define FAULT_MEASUREMENT_INVALID 1U
#define FAULT_MEASUREMENT_STALE 2U
#define FAULT_SENSOR_CHANNEL 3U
#define FAULT_PROTECTIVE_TRIP 7U
#define FAULT_TARGET_WAIT 11U
#define FAULT_OVER_TEMPERATURE 16U

static uint8_t lowest_fault(uint32_t ids);

static int32_t whole_degrees(int32_t decicelsius)
{
    return (decicelsius >= 0) ? (decicelsius + 5) / 10 : (decicelsius - 5) / 10;
}

static furnace_hmi_i32_value_t i32_value(int32_t value, furnace_hmi_value_provenance_t provenance,
                                         furnace_hmi_value_validity_t validity)
{
    return (furnace_hmi_i32_value_t){.value = value, .provenance = provenance, .validity = validity};
}

static furnace_hmi_bool_value_t bool_value(bool value, furnace_hmi_value_validity_t validity)
{
    return (furnace_hmi_bool_value_t){
        .value = value, .provenance = FURNACE_HMI_VALUE_PROVENANCE_MEASURED, .validity = validity};
}

/* A copy cut at a character boundary: no half UTF-8 sequence at the end. */
static void copy_name(char *target, size_t capacity, const char *source)
{
    size_t length = strlen(source);

    if (length >= capacity) {
        length = capacity - 1U;
        while (length > 0U && ((unsigned char)source[length] & 0xC0U) == 0x80U) {
            length--;
        }
    }
    (void)memcpy(target, source, length);
    target[length] = '\0';
}

static void map_run(const furnace_hmi_controller_observation_t *o, furnace_hmi_dashboard_state_t *s)
{
    bool executing = false;

    switch (o->run_state) {
    case RUN_RUNNING:
        s->run_state = FURNACE_HMI_RUN_STATE_RUNNING;
        executing = true;
        break;
    case RUN_PAUSED:
        s->run_state = FURNACE_HMI_RUN_STATE_PAUSED;
        executing = true;
        break;
    case RUN_AWAITING_DECISION:
        /* Heating is held; the decision itself is M21-5. */
        s->run_state = FURNACE_HMI_RUN_STATE_PAUSED;
        s->awaiting_decision = true;
        executing = true;
        break;
    case RUN_COMPLETED:
        s->run_state = FURNACE_HMI_RUN_STATE_COMPLETE;
        break;
    case RUN_STOPPED:
        s->run_state = FURNACE_HMI_RUN_STATE_COMPLETE;
        s->run_stopped = true;
        break;
    case RUN_FAULTED:
        s->run_state = FURNACE_HMI_RUN_STATE_FAULT;
        break;
    case RUN_IDLE:
    default:
        s->run_state = FURNACE_HMI_RUN_STATE_IDLE;
        break;
    }
    if (s->run_state != FURNACE_HMI_RUN_STATE_IDLE) {
        s->mode = (o->session_kind == SESSION_PROGRAM) ? FURNACE_HMI_MACHINE_MODE_PROGRAM
                  : (o->session_kind == SESSION_MANUAL) ? FURNACE_HMI_MACHINE_MODE_MANUAL
                                                         : FURNACE_HMI_MACHINE_MODE_IDLE;
    }
    if (s->mode == FURNACE_HMI_MACHINE_MODE_IDLE) {
        /* No session the screens can name: nothing to present as a run. */
        s->run_state = FURNACE_HMI_RUN_STATE_IDLE;
        s->run_stopped = false;
        s->awaiting_decision = false;
        return;
    }
    s->run_phase = (o->cooling == COOLING_ACTIVE) ? FURNACE_HMI_RUN_PHASE_COOLING
                   : executing                    ? FURNACE_HMI_RUN_PHASE_CURING
                                                  : FURNACE_HMI_RUN_PHASE_NONE;
}

/*
 * The fault behind a faulted run: the protection's trip, then the firmware's
 * over-temperature, then a sensor fault, then the lowest other ID (review M4).
 */
static uint8_t leading_fault(uint32_t ids)
{
    static const uint8_t first[] = {FAULT_PROTECTIVE_TRIP, FAULT_OVER_TEMPERATURE, FAULT_SENSOR_CHANNEL,
                                    FAULT_MEASUREMENT_INVALID, FAULT_MEASUREMENT_STALE};

    for (size_t i = 0U; i < sizeof(first); i++) {
        if ((ids & (1U << first[i])) != 0U) {
            return first[i];
        }
    }
    return lowest_fault(ids);
}

/* The lowest fault ID in a set (bit per ID); 0 when empty. */
static uint8_t lowest_fault(uint32_t ids)
{
    for (uint8_t id = 1U; id < FAULT_ID_LIMIT; id++) {
        if ((ids & (1U << id)) != 0U) {
            return id;
        }
    }
    return 0U;
}

/*
 * The screens' alert kind of a controller fault (HMI-FAULT-CATEGORY). Only
 * faults that clearly match a kind map; every other one has none, and the
 * Faults dialog names it from its ID.
 */
static furnace_hmi_fault_kind_t fault_kind_of(uint8_t id)
{
    switch (id) {
    case FAULT_OVER_TEMPERATURE:
        return FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE;
    case FAULT_MEASUREMENT_INVALID:
    case FAULT_MEASUREMENT_STALE:
    case FAULT_SENSOR_CHANNEL:
        return FURNACE_HMI_FAULT_KIND_SENSOR;
    case FAULT_TARGET_WAIT:
        return FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED;
    default:
        return FURNACE_HMI_FAULT_KIND_NONE;
    }
}

static furnace_hmi_fault_recovery_t recovery_of(uint8_t code)
{
    switch (code) {
    case 1U:
        return FURNACE_HMI_FAULT_RECOVERY_NOT_REQUIRED;
    case 2U:
        return FURNACE_HMI_FAULT_RECOVERY_AWAIT_CLEAR;
    case 3U:
        return FURNACE_HMI_FAULT_RECOVERY_RESUME;
    case 5U:
        return FURNACE_HMI_FAULT_RECOVERY_PROTECTIVE_EXTERNAL;
    case 6U:
        return FURNACE_HMI_FAULT_RECOVERY_OPERATOR;
    default:
        return FURNACE_HMI_FAULT_RECOVERY_UNKNOWN; /* 4 is no longer produced */
    }
}

static void map_faults(const furnace_hmi_controller_observation_t *o, furnace_hmi_dashboard_state_t *s)
{
    if (!o->faults_valid) {
        return;
    }
    s->faults_known = true;
    s->faults_outdated = o->faults_outdated;
    s->fault_total = o->fault_count;
    s->fault_count = (o->fault_count < FURNACE_HMI_DASHBOARD_MAX_FAULTS) ? o->fault_count
                                                                       : FURNACE_HMI_DASHBOARD_MAX_FAULTS;
    for (uint8_t i = 0U; i < s->fault_count; i++) {
        const furnace_hmi_controller_fault_t *f = &o->faults[i];

        s->faults[i] = (furnace_hmi_fault_entry_t){
            .fault_id = f->fault_id, .source = f->source, .active = f->condition_active,
            .latched = f->latched, .acknowledged = f->acknowledged, .continuable = f->continuable,
            .recovery = recovery_of(f->recovery)};
    }
}

/* Submission numbers count up from 1 and may wrap: a is newer than b. */
static bool newer(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) > 0;
}

/* The newest of a slot's outcome and its local refusal. */
static furnace_hmi_command_feedback_t feedback(const furnace_hmi_command_outcome_t *outcome,
                                               const furnace_hmi_command_refusal_t *refusal)
{
    furnace_hmi_command_feedback_t f = {.submission = 0U, .state = FURNACE_HMI_FEEDBACK_NONE};

    if (refusal->reason != FURNACE_HMI_COMMAND_LOCAL_NONE &&
        (outcome->state == FURNACE_HMI_COMMAND_IDLE || newer(refusal->submission, outcome->submission))) {
        f.submission = refusal->submission;
        f.state = FURNACE_HMI_FEEDBACK_NOT_SENT;
        f.reason = (refusal->reason == FURNACE_HMI_COMMAND_LOCAL_BUSY)       ? FURNACE_HMI_NOT_SENT_BUSY
                   : (refusal->reason == FURNACE_HMI_COMMAND_LOCAL_INVALID) ? FURNACE_HMI_NOT_SENT_INVALID
                   : (refusal->reason == FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP)
                       ? FURNACE_HMI_NOT_SENT_SUPERSEDED_BY_STOP
                       : FURNACE_HMI_NOT_SENT_LINK;
        return f;
    }
    f.submission = outcome->submission;
    switch (outcome->state) {
    case FURNACE_HMI_COMMAND_PENDING:
        f.state = FURNACE_HMI_FEEDBACK_PENDING;
        break;
    case FURNACE_HMI_COMMAND_ACCEPTED:
        f.state = FURNACE_HMI_FEEDBACK_ACCEPTED;
        break;
    case FURNACE_HMI_COMMAND_REJECTED:
        f.state = FURNACE_HMI_FEEDBACK_REJECTED;
        f.reason = outcome->reason;
        break;
    case FURNACE_HMI_COMMAND_REFUSED:
        f.state = FURNACE_HMI_FEEDBACK_REFUSED;
        f.reason = outcome->reason;
        break;
    case FURNACE_HMI_COMMAND_UNRESOLVED:
        f.state = FURNACE_HMI_FEEDBACK_UNRESOLVED;
        f.reason = (uint8_t)outcome->unresolved_cause;
        break;
    default:
        f.submission = 0U;
        break;
    }
    return f;
}

void furnace_hmi_controller_dashboard_state(const furnace_hmi_controller_observation_t *o,
                                            furnace_hmi_dashboard_state_t *s)
{
    furnace_hmi_value_validity_t validity;

    furnace_hmi_dashboard_state_initialize(s);
    s->availability = o->availability;
    s->controller_session_epoch = o->controller_session_changes;
    /* An outcome matters most when the link is lost: always passed on. */
    s->command_feedback = feedback(&o->command, &o->command_refusal);
    s->stop_feedback = feedback(&o->stop, &o->stop_refusal);
    if (!o->status_valid || o->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return;
    }
    validity = (o->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT)
                   ? FURNACE_HMI_VALUE_VALIDITY_CURRENT
                   : FURNACE_HMI_VALUE_VALIDITY_STALE;
    s->state_revision = o->status_sequence;

    map_run(o, s);
    map_faults(o, s);
    if (s->run_state == FURNACE_HMI_RUN_STATE_FAULT) {
        s->fault_id = leading_fault(o->inhibit_ids);
        s->fault_kind = fault_kind_of(s->fault_id);
    }
    if (s->awaiting_decision) {
        s->decision_remaining_seconds = i32_value((int32_t)o->decision_remaining_s,
                                                  FURNACE_HMI_VALUE_PROVENANCE_MEASURED, validity);
        s->decision_fault_id = lowest_fault(o->continuable_ids);
    }
    s->navigation_permitted = (furnace_hmi_bool_value_t){
        .value = o->navigation_permitted, .provenance = FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        .validity = validity};
    s->start_ready = (furnace_hmi_bool_value_t){
        .value = o->store_readiness == STORE_READY, .provenance = FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        .validity = validity};
    if (o->capabilities_valid) {
        s->manual_max_minutes = i32_value((int32_t)o->manual_max_minutes,
                                          FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED, validity);
    }
    if (s->mode == FURNACE_HMI_MACHINE_MODE_MANUAL) {
        s->manual_target_c = i32_value(whole_degrees(o->manual_target_decicelsius),
                                       FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, validity);
        s->manual_rate_tenths_c_per_minute = i32_value(o->manual_rate_decicelsius_per_min,
                                                       FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, validity);
        s->manual_remaining_seconds = i32_value((int32_t)o->manual_remaining_s,
                                                FURNACE_HMI_VALUE_PROVENANCE_MEASURED, validity);
    }
    if (s->mode != FURNACE_HMI_MACHINE_MODE_IDLE) {
        s->run_id = o->run_id;
        s->program_id = o->program_id;
        s->program_revision = o->program_revision;
        copy_name(s->program_name, sizeof(s->program_name), o->program_name);
        s->stage_index = o->stage_index;
        s->stage_count = o->stage_count;
        s->stage_kind = (o->stage_kind == STAGE_HEATING) ? FURNACE_HMI_STAGE_KIND_HEATING
                        : (o->stage_kind == STAGE_HOLD)  ? FURNACE_HMI_STAGE_KIND_HOLD
                                                         : FURNACE_HMI_STAGE_KIND_UNKNOWN;
        s->elapsed_seconds = i32_value((int32_t)o->run_elapsed_s,
                                       FURNACE_HMI_VALUE_PROVENANCE_MEASURED, validity);
        if (o->cooling == COOLING_ACTIVE || o->cooling == COOLING_COMPLETE) {
            s->cooling_elapsed_seconds = i32_value((int32_t)o->cooling_s,
                                                   FURNACE_HMI_VALUE_PROVENANCE_MEASURED, validity);
        }
    }

    s->current_temperature_c = i32_value(whole_degrees(o->temperature_decicelsius),
                                         FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
                                         furnace_hmi_controller_temperature_validity(o));
    if (o->setpoint_valid) {
        s->target_temperature_c = i32_value(whole_degrees(o->setpoint_decicelsius),
                                            FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, validity);
    }
    if (o->door_state == DOOR_CLOSED || o->door_state == DOOR_OPEN) {
        s->door_closed = bool_value(o->door_state == DOOR_CLOSED, validity);
    }
    if (o->fan_feedback == FAN_SPINNING || o->fan_feedback == FAN_NOT_SPINNING) {
        s->fan_running = bool_value(o->fan_feedback == FAN_SPINNING, validity);
    }
}
