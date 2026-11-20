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

/* The controller's limits and planning reference as the screens receive them (M21-7d): the
 * furnace firmware's fixture profile A, the default reference 23.0 C within 5.0 to 40.0 C. */
static const furnace_hmi_operator_program_limits_t fixture_limits = {
    .known = true, .max_stages = 18U, .hold_allowed = true, .target_min_c = 20, .target_max_c = 180,
    .rate_min_tenths = 1, .rate_max_tenths = 30, .curing_max_minutes = 48 * 60, .name_max_code_points = 31U,
};
static const furnace_hmi_operator_planning_reference_t fixture_reference = {
    .known = true, .decicelsius = 230, .min_decicelsius = 50, .max_decicelsius = 400, .change_permitted = true,
};

/* Content of a controller that has reported its limits and reference. */
static void reported_limits(void)
{
    content.program_limits = fixture_limits;
    content.planning_reference = fixture_reference;
}

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
    reported_limits();

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
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 200);
    }
    const int result = manual_commands(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* ---- Faults and the decision against a controller (M21-5c) ---- */

static bool shows(lv_obj_t *root, furnace_hmi_ui_string_id_t id)
{
    lv_refr_now(NULL);
    return root != NULL && tree_contains(root, furnace_hmi_ui_string(id));
}

static lv_obj_t *modal_button(furnace_hmi_ui_string_id_t id)
{
    return button_with_text(screens.modal, furnace_hmi_ui_string(id));
}

static void fault_entry(uint8_t index, uint8_t id, bool active, bool latched, bool acknowledged,
                        furnace_hmi_fault_recovery_t recovery)
{
    state.faults[index] = (furnace_hmi_fault_entry_t){
        .fault_id = id, .source = 0U, .active = active, .latched = latched,
        .acknowledged = acknowledged, .recovery = recovery};
}

/* A faulted Manual session 7 with a sensor fault and a protective trip. */
static void faulted_status(void)
{
    manual_status(FURNACE_HMI_RUN_STATE_FAULT);
    state.run_id = 7U;
    state.faults_known = true;
    state.fault_count = 2U;
    state.fault_total = 2U;
    fault_entry(0U, 1U, true, true, false, FURNACE_HMI_FAULT_RECOVERY_OPERATOR);
    fault_entry(1U, 7U, false, true, true, FURNACE_HMI_FAULT_RECOVERY_PROTECTIVE_EXTERNAL);
}

static int fault_commands(const char **failure)
{
    furnace_hmi_operator_request_t request;
    char text[48];

    /* The Faults button opens the controller's entries; a protective trip has no Recover. */
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BUTTON_FORMAT), 2U);
    lv_refr_now(NULL);
    click(button_with_text(screens.root, text));
    if (screens.modal == NULL || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_TITLE) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_7) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RESET_EXTERNAL)) {
        return failed(failure, "The Faults dialog does not show the controller's entries.", 301);
    }
    if (modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RECOVER) != NULL) {
        return failed(failure, "Recover is offered before acknowledgement or for a trip.", 302);
    }

    /* While a Stop is pending, no fault command is offered (review F6). */
    click(screens.manual_stop_button);
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 38U, &request) ||
        enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE))) {
        return failed(failure, "Acknowledge is offered while a Stop is pending.", 313);
    }
    answer(&state.stop_feedback, 38U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!update() || !enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE))) {
        return failed(failure, "Acknowledge does not come back after the Stop.", 314);
    }

    /* A tap names the entry it was drawn for, also when the list moves first (review L1). */
    lv_obj_send_event(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE), LV_EVENT_CLICKED, NULL);
    state.fault_count = 3U;
    state.fault_total = 3U;
    state.faults[2] = state.faults[1];
    state.faults[1] = state.faults[0];
    fault_entry(0U, 2U, true, false, false, FURNACE_HMI_FAULT_RECOVERY_AWAIT_CLEAR);
    if (!furnace_hmi_operator_screens_update(&screens, &state)) {
        return failed(failure, "The status was refused.", 310);
    }
    settle();
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_FAULT_ACKNOWLEDGE, 39U, &request) || request.fault_id != 1U) {
        return failed(failure, "A tap acknowledged another entry after the list moved.", 311);
    }
    answer(&state.command_feedback, 39U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    faulted_status();
    if (!update()) {
        return failed(failure, "The status was refused.", 312);
    }

    /* Acknowledge is a request; the entry changes only when the controller says so. */
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_FAULT_ACKNOWLEDGE, 40U, &request) ||
        request.fault_id != 1U || request.source != 0U || screens.modal == NULL) {
        return failed(failure, "Acknowledge is not a request for the entry.", 303);
    }
    answer(&state.command_feedback, 40U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    state.faults[0].acknowledged = true;
    if (!update() || modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RECOVER) == NULL ||
        modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE) != NULL) {
        return failed(failure, "An acknowledged latched fault does not offer Recover.", 304);
    }

    /* Recover: the controller's reply is shown in the dialog, nothing is assumed. */
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RECOVER));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_FAULT_RECOVERY, 41U, &request) || request.fault_id != 1U) {
        return failed(failure, "Recover is not a request for the entry.", 305);
    }
    answer(&state.command_feedback, 41U, FURNACE_HMI_FEEDBACK_REJECTED, 2U);
    if (!update() || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_RECOVERY_REPLY_2) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_TITLE)) {
        return failed(failure, "A refused recovery is not shown in the Faults dialog.", 306);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RECOVER));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_FAULT_RECOVERY, 42U, &request)) {
        return failed(failure, "Recover is not offered again.", 307);
    }
    answer(&state.command_feedback, 42U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    state.fault_count = 1U;
    state.fault_total = 1U;
    state.faults[0] = state.faults[1];
    if (!update() || shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_7)) {
        return failed(failure, "The Faults dialog does not follow the controller's list.", 308);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE));
    if (screens.modal != NULL) {
        return failed(failure, "The Faults dialog cannot be closed.", 309);
    }
    return 0;
}

