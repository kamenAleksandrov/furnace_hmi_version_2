/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>

#include <furnace_hmi/ui_strings.h>

static const char *const english_strings[FURNACE_HMI_UI_STRING_COUNT] = {
    [FURNACE_HMI_UI_STRING_APPLICATION_TITLE] = "Furnace HMI V2",
    [FURNACE_HMI_UI_STRING_FOUNDATION_VIEW_TITLE] =
        "Furnace HMI V2 - desktop foundation",
    [FURNACE_HMI_UI_STRING_FOUNDATION_DESCRIPTION] =
        "Generic LVGL + SDL window and input proof. No furnace screen or control behavior.",
    [FURNACE_HMI_UI_STRING_BUILD_PROBE_LABEL] = "Portable foundation build probe",
    [FURNACE_HMI_UI_STRING_INPUT_EVENTS_LABEL] = "Input events observed",
    [FURNACE_HMI_UI_STRING_INPUT_ACTION_LABEL] = "Exercise pointer or keyboard input",
    [FURNACE_HMI_UI_STRING_TEXT_INPUT_PLACEHOLDER] =
        "Type here to exercise keyboard input",
    [FURNACE_HMI_UI_STRING_REMOTE_INTERFACE_LABEL] = "Remote operator interface",
    [FURNACE_HMI_UI_STRING_DESKTOP_DEVELOPMENT_LABEL] =
        "Desktop UI development mode",
    [FURNACE_HMI_UI_STRING_STATE_CURRENT_BADGE] = "STATE CURRENT",
    [FURNACE_HMI_UI_STRING_STATE_CURRENT_HEADING] = "Controller state available",
    [FURNACE_HMI_UI_STRING_STATE_CURRENT_DETAIL] =
        "Current authoritative controller state is available.",
    [FURNACE_HMI_UI_STRING_STATE_STALE_BADGE] = "STATE STALE",
    [FURNACE_HMI_UI_STRING_STATE_STALE_HEADING] =
        "Furnace state may be out of date",
    [FURNACE_HMI_UI_STRING_STATE_STALE_DETAIL] =
        "The last controller observation is stale. Do not treat it as current.",
    [FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_BADGE] = "STATE UNAVAILABLE",
    [FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_HEADING] = "Furnace state unavailable",
    [FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_DETAIL] =
        "Waiting for authoritative controller state. Furnace values and controls are hidden.",
};

_Static_assert(
    sizeof(english_strings) / sizeof(english_strings[0]) == FURNACE_HMI_UI_STRING_COUNT,
    "UI string catalog size must match its identifier count"
);

const char *furnace_hmi_ui_string(furnace_hmi_ui_string_id_t id)
{
    if ((int)id < 0 || id >= FURNACE_HMI_UI_STRING_COUNT) {
        return NULL;
    }

    return english_strings[id];
}
