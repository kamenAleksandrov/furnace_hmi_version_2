/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/ztest.h>

#include <furnace_hmi/foundation.h>
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

ZTEST_SUITE(furnace_hmi_foundation, NULL, NULL, NULL, NULL, NULL);