static int decision_commands(const char **failure)
{
    furnace_hmi_operator_request_t request;
    char title[96];

    /* The decision panel opens by itself, names the fault and cannot be closed. */
    manual_status(FURNACE_HMI_RUN_STATE_PAUSED);
    state.run_id = 7U;
    state.awaiting_decision = true;
    state.decision_fault_id = 10U;
    state.decision_remaining_seconds = (furnace_hmi_i32_value_t){
        .value = 95, .provenance = FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        .validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT};
    (void)snprintf(title, sizeof(title), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_TITLE_FORMAT),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_10));
    if (!update() || screens.modal == NULL || !tree_contains(screens.modal, title) ||
        !tree_contains(screens.modal, "1:35") || modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) != NULL ||
        modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE) == NULL ||
        modal_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) == NULL) {
        return failed(failure, "The decision panel does not show the decision with Continue and Stop.", 320);
    }
    /* The countdown follows the controller without rebuilding the panel. */
    lv_obj_t *panel = screens.modal;
    state.decision_remaining_seconds.value = 94;
    if (!update() || screens.modal != panel || !tree_contains(screens.modal, "1:34")) {
        return failed(failure, "The decision countdown does not follow the controller.", 321);
    }
    /* The buttons follow the link: a stale link offers neither, and both come back (review H1). */
    state.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    state.decision_remaining_seconds.validity = FURNACE_HMI_VALUE_VALIDITY_STALE;
    if (!update() || screens.modal != panel || enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE)) ||
        enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW)) || tree_contains(screens.modal, "1:34")) {
        return failed(failure, "A stale decision looks live or offers commands.", 326);
    }
    state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state.decision_remaining_seconds.validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT;
    if (!update() || !enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE)) ||
        !enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW))) {
        return failed(failure, "Continue and Stop do not come back with the link.", 327);
    }
    /* Continue is a request for the run; a rejection is shown in the panel and Continue is offered again. */
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_CONTINUE, 50U, &request) || request.run_id != 7U ||
        enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE))) {
        return failed(failure, "Continue is not a single request for the run.", 322);
    }
    answer(&state.command_feedback, 50U, FURNACE_HMI_FEEDBACK_REJECTED, 16U);
    if (!update() || screens.modal == NULL || !tree_contains(screens.modal, title) ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_REASON_CONTINUE_BLOCKED) ||
        !enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE))) {
        return failed(failure, "A rejected Continue is not shown in the decision panel.", 323);
    }
    /* Stop from the panel uses the Stop slot. */
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_STOP, 51U, &request) || request.run_id != 7U) {
        return failed(failure, "Stop from the decision panel is not a Stop request.", 324);
    }
    /* A decision with no fault named has a plain title. */
    state.decision_fault_id = 0U;
    char unnamed[64];
    (void)snprintf(unnamed, sizeof(unnamed), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_TITLE_FORMAT), 0U);
    if (!update() || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_DECISION_TITLE) ||
        tree_contains(screens.modal, title) || tree_contains(screens.modal, unnamed)) {
        return failed(failure, "A decision without a fault is not titled plainly.", 328);
    }
    /* A decision that becomes a fault closes, and the fault's alert is shown (review M1). */
    answer(&state.stop_feedback, 51U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    state.awaiting_decision = false;
    state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
    state.fault_id = 16U;
    state.fault_kind = FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE;
    if (!update() || screens.decision_open || screens.alert_open == 0U ||
        !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_OVER_TEMPERATURE_TITLE)) {
        return failed(failure, "The fault after a decision is not alerted.", 329);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE));
    state.fault_id = 0U;
    state.fault_kind = FURNACE_HMI_FAULT_KIND_NONE;
    /* The panel closes when the controller leaves the decision. */
    state.awaiting_decision = false;
    state.run_state = FURNACE_HMI_RUN_STATE_COMPLETE;
    state.run_phase = FURNACE_HMI_RUN_PHASE_COOLING;
    state.navigation_permitted.value = true;
    if (!update() || screens.modal != NULL) {
        return failed(failure, "The decision panel stays after the decision.", 325);
    }
    return 0;
}

/* Home after the run: a banner opens the Faults dialog. */
static int fault_banner(const char **failure)
{
    char text[64];

    /* The trip stays latched after the session. */
    state.faults_known = true;
    state.fault_count = 1U;
    state.fault_total = 1U;
    fault_entry(0U, 7U, false, true, true, FURNACE_HMI_FAULT_RECOVERY_PROTECTIVE_EXTERNAL);
    (void)update();
    click(screens.manual_exit_button);
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BANNER_FORMAT), 1U);
    lv_refr_now(NULL);
    if (screens.page != FURNACE_HMI_OPERATOR_PAGE_HOME || button_with_text(screens.root, text) == NULL) {
        return failed(failure, "Home has no faults banner while faults remain.", 330);
    }
    click(button_with_text(screens.root, text));
    if (screens.modal == NULL || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_7)) {
        return failed(failure, "The faults banner does not open the Faults dialog.", 331);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE));
    state.fault_count = 0U;
    state.fault_total = 0U;
    if (!update() || button_with_text(screens.root, text) != NULL) {
        return failed(failure, "The faults banner stays without faults.", 332);
    }
    return 0;
}

int furnace_hmi_operator_fault_check(lv_display_t *display, const char **failure)
{
    int result;

    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    faulted_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 300);
    }
    settle();
    /* Manual's Faults button lasts through many observations (review M2), with LVGL's timers run
     * between them as the application's GUI loop does. No observation restarts animations: the
     * steppers of a faulted session were enabled and disabled again on each one. */
    (void)update();
    const uint32_t animations = lv_anim_count_running();
    for (int pass = 0; pass < 70; ++pass) {
        (void)update();
    }
    if (lv_anim_count_running() > animations) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Observations restart animations on a faulted session.", 346);
    }
    char faults_text[32];
    (void)snprintf(faults_text, sizeof(faults_text),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BUTTON_FORMAT), 2U);
    lv_refr_now(NULL);
    if (!enabled(button_with_text(screens.root, faults_text))) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Manual's Faults button does not last.", 340);
    }
    if (screens.page_action_count >= 64U) {
        furnace_hmi_operator_screens_destroy(&screens);
        return failed(failure, "Manual uses up its button slots.", 343);
    }
    if ((result = fault_commands(failure)) == 0 && (result = decision_commands(failure)) == 0) {
        result = fault_banner(failure);
    }
    furnace_hmi_operator_screens_destroy(&screens);

    /* On Running, held by the controller, the Faults dialog stays open as the run moves (review F5). */
    if (result == 0) {
        controller_status(FURNACE_HMI_MACHINE_MODE_PROGRAM, FURNACE_HMI_RUN_STATE_FAULT, false);
        state.faults_known = true;
        state.fault_count = 1U;
        state.fault_total = 1U;
        fault_entry(0U, 3U, true, true, false, FURNACE_HMI_FAULT_RECOVERY_OPERATOR);
        /* A controller fault the screens have no kind for is alerted by its name (review M4). */
        state.fault_id = 6U;
        if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
            return failed(failure, "The operator screens could not be created.", 341);
        }
        settle();
        if (screens.alert_open == 0U || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULT_6)) {
            furnace_hmi_operator_screens_destroy(&screens);
            return failed(failure, "A fault without a kind is not alerted by its name.", 344);
        }
        click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE));
        (void)snprintf(faults_text, sizeof(faults_text),
                       furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BUTTON_FORMAT), 1U);
        lv_refr_now(NULL);
        click(button_with_text(screens.root, faults_text));
        state.stage_index = 1U;
        if (screens.modal == NULL || !update() || !screens.faults_open || screens.modal == NULL) {
            result = failed(failure, "The Faults dialog closes as the run moves.", 342);
        }
        /* No list is not "no faults" (review F4). */
        state.faults_known = false;
        state.fault_count = 0U;
        state.fault_total = 0U;
        if (result == 0 && (!update() || !shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_UNAVAILABLE) ||
                            shows(screens.modal, FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_NONE))) {
            result = failed(failure, "An unavailable fault list reads as no faults.", 345);
        }
        furnace_hmi_operator_screens_destroy(&screens);
        return result;
    }
    return result;
}

/* M21-7b: the controller's library on the screens. */
static furnace_hmi_operator_library_t library;

/* A controller program: Heating and Hold in turn, from 40 C up in 5 C steps. */
static void library_program(uint8_t slot, uint32_t id, const char *name, uint8_t stages)
{
    furnace_hmi_operator_program_t *program = &library.programs[slot];

    (void)memset(program, 0, sizeof(*program));
    program->present = true;
    program->id = id;
    program->revision = 1U;
    (void)snprintf(program->name, sizeof(program->name), "%s", name);
    program->stage_count = stages;
    for (uint8_t index = 0U; index < stages; ++index) {
        const int32_t target = 40 + (int32_t)(index / 2U) * 5;
        program->stages[index] = (index % 2U == 0U)
            ? (furnace_hmi_operator_program_stage_t){FURNACE_HMI_OPERATOR_STAGE_HEATING, target, 10, 5}
            : (furnace_hmi_operator_program_stage_t){FURNACE_HMI_OPERATOR_STAGE_HOLD, target, 0, 10};
    }
}

static bool give_library(furnace_hmi_operator_library_state_t library_state, uint8_t count,
                         bool updating)
{
    library.state = library_state;
    library.count = count;
    library.updating = updating;
    if (!furnace_hmi_operator_screens_set_library(&screens, &library)) {
        return false;
    }
    settle();
    lv_refr_now(NULL);
    return true;
}

static bool page_says(furnace_hmi_ui_string_id_t id)
{
    return tree_contains(screens.root, furnace_hmi_ui_string(id));
}

