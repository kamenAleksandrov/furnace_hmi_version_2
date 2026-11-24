/* SPDX-License-Identifier: Apache-2.0 */

/* The controller's library for the operator screens (Phase 21 M21-7b). */

#include <string.h>

#include "operator_library.h"

_Static_assert(FURNACE_HMI_OPERATOR_MAX_PROGRAMS >= FURNACE_HMI_CONTROLLER_MAX_PROGRAMS,
               "the screens hold every program of a listed library");
_Static_assert(FURNACE_HMI_OPERATOR_MAX_STAGES >= FURNACE_HMI_CONTROLLER_MAX_STAGES,
               "the screens hold every stage of a program");
_Static_assert(FURNACE_HMI_OPERATOR_PROGRAM_NAME_CAPACITY >=
                   FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U,
               "the screens hold every name");

/* A program's first ramp starts at the planning reference (PROG-12); without one, at its first
 * target, so no temperature is assumed (PROG-14). */
#define NO_REFERENCE INT32_MIN
/* SETTINGS settings_state NONE: the controller has no settings store (interface, Settings). */
#define SETTINGS_STATE_NONE 0U

/* Whole degrees within [low, high] tenths: low rounded up, high rounded down. */
static int32_t ceil_degrees(int32_t tenths)
{
    const int64_t t = tenths;
    return (int32_t)(t >= 0 ? (t + 9) / 10 : -((-t) / 10));
}

static int32_t floor_degrees(int32_t tenths)
{
    const int64_t t = tenths;
    return (int32_t)(t >= 0 ? t / 10 : -((-t + 9) / 10));
}

void furnace_hmi_operator_limits_from_controller(const furnace_hmi_controller_observation_t *o,
                                                 furnace_hmi_operator_program_limits_t *limits,
                                                 furnace_hmi_operator_planning_reference_t *reference)
{
    (void)memset(limits, 0, sizeof(*limits));
    (void)memset(reference, 0, sizeof(*reference));
    /* The controller's own consistency rule (furnace_program_limits_are_valid). */
    if (!o->capabilities_valid || !o->configured || o->max_stages == 0U ||
        o->target_min_decicelsius > o->target_max_decicelsius || o->rate_min_decicelsius_per_min < 1 ||
        o->rate_min_decicelsius_per_min > o->rate_max_decicelsius_per_min || o->curing_max_minutes == 0U ||
        (o->stage_kinds & (1U << FURNACE_HMI_CONTROLLER_STAGE_HEATING)) == 0U) {
        return;
    }
    limits->target_min_c = ceil_degrees(o->target_min_decicelsius);
    limits->target_max_c = floor_degrees(o->target_max_decicelsius);
    if (limits->target_min_c > limits->target_max_c) {
        return; /* no whole degree within the range: nothing the editors can offer */
    }
    limits->known = true;
    limits->max_stages = o->max_stages < FURNACE_HMI_OPERATOR_MAX_STAGES ? o->max_stages
                                                                         : (uint8_t)FURNACE_HMI_OPERATOR_MAX_STAGES;
    limits->hold_allowed = (o->stage_kinds & (1U << FURNACE_HMI_CONTROLLER_STAGE_HOLD)) != 0U;
    limits->rate_min_tenths = o->rate_min_decicelsius_per_min;
    limits->rate_max_tenths = o->rate_max_decicelsius_per_min;
    limits->curing_max_minutes = o->curing_max_minutes > (uint32_t)INT32_MAX ? INT32_MAX
                                                                             : (int32_t)o->curing_max_minutes;
    /* Above the maximum it is the controller's invalid configuration: none is assumed. */
    limits->recommended_curing_minutes = o->recommended_curing_minutes > o->curing_max_minutes ? 0
        : o->recommended_curing_minutes > (uint32_t)INT32_MAX ? INT32_MAX
                                                             : (int32_t)o->recommended_curing_minutes;
    limits->name_max_code_points = o->name_max_code_points;
    reference->known = o->settings_valid;
    reference->decicelsius = o->planning_reference_decicelsius;
    reference->min_decicelsius = o->planning_reference_min_decicelsius;
    reference->max_decicelsius = o->planning_reference_max_decicelsius;
    reference->change_permitted = o->status_valid && o->settings_changes_permitted;
    reference->storable = o->settings_state != SETTINGS_STATE_NONE;
}

void furnace_hmi_operator_clock_from_controller(const furnace_hmi_controller_observation_t *o, uint32_t now_ms,
                                                furnace_hmi_operator_controller_clock_t *clock)
{
    (void)memset(clock, 0, sizeof(*clock));
    if (!o->clock_valid) {
        return;
    }
    clock->reported = true;
    clock->set = o->wall_state != FURNACE_HMI_WALL_UNKNOWN;
    /* Unsigned difference: correct across a wrap of the 32-bit uptime. */
    clock->wall_ms = clock->set ? o->wall_ms + (int64_t)(uint32_t)(now_ms - o->clock_at_ms) : 0;
}

