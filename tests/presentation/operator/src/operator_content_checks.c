/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/dashboard_model.h>
#include <furnace_hmi/operator_content_checks.h>
#include <furnace_hmi/operator_screens.h>
#include <furnace_hmi/ui_strings.h>

static furnace_hmi_operator_screens_t screens;
static furnace_hmi_operator_content_t content;
static furnace_hmi_dashboard_state_t state;

static int failed(const char **failure, const char *description, int code)
{
    *failure = description;
    return code;
}

static bool tree_contains(const lv_obj_t *object, const char *text)
{
    if (lv_obj_check_type(object, &lv_label_class) && strstr(lv_label_get_text(object), text) != NULL) {
        return true;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        if (tree_contains(lv_obj_get_child(object, (int32_t)index), text)) {
            return true;
        }
    }
    return false;
}

static lv_obj_t *first_button(lv_obj_t *object)
{
    if (object == NULL) {
        return NULL;
    }
    if (lv_obj_check_type(object, &lv_button_class)) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = first_button(lv_obj_get_child(object, (int32_t)index));
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

static bool show(furnace_hmi_operator_page_t page)
{
    if (!furnace_hmi_operator_screens_navigate(&screens, page)) {
        return false;
    }
    lv_refr_now(NULL);
    return true;
}

/* Simulated content that must never appear without the controller. */
static int no_simulated_content(const char **failure, int code)
{
    static const furnace_hmi_ui_string_id_t delivered[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_1,
        FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_2,
        FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_3,
    };

    for (size_t index = 0U; index < sizeof(delivered) / sizeof(delivered[0]); ++index) {
        if (tree_contains(screens.root, furnace_hmi_ui_string(delivered[index]))) {
            return failed(failure, "A delivered program of the simulation is shown.", code);
        }
    }
    if (tree_contains(screens.root, "2026") || tree_contains(screens.root, "14:32")) {
        return failed(failure, "The simulation's date or time is shown.", code + 1);
    }
    return 0;
}

static int check_page(furnace_hmi_operator_page_t page, const char **failure, int code)
{
    if (!show(page)) {
        return failed(failure, "A page could not be shown.", code);
    }
    return no_simulated_content(failure, code + 1);
}

int furnace_hmi_operator_content_check(lv_display_t *display, const char **failure)
{
    char fallback_forecast[32];
    int result;

    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    furnace_hmi_dashboard_state_initialize(&state);
    state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created without content.", 1);
    }
    lv_refr_now(NULL);

    if ((result = no_simulated_content(failure, 10)) != 0 ||
        (result = check_page(FURNACE_HMI_OPERATOR_PAGE_HOME, failure, 20)) != 0 ||
        (result = check_page(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS, failure, 30)) != 0 ||
        (result = check_page(FURNACE_HMI_OPERATOR_PAGE_DEVICE, failure, 40)) != 0 ||
        (result = check_page(FURNACE_HMI_OPERATOR_PAGE_SETTINGS, failure, 50)) != 0 ||
        (result = check_page(FURNACE_HMI_OPERATOR_PAGE_MANUAL, failure, 60)) != 0) {
        furnace_hmi_operator_screens_destroy(&screens);
        return result;
    }

    /*
     * Manual with no measured temperature: no forecast. With the assumed 25 C,
     * the default draft (120 C at 2.0 C/min) would forecast 48 minutes.
     */
    (void)snprintf(fallback_forecast, sizeof(fallback_forecast),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HOURS_MINUTES_FORMAT), 0L, 48L);
    if (tree_contains(screens.root, fallback_forecast)) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Manual forecasts from an assumed temperature.", 70);
    }

    /* Start on Manual: no command and no simulated outcome, only the notice. */
    {
        furnace_hmi_operator_request_t request;

        if (screens.manual_start_button == NULL) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Manual has no Start button.", 71);
        }
        lv_obj_send_event(screens.manual_start_button, LV_EVENT_CLICKED, NULL);
        for (int pass = 0; pass < 3; ++pass) {
            (void)lv_timer_handler();
        }
        if (screens.modal == NULL ||
            !tree_contains(screens.modal,
                           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACTION_NOT_CONNECTED_TITLE))) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Start on Manual does not say that it is not connected.", 72);
        }
        if (furnace_hmi_operator_screens_take_request(&screens, &request) || screens.manual_running) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Start on Manual raised a request or a simulated session.", 73);
        }
        lv_obj_t *close = first_button(screens.modal);

        if (close != NULL) {
            lv_obj_send_event(close, LV_EVENT_CLICKED, NULL);
            for (int pass = 0; pass < 3; ++pass) {
                (void)lv_timer_handler();
            }
        }
        if (screens.modal != NULL) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "The notice could not be closed.", 74);
        }
    }

    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS) ||
        !tree_contains(screens.root,
                       furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_UNAVAILABLE))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Programs does not say that the library is unavailable.", 80);
    }
    if (!show(FURNACE_HMI_OPERATOR_PAGE_DEVICE) ||
        !tree_contains(screens.root,
                       furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_UNAVAILABLE))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Device does not say that its data are unavailable.", 90);
    }

    furnace_hmi_operator_screens_destroy(&screens);
    return 0;
}