static int library_checks(const char **failure)
{
    (void)memset(&library, 0, sizeof(library));
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_LOADING, 0U, false) ||
        !show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS) || !page_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_LOADING) ||
        page_says(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_LIBRARY)) {
        return failed(failure, "A library being read is not shown as being read.", 401);
    }
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE, 0U, false) ||
        !page_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_STORAGE_UNAVAILABLE)) {
        return failed(failure, "An unavailable library is not shown as unavailable.", 402);
    }
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_TOO_LARGE, 0U, false) ||
        !page_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_TOO_LARGE) ||
        page_says(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_LIBRARY)) {
        return failed(failure, "A library too large to list is not said to be.", 403);
    }
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 0U, false) ||
        !page_says(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_LIBRARY)) {
        return failed(failure, "An empty library is not shown as empty.", 404);
    }

    library_program(0U, 7U, "Controller cure 7", 4U);
    library_program(1U, 9U, "Eighteen stages", 18U);
    library_program(2U, 11U, "Unreadable", 0U);
    library.programs[2].damaged = true;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, true) ||
        !tree_contains(screens.root, "Controller cure 7") || !tree_contains(screens.root, "Eighteen stages")) {
        return failed(failure, "The controller's programs are not listed by their names.", 405);
    }
    if (!page_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_UPDATING)) {
        return failed(failure, "A library being read again is not marked as updating.", 406);
    }
    if (!page_says(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DAMAGED) ||
        enabled(button_with_text(screens.root, "Unreadable"))) {
        return failed(failure, "A damaged program is not shown as damaged, or can be opened.", 407);
    }
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) ||
        page_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_UPDATING)) {
        return failed(failure, "A current library is still marked as updating.", 408);
    }

    /* Eighteen stages: nine to a page, on two pages. */
    click(button_with_text(screens.root, "Eighteen stages"));
    if (screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL || screens.selected_program != 1U ||
        !show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_STAGES) || !tree_contains(screens.root, "1 / 2")) {
        return failed(failure, "A program of 18 stages is not shown on two stage pages.", 409);
    }
    /* Values the editors cannot show exactly: not offered for editing. */
    library.programs[1].inexact = true;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) ||
        !show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL) ||
        enabled(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM))) ||
        !enabled(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM)))) {
        return failed(failure, "A program the editors would round can be edited, or not started.", 410);
    }

    /* The open program and the favourites follow the controller's IDs, not the places. */
    screens.quick_launch_favourites[0] = 1U;
    screens.favourite_count = 1U;
    const furnace_hmi_operator_program_t first = library.programs[0];
    library.programs[0] = library.programs[1];
    library.programs[1] = first;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) || screens.selected_program != 0U ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL || screens.favourite_count != 1U ||
        screens.quick_launch_favourites[0] != 0U || !tree_contains(screens.root, "Eighteen stages")) {
        return failed(failure, "The open program or a favourite does not follow its controller ID.", 411);
    }
    /* Deleted on the controller: its page closes, and it is no favourite. */
    library.programs[0] = library.programs[1];
    library.programs[1] = library.programs[2];
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 2U, false) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAMS || screens.favourite_count != 0U ||
        tree_contains(screens.root, "Eighteen stages")) {
        return failed(failure, "A program deleted on the controller stays open or a favourite.", 412);
    }
    /* No longer listed: nothing of the previous listing is shown. */
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE, 0U, false) ||
        tree_contains(screens.root, "Controller cure 7")) {
        return failed(failure, "An unavailable library still shows the previous listing.", 413);
    }
    return 0;
}

int furnace_hmi_operator_library_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 400);
    }
    settle();
    const int result = library_checks(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* The label that reads text. */
static lv_obj_t *label_with_text(lv_obj_t *object, const char *text)
{
    if (lv_obj_check_type(object, &lv_label_class) && strcmp(lv_label_get_text(object), text) == 0) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = label_with_text(lv_obj_get_child(object, (int32_t)index), text);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

/* The clickable card holding the label that reads text (Settings cards are not buttons). */
static lv_obj_t *card_with_text(lv_obj_t *root, const char *text)
{
    lv_obj_t *object = label_with_text(root, text);
    while (object != NULL && object != root && !lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE)) {
        object = lv_obj_get_parent(object);
    }
    return object == root ? NULL : object;
}

/* M21-7b: saves and deletes go to the controller, and only its answer changes anything. */
static bool modal_says(furnace_hmi_ui_string_id_t id)
{
    lv_refr_now(NULL);
    return screens.modal != NULL && tree_contains(screens.modal, furnace_hmi_ui_string(id));
}

/* The ordinary slot's answer to submission, as the next observation carries it, with the
 * error fields and estimate the test set before. */
static bool library_answer(uint32_t submission, furnace_hmi_feedback_state_t outcome, uint8_t reason)
{
    const furnace_hmi_command_feedback_t details = state.command_feedback;

    answer(&state.command_feedback, submission, outcome, reason);
    state.command_feedback.error_field = details.error_field;
    state.command_feedback.error_reason = details.error_reason;
    state.command_feedback.error_stage = details.error_stage;
    state.command_feedback.curing_estimate_s = details.curing_estimate_s;
    return update();
}

static int library_command_checks(const char **failure)
{
    furnace_hmi_operator_request_t request;

    (void)memset(&library, 0, sizeof(library));
    library_program(0U, 7U, "Controller cure 7", 4U);
    library_program(1U, 9U, "Second cure", 2U);
    library.programs[0].revision = 3U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 2U, false) ||
        !show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return failed(failure, "The library could not be shown.", 501);
    }
    click(button_with_text(screens.root, "Controller cure 7"));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM)));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!screens.program_editor_active || !next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 21U, &request) ||
        request.program_id != 7U || request.program_revision != 3U || request.acknowledged_duration_s != 0U ||
        strcmp(request.program_name, "Controller cure 7") != 0 || request.program.stage_count != 4U ||
        request.program.stages[1].kind != FURNACE_HMI_OPERATOR_STAGE_HOLD || request.program.stages[1].minutes != 10) {
        return failed(failure, "Save does not send the edited program with its ID and revision.", 502);
    }
    if (!modal_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_SENDING_TITLE) || !no_request()) {
        return failed(failure, "A save being answered is not shown as sent, or sends more.", 503);
    }

    /* Refused as invalid: the first error is named, and the draft stays open. */
    state.command_feedback.error_field = 6U;  /* target */
    state.command_feedback.error_reason = 11U; /* not increasing */
    state.command_feedback.error_stage = 2U;
    if (!library_answer(21U, FURNACE_HMI_FEEDBACK_REJECTED, 1U) || !screens.program_editor_active ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_NOT_ACCEPTED) ||
        !tree_contains(screens.modal, "Stage 3, Target: must be higher than the previous target.")) {
        return failed(failure, "An invalid program is not explained, or the draft is closed.", 504);
    }
    click(first_button(screens.modal));

    /* Longer than advised: saved only after the operator acknowledges the controller's estimate. */
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 22U, &request)) {
        return failed(failure, "Save cannot be sent again after a refusal.", 505);
    }
    state.command_feedback.curing_estimate_s = 36000U;
    if (!library_answer(22U, FURNACE_HMI_FEEDBACK_REJECTED, 9U) ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_ACKNOWLEDGE_TITLE) || !tree_contains(screens.modal, "600 min") ||
        !no_request()) {
        return failed(failure, "A save needing acknowledgement does not ask for it, or sends by itself.", 506);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_ACKNOWLEDGE_CONFIRM));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 23U, &request) ||
        request.acknowledged_duration_s != 36000U || request.program_id != 7U) {
        return failed(failure, "The acknowledged save does not carry the controller's estimate.", 507);
    }
    state.command_feedback.curing_estimate_s = 0U;
    if (!library_answer(23U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.program_editor_active ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_SAVED)) {
        return failed(failure, "A saved program does not close the editor on its details.", 508);
    }
    click(first_button(screens.modal));

    /* Not known whether it was saved: said so, and the draft stays. */
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM)));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 24U, &request) ||
        !library_answer(24U, FURNACE_HMI_FEEDBACK_UNRESOLVED, 0U) || !screens.program_editor_active ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_COMMAND_UNRESOLVED_TITLE) ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_UNRESOLVED_DETAIL)) {
        return failed(failure, "An unresolved save is not said to be unresolved, or the draft is closed.", 509);
    }
    click(first_button(screens.modal));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)));
    if (screens.program_editor_active) {
        return failed(failure, "The editor does not close.", 510);
    }

    /* A new program is sent without an ID. */
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return failed(failure, "Programs could not be shown.", 511);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT)));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE_ACTION)));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 25U, &request) || request.program_id != 0U ||
        request.program_revision != 0U || request.program.stage_count != 1U) {
        return failed(failure, "A new program is not sent as new.", 512);
    }
    if (!library_answer(25U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.program_editor_active ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAMS) {
        return failed(failure, "A saved new program does not return to Programs.", 513);
    }
    click(first_button(screens.modal));

    /* Delete: by ID and revision; refused, it stays; deleted, Programs is shown. */
    click(button_with_text(screens.root, "Controller cure 7"));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE, 26U, &request) || request.program_id != 7U ||
        request.program_revision != 3U) {
        return failed(failure, "Delete does not name the program and revision shown.", 514);
    }
    if (!library_answer(26U, FURNACE_HMI_FEEDBACK_REJECTED, 3U) ||
        screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_RESULT_3)) {
        return failed(failure, "A refused delete is not explained, or leaves the program.", 515);
    }
    click(first_button(screens.modal));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE, 27U, &request) ||
        !library_answer(27U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.page != FURNACE_HMI_OPERATOR_PAGE_PROGRAMS ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DELETED)) {
        return failed(failure, "A deleted program does not return to Programs.", 516);
    }
    click(first_button(screens.modal));

    /* Settings > Delete programs: one at a time, by ID, while the library changes. */
    screens.settings_tab = 3U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_SETTINGS)) {
        return failed(failure, "Settings could not be shown.", 517);
    }
    click(card_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ALL));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE, 28U, &request) || request.program_id != 7U) {
        return failed(failure, "Delete programs does not start with the first program.", 518);
    }
    /* The library is listed again before the answer: the second program moves to the first place. */
    library.programs[0] = library.programs[1];
    library.programs[0].revision = 4U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 1U, false) ||
        !library_answer(28U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) ||
        !next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE, 29U, &request) || request.program_id != 9U ||
        request.program_revision != 4U) {
        return failed(failure, "Delete programs does not follow the next program by its ID.", 519);
    }
    char deleted[96];
    (void)snprintf(deleted, sizeof(deleted),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAMS_DELETED_DETAIL_FORMAT), 2U);
    if (!library_answer(29U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || !no_request() || screens.settings_deleting ||
        !tree_contains(screens.root, deleted)) {
        return failed(failure, "Delete programs does not report what the controller deleted.", 520);
    }
    return 0;
}


