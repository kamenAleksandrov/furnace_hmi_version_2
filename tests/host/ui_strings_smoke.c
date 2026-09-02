/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>

#include <furnace_hmi/ui_strings.h>

int main(void)
{
    for (int id = 0; id < FURNACE_HMI_UI_STRING_COUNT; ++id) {
        const char *text = furnace_hmi_ui_string((furnace_hmi_ui_string_id_t)id);

        if (text == NULL || text[0] == '\0') {
            (void)fprintf(stderr, "UI string %d is absent or empty.\n", id);
            return 1;
        }
    }

    if (furnace_hmi_ui_string((furnace_hmi_ui_string_id_t)-1) != NULL ||
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_COUNT) != NULL) {
        (void)fprintf(stderr, "Invalid UI string identifier was accepted.\n");
        return 2;
    }

    return 0;
}
