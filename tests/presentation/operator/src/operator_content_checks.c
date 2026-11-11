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

/* The button whose label reads text. */
static lv_obj_t *button_with_text(lv_obj_t *object, const char *text)
{
    if (object == NULL) {
        return NULL;
    }
    if (lv_obj_check_type(object, &lv_button_class) && tree_contains(object, text)) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = button_with_text(lv_obj_get_child(object, (int32_t)index), text);
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
    if (tree_contains(screens.root, fallback_forecast) ||
        !tree_contains(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Manual forecasts from an assumed temperature.", 70);
    }

    /*
     * Start on Manual before the controller is ready to start: disabled, no
     * Start request and no simulated session (M21-5b). Opening Manual asked
     * only for readiness.
     */
    {
        furnace_hmi_operator_request_t request;

        if (screens.manual_start_button == NULL) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Manual has no Start button.", 71);
        }
        if (!lv_obj_has_state(screens.manual_start_button, LV_STATE_DISABLED)) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Start is offered before the controller is ready.", 72);
        }
        while (furnace_hmi_operator_screens_take_request(&screens, &request)) {
            if (request.kind != FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE) {
                furnace_hmi_operator_screens_destroy(&screens);
                return failed(failure, "Opening Manual asked for more than readiness.", 73);
            }
        }
        lv_obj_send_event(screens.manual_start_button, LV_EVENT_CLICKED, NULL);
        for (int pass = 0; pass < 3; ++pass) {
            (void)lv_timer_handler();
        }
        if (furnace_hmi_operator_screens_take_request(&screens, &request) || screens.manual_running) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Start on Manual raised a request or a simulated session.", 74);
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
    /* Settings > Date & time (the screens' second Settings tab): no date without the clock. */
    screens.settings_tab = 1U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_SETTINGS) ||
        !tree_contains(screens.root,
                       furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNAVAILABLE)) ||
        tree_contains(screens.root, "00:00")) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Date & time shows a clock the controller did not provide.", 95);
    }

    furnace_hmi_operator_screens_destroy(&screens);
    return 0;
}

