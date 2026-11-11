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

#include "operator_commands.h"

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
    };
    for (size_t i = 0U; i < ARRAY_SIZE(runs); i++) {
        request.kind = runs[i].request;
        zassert_true(furnace_hmi_operator_command_for(&request, &command));
        zassert_true(command.kind == runs[i].command && command.run_id == 5U, "request %u", (unsigned int)i);
    }

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

ZTEST_SUITE(furnace_hmi_operator, NULL, NULL, NULL, NULL, NULL);
