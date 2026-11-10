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
    case RUN_AWAITING_DECISION: /* the model has no decision state (M21-5) */
        s->run_state = FURNACE_HMI_RUN_STATE_PAUSED;
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
        return;
    }
    s->run_phase = (o->cooling == COOLING_ACTIVE) ? FURNACE_HMI_RUN_PHASE_COOLING
                   : executing                    ? FURNACE_HMI_RUN_PHASE_CURING
                                                  : FURNACE_HMI_RUN_PHASE_NONE;
}

void furnace_hmi_controller_dashboard_state(const furnace_hmi_controller_observation_t *o,
                                            furnace_hmi_dashboard_state_t *s)
{
    furnace_hmi_value_validity_t validity;

    furnace_hmi_dashboard_state_initialize(s);
    s->availability = o->availability;
    s->controller_session_epoch = o->controller_session_changes;
    if (!o->status_valid || o->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return;
    }
    validity = (o->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT)
                   ? FURNACE_HMI_VALUE_VALIDITY_CURRENT
                   : FURNACE_HMI_VALUE_VALIDITY_STALE;
    s->state_revision = o->status_sequence;

    map_run(o, s);
    if (s->mode != FURNACE_HMI_MACHINE_MODE_IDLE) {
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
