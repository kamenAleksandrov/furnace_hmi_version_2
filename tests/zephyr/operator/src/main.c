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
    zassert_false(furnace_hmi_operator_command_for(&request, &command), "program Start is M21-7");
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

/* The controller's library in the screens' units (M21-7b). */
ZTEST(furnace_hmi_operator, test_the_controller_library_for_the_screens)
{
    static furnace_hmi_controller_library_t in;
    static furnace_hmi_operator_library_t out;

    (void)memset(&in, 0, sizeof(in));
    furnace_hmi_operator_library_from_controller(&in, &out);
    zassert_equal(out.state, FURNACE_HMI_OPERATOR_LIBRARY_LOADING, "no listing yet: being read");
    in.state = FURNACE_HMI_LIBRARY_UNAVAILABLE;
    in.count = 1U;
    furnace_hmi_operator_library_from_controller(&in, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE && out.count == 0U);
    in.state = FURNACE_HMI_LIBRARY_TOO_LARGE;
    furnace_hmi_operator_library_from_controller(&in, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_TOO_LARGE && out.count == 0U);

    in.state = FURNACE_HMI_LIBRARY_CURRENT;
    in.updating = true;
    in.count = 3U;
    /* 0: Heating to 85.0 C at 1.5 C/min from 25 C (40 min), Hold 30 min. */
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
    furnace_hmi_operator_library_from_controller(&in, &out);
    zassert_true(out.state == FURNACE_HMI_OPERATOR_LIBRARY_CURRENT && out.updating && out.count == 3U);

    const furnace_hmi_operator_program_t *p = &out.programs[0];
    zassert_true(p->present && p->id == 4U && p->revision == 6U && strcmp(p->name, "Kür 120") == 0,
                 "identity and the UTF-8 name");
    zassert_true(!p->damaged && !p->inexact && p->stage_count == 2U);
    zassert_true(p->stages[0].kind == FURNACE_HMI_OPERATOR_STAGE_HEATING && p->stages[0].target_c == 85 &&
                     p->stages[0].rate_tenths_c_per_minute == 15 && p->stages[0].minutes == 40,
                 "Heating in whole degrees, its minutes from 25 C");
    zassert_true(p->stages[1].kind == FURNACE_HMI_OPERATOR_STAGE_HOLD && p->stages[1].target_c == 85 &&
                     p->stages[1].minutes == 30 && p->stages[1].rate_tenths_c_per_minute == 0,
                 "Hold at the previous target");
    zassert_true(out.programs[1].inexact && out.programs[1].stages[0].target_c == 86,
                 "85.5 C is shown rounded and marked inexact");
    zassert_true(out.programs[2].damaged && !out.programs[2].inexact && out.programs[2].stage_count == 0U);

    /* Outside the editors' range: a rate of 0, over 3.0 C/min, over 180 C, a Hold of 0 min. */
    static const struct {
        int32_t target;
        int32_t rate;
        uint32_t hold;
    } outside[] = {{850, 0, 30U}, {850, 31, 30U}, {1810, 15, 30U}, {850, 15, 0U}};
    for (size_t i = 0U; i < ARRAY_SIZE(outside); i++) {
        in.programs[0].stages[0].target_decicelsius = outside[i].target;
        in.programs[0].stages[0].rate_decicelsius_per_min = outside[i].rate;
        in.programs[0].stages[1].duration_minutes = outside[i].hold;
        furnace_hmi_operator_library_from_controller(&in, &out);
        zassert_true(out.programs[0].inexact, "case %u is not inexact", (unsigned int)i);
    }
    in.programs[0].stages[0].target_decicelsius = 850;
    in.programs[0].stages[0].rate_decicelsius_per_min = 15;
    in.programs[0].stages[1].duration_minutes = 30U;
    in.programs[0].stages[1].kind = 3U;
    furnace_hmi_operator_library_from_controller(&in, &out);
    zassert_true(out.programs[0].inexact, "an unknown stage kind is inexact");
}

ZTEST_SUITE(furnace_hmi_operator, NULL, NULL, NULL, NULL, NULL);
