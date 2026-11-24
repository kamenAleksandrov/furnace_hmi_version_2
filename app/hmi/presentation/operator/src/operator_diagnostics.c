/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The texts of the controller's history entries and health (Phase 21 M21-7c;
 * plan, "M21-7c, the Device log and health"). Every entry gets a text: an
 * event this HMI does not know is named by its number, never dropped.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/ui_strings.h>

#include "operator_diagnostics.h"

/* Event IDs (furnace core/diag, enum furnace_diag_event_id; stable values). */
#define EVENT_RECORDS_LOST 1U
#define EVENT_WALL_CLOCK 2U
#define EVENT_CONTROLLER_STARTED 10U
#define EVENT_RUN 11U
#define EVENT_FAULT 12U
#define EVENT_SENSING_STATE 13U
#define EVENT_SENSOR_CHANNEL 14U
#define EVENT_LINK 15U
#define EVENT_FAULT_EVENTS_MISSED 16U
#define EVENT_MEASUREMENT_STATE 17U
#define EVENT_BOOT 30U
#define EVENT_CONFIGURATION 31U
#define EVENT_COUNTER_WRITE_FAILED 32U

#define RUN_CODES 39U
#define FAULT_TRANSITIONS 9U
#define CONFIG_ACTIONS 14U /* 13, 14: CLOCK_SET applied and refused (M21-7e) */
#define MEASUREMENT_STATES 6U
#define SENSING_STATES 7U
#define SUBSYSTEMS 10U

/* Health subsystems whose detail is a count (controller interface, 20d-4). */
#define SUBSYSTEM_CONTROL_LOOP 1U
#define SUBSYSTEM_RECORDER 6U
#define SUBSYSTEM_EVENT_LOG 9U

static const char *text_of(furnace_hmi_ui_string_id_t id)
{
    return furnace_hmi_ui_string(id);
}

/* Parameter index's value, 0 when the entry has fewer. */
static uint32_t param(const furnace_hmi_controller_history_entry_t *e, uint8_t index)
{
    return (index < e->param_count) ? e->param_values[index] : 0U;
}

static bool history_alarm(const furnace_hmi_controller_history_entry_t *e)
{
    return e->category == OPERATOR_DIAG_CATEGORY_FAULT || e->category == OPERATOR_DIAG_CATEGORY_SENSOR ||
           e->category == OPERATOR_DIAG_CATEGORY_ACTUATOR || e->severity >= OPERATOR_DIAG_SEVERITY_ERROR;
}

bool operator_history_listed(const furnace_hmi_controller_history_entry_t *e, operator_history_group_t group)
{
    const bool run = e->category == OPERATOR_DIAG_CATEGORY_RUN;

    switch (group) {
    case OPERATOR_HISTORY_PROGRAMS:
        return run;
    case OPERATOR_HISTORY_ALARMS:
        return history_alarm(e);
    case OPERATOR_HISTORY_SYSTEM:
        return !run && !history_alarm(e);
    default:
        return false;
    }
}

/* Run event codes whose stage is meaningful (furnace core/run, enum furnace_run_event_code). */
static bool run_code_names_stage(uint32_t code)
{
    return (code >= 2U && code <= 15U) || code == 22U || code == 29U || code == 30U;
}

static void run_text(const furnace_hmi_controller_history_entry_t *e, char *buffer, size_t capacity)
{
    static const furnace_hmi_ui_string_id_t codes[RUN_CODES] = {
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_1,  FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_2,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_3,  FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_4,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_5,  FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_6,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_7,  FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_8,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_9,  FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_10,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_11, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_12,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_13, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_14,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_15, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_16,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_17, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_18,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_19, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_20,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_21, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_22,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_23, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_24,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_25, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_26,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_27, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_28,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_29, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_30,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_31, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_32,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_33, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_34,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_35, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_36,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_37, FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_38,
        FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_39,
    };
    const uint32_t code = param(e, 0U);
    const uint32_t run = param(e, 3U);
    char unknown[32];
    const char *what;

    if (code >= 1U && code <= RUN_CODES) {
        what = text_of(codes[code - 1U]);
    } else {
        (void)snprintf(unknown, sizeof(unknown), text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_STATE_FORMAT),
                       (unsigned int)code);
        what = unknown;
    }
    if (run_code_names_stage(code)) {
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_STAGE_FORMAT),
                       (unsigned int)run, (unsigned int)(param(e, 1U) + 1U), what);
    } else {
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_RUN_FORMAT),
                       (unsigned int)run, what);
    }
}

/* "<format>" with a table text for code 1..count, or "state <code>". */
static void coded_text(furnace_hmi_ui_string_id_t format, const furnace_hmi_ui_string_id_t *table,
                       uint32_t count, uint32_t code, char *buffer, size_t capacity)
{
    char state[32];
    const char *what = state;

    if (code >= 1U && code <= count) {
        what = text_of(table[code - 1U]);
    } else {
        (void)snprintf(state, sizeof(state), text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_STATE_FORMAT),
                       (unsigned int)code);
    }
    (void)snprintf(buffer, capacity, text_of(format), what);
}

