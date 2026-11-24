/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The operator screens on Zephyr without controller-owned content, with the
 * checks the desktop program runs (tests/presentation/operator). LVGL is
 * initialized before the tests by the Zephyr LVGL module; the test thread is
 * then the only one that touches it (ADR-0005).
 */

#include <zephyr/ztest.h>

#include <furnace_hmi/operator_content_checks.h>
#include <lvgl.h>

#include <string.h>

#include "operator_commands.h"
#include "operator_library.h"

/* The application's requests as controller commands (M21-5b). */
ZTEST(furnace_hmi_operator, test_operator_requests_as_commands)
{
    furnace_hmi_operator_request_t request = {
        .kind = FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START,
        .target_c = 120,
        .rate_tenths_c_per_minute = 25,
        .session_minutes = 2880,
    };
    furnace_hmi_command_t command;

    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_equal(command.kind, FURNACE_HMI_COMMAND_MANUAL_START);
    zassert_equal(command.target_decicelsius, 1200, "whole degrees as 0.1 C");
    zassert_equal(command.rate_decicelsius_per_min, 25);
    zassert_equal(command.session_minutes, 2880U);

    request.session_minutes = 0;
    zassert_false(furnace_hmi_operator_command_for(&request, &command),
                  "no Manual Start without the controller's maximum");

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET;
    request.target_c = 121;
    request.run_id = 5U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_MANUAL_RETARGET && command.run_id == 5U &&
                     command.target_decicelsius == 1210 && command.rate_decicelsius_per_min == 25,
                 "retarget of run 5");

    static const struct {
        furnace_hmi_operator_request_kind_t request;
        furnace_hmi_command_kind_t command;
    } runs[] = {
        {FURNACE_HMI_OPERATOR_REQUEST_PAUSE, FURNACE_HMI_COMMAND_PAUSE},
        {FURNACE_HMI_OPERATOR_REQUEST_RESUME, FURNACE_HMI_COMMAND_RESUME},
        {FURNACE_HMI_OPERATOR_REQUEST_STOP, FURNACE_HMI_COMMAND_STOP},
        {FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE, FURNACE_HMI_COMMAND_START_PREPARE},
        {FURNACE_HMI_OPERATOR_REQUEST_CONTINUE, FURNACE_HMI_COMMAND_CONTINUE},
    };
    for (size_t i = 0U; i < ARRAY_SIZE(runs); i++) {
        request.kind = runs[i].request;
        zassert_true(furnace_hmi_operator_command_for(&request, &command));
        zassert_true(command.kind == runs[i].command && command.run_id == 5U, "request %u", (unsigned int)i);
    }

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_FAULT_RECOVERY;
    request.fault_id = 3U;
    request.source = 1U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_FAULT_RECOVERY && command.fault_id == 3U &&
                     command.source == 1U,
                 "recovery of entry 3/1");
    request.kind = FURNACE_HMI_OPERATOR_REQUEST_FAULT_ACKNOWLEDGE;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE && command.fault_id == 3U,
                 "acknowledgement of entry 3/1");

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a program Start names a program (M21-7b)");
    request.kind = FURNACE_HMI_OPERATOR_REQUEST_CLOSE_RUN;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "nothing to close on a controller");
}

ZTEST(furnace_hmi_operator, test_operator_screens_without_content)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_content_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_follow_the_controller_lifecycle)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_lifecycle_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_send_manual_commands)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_manual_command_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_faults_and_decision)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_fault_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_show_the_controller_library)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_library_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_save_and_delete_on_the_controller)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_library_command_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_start_on_the_controller)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_start_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_follow_library_changes)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_library_review_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_operator_screens_show_controller_diagnostics)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_diagnostics_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_editors_take_the_controller_limits)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_limits_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST(furnace_hmi_operator, test_settings_show_and_set_the_controller_clock)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_clock_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