/* M21-7b: names of 31 characters (PROG-09, REQ-FUN-020): whole in the editor and the save,
 * however many bytes they take, and never longer from the keyboard. */
static int name_checks(const char **failure)
{
    furnace_hmi_operator_request_t request;
    char name[80];
    size_t used = 0U;

    for (int character = 0; character < 31; ++character) {
        name[used++] = '\xD0'; /* U+0416, two bytes */
        name[used++] = '\x96';
    }
    name[used] = '\0';
    if (screens.modal != NULL) {
        click(first_button(screens.modal)); /* the previous checks' last message */
    }
    (void)memset(&library, 0, sizeof(library));
    library_program(0U, 20U, name, 2U);
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 1U, false)) {
        return failed(failure, "The library could not be shown.", 701);
    }
    screens.selected_program = 0U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL)) {
        return failed(failure, "The program could not be shown.", 702);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM)));
    if (!screens.program_editor_active || strcmp(screens.program_editor_name, name) != 0) {
        return failed(failure, "A name of 31 two-byte characters is cut in the editor.", 703);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 41U, &request) ||
        strcmp(request.program_name, name) != 0 || !library_answer(41U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U)) {
        return failed(failure, "A name of 31 two-byte characters is cut in the save.", 704);
    }
    click(first_button(screens.modal));

    /* The keyboard stops at 31 characters. */
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return failed(failure, "Programs could not be shown.", 705);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT)));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_PROGRAM_NAME)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR));
    lv_obj_t *key = NULL;
    for (int press = 0; press < 32 && screens.modal != NULL; ++press) {
        key = button_with_text(screens.modal, "1");
        click(key);
    }
    if (key == NULL || screens.keyboard_length != 31U ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_MAXIMUM_LENGTH)) {
        return failed(failure, "The keyboard takes more than 31 characters.", 706);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY));
    if (screens.modal != NULL || strlen(screens.program_editor_name) != 31U) {
        return failed(failure, "A name of 31 characters is not applied.", 707);
    }
    return 0;
}

int furnace_hmi_operator_library_command_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 500);
    }
    settle();
    int result = library_command_checks(failure);

    if (result == 0) {
        result = name_checks(failure);
    }
    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* M21-7b: program Start against a controller: the proposal panel, Confirm, Cancel, and a
 * transaction this display has no proposal for. */
static void proposal(uint32_t transaction, uint32_t revision, bool measured_valid)
{
    furnace_hmi_start_proposal_view_t *p = &state.start_proposal;

    (void)memset(p, 0, sizeof(*p));
    p->state = FURNACE_HMI_START_PROPOSAL_OPEN;
    p->transaction_id = transaction;
    p->proposal_revision = revision;
    p->program_id = 7U;
    p->program_revision = 3U;
    p->measured_valid = measured_valid;
    p->measured_decicelsius = 824;
    p->proposed_stage = 2U;
    p->skipped_count = 2U;
    p->skipped[0] = (furnace_hmi_skipped_stage_t){0U, false, 850, 0U};
    p->skipped[1] = (furnace_hmi_skipped_stage_t){1U, true, 0, 30U};
    p->remaining_curing_s = 11400U;
    p->duration_ack_required = true;
    p->recommended_s = 10800U;
    p->maximum_s = 36000U;
    p->warnings_count = 2U;
    p->warnings[0] = 1U;
    p->warnings[1] = 2U;
    state.start_pending_transaction_id = transaction;
    state.start_pending_revision = revision;
}

static bool modal_contains(const char *text)
{
    lv_refr_now(NULL);
    return screens.modal != NULL && tree_contains(screens.modal, text);
}

/* Start on the program's details, through the screens' own confirmation. */
static bool start_from_details(void)
{
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return false;
    }
    click(button_with_text(screens.root, "Controller cure 7"));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM));
    return true;
}

