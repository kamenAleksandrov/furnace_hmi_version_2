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

ZTEST(furnace_hmi_operator, test_operator_screens_without_content)
{
    const char *failure = "";
    lv_display_t *display = lv_display_get_default();

    zassert_not_null(display, "LVGL has no display");
    const int result = furnace_hmi_operator_content_check(display, &failure);

    zassert_equal(result, 0, "check %d: %s", result, failure);
}

ZTEST_SUITE(furnace_hmi_operator, NULL, NULL, NULL, NULL, NULL);
