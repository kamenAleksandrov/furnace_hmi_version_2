/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include "controller_client_tests.h"

void controller_client_tests_report(const char *check, const char *scenario)
{
    printk("FAIL %s: %s\n", scenario, check);
}

/** The controller client's scenarios, against a scripted controller, on Zephyr. */
ZTEST(furnace_hmi_client, test_controller_link_scenarios_pass)
{
    zassert_equal(controller_client_tests_run(), 0, "controller client failures (see the log)");
    zassert_equal(controller_dashboard_tests_run(), 0, "dashboard mapping failures (see the log)");
}

ZTEST_SUITE(furnace_hmi_client, NULL, NULL, NULL, NULL, NULL);
