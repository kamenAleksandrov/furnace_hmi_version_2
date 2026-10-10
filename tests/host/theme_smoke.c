/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>

#include <furnace_hmi/hmi_theme.h>

int main(void)
{
    for (int id = 0; id < FURNACE_HMI_THEME_COUNT; ++id) {
        const furnace_hmi_theme_t *theme = furnace_hmi_theme((furnace_hmi_theme_id_t)id);
        if (theme == NULL || theme->id == NULL || theme->id[0] == '\0' || theme->colors == NULL) {
            (void)fprintf(stderr, "Theme %d is incomplete.\n", id);
            return 1;
        }
        if (furnace_hmi_ui_string(theme->name) == NULL) {
            (void)fprintf(stderr, "Theme %s has no display name.\n", theme->id);
            return 2;
        }
        for (int role = 0; role < FURNACE_HMI_COLOR_COUNT; ++role) {
            if (theme->colors[role].rgb > 0xFFFFFFU) {
                (void)fprintf(stderr, "Theme %s colour %d is not RGB.\n", theme->id, role);
                return 3;
            }
        }
        for (int other = 0; other < id; ++other) {
            if (strcmp(furnace_hmi_theme((furnace_hmi_theme_id_t)other)->id, theme->id) == 0) {
                (void)fprintf(stderr, "Theme id %s is duplicated.\n", theme->id);
                return 4;
            }
        }
    }
    if (furnace_hmi_theme((furnace_hmi_theme_id_t)-1) != NULL ||
        furnace_hmi_theme(FURNACE_HMI_THEME_COUNT) != NULL) {
        (void)fprintf(stderr, "Invalid theme identifier was accepted.\n");
        return 5;
    }
    return 0;
}
