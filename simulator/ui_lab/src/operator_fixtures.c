/* SPDX-License-Identifier: Apache-2.0 */

/*
 * SIMULATION ONLY. The UI lab's controller-owned content: the delivered program
 * library, the recent programs, the controller clock, a month of event log and
 * the program limits, with the Device catalogue the lab reads from its file.
 * The shared operator screens receive it as furnace_hmi_operator_content_t
 * (Phase 21 M21-2b). Moved from the screens unchanged, so the lab shows what it
 * showed before; it is never part of the Zephyr application.
 */

#include <stdint.h>
#include <string.h>

#include <furnace_hmi/dashboard_model.h>
#include <furnace_hmi/ui_strings.h>

#include "operator_fixtures.h"

/* The delivered program library (web programCatalogue): three epoxy cure
 * schedules within the furnace limits (180 C, 3.0 C/min, 8 stages). Heating
 * ramps from the previous target at the given rate (tenths of a degree per
 * minute); its minutes follow from that. Illustrations, not datasheet values. */
typedef struct {
    furnace_hmi_ui_string_id_t name;
    uint8_t stage_count;
    furnace_hmi_operator_program_stage_t stages[FURNACE_HMI_OPERATOR_MAX_STAGES];
} operator_library_program_t;

#define OPERATOR_HEAT(target, rate, minutes) { FURNACE_HMI_OPERATOR_STAGE_HEATING, (target), (rate), (minutes) }
#define OPERATOR_HOLD(target, minutes) { FURNACE_HMI_OPERATOR_STAGE_HOLD, (target), 0, (minutes) }
static const operator_library_program_t library_programs[] = {
    /* Room-temperature laminating resin with an elevated post-cure. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_1, 4U,
      { OPERATOR_HEAT(40, 3, 50), OPERATOR_HOLD(40, 120), OPERATOR_HEAT(60, 3, 67), OPERATOR_HOLD(60, 240) } },
    /* 120 C prepreg cure with a resin-flow dwell. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_2, 4U,
      { OPERATOR_HEAT(85, 15, 40), OPERATOR_HOLD(85, 30), OPERATOR_HEAT(120, 15, 24), OPERATOR_HOLD(120, 90) } },
    /* High-Tg structural resin: 180 C cure in steps. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_3, 6U,
      { OPERATOR_HEAT(90, 10, 65), OPERATOR_HOLD(90, 60), OPERATOR_HEAT(135, 10, 45), OPERATOR_HOLD(135, 90),
        OPERATOR_HEAT(180, 5, 90), OPERATOR_HOLD(180, 120) } },
};
#undef OPERATOR_HEAT
#undef OPERATOR_HOLD
#define OPERATOR_LIBRARY_PROGRAM_COUNT (sizeof(library_programs) / sizeof(library_programs[0]))

_Static_assert(
    OPERATOR_LIBRARY_PROGRAM_COUNT < FURNACE_HMI_OPERATOR_MAX_PROGRAMS,
    "program storage must leave room for operator programs"
);


/* Web rand(): Park-Miller minimal standard generator. */
static int32_t log_rand(uint64_t *seed, uint32_t range)
{
    *seed = (*seed * 48271U) % 2147483647U;
    return (int32_t)(*seed % range);
}

static furnace_hmi_operator_log_event_t log_event(int32_t minute, uint8_t type)
{
    furnace_hmi_operator_log_event_t event;
    memset(&event, 0, sizeof(event));
    event.minute = minute;
    event.type = type;
    event.program = UINT8_MAX;
    return event;
}

#define OPERATOR_LOG_DAYS 30

/* Web LOG_FAULTS order. */
static const uint8_t log_faults[] = {
    FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE, FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED,
    FURNACE_HMI_FAULT_KIND_DOOR_OPEN, FURNACE_HMI_FAULT_KIND_SENSOR, FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED,
};

static bool log_find_task(const furnace_hmi_operator_content_t *content, const char *component_id, const char *task_id,
                          uint8_t *component, uint8_t *task)
{
    for (uint8_t part = 0U; part < content->device_catalogue.component_count; ++part) {
        const furnace_hmi_operator_device_component_t *item = &content->device_catalogue.components[part];
        for (uint8_t index = 0U; strcmp(item->id, component_id) == 0 && index < item->task_count; ++index) {
            if (strcmp(item->tasks[index].id, task_id) == 0) {
                *component = part;
                *task = index;
                return true;
            }
        }
    }
    return false;
}

/* Web buildDeviceLog: the same seed and draws in the same order. */
static void log_build(furnace_hmi_operator_content_t *content)
{
    const int32_t now = furnace_hmi_operator_clock_minute(&content->clock);
    const int32_t today = now / 1440;
    uint64_t seed = 20261006U;
    furnace_hmi_operator_log_event_t *events = content->log_events;
    size_t count = 0U;
#define OPERATOR_LOG_PUSH(event_value) do { const furnace_hmi_operator_log_event_t pushed_ = (event_value); \
        if (pushed_.minute <= now && count < FURNACE_HMI_OPERATOR_LOG_CAPACITY) { events[count++] = pushed_; } } while (0)
    for (int32_t ago = OPERATOR_LOG_DAYS - 1; ago >= 0; --ago) {
        const int32_t day = (today - ago) * 1440;
        if (log_rand(&seed, 12U) == 0) {
            OPERATOR_LOG_PUSH(log_event(day + 6 * 60 + log_rand(&seed, 60U), FURNACE_HMI_OPERATOR_LOG_DISPLAY_STARTED));
        }
        const int32_t runs = log_rand(&seed, 10U) < 3 ? 0 : (log_rand(&seed, 10U) < 7 ? 1 : 2);
        int32_t at = day + 7 * 60 + log_rand(&seed, 120U);
        for (int32_t run = 0; run < runs; ++run) {
            const uint8_t program = (uint8_t)log_rand(&seed, 3U);
            const int32_t curing = furnace_hmi_operator_program_curing_minutes(&content->programs[program]);
            const int32_t length = curing + furnace_hmi_operator_program_cooling_minutes(&content->programs[program]);
            const int32_t roll = log_rand(&seed, 20U);
            if (at + length > now) {
                break; /* no run is still going on the controller's clock */
            }
            furnace_hmi_operator_log_event_t started = log_event(at, FURNACE_HMI_OPERATOR_LOG_PROGRAM_STARTED);
            started.program = program;
            OPERATOR_LOG_PUSH(started);
            if (roll == 0) {
                const int32_t end = at + curing * (30 + log_rand(&seed, 50U)) / 100;
                furnace_hmi_operator_log_event_t alarm = log_event(end, FURNACE_HMI_OPERATOR_LOG_ALARM);
                alarm.program = program;
                alarm.fault = log_faults[log_rand(&seed, 5U)];
                OPERATOR_LOG_PUSH(alarm);
                at = end + 30;
            }
            else if (roll < 3) {
                const int32_t end = at + curing * (20 + log_rand(&seed, 60U)) / 100;
                furnace_hmi_operator_log_event_t stopped = log_event(end, FURNACE_HMI_OPERATOR_LOG_PROGRAM_STOPPED);
                stopped.program = program;
                OPERATOR_LOG_PUSH(stopped);
                at = end + 20;
            }
            else {
                furnace_hmi_operator_log_event_t completed = log_event(at + length, FURNACE_HMI_OPERATOR_LOG_PROGRAM_COMPLETED);
                completed.program = program;
                OPERATOR_LOG_PUSH(completed);
                at += length + 30;
            }
            at += 15 + log_rand(&seed, 60U);
        }
    }
    /* The controller's service reminders, a short connection loss and a settings change. */
    static const struct { int32_t ago; int32_t minute; const char *component; const char *task; } reminders[] = {
        { 4, 8 * 60 + 2, "air_filter", "filter_inspect" },
        { 11, 8 * 60 + 1, "cabinet_fan", "fan_calendar" },
    };
    for (size_t index = 0U; index < sizeof(reminders) / sizeof(reminders[0]); ++index) {
        furnace_hmi_operator_log_event_t due = log_event((today - reminders[index].ago) * 1440 + reminders[index].minute,
                                                       FURNACE_HMI_OPERATOR_LOG_SERVICE_DUE);
        if (log_find_task(content, reminders[index].component, reminders[index].task, &due.component, &due.task)) {
            OPERATOR_LOG_PUSH(due);
        }
    }
    OPERATOR_LOG_PUSH(log_event((today - 9) * 1440 + 13 * 60 + 5, FURNACE_HMI_OPERATOR_LOG_LINK_LOST));
    OPERATOR_LOG_PUSH(log_event((today - 9) * 1440 + 13 * 60 + 7, FURNACE_HMI_OPERATOR_LOG_LINK_RESTORED));
    OPERATOR_LOG_PUSH(log_event((today - 17) * 1440 + 10 * 60 + 15, FURNACE_HMI_OPERATOR_LOG_SETTINGS_SAVED));
#undef OPERATOR_LOG_PUSH
    /* Newest first; equal times keep their order (web stable sort). */
    for (size_t index = 1U; index < count; ++index) {
        const furnace_hmi_operator_log_event_t moving = events[index];
        size_t at = index;
        while (at > 0U && events[at - 1U].minute < moving.minute) {
            events[at] = events[at - 1U];
            --at;
        }
        events[at] = moving;
    }
    content->log_count = (uint16_t)count;
}

