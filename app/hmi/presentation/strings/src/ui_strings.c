/* SPDX-License-Identifier: Apache-2.0 */
/* Lookup over the generated catalogue (catalogue/<tag>.json via tools/build_strings.py). */

#include <stddef.h>

#include "ui_strings_internal.h"

/* GUI context only; see ui_strings.h. */
static furnace_hmi_ui_language_t selected_language = (furnace_hmi_ui_language_t)0;
static const char *const *override_table;

static bool language_valid(furnace_hmi_ui_language_t language)
{
    return (int)language >= 0 && language < FURNACE_HMI_UI_LANGUAGE_COUNT;
}

const char *furnace_hmi_ui_string(furnace_hmi_ui_string_id_t id)
{
    if ((int)id < 0 || id >= FURNACE_HMI_UI_STRING_COUNT) {
        return NULL;
    }
    if (override_table != NULL && override_table[id] != NULL) {
        return override_table[id];
    }
    const char *text = furnace_hmi_ui_language_catalogue[selected_language].strings[id];
    return text != NULL ? text : furnace_hmi_ui_language_catalogue[0].strings[id];
}

bool furnace_hmi_ui_set_language(furnace_hmi_ui_language_t language)
{
    if (!language_valid(language)) {
        return false;
    }
    selected_language = language;
    return true;
}

furnace_hmi_ui_language_t furnace_hmi_ui_language(void)
{
    return selected_language;
}

const char *furnace_hmi_ui_language_tag(furnace_hmi_ui_language_t language)
{
    return language_valid(language) ? furnace_hmi_ui_language_catalogue[language].tag : NULL;
}

const char *furnace_hmi_ui_language_name(furnace_hmi_ui_language_t language)
{
    return language_valid(language) ? furnace_hmi_ui_language_catalogue[language].name : NULL;
}

void furnace_hmi_ui_strings_override(const char *const *table)
{
    override_table = table;
}