/* The controller's clock for the screens (M21-7e): advanced to now, unknown not set. */
ZTEST(furnace_hmi_operator, test_the_clock_from_the_controller)
{
    static furnace_hmi_controller_observation_t o;
    furnace_hmi_operator_controller_clock_t clock;

    (void)memset(&o, 0, sizeof(o));
    furnace_hmi_operator_clock_from_controller(&o, 1000U, &clock);
    zassert_false(clock.reported, "no clock before the controller's STATUS");
    o.clock_valid = true;
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    furnace_hmi_operator_clock_from_controller(&o, 1000U, &clock);
    zassert_true(clock.reported && !clock.set, "UNKNOWN is reported, not set");
    o.wall_state = FURNACE_HMI_WALL_ADJUSTED;
    o.wall_ms = 1795435200000LL;
    o.clock_at_ms = UINT32_MAX - 499U;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_true(clock.set && clock.wall_ms == 1795435200000LL + 2000,
                 "advanced from its reading, across the uptime's wrap");
    o.clock_at_ms = 1600U;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_true(clock.set && clock.wall_ms == 1795435200000LL,
                 "a reading published after now is not a wrap: nothing added");
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_false(clock.reported, "no clock shown while the link is not current");
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    o.wall_ms = FURNACE_HMI_CLOCK_MAX_MS + 1;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_false(clock.reported, "a time outside the controller's range is not shown");
    o.wall_ms = INT64_MAX;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_false(clock.reported, "no overflow on a garbled time");
    o.wall_ms = 1795435200000LL;
    o.wall_state = 7U;
    furnace_hmi_operator_clock_from_controller(&o, 1500U, &clock);
    zassert_false(clock.reported, "an unknown state is not shown as set");
}

/* A program Start, Confirm and Cancel as controller commands (M21-7b). */
ZTEST(furnace_hmi_operator, test_program_start_as_commands)
{
    static furnace_hmi_operator_request_t request;
    static furnace_hmi_command_t command;

    (void)memset(&request, 0, sizeof(request));
    request.kind = FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a Start names a program");
    request.program_id = 7U;
    request.program_revision = 3U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_PROGRAM_START && command.program_id == 7U &&
                 command.program_revision == 3U);

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_START_CONFIRM;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a Confirm names a transaction");
    request.transaction_id = 12U;
    request.proposal_revision = 4U;
    request.proposed_stage = 2U;
    request.accepted_duration_s = 11400U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_START_CONFIRM && command.transaction_id == 12U &&
                     command.proposal_revision == 4U && command.proposed_stage == 2U && command.program_id == 7U &&
                     command.program_revision == 3U && command.accepted_duration_s == 11400U,
                 "exactly the proposal shown");

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_START_CANCEL;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_START_CANCEL && command.transaction_id == 12U);
    request.transaction_id = 0U;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a Cancel names a transaction");
}

/* Library changes as controller commands (M21-7b). */
ZTEST(furnace_hmi_operator, test_library_changes_as_commands)
{
    static furnace_hmi_operator_request_t request;
    static furnace_hmi_command_t command;

    (void)memset(&request, 0, sizeof(request));
    request.kind = FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE;
    request.program_id = 7U;
    request.program_revision = 3U;
    request.acknowledged_duration_s = 36000U;
    (void)strcpy(request.program_name, "Kür 120");
    request.program.stage_count = 2U;
    request.program.stages[0] = (furnace_hmi_operator_program_stage_t){FURNACE_HMI_OPERATOR_STAGE_HEATING, 85, 15, 40};
    request.program.stages[1] = (furnace_hmi_operator_program_stage_t){FURNACE_HMI_OPERATOR_STAGE_HOLD, 85, 0, 30};
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_PROGRAM_SAVE && command.program_id == 7U &&
                     command.program_revision == 3U && command.acknowledged_duration_s == 36000U &&
                     strcmp(command.name, "Kür 120") == 0 && command.stage_count == 2U,
                 "the save's identity, acknowledgement and name");
    zassert_true(command.stages[0].kind == FURNACE_HMI_CONTROLLER_STAGE_HEATING &&
                     command.stages[0].target_decicelsius == 850 && command.stages[0].rate_decicelsius_per_min == 15,
                 "Heating: target in 0.1 C and rate");
    zassert_true(command.stages[1].kind == FURNACE_HMI_CONTROLLER_STAGE_HOLD &&
                     command.stages[1].duration_minutes == 30U,
                 "Hold: its duration");

    request.program_id = 0U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.program_id == 0U && command.program_revision == 0U, "a new program has no revision");
    request.program.stages[1].minutes = 0;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a Hold of 0 min");
    request.program.stages[1].minutes = 30;
    request.program.stage_count = 0U;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "no stages");
    request.program.stage_count = 2U;
    request.program_name[0] = '\0';
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "no name");

    request.kind = FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE;
    request.program_id = 9U;
    request.program_revision = 4U;
    zassert_true(furnace_hmi_operator_command_for(&request, &command));
    zassert_true(command.kind == FURNACE_HMI_COMMAND_PROGRAM_DELETE && command.program_id == 9U &&
                     command.program_revision == 4U);
    request.program_id = 0U;
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "a delete names a program");
}

