/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_STRINGS_H
#define FURNACE_HMI_UI_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Stable identifiers for the initial foundation-only visible strings. */
typedef enum {
    FURNACE_HMI_UI_STRING_APPLICATION_TITLE = 0,
    FURNACE_HMI_UI_STRING_FOUNDATION_VIEW_TITLE,
    FURNACE_HMI_UI_STRING_FOUNDATION_DESCRIPTION,
    FURNACE_HMI_UI_STRING_BUILD_PROBE_LABEL,
    FURNACE_HMI_UI_STRING_INPUT_EVENTS_LABEL,
    FURNACE_HMI_UI_STRING_INPUT_ACTION_LABEL,
    FURNACE_HMI_UI_STRING_TEXT_INPUT_PLACEHOLDER,
    FURNACE_HMI_UI_STRING_REMOTE_INTERFACE_LABEL,
    FURNACE_HMI_UI_STRING_DESKTOP_DEVELOPMENT_LABEL,
    FURNACE_HMI_UI_STRING_STATE_CURRENT_BADGE,
    FURNACE_HMI_UI_STRING_STATE_CURRENT_HEADING,
    FURNACE_HMI_UI_STRING_STATE_CURRENT_DETAIL,
    FURNACE_HMI_UI_STRING_STATE_STALE_BADGE,
    FURNACE_HMI_UI_STRING_STATE_STALE_HEADING,
    FURNACE_HMI_UI_STRING_STATE_STALE_DETAIL,
    FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_BADGE,
    FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_HEADING,
    FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_DETAIL,
    FURNACE_HMI_UI_STRING_COUNT
} furnace_hmi_ui_string_id_t;

/**
 * Return the initial English text for a visible-string identifier.
 *
 * Language selection and persistence are deliberately not implemented during
 * the foundation phase. A valid identifier always returns a non-empty string;
 * an invalid identifier returns NULL.
 */
const char *furnace_hmi_ui_string(furnace_hmi_ui_string_id_t id);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_STRINGS_H */
