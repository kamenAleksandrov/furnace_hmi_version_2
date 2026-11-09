/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include "wire_conformance.h"

void wire_conformance_report(const char *check, const char *name)
{
    printk("FAIL %s: %s\n", name, check);
}

/** The copy of the controller's codec passes the controller's golden vectors on Zephyr. */
ZTEST(furnace_hmi_wire, test_controller_golden_vectors_pass)
{
    zassert_equal(wire_conformance_run(), 0, "wire conformance failures (see the log)");
}

ZTEST_SUITE(furnace_hmi_wire, NULL, NULL, NULL, NULL, NULL);