static int start_checks(const char **failure)
{
    furnace_hmi_operator_request_t request;

    (void)memset(&library, 0, sizeof(library));
    library_program(0U, 7U, "Controller cure 7", 6U);
    library.programs[0].revision = 3U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 1U, false) || !start_from_details() ||
        !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM, 31U, &request) || request.program_id != 7U ||
        request.program_revision != 3U || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_SENDING_TITLE)) {
        return failed(failure, "Start does not ask the controller for the program and revision shown.", 601);
    }

    /* Proposed: every skipped stage, the estimate, the duration to accept and the warnings. */
    proposal(12U, 2U, true);
    if (!library_answer(31U, FURNACE_HMI_FEEDBACK_PROPOSED, 0U) || !screens.proposal_open ||
        !modal_contains("Start Controller cure 7?") || !modal_contains("82.4") || !modal_contains("stage 3") ||
        !modal_contains("stage 1, Heating to 85.0") || !modal_contains("stage 2, Hold 30 min") ||
        !modal_contains("190 min") || !modal_contains("180 min") || !modal_contains("600 min") ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_WARNING_1) ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_WARNING_2) || !no_request()) {
        return failed(failure, "The proposal panel does not show the controller's proposal.", 602);
    }
    if (screens.page == FURNACE_HMI_OPERATOR_PAGE_RUNNING) {
        return failed(failure, "A proposal is shown as a run.", 603);
    }
    /* A new revision with an invalid temperature: shown as it now is, and Confirm is off. */
    proposal(12U, 3U, false);
    if (!update() || !screens.proposal_open ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_PROPOSAL_MEASURED_INVALID) ||
        enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CONFIRM)) ||
        !enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CANCEL))) {
        return failed(failure, "A proposal without a valid temperature can be confirmed.", 604);
    }
    proposal(12U, 3U, true);
    if (!update()) {
        return failed(failure, "The proposal could not follow the controller.", 605);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CONFIRM));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_START_CONFIRM, 32U, &request) || request.transaction_id != 12U ||
        request.proposal_revision != 3U || request.proposed_stage != 2U || request.program_id != 7U ||
        request.program_revision != 3U || request.accepted_duration_s != 11400U) {
        return failed(failure, "Confirm does not name exactly the proposal shown and the duration accepted.", 606);
    }
    /* Refused because the proposal changed: the newest one is shown again, with why. */
    proposal(12U, 4U, true);
    if (!library_answer(32U, FURNACE_HMI_FEEDBACK_REJECTED, 10U) || !screens.proposal_open ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_REASON_10) || !no_request()) {
        return failed(failure, "A refused Confirm does not show the proposal again with the reason.", 607);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CANCEL));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_START_CANCEL, 33U, &request) || request.transaction_id != 12U) {
        return failed(failure, "Cancel does not name the transaction.", 608);
    }
    state.start_proposal.state = FURNACE_HMI_START_PROPOSAL_CANCELLED;
    state.start_pending_transaction_id = 0U;
    if (!library_answer(33U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.modal != NULL || !no_request()) {
        return failed(failure, "A cancelled proposal stays on screen or sends more.", 609);
    }

    /* Withdrawn by the controller: the panel closes and says why. */
    if (!start_from_details() || !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM, 34U, &request)) {
        return failed(failure, "Start cannot be asked for again.", 610);
    }
    proposal(14U, 1U, true);
    if (!library_answer(34U, FURNACE_HMI_FEEDBACK_PROPOSED, 0U) || !screens.proposal_open) {
        return failed(failure, "The second proposal is not shown.", 611);
    }
    state.start_proposal.state = FURNACE_HMI_START_PROPOSAL_INVALIDATED;
    state.start_proposal.invalidation_reason = 20U;
    state.start_pending_transaction_id = 0U;
    if (!update() || screens.proposal_open || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_INVALIDATED_TITLE) ||
        !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_REASON_20)) {
        return failed(failure, "A withdrawn proposal is not said to be withdrawn, with its reason.", 612);
    }
    click(first_button(screens.modal));

    /* A transaction this display has no proposal for: pointed out once, and cancelled on request. */
    (void)memset(&state.start_proposal, 0, sizeof(state.start_proposal));
    state.start_pending_transaction_id = 15U;
    if (!update() || !screens.start_waiting_open || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_WAITING_TITLE) ||
        !no_request()) {
        return failed(failure, "A waiting transaction is not pointed out, or is cancelled by itself.", 613);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_WAITING_CANCEL));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_START_CANCEL, 35U, &request) || request.transaction_id != 15U) {
        return failed(failure, "Cancel of a waiting transaction does not name it.", 614);
    }
    state.start_pending_transaction_id = 0U;
    if (!library_answer(35U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.modal != NULL) {
        return failed(failure, "The waiting transaction's notice stays after its Cancel.", 615);
    }
    state.start_pending_transaction_id = 16U;
    if (!update() || !screens.start_waiting_open) {
        return failed(failure, "A second waiting transaction is not pointed out.", 616);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_WAITING_KEEP));
    if (!update() || screens.modal != NULL || !no_request()) {
        return failed(failure, "A kept waiting transaction is pointed out again, or cancelled.", 617);
    }
    /* A Start refused because of it: pointed out again. */
    if (!start_from_details() || !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM, 36U, &request) ||
        !library_answer(36U, FURNACE_HMI_FEEDBACK_REJECTED, 6U) || !screens.start_waiting_open) {
        return failed(failure, "A Start refused because of a waiting transaction does not point it out.", 618);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_WAITING_KEEP));

    /* Accepted at once (nothing to confirm): the program is the most recent. */
    state.start_pending_transaction_id = 0U;
    if (!start_from_details() || !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM, 37U, &request) ||
        !library_answer(37U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || screens.modal != NULL ||
        screens.recent_count == 0U || screens.quick_launch_recent[0] != 0U) {
        return failed(failure, "An accepted Start does not close, or is not the most recent program.", 619);
    }
    return 0;
}

int furnace_hmi_operator_start_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 600);
    }
    settle();
    const int result = start_checks(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* M21-7b review: the controller's library changing under an open editor, dialog or panel. */
static bool open_editor_on(const char *name)
{
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return false;
    }
    click(button_with_text(screens.root, name));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM)));
    return screens.program_editor_active;
}

static void three_programs(void)
{
    (void)memset(&library, 0, sizeof(library));
    library_program(0U, 7U, "Controller cure 7", 4U);
    library_program(1U, 9U, "Second cure", 2U);
    library_program(2U, 11U, "Third cure", 2U);
    library.programs[0].revision = 3U;
}

