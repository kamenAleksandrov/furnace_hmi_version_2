/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_STRINGS_INTERNAL_H
#define FURNACE_HMI_UI_STRINGS_INTERNAL_H

#include <furnace_hmi/ui_strings.h>

/* One language of the generated catalogue (generated/ui_strings_catalogue.c). */
typedef struct {
    const char *tag;
    const char *name;
    /* FURNACE_HMI_UI_STRING_COUNT entries; NULL where the language has no translation. */
    const char *const *strings;
} furnace_hmi_ui_language_info_t;

extern const furnace_hmi_ui_language_info_t furnace_hmi_ui_language_catalogue[FURNACE_HMI_UI_LANGUAGE_COUNT];

#endif /* FURNACE_HMI_UI_STRINGS_INTERNAL_H */