void furnace_hmi_operator_fixture_content(
    furnace_hmi_operator_content_t *content,
    const furnace_hmi_operator_device_catalogue_t *device_catalogue
)
{
    memset(content, 0, sizeof(*content));
    content->device_catalogue_known = true;
    content->device_catalogue = *device_catalogue;
    for (size_t index = 0U; index < OPERATOR_LIBRARY_PROGRAM_COUNT; ++index) {
        furnace_hmi_operator_program_t *program = &content->programs[index];
        program->present = true;
        program->library = true;
        program->name_string = (uint16_t)library_programs[index].name;
        program->stage_count = library_programs[index].stage_count;
        memcpy(program->stages, library_programs[index].stages, sizeof(program->stages));
    }
    content->library_count = (uint8_t)OPERATOR_LIBRARY_PROGRAM_COUNT;
    content->library_known = true;
    /* Web demoRecent: the programs run most recently, newest first. */
    content->recent[0] = 1U;
    content->recent[1] = 0U;
    content->recent_count = 2U;
    content->clock = (furnace_hmi_operator_clock_t){ 2026U, 9U, 14U, 14U, 32U };
    content->clock_known = true;
    content->max_rate_tenths_c_per_minute = 30;
    content->max_temperature_c = 180;
    content->default_duration_minutes = 720;
    content->limits_known = true;
    log_build(content);
    content->log_known = true;
}