static int review_checks(const char **failure)
{
    furnace_hmi_operator_request_t request;

    /* The editor holds the program it opened: another revision listed meanwhile changes nothing,
     * and the save names the revision it was edited from (review H2). */
    three_programs();
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) || !open_editor_on("Controller cure 7")) {
        return failed(failure, "The editor could not be opened.", 801);
    }
    library.programs[0].revision = 4U;
    library.programs[0].stages[0].target_c = 99;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not change.", 802);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 51U, &request) || request.program_id != 7U ||
        request.program_revision != 3U || request.program.stages[0].target_c != 40) {
        return failed(failure, "A save names a revision or stages the draft was not made from.", 803);
    }
    if (!library_answer(51U, FURNACE_HMI_FEEDBACK_REJECTED, 3U) || !screens.program_editor_active) {
        return failed(failure, "A save refused for a newer revision closes the draft.", 804);
    }
    click(first_button(screens.modal));
    /* The library being read again (a new controller session) turns no draft into a new program (H3). */
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_LOADING, 0U, false) ||
        !give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not be read again.", 805);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE, 52U, &request) || request.program_id != 7U ||
        request.program_revision != 3U) {
        return failed(failure, "A draft becomes a new program after the library is read again.", 806);
    }
    /* The answer belongs to that draft: one opened after it stays open (M7). */
    click(first_button(screens.modal));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)));
    if (screens.modal != NULL) {
        click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES));
    }
    if (screens.program_editor_active || !open_editor_on("Second cure") ||
        !library_answer(52U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U) || !screens.program_editor_active) {
        return failed(failure, "A save's answer closes another draft.", 807);
    }
    click(first_button(screens.modal));
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)));

    /* An inexact program is edited from no page (review H4). */
    library.programs[1].inexact = true;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not change.", 808);
    }
    screens.selected_program = 1U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_STAGES)) {
        return failed(failure, "The stages could not be shown.", 809);
    }
    lv_obj_t *edit = button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM));
    const bool offered = enabled(edit);
    click(edit);
    if (offered || screens.program_editor_active) {
        return failed(failure, "An inexact program can be edited from its stages.", 810);
    }
    library.programs[1].inexact = false;

    /* Delete programs deletes exactly the programs it named (review H1). */
    three_programs();
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not change.", 811);
    }
    screens.settings_tab = 3U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_SETTINGS)) {
        return failed(failure, "Settings could not be shown.", 812);
    }
    click(card_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)));
    /* Before Confirm, another display deletes the first and saves a fourth. */
    library.programs[0] = library.programs[1];
    library.programs[1] = library.programs[2];
    library_program(2U, 13U, "Fourth cure", 2U);
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not change.", 813);
    }
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ALL));
    uint32_t deleted[4] = {0U, 0U, 0U, 0U};
    for (uint32_t step = 0U; step < 4U; ++step) {
        if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE, 53U + step, &request)) {
            break;
        }
        deleted[step] = request.program_id;
        (void)library_answer(53U + step, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    }
    if (deleted[0] != 9U || deleted[1] != 11U || deleted[2] != 0U) {
        return failed(failure, "Delete programs deletes a program it did not name.", 814);
    }
    if (screens.modal != NULL) {
        click(first_button(screens.modal));
    }

    /* Favourites last through a library briefly unavailable (M5). */
    three_programs();
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false)) {
        return failed(failure, "The library could not change.", 815);
    }
    screens.quick_launch_favourites[0] = 1U;
    screens.favourite_count = 1U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_UNAVAILABLE, 0U, false) ||
        !give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) || screens.favourite_count != 1U ||
        screens.quick_launch_favourites[0] != 1U) {
        return failed(failure, "A favourite is lost while the library is unavailable.", 816);
    }

    /* The controller's estimate is the duration shown, not one from an assumed start (PROG-14). */
    library.programs[2].curing_estimate_s = 6000U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) || !show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS) ||
        !tree_contains(screens.root, "100 min")) {
        return failed(failure, "A program's duration is not the controller's estimate.", 817);
    }

    /* A Start dialog for a revision no longer listed closes (M2). */
    screens.selected_program = 0U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL)) {
        return failed(failure, "The program could not be shown.", 818);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM)));
    library.programs[0].revision = 5U;
    if (screens.modal == NULL || !give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) ||
        screens.modal != NULL || !no_request()) {
        return failed(failure, "A Start dialog stays open for a revision no longer listed.", 819);
    }

    /* Confirm acts only on the proposal on screen (M1). */
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM)));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM, 60U, &request) || request.program_revision != 5U) {
        return failed(failure, "Start does not name the revision shown.", 820);
    }
    proposal(21U, 2U, true);
    if (!library_answer(60U, FURNACE_HMI_FEEDBACK_PROPOSED, 0U) || !screens.proposal_open) {
        return failed(failure, "The proposal is not shown.", 821);
    }
    screens.observation.start_proposal.proposal_revision = 3U; /* newer, not drawn yet */
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CONFIRM));
    if (!no_request()) {
        return failed(failure, "Confirm names a proposal revision that was not on screen.", 822);
    }
    /* Every skipped stage is listed, also eighteen (M3). */
    state.start_proposal.proposal_revision = 4U;
    state.start_proposal.skipped_count = 18U;
    for (uint8_t index = 0U; index < 18U; ++index) {
        state.start_proposal.skipped[index] = (furnace_hmi_skipped_stage_t){index, index % 2U == 1U, 1855, 600U};
    }
    if (!update() || !screens.proposal_open || !modal_contains("stage 18, Hold 600 min")) {
        return failed(failure, "Skipped stages are dropped from the proposal.", 823);
    }
    /* A library change does not close the proposal panel (audit 15). */
    library.programs[0].revision = 6U;
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 3U, false) || !screens.proposal_open) {
        return failed(failure, "A library change closes the proposal panel.", 824);
    }
    /* Ended by a session change: said so (M4). */
    state.start_proposal.state = FURNACE_HMI_START_PROPOSAL_ENDED;
    state.start_pending_transaction_id = 0U;
    if (!update() || screens.proposal_open || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_START_ENDED_DETAIL)) {
        return failed(failure, "A proposal ended by a session change disappears without a word.", 825);
    }
    click(first_button(screens.modal));
    return 0;
}

int furnace_hmi_operator_library_review_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 800);
    }
    settle();
    const int result = review_checks(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* ---- M21-7c: the controller's Device log and health ---------------------------------------- */

static furnace_hmi_controller_health_t health_fixture;
static furnace_hmi_controller_history_t history_fixture;

/* Entry `index` of the history fixture, with up to four parameters. */
static void history_entry(uint16_t index, uint8_t log, uint32_t boot, uint64_t mono_ms,
                          uint16_t event, uint8_t severity, uint8_t category, uint8_t params,
                          uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3)
{
    furnace_hmi_controller_history_entry_t *e = &history_fixture.entries[index];
    const uint32_t values[4] = {p0, p1, p2, p3};

    (void)memset(e, 0, sizeof(*e));
    e->log = log;
    e->log_sequence = 100U + index;
    e->boot_id = boot;
    e->mono_ms = mono_ms;
    e->event_id = event;
    e->severity = severity;
    e->category = category;
    e->param_count = params;
    for (uint8_t i = 0U; i < params && i < 4U; ++i) {
        e->param_types[i] = 6U;
        e->param_values[i] = values[i];
    }
}

static bool give_diagnostics(void)
{
    if (!furnace_hmi_operator_screens_set_diagnostics(&screens, &health_fixture, &history_fixture)) {
        return false;
    }
    settle();
    lv_refr_now(NULL);
    return true;
}

static bool open_device_tab(furnace_hmi_ui_string_id_t tab)
{
    if (!show(FURNACE_HMI_OPERATOR_PAGE_DEVICE)) {
        return false;
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(tab)));
    lv_refr_now(NULL);
    return true;
}

/* The Device page's body, beside its category rail (whose names a filter may share). */
static lv_obj_t *device_body(void)
{
    return lv_obj_get_child(screens.content, 1);
}

/* The Device log shows text on one of its pages; it is left on the first page. */
static bool log_shows(const char *text)
{
    bool found = false;

    for (uint8_t page = 0U; page < 4U && !found; ++page) {
        screens.device_page = page;
        found = give_diagnostics() && tree_contains(screens.root, text);
    }
    screens.device_page = 0U;
    return give_diagnostics() && found;
}

/* "<format>" filled with one unsigned number, as the screens write it. */
static const char *formatted(furnace_hmi_ui_string_id_t format, unsigned int value)
{
    static char text[96];

    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(format), value);
    return text;
}