static furnace_hmi_operator_library_state_t state_of(furnace_hmi_library_state_t state)
{
    switch (state) {
    case FURNACE_HMI_LIBRARY_CURRENT:
        return FURNACE_HMI_OPERATOR_LIBRARY_CURRENT;
    case FURNACE_HMI_LIBRARY_UNAVAILABLE:
        return FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE;
    case FURNACE_HMI_LIBRARY_TOO_LARGE:
        return FURNACE_HMI_OPERATOR_LIBRARY_TOO_LARGE;
    default:
        return FURNACE_HMI_OPERATOR_LIBRARY_LOADING;
    }
}

/* One program; false when a value cannot be shown exactly (a damaged one has none to show). */
static bool program_of(const furnace_hmi_controller_program_t *in,
                       const furnace_hmi_operator_program_limits_t *limits, int32_t start,
                       furnace_hmi_operator_program_t *out)
{
    int32_t previous = start;
    bool exact = in->stage_count > 0U && in->stage_count <= FURNACE_HMI_CONTROLLER_MAX_STAGES;

    out->present = true;
    out->id = in->program_id;
    out->revision = in->program_revision;
    out->damaged = in->damaged;
    /* The controller's planning estimate is the duration shown (PROG-14). */
    out->curing_estimate_s = in->curing_estimate_s;
    (void)memcpy(out->name, in->name, sizeof(in->name));
    out->name[sizeof(out->name) - 1U] = '\0';
    if (in->damaged) {
        return true;
    }
    if (!exact) {
        return false;
    }
    out->stage_count = in->stage_count;
    for (uint8_t i = 0U; i < in->stage_count; i++) {
        const furnace_hmi_controller_stage_t *s = &in->stages[i];
        furnace_hmi_operator_program_stage_t *t = &out->stages[i];

        if (s->kind == FURNACE_HMI_CONTROLLER_STAGE_HOLD) {
            t->kind = FURNACE_HMI_OPERATOR_STAGE_HOLD;
            t->target_c = (previous == NO_REFERENCE ? 0 : previous) / 10;
            t->rate_tenths_c_per_minute = 0;
            t->minutes = (int32_t)s->duration_minutes;
            exact = exact && s->duration_minutes >= 1U &&
                    s->duration_minutes <= (uint32_t)FURNACE_HMI_OPERATOR_PROGRAM_MAX_STAGE_MINUTES;
            continue;
        }
        const int32_t target = s->target_decicelsius;
        const int32_t rate = s->rate_decicelsius_per_min;
        if (previous == NO_REFERENCE) {
            previous = target; /* no reference: the first ramp takes no time */
        }
        const int64_t rise = target > previous ? (int64_t)target - previous : (int64_t)previous - target;

        /* Exact: what the editors can show and save again unchanged, within the controller's limits. */
        exact = exact && s->kind == FURNACE_HMI_CONTROLLER_STAGE_HEATING && target % 10 == 0 &&
                (!limits->known ||
                 (target >= limits->target_min_c * 10 && target <= limits->target_max_c * 10 &&
                  rate >= limits->rate_min_tenths && rate <= limits->rate_max_tenths));
        t->kind = FURNACE_HMI_OPERATOR_STAGE_HEATING;
        /* Rounded to the nearest degree for the display only; inexact marks it. */
        t->target_c = (int32_t)(((int64_t)target + (target >= 0 ? 5 : -5)) / 10);
        t->rate_tenths_c_per_minute = rate;
        /* Minutes of the ramp at the rate, at least one (the fixtures' rule). */
        const int64_t minutes = rate > 0 ? (rise + rate - 1) / rate : 0;
        t->minutes = minutes > INT32_MAX ? INT32_MAX : (int32_t)minutes;
        if (t->minutes < 1) {
            t->minutes = 1;
        }
        previous = target;
    }
    return exact;
}

void furnace_hmi_operator_library_from_controller(const furnace_hmi_controller_library_t *library,
                                                  const furnace_hmi_operator_program_limits_t *limits,
                                                  const furnace_hmi_operator_planning_reference_t *reference,
                                                  furnace_hmi_operator_library_t *out)
{
    const int32_t start = reference->known ? reference->decicelsius : NO_REFERENCE;
    (void)memset(out, 0, sizeof(*out));
    out->state = state_of(library->state);
    if (out->state != FURNACE_HMI_OPERATOR_LIBRARY_CURRENT) {
        return;
    }
    out->updating = library->updating;
    out->count = library->count <= FURNACE_HMI_CONTROLLER_MAX_PROGRAMS
                     ? library->count
                     : (uint8_t)FURNACE_HMI_CONTROLLER_MAX_PROGRAMS;
    for (uint8_t i = 0U; i < out->count; i++) {
        out->programs[i].inexact = !program_of(&library->programs[i], limits, start, &out->programs[i]);
    }
}