/* The controller's limits (fixture profile A) and planning reference, as the screens take them. */
static const furnace_hmi_operator_program_limits_t limits_a = {
    .known = true, .max_stages = 18U, .hold_allowed = true, .target_min_c = 20, .target_max_c = 180,
    .rate_min_tenths = 1, .rate_max_tenths = 30, .curing_max_minutes = 2880, .name_max_code_points = 31U};
static const furnace_hmi_operator_planning_reference_t reference_23 = {
    .known = true, .decicelsius = 230, .min_decicelsius = 50, .max_decicelsius = 400, .change_permitted = true};

/* The controller's library in the screens' units (M21-7b; M21-7d: its limits and reference). */
ZTEST(furnace_hmi_operator, test_the_controller_library_for_the_screens)
{
    static furnace_hmi_controller_library_t in;
    static furnace_hmi_operator_library_t out;
    static const furnace_hmi_operator_planning_reference_t no_reference = {.known = false};

    (void)memset(&in, 0, sizeof(in));
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
    zassert_equal(out.state, FURNACE_HMI_OPERATOR_LIBRARY_LOADING, "no listing yet: being read");
    in.state = FURNACE_HMI_LIBRARY_UNAVAILABLE;
    in.count = 1U;
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE && out.count == 0U);
    in.state = FURNACE_HMI_LIBRARY_TOO_LARGE;
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_TOO_LARGE && out.count == 0U);

    in.state = FURNACE_HMI_LIBRARY_CURRENT;
    in.updating = true;
    in.count = 3U;
    /* 0: Heating to 85.0 C at 1.5 C/min from the reference 23.0 C (42 min), Hold 30 min. */
    in.programs[0].program_id = 4U;
    in.programs[0].program_revision = 6U;
    (void)strcpy(in.programs[0].name, "Kür 120");
    in.programs[0].stage_count = 2U;
    in.programs[0].stages[0] = (furnace_hmi_controller_stage_t){
        .kind = FURNACE_HMI_CONTROLLER_STAGE_HEATING, .target_decicelsius = 850,
        .rate_decicelsius_per_min = 15};
    in.programs[0].stages[1] = (furnace_hmi_controller_stage_t){
        .kind = FURNACE_HMI_CONTROLLER_STAGE_HOLD, .duration_minutes = 30U};
    /* 1: a target in tenths of a degree. */
    in.programs[1] = in.programs[0];
    in.programs[1].program_id = 5U;
    in.programs[1].stages[0].target_decicelsius = 855;
    /* 2: damaged. */
    in.programs[2].program_id = 8U;
    in.programs[2].damaged = true;
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_CURRENT && out.updating && out.count == 3U);

    const furnace_hmi_operator_program_t *p = &out.programs[0];
    zassert_true(p->present && p->id == 4U && p->revision == 6U && strcmp(p->name, "Kür 120") == 0,
                 "identity and the UTF-8 name");
    zassert_true(!p->damaged && !p->inexact && p->stage_count == 2U);
    zassert_true(p->stages[0].kind == FURNACE_HMI_OPERATOR_STAGE_HEATING && p->stages[0].target_c == 85 &&
                     p->stages[0].rate_tenths_c_per_minute == 15 && p->stages[0].minutes == 42,
                 "Heating in whole degrees, its minutes from the planning reference");
    zassert_true(p->stages[1].kind == FURNACE_HMI_OPERATOR_STAGE_HOLD && p->stages[1].target_c == 85 &&
                     p->stages[1].minutes == 30 && p->stages[1].rate_tenths_c_per_minute == 0,
                 "Hold at the previous target");
    zassert_true(out.programs[1].inexact && out.programs[1].stages[0].target_c == 86,
                 "85.5 C is shown rounded and marked inexact");
    zassert_true(out.programs[2].damaged && !out.programs[2].inexact && out.programs[2].stage_count == 0U);
    /* Without a reference no start is assumed: the first ramp takes no time (PROG-14). */
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &no_reference, &out);
    zassert_true(out.programs[0].stages[0].minutes == 1 && !out.programs[0].inexact,
                 "no reference: the first ramp is not estimated");

    /* Outside the controller's limits: a rate of 0, over 3.0 C/min, over 180 C, below 20 C, a Hold of 0 min. */
    static const struct {
        int32_t target;
        int32_t rate;
        uint32_t hold;
    } outside[] = {{850, 0, 30U}, {850, 31, 30U}, {1810, 15, 30U}, {190, 15, 30U}, {850, 15, 0U}};
    for (size_t i = 0U; i < ARRAY_SIZE(outside); i++) {
        in.programs[0].stages[0].target_decicelsius = outside[i].target;
        in.programs[0].stages[0].rate_decicelsius_per_min = outside[i].rate;
        in.programs[0].stages[1].duration_minutes = outside[i].hold;
        furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
        zassert_true(out.programs[0].inexact, "case %u is not inexact", (unsigned int)i);
    }
    in.programs[0].stages[0].target_decicelsius = 850;
    in.programs[0].stages[0].rate_decicelsius_per_min = 15;
    in.programs[0].stages[1].duration_minutes = 30U;
    in.programs[0].stages[1].kind = 3U;
    furnace_hmi_operator_library_from_controller(&in, &limits_a, &reference_23, &out);
    zassert_true(out.programs[0].inexact, "an unknown stage kind is inexact");
}