/* A status as the adapter gives it, with the controller's navigation permission. */
static void controller_status(furnace_hmi_machine_mode_t mode, furnace_hmi_run_state_t run,
                              bool navigation)
{
    furnace_hmi_dashboard_state_initialize(&state);
    state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state.mode = mode;
    state.run_state = run;
    state.program_id = 3U;
    state.stage_count = 2U;
    state.run_phase = (run == FURNACE_HMI_RUN_STATE_RUNNING || run == FURNACE_HMI_RUN_STATE_PAUSED)
                          ? FURNACE_HMI_RUN_PHASE_CURING
                          : FURNACE_HMI_RUN_PHASE_NONE;
    state.navigation_permitted = (furnace_hmi_bool_value_t){
        .value = navigation, .provenance = FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
}

int furnace_hmi_operator_lifecycle_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));

    /* A faulted run the controller holds: Running, and nothing leaves it. */
    controller_status(FURNACE_HMI_MACHINE_MODE_PROGRAM, FURNACE_HMI_RUN_STATE_FAULT, false);
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 101);
    }
    if (screens.page != FURNACE_HMI_OPERATOR_PAGE_RUNNING ||
        furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_HOME) ||
        !furnace_hmi_operator_screens_update(&screens, &state) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_RUNNING) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "A run the controller holds could be left.", 102);
    }
    /* It offers Stop, not Return Home, and Stop is a request (owner 2026-11-11). */
    {
        furnace_hmi_operator_request_t request;

        lv_refr_now(NULL);
        lv_obj_t *stop = button_with_text(screens.root,
                                          furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
        if (stop == NULL ||
            button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME)) != NULL) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "A faulted run offers Return Home instead of Stop.", 110);
        }
        lv_obj_send_event(stop, LV_EVENT_CLICKED, NULL);
        for (int pass = 0; pass < 3; ++pass) {
            (void)lv_timer_handler();
        }
        if (!furnace_hmi_operator_screens_take_request(&screens, &request) ||
            request.kind != FURNACE_HMI_OPERATOR_REQUEST_STOP) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Stop on a faulted run is not a request.", 111);
        }
    }

    /* Stopped and cooling, navigation permitted: says so, and Home may be shown. */
    controller_status(FURNACE_HMI_MACHINE_MODE_PROGRAM, FURNACE_HMI_RUN_STATE_COMPLETE, true);
    state.run_stopped = true;
    state.run_phase = FURNACE_HMI_RUN_PHASE_COOLING;
    if (!furnace_hmi_operator_screens_update(&screens, &state)) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "The cooling status was refused.", 103);
    }
    lv_refr_now(NULL);
    if (tree_contains(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOLING_COMPLETE)) ||
        !tree_contains(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_STOPPED_COOLING))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "A run that still cools is shown as complete.", 104);
    }
    /* Running is left by its Return Home button: during background cooling it shows Home. */
    {
        lv_obj_t *home = button_with_text(
            screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME));

        if (home != NULL) {
            lv_obj_send_event(home, LV_EVENT_CLICKED, NULL);
            for (int pass = 0; pass < 3; ++pass) {
                (void)lv_timer_handler();
            }
        }
        if (home == NULL || screens.page != FURNACE_HMI_OPERATOR_PAGE_HOME) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "Home could not be shown during background cooling.", 105);
        }
    }

    /* Awaiting a decision, held: named as such, on Running. */
    controller_status(FURNACE_HMI_MACHINE_MODE_PROGRAM, FURNACE_HMI_RUN_STATE_PAUSED, false);
    state.awaiting_decision = true;
    if (!furnace_hmi_operator_screens_update(&screens, &state) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_RUNNING) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "A run awaiting a decision is not on Running.", 106);
    }
    lv_refr_now(NULL);
    if (!tree_contains(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_AWAITING_DECISION))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Awaiting a decision is not named.", 107);
    }

    /* A Manual session of the controller, held: Manual, its own running screen (M21-5b). */
    controller_status(FURNACE_HMI_MACHINE_MODE_IDLE, FURNACE_HMI_RUN_STATE_IDLE, true);
    if (!furnace_hmi_operator_screens_update(&screens, &state) ||
        !furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_MANUAL)) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Manual could not be shown while idle.", 108);
    }
    controller_status(FURNACE_HMI_MACHINE_MODE_MANUAL, FURNACE_HMI_RUN_STATE_RUNNING, false);
    if (!furnace_hmi_operator_screens_update(&screens, &state) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_MANUAL ||
        furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_HOME) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_MANUAL) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "A controller Manual session does not hold Manual.", 109);
    }

    furnace_hmi_operator_screens_destroy(&screens);
    return 0;
}

/* ---- Manual against a controller (M21-5b) ---- */

static void settle(void)
{
    for (int pass = 0; pass < 3; ++pass) {
        (void)lv_timer_handler();
    }
}

static void click(lv_obj_t *button)
{
    if (button != NULL) {
        lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
        settle();
    }
}

static bool enabled(const lv_obj_t *button)
{
    return button != NULL && !lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_state(button, LV_STATE_DISABLED);
}

