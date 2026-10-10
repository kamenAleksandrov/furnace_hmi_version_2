/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>

#include <furnace_hmi/ui_strings.h>

static int check_catalogue(void)
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

int main(void)
{
    /* Every language returns every string (its own or the English fallback). */
    for (int language = 0; language < FURNACE_HMI_UI_LANGUAGE_COUNT; ++language) {
        if (!furnace_hmi_ui_set_language((furnace_hmi_ui_language_t)language) ||
            furnace_hmi_ui_language() != (furnace_hmi_ui_language_t)language ||
            furnace_hmi_ui_language_tag((furnace_hmi_ui_language_t)language) == NULL ||
            furnace_hmi_ui_language_name((furnace_hmi_ui_language_t)language) == NULL) {
            (void)fprintf(stderr, "Language %d cannot be selected.\n", language);
            return 3;
        }
        const int result = check_catalogue();
        if (result != 0) {
            return result;
        }
    }
    if (!furnace_hmi_ui_set_language((furnace_hmi_ui_language_t)0) ||
        furnace_hmi_ui_set_language(FURNACE_HMI_UI_LANGUAGE_COUNT) ||
        furnace_hmi_ui_language() != (furnace_hmi_ui_language_t)0 ||
        furnace_hmi_ui_language_tag(FURNACE_HMI_UI_LANGUAGE_COUNT) != NULL ||
        strcmp(furnace_hmi_ui_language_tag((furnace_hmi_ui_language_t)0), "en") != 0) {
        (void)fprintf(stderr, "Invalid language was accepted or English is not the default.\n");
        return 4;
    }

    /* An override replaces only the entries it provides. */
    static const char *override[FURNACE_HMI_UI_STRING_COUNT];
    override[FURNACE_HMI_UI_STRING_APPLICATION_TITLE] = "Override";
    const char *english = furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_BUILD_PROBE_LABEL);
    furnace_hmi_ui_strings_override(override);
    const bool replaced =
        strcmp(furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE), "Override") == 0 &&
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_BUILD_PROBE_LABEL) == english;
    furnace_hmi_ui_strings_override(NULL);
    if (!replaced || strcmp(furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE), "Override") == 0) {
        (void)fprintf(stderr, "String override does not replace or restore correctly.\n");
        return 5;
    }
    return 0;
}