static void fault_text(const furnace_hmi_controller_history_entry_t *e, char *buffer,
                       size_t capacity)
{
    static const furnace_hmi_ui_string_id_t transitions[FAULT_TRANSITIONS] = {
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_1,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_2,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_3,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_4,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_5,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_6,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_7,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_8,
        FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_9,
    };
    char name[64];
    char state[32];
    const uint32_t transition = param(e, 3U);
    const char *what = state;

    if (transition >= 1U && transition <= FAULT_TRANSITIONS) {
        what = text_of(transitions[transition - 1U]);
    } else {
        (void)snprintf(state, sizeof(state), text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_STATE_FORMAT),
                       (unsigned int)transition);
    }
    (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_FORMAT),
                   operator_fault_title((uint8_t)param(e, 0U), name, sizeof(name)), what);
}

static void config_text(const furnace_hmi_controller_history_entry_t *e, char *buffer,
                        size_t capacity)
{
    static const furnace_hmi_ui_string_id_t actions[CONFIG_ACTIONS] = {
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_1,  FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_2,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_3,  FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_4,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_5,  FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_6,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_7,  FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_8,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_9,  FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_10,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_11, FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_12,
        FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_13, FURNACE_HMI_UI_STRING_CTRL_LOG_CONFIG_14,
    };
    const uint32_t action = param(e, 0U);

    if (action >= 1U && action <= CONFIG_ACTIONS) {
        (void)snprintf(buffer, capacity, "%s", text_of(actions[action - 1U]));
    } else {
        char severity[32];

        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_UNKNOWN_FORMAT),
                       (unsigned int)e->event_id, operator_severity_text(e->severity, severity, sizeof(severity)));
    }
}

void operator_history_text(const furnace_hmi_controller_history_entry_t *e, char *buffer,
                           size_t capacity)
{
    static const furnace_hmi_ui_string_id_t measurement[MEASUREMENT_STATES] = {
        FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_1, FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_2,
        FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_3, FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_4,
        FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_5, FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_6,
    };
    static const furnace_hmi_ui_string_id_t sensing[SENSING_STATES] = {
        FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_1, FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_2,
        FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_3, FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_4,
        FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_5, FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_6,
        FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_7,
    };

    switch (e->event_id) {
    case EVENT_RUN:
        run_text(e, buffer, capacity);
        return;
    case EVENT_FAULT:
        fault_text(e, buffer, capacity);
        return;
    case EVENT_CONFIGURATION:
        config_text(e, buffer, capacity);
        return;
    case EVENT_MEASUREMENT_STATE:
        coded_text(FURNACE_HMI_UI_STRING_CTRL_LOG_MEASUREMENT_FORMAT, measurement, MEASUREMENT_STATES,
                   param(e, 0U), buffer, capacity);
        return;
    case EVENT_SENSING_STATE:
        coded_text(FURNACE_HMI_UI_STRING_CTRL_LOG_SENSING_FORMAT, sensing, SENSING_STATES,
                   param(e, 0U), buffer, capacity);
        return;
    case EVENT_SENSOR_CHANNEL:
        (void)snprintf(buffer, capacity,
                       text_of(param(e, 1U) != 0U ? FURNACE_HMI_UI_STRING_CTRL_LOG_SENSOR_FAULT_FORMAT
                                                  : FURNACE_HMI_UI_STRING_CTRL_LOG_SENSOR_OK_FORMAT),
                       (unsigned int)param(e, 0U));
        return;
    case EVENT_LINK:
        (void)snprintf(buffer, capacity, "%s",
                       text_of(param(e, 0U) != 0U ? FURNACE_HMI_UI_STRING_CTRL_LOG_LINK_UP
                                                  : FURNACE_HMI_UI_STRING_CTRL_LOG_LINK_DOWN));
        return;
    case EVENT_RECORDS_LOST:
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_RECORDS_LOST_FORMAT),
                       (unsigned int)param(e, 1U));
        return;
    case EVENT_FAULT_EVENTS_MISSED:
        (void)snprintf(buffer, capacity,
                       text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_EVENTS_MISSED_FORMAT),
                       (unsigned int)param(e, 0U));
        return;
    case EVENT_WALL_CLOCK:
        (void)snprintf(buffer, capacity, "%s", text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_WALL_CLOCK));
        return;
    case EVENT_CONTROLLER_STARTED:
        (void)snprintf(buffer, capacity, "%s",
                       text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_CONTROLLER_STARTED));
        return;
    case EVENT_BOOT:
        /* WARNING when the reset cause is known and unusual (furnace ADR-0017). */
        (void)snprintf(buffer, capacity,
                       text_of(e->severity >= OPERATOR_DIAG_SEVERITY_WARNING
                                   ? FURNACE_HMI_UI_STRING_CTRL_LOG_BOOT_RESET_FORMAT
                                   : FURNACE_HMI_UI_STRING_CTRL_LOG_BOOT_FORMAT),
                       (unsigned int)param(e, 0U));
        return;
    case EVENT_COUNTER_WRITE_FAILED:
        (void)snprintf(buffer, capacity, "%s",
                       text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_COUNTER_WRITE_FAILED));
        return;
    default: {
        char severity[32];

        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_UNKNOWN_FORMAT),
                       (unsigned int)e->event_id, operator_severity_text(e->severity, severity, sizeof(severity)));
        return;
    }
    }
}

