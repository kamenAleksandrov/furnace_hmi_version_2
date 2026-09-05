/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/ztest.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/dashboard_model.h>
#include <furnace_hmi/ui_strings.h>

/** Verify that the shared foundation object exposes its declared probe value. */
ZTEST(furnace_hmi_foundation, test_probe_version_is_declared_value)
{
    zassert_equal(furnace_hmi_foundation_probe_version(),
                  FURNACE_HMI_FOUNDATION_PROBE_VERSION,
                  "portable foundation build probe version mismatch");
}

/** Verify that every initial English catalog entry is available on target. */
ZTEST(furnace_hmi_foundation, test_ui_string_catalog_is_complete)
{
    for (int id = 0; id < FURNACE_HMI_UI_STRING_COUNT; ++id) {
        const char *text = furnace_hmi_ui_string((furnace_hmi_ui_string_id_t)id);

        zassert_not_null(text, "UI string %d is absent", id);
        zassert_not_equal(text[0], '\0', "UI string %d is empty", id);
    }
}

/** Verify the shared observation model starts fail-closed on target. */
ZTEST(furnace_hmi_foundation, test_dashboard_model_starts_unavailable)
{
    furnace_hmi_dashboard_state_t state;

    furnace_hmi_dashboard_state_initialize(&state);
    zassert_equal(
        state.availability,
        FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE,
        "dashboard model did not start unavailable"
    );
    zassert_true(
        furnace_hmi_dashboard_state_is_well_formed(&state),
        "safe dashboard model baseline is not well formed"
    );
    zassert_equal(
        state.service_run_gate,
        FURNACE_HMI_SERVICE_RUN_GATE_UNKNOWN,
        "dashboard service gate did not start fail-closed"
    );
}

ZTEST_SUITE(furnace_hmi_foundation, NULL, NULL, NULL, NULL, NULL);