static int diagnostics_checks(const char **failure)
{
    char text[128];

    (void)memset(&health_fixture, 0, sizeof(health_fixture));
    (void)memset(&history_fixture, 0, sizeof(history_fixture));
    if (!give_diagnostics() || !open_device_tab(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HEALTH)) {
        return failed(failure, "The Device page has no Health category.", 900);
    }
    if (!page_says(FURNACE_HMI_UI_STRING_HEALTH_NOT_REPORTED)) {
        return failed(failure, "Health not yet reported is not said to be.", 901);
    }
    if (furnace_hmi_operator_screens_take_history_read(&screens)) {
        return failed(failure, "Health asks for a history read.", 902);
    }
    if (!open_device_tab(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG) ||
        !furnace_hmi_operator_screens_take_history_read(&screens) ||
        furnace_hmi_operator_screens_take_history_read(&screens)) {
        return failed(failure, "Opening the Device log does not ask once for the controller's history.", 903);
    }
    if (!page_says(FURNACE_HMI_UI_STRING_CTRL_LOG_NOT_READ) ||
        page_says(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_UNAVAILABLE)) {
        return failed(failure, "A history not read yet is not said to be.", 904);
    }
    history_fixture.reading = true;
    if (!give_diagnostics() || !page_says(FURNACE_HMI_UI_STRING_CTRL_LOG_READING)) {
        return failed(failure, "A history being read is not said to be.", 905);
    }

    /* Newest first: start 3 (a run and a fault), then start 2. */
    history_fixture.state = FURNACE_HMI_HISTORY_READ;
    history_fixture.reading = false;
    history_fixture.count = 8U;
    history_entry(0U, 2U, 3U, 3723000U, 11U, 0U, 3U, 4U, 1U, 0U, 0U, 12U);
    history_entry(1U, 2U, 3U, 3000000U, 11U, 0U, 3U, 4U, 5U, 1U, 0U, 12U);
    history_entry(2U, 1U, 3U, 2000000U, 12U, 2U, 4U, 4U, 1U, 0U, 3U, 1U); /* class 3, transition 1 */
    history_entry(3U, 1U, 3U, 1000000U, 99U, 0U, 1U, 0U, 0U, 0U, 0U, 0U);
    history_entry(4U, 1U, 2U, 5000U, 30U, 0U, 1U, 1U, 2U, 0U, 0U, 0U);
    history_entry(5U, 1U, 2U, 4000U, 11U, 3U, 3U, 4U, 20U, 0U, 0U, 9U);  /* a faulted run: an alarm */
    history_entry(6U, 1U, 1U, 1000U, 10U, 0U, 1U, 1U, 1U, 0U, 0U, 0U);   /* on a valid clock */
    history_fixture.entries[6].wall_state = 1U;
    history_fixture.entries[6].wall_ms = INT64_C(1795163400000);         /* 2026-11-20 08:30 */
    history_entry(7U, 1U, 1U, 500U, 77U, 7U, 1U, 0U, 0U, 0U, 0U, 0U);    /* unknown event and severity */
    history_fixture.logs[0].truncated = true;
    history_fixture.logs[0].state = FURNACE_HMI_HISTORY_LOG_READ;
    history_fixture.logs[1].state = FURNACE_HMI_HISTORY_LOG_READ;
    history_fixture.logs[2].state = FURNACE_HMI_HISTORY_LOG_UNAVAILABLE;
    if (!give_diagnostics()) {
        return failed(failure, "The history could not be shown.", 906);
    }
    if (!log_shows("Run 12: started") || !log_shows("Run 12, stage 2: target reached") ||
        !log_shows(formatted(FURNACE_HMI_UI_STRING_CTRL_LOG_BOOT_FORMAT, 2U))) {
        return failed(failure, "The controller's entries are not listed with their texts.", 907);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_UNKNOWN_FORMAT), 99U,
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_SEVERITY_0));
    if (!log_shows(text)) {
        return failed(failure, "An unknown event is not listed by its number and severity.", 924);
    }
    (void)snprintf(text, sizeof(text), "%s: %s", furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_FAULT_TRANSITION_1));
    if (!log_shows(text)) {
        return failed(failure, "A fault entry is not named by the fault and its transition.", 908);
    }
    if (!log_shows(formatted(FURNACE_HMI_UI_STRING_CTRL_LOG_START_HEADING_FORMAT, 3U)) ||
        !log_shows(formatted(FURNACE_HMI_UI_STRING_CTRL_LOG_START_HEADING_FORMAT, 2U)) ||
        !log_shows("+1:02:03")) {
        return failed(failure, "Entries without a clock are not grouped by start with their time since it.", 909);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_UNAVAILABLE_FORMAT),
                   furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_NAME_3));
    if (!tree_contains(screens.root, text) || !page_says(FURNACE_HMI_UI_STRING_CTRL_LOG_OLDER_NOT_SHOWN)) {
        return failed(failure, "An unavailable log, or older entries not shown, are not said.", 910);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_UNKNOWN_FORMAT), 77U,
                   formatted(FURNACE_HMI_UI_STRING_CTRL_SEVERITY_FORMAT, 7U));
    if (!log_shows(text) || !log_shows("Controller ready") ||
        log_shows("+0:00:01")) { /* on a valid clock: its clock time, not the time since its start */
        return failed(failure, "An unknown severity, or an entry on a valid clock, is not shown as it is.", 922);
    }
    click(button_with_text(device_body(), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_ALARMS)));
    lv_refr_now(NULL);
    if (log_shows("Run 12: started") || !log_shows(furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1)) ||
        !log_shows("Run 9: faulted, heating stopped")) {
        return failed(failure, "Alarms does not list only the alarms.", 911);
    }
    click(button_with_text(device_body(), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_PROGRAMS)));
    lv_refr_now(NULL);
    /* A faulted run is both a run and an alarm (owner 2026-11-20). */
    if (!log_shows("Run 12: started") || !log_shows("Run 9: faulted, heating stopped") ||
        log_shows(furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1))) {
        return failed(failure, "Programs does not list every run, a faulted one included, or lists more.", 923);
    }
    click(button_with_text(device_body(), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_MAINTENANCE)));
    lv_refr_now(NULL);
    if (!page_says(FURNACE_HMI_UI_STRING_CTRL_LOG_MAINTENANCE_NOT_RECORDED)) {
        return failed(failure, "Maintenance does not say that the controller does not record it.", 912);
    }
    click(button_with_text(device_body(), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ALL)));
    lv_refr_now(NULL);
    click(card_with_text(screens.root, "Run 12: started"));
    if (screens.modal == NULL || !modal_contains(furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_NAME_2)) ||
        !modal_contains("+1:02:03")) {
        return failed(failure, "An entry's details do not name its log and time.", 913);
    }
    click(first_button(screens.modal));
    history_fixture.interrupted = true;
    if (!give_diagnostics() || !page_says(FURNACE_HMI_UI_STRING_CTRL_LOG_NOT_CURRENT)) {
        return failed(failure, "A history not current is not said to be.", 914);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_CTRL_LOG_REFRESH)));
    if (!furnace_hmi_operator_screens_take_history_read(&screens)) {
        return failed(failure, "Refresh does not ask for the history.", 915);
    }

    /* Health. */
    health_fixture.valid = true;
    health_fixture.current = true;
    health_fixture.overall = FURNACE_HMI_HEALTH_WARNING;
    health_fixture.uptime_ms = 3723000U;
    health_fixture.boot_count = 7U;
    health_fixture.reset_cause_known = true;
    health_fixture.reset_cause = 0x2U;
    health_fixture.subsystem_count = 3U;
    health_fixture.subsystems[0] = (furnace_hmi_controller_subsystem_t){1U, FURNACE_HMI_HEALTH_WARNING, 3U};
    health_fixture.subsystems[1] = (furnace_hmi_controller_subsystem_t){3U, FURNACE_HMI_HEALTH_UNKNOWN, 0U};
    health_fixture.subsystems[2] = (furnace_hmi_controller_subsystem_t){10U, FURNACE_HMI_HEALTH_OK, 0U};
    health_fixture.faults_active = 1U;
    health_fixture.faults_latched = 2U;
    health_fixture.faults_unacknowledged = 3U;
    health_fixture.service_valid = true;
    health_fixture.service_s = 7200U;
    if (!give_diagnostics() || !open_device_tab(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HEALTH) ||
        !page_says(FURNACE_HMI_UI_STRING_HEALTH_TITLE)) {
        return failed(failure, "Reported health is not shown.", 916);
    }
    if (!page_says(FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_1) || !page_says(FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_3) ||
        !page_says(FURNACE_HMI_UI_STRING_HEALTH_SUBSYSTEM_10) || !page_says(FURNACE_HMI_UI_STRING_HEALTH_STATE_0) ||
        !page_says(FURNACE_HMI_UI_STRING_HEALTH_STATE_2) ||
        !tree_contains(screens.root, formatted(FURNACE_HMI_UI_STRING_HEALTH_DETAIL_SKIPPED_FORMAT, 3U))) {
        return failed(failure, "The subsystems are not shown with their states, unknown as unknown.", 917);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_HEALTH_FAULTS_DETAIL_FORMAT), 2U, 3U);
    if (!tree_contains(screens.root, formatted(FURNACE_HMI_UI_STRING_HEALTH_FAULTS_FORMAT, 1U)) ||
        !tree_contains(screens.root, text) || !tree_contains(screens.root, "0x2") ||
        !tree_contains(screens.root, formatted(FURNACE_HMI_UI_STRING_HEALTH_SERVICE_FORMAT, 2U))) {
        return failed(failure, "The fault counts, starts or service hours are not shown.", 918);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_HEALTH_UPTIME_FORMAT), 1U, 2U);
    if (!tree_contains(screens.root, text) || page_says(FURNACE_HMI_UI_STRING_HEALTH_STALE)) {
        return failed(failure, "The uptime is not shown, or current health is called stale.", 919);
    }
    health_fixture.current = false;
    if (!give_diagnostics() || !page_says(FURNACE_HMI_UI_STRING_HEALTH_STALE)) {
        return failed(failure, "Health that is not current is not said to be.", 920);
    }
    health_fixture.service_valid = false;
    if (!give_diagnostics() || !page_says(FURNACE_HMI_UI_STRING_HEALTH_SERVICE_UNKNOWN)) {
        return failed(failure, "Unknown service hours are shown as a number.", 921);
    }
    return 0;
}