/* The controller's idle status, ready to start, with a Manual maximum of 48 h. */
static void ready_status(void)
{
    const furnace_hmi_command_feedback_t command = state.command_feedback;
    const furnace_hmi_command_feedback_t stop = state.stop_feedback;

    controller_status(FURNACE_HMI_MACHINE_MODE_IDLE, FURNACE_HMI_RUN_STATE_IDLE, true);
    state.program_id = 0U;
    state.current_temperature_c = (furnace_hmi_i32_value_t){
        .value = 25, .provenance = FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
    state.manual_max_minutes = (furnace_hmi_i32_value_t){
        .value = 2880, .provenance = FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
    state.start_ready = (furnace_hmi_bool_value_t){
        .value = true, .provenance = FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
    state.command_feedback = command;
    state.stop_feedback = stop;
}

/* The controller's Manual session 5 at 120 C and 2.0 C/min. */
static void manual_status(furnace_hmi_run_state_t run)
{
    ready_status();
    state.mode = FURNACE_HMI_MACHINE_MODE_MANUAL;
    state.run_state = run;
    state.run_id = 5U;
    state.run_phase = (run == FURNACE_HMI_RUN_STATE_COMPLETE) ? FURNACE_HMI_RUN_PHASE_COOLING
                                                              : FURNACE_HMI_RUN_PHASE_CURING;
    state.navigation_permitted.value = (run == FURNACE_HMI_RUN_STATE_COMPLETE);
    state.manual_target_c = (furnace_hmi_i32_value_t){
        .value = 120, .provenance = FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
    state.manual_rate_tenths_c_per_minute = (furnace_hmi_i32_value_t){
        .value = 20, .provenance = FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
}

static void answer(furnace_hmi_command_feedback_t *slot, uint32_t submission,
                   furnace_hmi_feedback_state_t outcome, uint8_t reason)
{
    *slot = (furnace_hmi_command_feedback_t){.submission = submission, .state = outcome, .reason = reason};
}

/* The next request is kind; it is handed over as submission. */
static bool next_request(furnace_hmi_operator_request_kind_t kind, uint32_t submission,
                         furnace_hmi_operator_request_t *request)
{
    if (!furnace_hmi_operator_screens_take_request(&screens, request) || request->kind != kind) {
        return false;
    }
    furnace_hmi_operator_screens_request_submitted(&screens, kind, submission);
    return true;
}

static bool no_request(void)
{
    furnace_hmi_operator_request_t request;

    return !furnace_hmi_operator_screens_take_request(&screens, &request);
}

/* A dialog shows title and detail; it is closed. */
static bool dialog_shows(furnace_hmi_ui_string_id_t title, furnace_hmi_ui_string_id_t detail)
{
    lv_refr_now(NULL);
    const bool shown = screens.modal != NULL && tree_contains(screens.modal, furnace_hmi_ui_string(title)) &&
                       tree_contains(screens.modal, furnace_hmi_ui_string(detail));

    click(first_button(screens.modal));
    return shown && screens.modal == NULL;
}

static bool update(void)
{
    if (!furnace_hmi_operator_screens_update(&screens, &state)) {
        return false;
    }
    settle();
    return true;
}

static int manual_commands(const char **failure)
{
    furnace_hmi_operator_request_t request;

    /* Opening Manual asks for readiness; Start waits for its answer. */
    if (!furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_MANUAL) ||
        !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE, 1U, &request) || !no_request()) {
        return failed(failure, "Opening Manual does not ask for start readiness.", 201);
    }
    answer(&state.command_feedback, 1U, FURNACE_HMI_FEEDBACK_PENDING, 0U);
    if (!update() || enabled(screens.manual_start_button)) {
        return failed(failure, "Start is offered while a command is pending.", 202);
    }
    answer(&state.command_feedback, 1U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    state.start_ready.value = false; /* PREPARING */
    if (!update() || enabled(screens.manual_start_button) ||
        !tree_contains(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_NOT_READY_NOTE))) {
        return failed(failure, "Start is offered before the controller is ready.", 230);
    }
    state.start_ready.value = true;
    if (!update() || !enabled(screens.manual_start_button)) {
        return failed(failure, "Start is not offered when the controller is ready.", 203);
    }

    /* Start asks for the controller's longest session; a refusal is shown with its reason. */
    click(screens.manual_start_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START, 2U, &request) ||
        request.target_c != 120 || request.rate_tenths_c_per_minute != 20 ||
        request.session_minutes != 2880 || !no_request()) {
        return failed(failure, "Start does not ask for target, rate and the longest session.", 204);
    }
    if (enabled(screens.manual_start_button) || screens.manual_running) {
        return failed(failure, "Start is offered twice, or a session is assumed.", 205);
    }
    answer(&state.command_feedback, 2U, FURNACE_HMI_FEEDBACK_REJECTED, 9U);
    if (!update() || !dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_REJECTED_TITLE,
                                   FURNACE_HMI_UI_STRING_UI_LAB_REASON_START_CONDITIONS) ||
        !enabled(screens.manual_start_button) || !no_request()) {
        return failed(failure, "A rejected Start is not shown, or is sent again.", 206);
    }
    click(screens.manual_start_button);
    answer(&state.command_feedback, 3U, FURNACE_HMI_FEEDBACK_REFUSED, 10U);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START, 3U, &request) || !update() ||
        !dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_REJECTED_TITLE,
                      FURNACE_HMI_UI_STRING_UI_LAB_REASON_STORE_NOT_READY) || !no_request()) {
        return failed(failure, "A Start refused as not ready is not shown, or is sent again.", 207);
    }

    /* Accepted: the session follows the controller and holds Manual. */
    click(screens.manual_start_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START, 4U, &request)) {
        return failed(failure, "Start is not sent again when pressed again.", 208);
    }
    answer(&state.command_feedback, 4U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    manual_status(FURNACE_HMI_RUN_STATE_RUNNING);
    if (!update() || screens.page != FURNACE_HMI_OPERATOR_PAGE_MANUAL || !screens.manual_running ||
        !enabled(screens.manual_pause_button) || !enabled(screens.manual_stop_button) ||
        furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_HOME)) {
        return failed(failure, "The controller's Manual session is not shown and held.", 209);
    }

    /* Retarget: one request at a time; taps while it is pending are merged. */
    lv_obj_t *increase = button_with_text(screens.manual_steppers,
                                          furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE));
    click(increase);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET, 5U, &request) ||
        request.target_c != 121 || request.rate_tenths_c_per_minute != 20) {
        return failed(failure, "A target change is not sent as a retarget.", 210);
    }
    answer(&state.command_feedback, 5U, FURNACE_HMI_FEEDBACK_PENDING, 0U);
    (void)update();
    click(increase);
    click(increase);
    if (!no_request() || screens.manual_target_c != 123) {
        return failed(failure, "Taps while a retarget is pending are not merged.", 211);
    }
    state.manual_target_c.value = 121;
    answer(&state.command_feedback, 5U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!update() || !next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET, 6U, &request) ||
        request.target_c != 123 || !no_request()) {
        return failed(failure, "The merged target is not sent after the answer.", 212);
    }
    /* A tap while it is pending is dropped with it: nothing is sent after a refusal. */
    click(increase);
    answer(&state.command_feedback, 6U, FURNACE_HMI_FEEDBACK_REJECTED, 19U);
    if (!update() || !dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_REJECTED_TITLE,
                                   FURNACE_HMI_UI_STRING_UI_LAB_REASON_MANUAL_REQUEST) ||
        screens.manual_target_c != 121 || !no_request()) {
        return failed(failure, "A rejected retarget does not return to the controller's target.", 213);
    }

    /*
     * A Pause tap means Pause: when the controller pauses the session before the
     * tap is handled, nothing is sent, never a Resume (review F1).
     */
    lv_obj_send_event(screens.manual_pause_button, LV_EVENT_CLICKED, NULL);
    manual_status(FURNACE_HMI_RUN_STATE_PAUSED);
    if (!update() || !no_request()) {
        return failed(failure, "A Pause tap became a Resume when the state changed.", 240);
    }
    manual_status(FURNACE_HMI_RUN_STATE_RUNNING);
    (void)update();

    /*
     * A tap merged behind a pending retarget is dropped by Stop: nothing pressed
     * before Stop goes out after it, and nothing ordinary while it is pending
     * (review F2).
     */
    click(increase);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET, 30U, &request)) {
        return failed(failure, "A target change is not sent.", 241);
    }
    answer(&state.command_feedback, 30U, FURNACE_HMI_FEEDBACK_PENDING, 0U);
    (void)update();
    click(increase);
    /* A Stop tap behind a tap not yet handled is never lost. */
    lv_obj_send_event(increase, LV_EVENT_CLICKED, NULL);
    click(screens.manual_stop_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 31U, &request) || request.run_id != 5U) {
        return failed(failure, "Stop is lost behind another tap, or does not name the session.", 242);
    }
    if (enabled(increase) || enabled(screens.manual_pause_button)) {
        return failed(failure, "Steppers or Pause are offered while Stop is pending.", 243);
    }
    answer(&state.command_feedback, 30U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!update() || !no_request()) {
        return failed(failure, "A tap from before Stop is sent after it.", 244);
    }
    /* Even a Pause event that reaches the screens sends nothing while Stop is pending. */
    click(screens.manual_pause_button);
    if (!no_request()) {
        return failed(failure, "An ordinary command goes out while Stop is pending.", 253);
    }
    answer(&state.stop_feedback, 31U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    state.manual_target_c.value = 121;
    if (!update() || !no_request() || screens.manual_target_c != 121 || !enabled(increase)) {
        return failed(failure, "After Stop the controller's target is not shown again.", 245);
    }

    /* Pause and Resume are requests for the run shown; the state is the controller's. */
    click(screens.manual_pause_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PAUSE, 7U, &request) || screens.manual_paused ||
        request.run_id != 5U) {
        return failed(failure, "Pause is not a request for the session shown.", 214);
    }
    answer(&state.command_feedback, 7U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    manual_status(FURNACE_HMI_RUN_STATE_PAUSED);
    if (!update() || !screens.manual_paused) {
        return failed(failure, "The controller's pause is not shown.", 215);
    }
    click(screens.manual_pause_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_RESUME, 8U, &request)) {
        return failed(failure, "Resume is not a request.", 216);
    }
    answer(&state.command_feedback, 8U, FURNACE_HMI_FEEDBACK_UNRESOLVED, 2U);
    if (!update() || !dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_UNRESOLVED_TITLE,
                                   FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_UNRESOLVED_DETAIL)) {
        return failed(failure, "An unresolved Resume is not shown.", 217);
    }

    /* Not handed over: shown, nothing assumed. */
    click(screens.manual_pause_button);
    if (!furnace_hmi_operator_screens_take_request(&screens, &request) ||
        request.kind != FURNACE_HMI_OPERATOR_REQUEST_RESUME) {
        return failed(failure, "Resume is not asked for again.", 218);
    }
    furnace_hmi_operator_screens_request_submitted(&screens, request.kind, 0U);
    settle();
    if (!dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_NOT_SENT_TITLE,
                      FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_BUSY_DETAIL) ||
        !enabled(screens.manual_pause_button)) {
        return failed(failure, "A command the host could not hand over is not shown.", 219);
    }

    /* Stop has its own slot: sent while an ordinary command is pending. */
    click(screens.manual_pause_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_RESUME, 9U, &request)) {
        return failed(failure, "Resume is not sent.", 220);
    }
    answer(&state.command_feedback, 9U, FURNACE_HMI_FEEDBACK_PENDING, 0U);
    (void)update();
    click(screens.manual_stop_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 10U, &request) || enabled(screens.manual_stop_button)) {
        return failed(failure, "Stop waits for an ordinary command, or is offered twice.", 221);
    }
    answer(&state.stop_feedback, 10U, FURNACE_HMI_FEEDBACK_UNRESOLVED, 5U);
    if (!update() || !dialog_shows(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_UNRESOLVED_TITLE,
                                   FURNACE_HMI_UI_STRING_UI_LAB_STOP_UNRESOLVED_DETAIL) ||
        !enabled(screens.manual_stop_button)) {
        return failed(failure, "An unresolved Stop is not shown, or Stop is not offered again.", 222);
    }

    /* The session ended: it cools; Exit leaves it and Manual is offered anew. */
    answer(&state.command_feedback, 9U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    click(screens.manual_stop_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 11U, &request)) {
        return failed(failure, "Stop is not sent again when pressed again.", 223);
    }
    answer(&state.stop_feedback, 11U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    manual_status(FURNACE_HMI_RUN_STATE_COMPLETE);
    if (!update() || !screens.manual_cooling || screens.manual_running ||
        !enabled(screens.manual_exit_button)) {
        return failed(failure, "The ended session is not shown cooling.", 224);
    }
    click(screens.manual_exit_button);
    if (screens.page != FURNACE_HMI_OPERATOR_PAGE_HOME || !update() ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_HOME) {
        return failed(failure, "Exit does not leave the ended session.", 225);
    }
    if (!furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_MANUAL) ||
        screens.manual_cooling || !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE, 12U, &request)) {
        return failed(failure, "Manual is not offered anew after the session.", 226);
    }
    answer(&state.command_feedback, 12U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!update() || !enabled(screens.manual_start_button)) {
        return failed(failure, "Start is not offered anew after the session.", 227);
    }

    /* A tap not sent when the session ends is dropped, never sent to the next session (review F3). */
    manual_status(FURNACE_HMI_RUN_STATE_RUNNING);
    state.run_id = 6U;
    (void)update();
    increase = button_with_text(screens.manual_steppers, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE));
    click(increase);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET, 13U, &request) || request.run_id != 6U) {
        return failed(failure, "A target change is not sent for session 6.", 246);
    }
    answer(&state.command_feedback, 13U, FURNACE_HMI_FEEDBACK_PENDING, 0U);
    (void)update();
    click(increase);
    answer(&state.command_feedback, 13U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    manual_status(FURNACE_HMI_RUN_STATE_COMPLETE);
    state.run_id = 6U;
    if (!update() || !no_request() || screens.manual_retarget_wanted) {
        return failed(failure, "A tap outlives its session.", 247);
    }

    /* Faulted: Stop only; awaiting a decision: no Pause or Resume (M21-5c decides). */
    manual_status(FURNACE_HMI_RUN_STATE_FAULT);
    state.run_id = 7U;
    if (!update() || screens.page != FURNACE_HMI_OPERATOR_PAGE_MANUAL || !enabled(screens.manual_stop_button) ||
        enabled(screens.manual_pause_button) || enabled(increase)) {
        return failed(failure, "A faulted session offers more than Stop.", 248);
    }
    manual_status(FURNACE_HMI_RUN_STATE_PAUSED);
    state.run_id = 7U;
    state.awaiting_decision = true;
    if (!update() || enabled(screens.manual_pause_button)) {
        return failed(failure, "Resume is offered while the controller awaits a decision.", 249);
    }
    state.awaiting_decision = false;

    /* A run ID seen again after the controller left Manual is a new session (review L1). */
    click(screens.manual_stop_button);
    answer(&state.stop_feedback, 32U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 32U, &request)) {
        return failed(failure, "Stop is not sent.", 250);
    }
    manual_status(FURNACE_HMI_RUN_STATE_COMPLETE);
    state.run_id = 7U;
    (void)update();
    click(screens.manual_exit_button);
    ready_status();
    (void)update();
    if (!furnace_hmi_operator_screens_navigate(&screens, FURNACE_HMI_OPERATOR_PAGE_MANUAL)) {
        return failed(failure, "Manual is not offered after the session.", 251);
    }
    manual_status(FURNACE_HMI_RUN_STATE_COMPLETE);
    state.run_id = 7U;
    if (!update() || !screens.manual_cooling) {
        return failed(failure, "A later session with a repeated run ID is not shown.", 252);
    }
    return 0;
}

int furnace_hmi_operator_manual_command_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 200);
    }
    const int result = manual_commands(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}
