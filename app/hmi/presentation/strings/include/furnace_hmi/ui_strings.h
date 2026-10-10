/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_STRINGS_H
#define FURNACE_HMI_UI_STRINGS_H

#include <stdbool.h>

/* Identifiers are generated from catalogue/en.json by tools/build_strings.py. */
#include <furnace_hmi/ui_string_ids.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Visible text lives in the catalogue (app/hmi/presentation/strings/catalogue),
 * never in screen code. Screens ask for text by identifier each time they
 * build, so a language change takes effect when the screen is rebuilt.
 *
 * Threading: the language selection and the override are plain globals. Change
 * them only from the GUI context (ADR-0005), the same context that reads text.
 */

/**
 * Return the text of a visible-string identifier in the selected language,
 * falling back to English for a string the language does not translate.
 * A valid identifier always returns a non-empty string; an invalid identifier
 * returns NULL.
 */
const char *furnace_hmi_ui_string(furnace_hmi_ui_string_id_t id);

/** Select the language of furnace_hmi_ui_string(). Returns false (and keeps
 * the current language) for an invalid language. */
bool furnace_hmi_ui_set_language(furnace_hmi_ui_language_t language);

/** The selected language (the default language until one is selected). */
furnace_hmi_ui_language_t furnace_hmi_ui_language(void);

/** BCP 47 tag of a language ("en"), or NULL for an invalid language. */
const char *furnace_hmi_ui_language_tag(furnace_hmi_ui_language_t language);

/** Name of a language in that language ("English", "Deutsch"), for the
 * language choice; NULL for an invalid language. */
const char *furnace_hmi_ui_language_name(furnace_hmi_ui_language_t language);

/**
 * Use a caller-owned table of FURNACE_HMI_UI_STRING_COUNT texts ahead of the
 * catalogue, for tests and pseudo-localisation; a NULL entry falls back to the
 * selected language. The table must stay valid until it is replaced. NULL
 * removes the override.
 */
void furnace_hmi_ui_strings_override(const char *const *table);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_STRINGS_H */