const char *operator_severity_text(uint8_t severity, char *buffer, size_t capacity)
{
    static const furnace_hmi_ui_string_id_t severities[OPERATOR_DIAG_SEVERITY_COUNT] = {
        FURNACE_HMI_UI_STRING_CTRL_SEVERITY_0, FURNACE_HMI_UI_STRING_CTRL_SEVERITY_1,
        FURNACE_HMI_UI_STRING_CTRL_SEVERITY_2, FURNACE_HMI_UI_STRING_CTRL_SEVERITY_3,
    };

    if (severity < OPERATOR_DIAG_SEVERITY_COUNT) {
        return text_of(severities[severity]);
    }
    (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_SEVERITY_FORMAT), (unsigned int)severity);
    return buffer;
}

void operator_history_since_start(uint64_t mono_ms, char *buffer, size_t capacity)
{
    const uint64_t seconds = mono_ms / 1000U;
    const uint64_t hours = seconds / 3600U;

    (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_CTRL_LOG_SINCE_START_FORMAT),
                   (unsigned int)(hours > UINT32_MAX ? UINT32_MAX : hours),
                   (unsigned int)((seconds / 60U) % 60U), (unsigned int)(seconds % 60U));
}

void operator_history_parameters(const furnace_hmi_controller_history_entry_t *e, char *buffer,
                                 size_t capacity)
{
    size_t used = 0U;

    buffer[0] = '\0';
    for (uint8_t i = 0U; i < e->param_count && i < FURNACE_HMI_CONTROLLER_EVENT_PARAMS; i++) {
        const int n = snprintf(buffer + used, capacity - used, "%s%u:%u", (i == 0U) ? "" : ", ",
                               (unsigned int)e->param_types[i], (unsigned int)e->param_values[i]);

        if (n < 0 || (size_t)n >= capacity - used) {
            return; /* truncated; still NUL-terminated */
        }
        used += (size_t)n;
    }
}

const char *operator_fault_title(uint8_t fault_id, char *buffer, size_t capacity)
{
    static const furnace_hmi_ui_string_id_t titles[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1,  FURNACE_HMI_UI_STRING_UI_LAB_FAULT_2,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_3,  FURNACE_HMI_UI_STRING_UI_LAB_FAULT_4,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_5,  FURNACE_HMI_UI_STRING_UI_LAB_FAULT_6,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_7,  FURNACE_HMI_UI_STRING_UI_LAB_FAULT_8,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_9,  FURNACE_HMI_UI_STRING_UI_LAB_FAULT_10,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_11, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_12,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_13, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_14,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_15, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_16,
        FURNACE_HMI_UI_STRING_UI_LAB_FAULT_17, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_18,
    };

    if (fault_id >= 1U && fault_id <= sizeof(titles) / sizeof(titles[0])) {
        return text_of(titles[fault_id - 1U]);
    }
    (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_TITLE_FORMAT),
                   (unsigned int)fault_id);
    return buffer;
}

const char *operator_subsystem_name(uint8_t subsystem, char *buffer, size_t capacity)
{
    static const furnace_hmi_ui_string_id_t names[SUBSYSTEMS] = {
        FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_1, FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_2,
        FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_3, FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_4,
        FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_5, FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_6,
        FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_7, FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_8,
        FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_9, FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_10,
    };

    if (subsystem >= 1U && subsystem <= SUBSYSTEMS) {
        return text_of(names[subsystem - 1U]);
    }
    (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_FORMAT),
                   (unsigned int)subsystem);
    return buffer;
}

void operator_subsystem_detail(const furnace_hmi_controller_subsystem_t *s, char *buffer,
                               size_t capacity)
{
    if (s->subsystem == SUBSYSTEM_CONTROL_LOOP) {
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_HEALTH_DETAIL_SKIPPED_FORMAT),
                       (unsigned int)s->detail);
    } else if (s->subsystem == SUBSYSTEM_RECORDER || s->subsystem == SUBSYSTEM_EVENT_LOG) {
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_HEALTH_DETAIL_LOST_FORMAT),
                       (unsigned int)s->detail);
    } else {
        (void)snprintf(buffer, capacity, text_of(FURNACE_HMI_UI_STRING_HEALTH_DETAIL_FLAGS_FORMAT),
                       (unsigned int)s->detail);
    }
}
