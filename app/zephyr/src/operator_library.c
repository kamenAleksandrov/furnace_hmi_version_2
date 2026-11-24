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

/* The screens' programs start from room temperature (the UI lab's fixtures: 25 C). */
#define START_DECICELSIUS 250

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
static bool program_of(const furnace_hmi_controller_program_t *in, furnace_hmi_operator_program_t *out)
{
    int32_t previous = START_DECICELSIUS;
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
            t->target_c = previous / 10;
            t->rate_tenths_c_per_minute = 0;
            t->minutes = (int32_t)s->duration_minutes;
            exact = exact && s->duration_minutes >= 1U &&
                    s->duration_minutes <= (uint32_t)FURNACE_HMI_OPERATOR_PROGRAM_MAX_STAGE_MINUTES;
            continue;
        }
        const int32_t target = s->target_decicelsius;
        const int32_t rate = s->rate_decicelsius_per_min;
        const int32_t rise = target > previous ? target - previous : previous - target;

        exact = exact && s->kind == FURNACE_HMI_CONTROLLER_STAGE_HEATING && target % 10 == 0 &&
                target >= 0 && target <= FURNACE_HMI_OPERATOR_PROGRAM_MAX_TEMPERATURE_C * 10 &&
                rate >= 1 && rate <= FURNACE_HMI_OPERATOR_PROGRAM_MAX_RATE_TENTHS;
        t->kind = FURNACE_HMI_OPERATOR_STAGE_HEATING;
        /* Rounded to the nearest degree for the display only; inexact marks it. */
        t->target_c = (target + (target >= 0 ? 5 : -5)) / 10;
        t->rate_tenths_c_per_minute = rate;
        /* Minutes of the ramp at the rate, at least one (the fixtures' rule). */
        t->minutes = rate > 0 ? (rise + rate - 1) / rate : 0;
        if (t->minutes < 1) {
            t->minutes = 1;
        }
        previous = target;
    }
    return exact;
}

void furnace_hmi_operator_library_from_controller(const furnace_hmi_controller_library_t *library,
                                                  furnace_hmi_operator_library_t *out)
{
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
        out->programs[i].inexact = !program_of(&library->programs[i], &out->programs[i]);
    }
}