int furnace_hmi_operator_diagnostics_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content));
    reported_limits();
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 899);
    }
    settle();
    const int result = diagnostics_checks(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}

/* ---- M21-7d: the controller's limits and the planning reference ---------------------------- */

/* One program of the given Heating targets (whole degrees) at 1.0 C/min, and a Hold when hold > 0. */
static void program_of_targets(uint8_t slot, uint32_t id, const char *name, const int32_t *targets,
                               uint8_t count, int32_t hold)
{
    furnace_hmi_operator_program_t *program = &library.programs[slot];

    (void)memset(program, 0, sizeof(*program));
    program->present = true;
    program->id = id;
    program->revision = 1U;
    (void)snprintf(program->name, sizeof(program->name), "%s", name);
    for (uint8_t index = 0U; index < count; ++index) {
        program->stages[index] = (furnace_hmi_operator_program_stage_t){
            FURNACE_HMI_OPERATOR_STAGE_HEATING, targets[index], 10, 10};
    }
    program->stage_count = count;
    if (hold > 0) {
        program->stages[count] = (furnace_hmi_operator_program_stage_t){
            FURNACE_HMI_OPERATOR_STAGE_HOLD, targets[count - 1U], 0, hold};
        program->stage_count = (uint8_t)(count + 1U);
    }
}

/* Save from the program editor of the named program: whether its dialog says text; nothing sent. */
static bool save_says(const char *name, const char *text)
{
    if (!open_editor_on(name)) {
        return false;
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)));
    const bool said = modal_contains(text);
    click(first_button(screens.modal));
    return said && no_request();
}

static int limits_checks(const char **failure)
{
    static const int32_t one[] = {120};
    static const int32_t falling[] = {100, 90};
    static const int32_t above[] = {190};
    furnace_hmi_operator_request_t request;
    char text[96];

    (void)memset(&library, 0, sizeof(library));
    program_of_targets(0U, 30U, "One stage", one, 1U, 0);
    program_of_targets(1U, 31U, "Falling", falling, 2U, 0);
    program_of_targets(2U, 32U, "Too hot", above, 1U, 0);
    program_of_targets(3U, 33U, "Too long", one, 1U, 3000);
    if (!give_library(FURNACE_HMI_OPERATOR_LIBRARY_CURRENT, 4U, false)) {
        return failed(failure, "The library could not be shown.", 1001);
    }

    /* Without the controller's limits nothing is edited, and Manual does not start. */
    screens.selected_program = 0U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL) ||
        enabled(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM)))) {
        return failed(failure, "A program can be edited without the controller's limits.", 1002);
    }
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS)) {
        return failed(failure, "Programs could not be shown.", 1003);
    }
    click(button_with_text(screens.root, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT)));
    /* An information dialog closes; it does not cancel (owner 2026-11-20). */
    if (screens.program_editor_active || !modal_says(FURNACE_HMI_UI_STRING_UI_LAB_LIMITS_UNKNOWN_TITLE) ||
        button_with_text(screens.modal, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) == NULL ||
        button_with_text(screens.modal, furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) != NULL) {
        return failed(failure, "A new program without limits is not refused with a note that closes.", 1004);
    }
    click(first_button(screens.modal));
    /* Manual asks for the controller's readiness when it opens; answered, it is ready. */
    if (!show(FURNACE_HMI_OPERATOR_PAGE_MANUAL) ||
        !next_request(FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE, 60U, &request)) {
        return failed(failure, "Manual does not ask for the controller's readiness.", 1017);
    }
    answer(&state.command_feedback, 60U, FURNACE_HMI_FEEDBACK_ACCEPTED, 0U);
    if (!update() || enabled(screens.manual_start_button)) {
        return failed(failure, "Manual starts without the controller's limits.", 1005);
    }

    /* The controller reports them: editing and Manual are offered. */
    if (!furnace_hmi_operator_screens_set_program_limits(&screens, &fixture_limits, &fixture_reference)) {
        return failed(failure, "The limits could not be given.", 1006);
    }
    settle();
    if (!enabled(screens.manual_start_button)) {
        return failed(failure, "Manual does not start with the controller's limits.", 1007);
    }

    /* One stage reads "1 stage". */
    if (!show(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS) ||
        !tree_contains(screens.root, formatted(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COUNT_ONE_FORMAT, 1U)) ||
        tree_contains(screens.root, formatted(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COUNT_FORMAT, 1U))) {
        return failed(failure, "One stage does not read as one stage.", 1008);
    }

    /* The controller's rules: rising Heating targets, its maximum, its longest curing. */
    if (!save_says("Falling", formatted(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_START_FORMAT, 2U))) {
        return failed(failure, "A Heating target below the previous one is not refused.", 1009);
    }
    if (!save_says("Too hot", formatted(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_TARGET_FORMAT, 1U))) {
        return failed(failure, "A target above the controller's maximum is not refused.", 1010);
    }
    (void)snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_TOTAL_FORMAT),
                   (long)fixture_limits.curing_max_minutes);
    if (!save_says("Too long", text)) {
        return failed(failure, "Curing beyond the controller's longest is not refused.", 1011);
    }

    /* Settings > Programs: the reference, its range, and a change the controller answers. */
    screens.program_editor_active = false;
    screens.settings_tab = 4U;
    if (!show(FURNACE_HMI_OPERATOR_PAGE_SETTINGS) || !tree_contains(screens.root, "23.0 °C") ||
        !tree_contains(screens.root, "5.0–40.0 °C") ||
        !page_says(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REFERENCE_CAPTION)) {
        return failed(failure, "Settings does not show the planning reference and its range.", 1012);
    }
    click(card_with_text(screens.root, "23.0 °C"));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR));
    click(button_with_text(screens.modal, "2"));
    click(button_with_text(screens.modal, "5"));
    click(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY));
    if (!next_request(FURNACE_HMI_OPERATOR_REQUEST_PLANNING_REFERENCE, 61U, &request) ||
        request.planning_reference_decicelsius != 250 ||
        !page_says(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REFERENCE_PENDING)) {
        return failed(failure, "A new reference is not asked of the controller.", 1013);
    }
    answer(&state.command_feedback, 61U, FURNACE_HMI_FEEDBACK_REJECTED, 1U);
    if (!update() || !page_says(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REFERENCE_OUT_OF_RANGE) ||
        !tree_contains(screens.root, "23.0 °C")) {
        return failed(failure, "A refusal is not shown, or the value shown is not the one in effect.", 1014);
    }

    /* During execution the controller refuses a change: not offered. */
    furnace_hmi_operator_planning_reference_t running = fixture_reference;
    running.change_permitted = false;
    if (!furnace_hmi_operator_screens_set_program_limits(&screens, &fixture_limits, &running)) {
        return failed(failure, "The limits could not be given.", 1015);
    }
    settle();
    click(card_with_text(screens.root, "23.0 °C"));
    if (screens.modal != NULL || !page_says(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REFERENCE_DURING_RUN)) {
        return failed(failure, "The reference is offered for change during execution.", 1016);
    }
    return 0;
}

int furnace_hmi_operator_limits_check(lv_display_t *display, const char **failure)
{
    lv_display_set_resolution(display, 800, 480);
    (void)memset(&content, 0, sizeof(content)); /* a controller that has not reported its limits */
    furnace_hmi_dashboard_state_initialize(&state);
    ready_status();
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &state, &content)) {
        return failed(failure, "The operator screens could not be created.", 1000);
    }
    settle();
    const int result = limits_checks(failure);

    furnace_hmi_operator_screens_destroy(&screens);
    return result;
}