/* The controller's CAPABILITIES and SETTINGS as the screens' limits and reference (M21-7d). */
ZTEST(furnace_hmi_operator, test_the_controller_limits_for_the_screens)
{
    static furnace_hmi_controller_observation_t o;
    furnace_hmi_operator_program_limits_t limits;
    furnace_hmi_operator_planning_reference_t reference;

    (void)memset(&o, 0, sizeof(o));
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_true(!limits.known && !reference.known, "nothing before CAPABILITIES");

    o.capabilities_valid = true;
    o.configured = true;
    o.max_stages = 12U;
    o.stage_kinds = 0x2U; /* Heating only */
    o.target_min_decicelsius = 205;
    o.target_max_decicelsius = 1805;
    o.rate_min_decicelsius_per_min = 2;
    o.rate_max_decicelsius_per_min = 25;
    o.curing_max_minutes = 1440U;
    o.name_max_code_points = 31U;
    o.planning_reference_min_decicelsius = 50;
    o.planning_reference_max_decicelsius = 400;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_true(limits.known && limits.max_stages == 12U && !limits.hold_allowed &&
                     limits.target_min_c == 21 && limits.target_max_c == 180 && limits.rate_min_tenths == 2 &&
                     limits.rate_max_tenths == 25 && limits.curing_max_minutes == 1440,
                 "targets rounded inward to whole degrees; rates, stages and kinds as reported");
    zassert_false(reference.known, "no reference before SETTINGS");
    zassert_equal(limits.recommended_curing_minutes, 0, "no advisory duration reported");
    o.recommended_curing_minutes = 600U;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_equal(limits.recommended_curing_minutes, 600, "the advisory duration as reported");
    o.recommended_curing_minutes = 1441U;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_true(limits.known && limits.recommended_curing_minutes == 0,
                 "an advisory above the maximum is the controller's invalid configuration: none assumed");

    o.settings_valid = true;
    o.planning_reference_decicelsius = 235;
    o.status_valid = true;
    o.settings_changes_permitted = false;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_true(reference.known && reference.decicelsius == 235 && reference.min_decicelsius == 50 &&
                     reference.max_decicelsius == 400 && !reference.change_permitted,
                 "the reference, its range, and no change during execution");

    o.configured = false;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_true(!limits.known && !reference.known, "an unconfigured controller offers nothing");
    o.configured = true;
    o.target_min_decicelsius = 1801;
    furnace_hmi_operator_limits_from_controller(&o, &limits, &reference);
    zassert_false(limits.known, "no whole degree within the range");
}

ZTEST_SUITE(furnace_hmi_operator, NULL, NULL, NULL, NULL, NULL);
