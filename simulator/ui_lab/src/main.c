/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/simulated_controller.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>
#include <SDL.h>

#include "ui_lab_explorer.h"

#if defined(__ZEPHYR__)
#error "The UI laboratory is host-only and must never be built for Zephyr."
#endif

#if !defined(FURNACE_HMI_DESKTOP_WIDTH) || !defined(FURNACE_HMI_DESKTOP_HEIGHT)
#error "UI laboratory dimensions must be supplied by the target build."
#endif

typedef struct {
    furnace_hmi_simulator_t simulator;
    furnace_hmi_ui_lab_explorer_t explorer;
    furnace_hmi_ui_lab_device_catalogue_t device_catalogue;
    lv_obj_t *simulation_marker;
    volatile int requested_scenario;
    volatile int requested_page;
    volatile bool requested_service_dialog;
    volatile bool quit_requested;
} ui_lab_context_t;

static const char *ui_string(furnace_hmi_ui_string_id_t id)
{
    const char *text = furnace_hmi_ui_string(id);
    return text == NULL ? "" : text;
}

static void handle_timers(uint32_t iterations)
{
    for (uint32_t iteration = 0; iteration < iterations; ++iteration) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms == LV_NO_TIMER_READY || delay_ms > 20U) {
            delay_ms = 5U;
        }
        lv_delay_ms(delay_ms);
    }
}

static void clear_views(ui_lab_context_t *context)
{
    furnace_hmi_ui_lab_explorer_destroy(&context->explorer);
    if (context->simulation_marker != NULL && lv_obj_is_valid(context->simulation_marker)) {
        lv_obj_delete(context->simulation_marker);
    }
    context->simulation_marker = NULL;
}

static bool create_simulation_marker(ui_lab_context_t *context)
{
    context->simulation_marker = lv_label_create(lv_layer_top());
    if (context->simulation_marker == NULL) {
        return false;
    }
    lv_label_set_text_static(
        context->simulation_marker,
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SIMULATION_BANNER)
    );
    lv_obj_set_style_text_font(context->simulation_marker, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(context->simulation_marker, lv_color_hex(0xE8A04B), LV_PART_MAIN);
    /* A dark tag on the top line keeps the marker out of page content. */
    lv_obj_set_style_bg_color(context->simulation_marker, lv_color_hex(0x171B1E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(context->simulation_marker, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(context->simulation_marker, 6, LV_PART_MAIN);
    lv_obj_align(context->simulation_marker, LV_ALIGN_TOP_RIGHT, -10, -4);
    lv_obj_remove_flag(
        context->simulation_marker,
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE
    );
    return true;
}

static bool render_scenario(ui_lab_context_t *context)
{
    const furnace_hmi_dashboard_state_t *state =
        furnace_hmi_simulator_state(&context->simulator);

    clear_views(context);
    if (state == NULL) {
        return false;
    }
    if (!furnace_hmi_ui_lab_explorer_create(
            &context->explorer, lv_screen_active(), state, &context->device_catalogue)) {
        return false;
    }

    return create_simulation_marker(context);
}

static int event_watch(void *userdata, SDL_Event *event)
{
    ui_lab_context_t *context = userdata;
    if (event->type == SDL_QUIT) {
        context->quit_requested = true;
        return 0;
    }
    if (event->type != SDL_KEYDOWN || event->key.repeat != 0) {
        return 0;
    }

    furnace_hmi_sim_scenario_t scenario;
    switch (event->key.keysym.sym) {
    case SDLK_1:
        scenario = FURNACE_HMI_SIM_SCENARIO_DISCONNECTED;
        break;
    case SDLK_2:
        scenario = FURNACE_HMI_SIM_SCENARIO_IDLE;
        break;
    case SDLK_3:
        scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL;
        break;
    case SDLK_4:
        scenario = FURNACE_HMI_SIM_SCENARIO_MANUAL;
        break;
    case SDLK_5:
        scenario = FURNACE_HMI_SIM_SCENARIO_PAUSED;
        break;
    case SDLK_6:
        scenario = FURNACE_HMI_SIM_SCENARIO_FAULT;
        break;
    case SDLK_7:
        scenario = FURNACE_HMI_SIM_SCENARIO_STALE;
        break;
    case SDLK_8:
        scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_OVERRUN;
        break;
    case SDLK_9:
        scenario = FURNACE_HMI_SIM_SCENARIO_COOLING;
        break;
    case SDLK_h:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_HOME;
        return 0;
    case SDLK_p:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_PROGRAMS;
        return 0;
    case SDLK_d:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_DEVICE;
        return 0;
    case SDLK_s:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_SETTINGS;
        return 0;
    case SDLK_r:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_RUNNING;
        return 0;
    case SDLK_i:
        context->requested_service_dialog = true;
        return 0;
    default:
        return 0;
    }
    context->requested_scenario = (int)scenario;
    return 0;
}

static bool parse_arguments(
    int argc,
    char **argv,
    furnace_hmi_sim_scenario_t *scenario,
    bool *smoke_test,
    bool *list_scenarios,
    const char **capture_directory
)
{
    *scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL;
    *smoke_test = false;
    *list_scenarios = false;
    *capture_directory = NULL;
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--capture") == 0 && index + 1 < argc) {
            *capture_directory = argv[++index];
        }
        else if (strcmp(argv[index], "--smoke-test") == 0) {
            *smoke_test = true;
        }
        else if (strcmp(argv[index], "--list-scenarios") == 0) {
            *list_scenarios = true;
        }
        else if (strcmp(argv[index], "--scenario") == 0 && index + 1 < argc) {
            ++index;
            if (!furnace_hmi_sim_scenario_from_name(argv[index], scenario)) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}

static void print_usage(const char *executable)
{
    (void)fprintf(
        stderr,
        "Usage: %s [--scenario NAME] [--smoke-test] [--list-scenarios] [--capture DIRECTORY]\n",
        executable
    );
    (void)fprintf(stderr, "Scenarios: disconnected, idle, running-normal, manual, paused, fault, stale, running-overrun\n");
    (void)fprintf(stderr, "Keyboard: 1-8 scenarios; H home; P programs; R running; D device; S settings; I service dialog\n");
}

/* Host-only deterministic captures run on the same owner as LVGL. SDL reads
 * the app renderer, never the desktop; one bounded surface is freed per frame. */
static unsigned audit_geometry(lv_obj_t *object)
{
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) { return 0; }
    unsigned errors = 0;
    bool label = lv_obj_check_type(object, &lv_label_class);
    bool button = lv_obj_check_type(object, &lv_button_class);
    if (!lv_obj_check_type(object, &lv_line_class)) {
        lv_area_t area; lv_obj_get_coords(object, &area);
        const char *text = label ? lv_label_get_text(object) : button ? "[button]" : "[container]";
        if (label && text[0] == '\0') { return 0; }
        for (lv_obj_t *parent = lv_obj_get_parent(object); parent != NULL; parent = lv_obj_get_parent(parent)) {
            if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) { return 0; }
            lv_area_t clip; lv_obj_get_coords(parent, &clip);
            if (area.x1 < clip.x1 || area.y1 < clip.y1 || area.x2 > clip.x2 || area.y2 > clip.y2) {
                printf("GEOMETRY clip '%s' (%ld,%ld,%ld,%ld) ancestor (%ld,%ld,%ld,%ld)\n", text,
                    (long)area.x1,(long)area.y1,(long)area.x2,(long)area.y2,
                    (long)clip.x1,(long)clip.y1,(long)clip.x2,(long)clip.y2);
                ++errors; break;
            }
        }
        if (label) {
            lv_point_t size;
            lv_text_get_size(&size, text, lv_obj_get_style_text_font(object, LV_PART_MAIN),
                lv_obj_get_style_text_letter_space(object, LV_PART_MAIN), lv_obj_get_style_text_line_space(object, LV_PART_MAIN),
                lv_obj_get_content_width(object), LV_TEXT_FLAG_NONE);
            if (size.y > lv_obj_get_content_height(object)) {
                printf("GEOMETRY text '%s' requires %ld has %ld\n",text,(long)size.y,(long)lv_obj_get_content_height(object)); ++errors;
            }
        }
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(object); ++i) { errors += audit_geometry(lv_obj_get_child(object, (int32_t)i)); }
    return errors;
}

static unsigned capture_geometry_errors;

static int capture_frame(lv_display_t *display, ui_lab_context_t *context, const char *directory, const char *name)
{
    handle_timers(12U);
    lv_refr_now(display);
    SDL_Renderer *renderer = lv_sdl_window_get_renderer(display);
    int width = lv_display_get_horizontal_resolution(display);
    int height = lv_display_get_vertical_resolution(display);
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL) { return 6; }
    char path[512];
    int length = snprintf(path, sizeof(path), "%s/%s.bmp", directory, name);
    int result = length < 0 || (size_t)length >= sizeof(path) ? -1 :
        SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
    if (result == 0) { result = SDL_SaveBMP(surface, path); }
    SDL_FreeSurface(surface);
    if (result != 0) { fprintf(stderr, "Capture failed: %s\n", SDL_GetError()); return 6; }
    printf("CAPTURE %s %dx%d\n", name, width, height);
    unsigned geometry_errors = audit_geometry(lv_screen_active());
    furnace_hmi_ui_lab_explorer_t *explorer = &context->explorer;
    if (explorer->trajectory != NULL) {
        int32_t w = lv_obj_get_width(explorer->trajectory), h = lv_obj_get_height(explorer->trajectory);
        const furnace_hmi_dashboard_state_t *state = explorer->trajectory_uses_program_preview ? &explorer->preview_graph_state : explorer->state;
        for (size_t i = 0; i < state->graph_sample_count; ++i) {
            lv_point_precise_t point = explorer->planned_points[i];
            if (!lv_obj_has_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN) && (point.x < 0 || point.x >= w || point.y < 0 || point.y >= h)) { ++geometry_errors; }
        }
        for (size_t i = 0; i + 1 < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++i) {
            if (!explorer->measured_segments[i] || lv_obj_has_flag(explorer->measured_segments[i], LV_OBJ_FLAG_HIDDEN)) { continue; }
            for (size_t k = 0; k < 2; ++k) {
                lv_point_precise_t point = explorer->measured_segment_points[i][k];
                if (point.x < 0 || point.x >= w || point.y < 0 || point.y >= h) { ++geometry_errors; }
            }
        }
    }
    capture_geometry_errors += geometry_errors;
    printf("GEOMETRY_RESULT %s %u\n", name, geometry_errors);
    return 0;
}

static lv_obj_t *find_button(lv_obj_t *root, const char *text)
{
    if (root == NULL) { return NULL; }
    if (lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN)) { return NULL; }
    if (lv_obj_check_type(root, &lv_button_class)) {
        lv_obj_t *label = lv_obj_get_child(root, 0);
        if (label != NULL && lv_obj_check_type(label, &lv_label_class) && strcmp(lv_label_get_text(label), text) == 0) { return root; }
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        lv_obj_t *button = find_button(lv_obj_get_child(root, (int32_t)i), text);
        if (button) { return button; }
    }
    return NULL;
}

static lv_obj_t *find_label(lv_obj_t *root, const char *text)
{
    if (root == NULL || lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN)) {
        return NULL;
    }
    if (lv_obj_check_type(root, &lv_label_class) &&
        strcmp(lv_label_get_text(root), text) == 0) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        lv_obj_t *label = find_label(lv_obj_get_child(root, (int32_t)i), text);
        if (label != NULL) {
            return label;
        }
    }
    return NULL;
}

static bool tap_row_text(ui_lab_context_t *context, const char *text)
{
    lv_obj_t *root = context->explorer.modal != NULL
        ? context->explorer.modal : context->explorer.root;
    lv_obj_t *object = find_label(root, text);
    if (object == NULL) {
        fprintf(stderr, "Missing row action: %s\n", text);
        return false;
    }
    lv_obj_t *parent = lv_obj_get_parent(object);
    if (parent != NULL && lv_obj_check_type(parent, &lv_button_class)) {
        object = parent;
    }
    else {
        object = parent;
        while (object != NULL && !lv_obj_has_flag(object, LV_OBJ_FLAG_USER_1)) {
            object = lv_obj_get_parent(object);
        }
    }
    while (object != NULL && !lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE)) {
        object = lv_obj_get_parent(object);
    }
    if (object == NULL || lv_obj_send_event(object, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        return false;
    }
    handle_timers(2U);
    return true;
}

/* Tap the nearest button that contains a label with `text` (e.g. a stage card). */
static bool tap_label_button(ui_lab_context_t *context, const char *text)
{
    lv_obj_t *object = find_label(context->explorer.content, text);
    while (object != NULL && !lv_obj_check_type(object, &lv_button_class)) {
        object = lv_obj_get_parent(object);
    }
    if (object == NULL || lv_obj_send_event(object, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        fprintf(stderr, "Missing card action: %s\n", text);
        return false;
    }
    handle_timers(2U);
    return true;
}

static bool tap_text(ui_lab_context_t *context, const char *text)
{
    lv_obj_t *button = find_button(context->explorer.modal ? context->explorer.modal : context->explorer.root, text);
    if (!button) { fprintf(stderr, "Missing action: %s\n", text); return false; }
    if (lv_obj_send_event(button, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) { return false; }
    handle_timers(2U); return true;
}

static bool tap_content_text(ui_lab_context_t *context, const char *text)
{
    lv_obj_t *button = find_button(context->explorer.content, text);
    if (!button) { fprintf(stderr, "Missing content action: %s\n", text); return false; }
    if (lv_obj_send_event(button, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) { return false; }
    handle_timers(2U); return true;
}

static lv_obj_t *find_program_favourite_button(ui_lab_context_t *context)
{
    if (context == NULL || context->explorer.content == NULL ||
        lv_obj_get_child_count(context->explorer.content) == 0U) {
        return NULL;
    }
    lv_obj_t *toolbar = lv_obj_get_child(context->explorer.content, 0);
    if (toolbar == NULL || lv_obj_get_child_count(toolbar) < 3U) {
        return NULL;
    }
    lv_obj_t *favourite = lv_obj_get_child(toolbar, 2);
    return favourite != NULL && lv_obj_check_type(favourite, &lv_button_class)
        ? favourite : NULL;
}

static bool tap_program_favourite(ui_lab_context_t *context)
{
    lv_obj_t *button = find_program_favourite_button(context);
    if (button == NULL || lv_obj_send_event(button, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        return false;
    }
    handle_timers(2U);
    return true;
}

static bool tap(ui_lab_context_t *context, furnace_hmi_ui_string_id_t text)
{
    return tap_text(context, ui_string(text));
}

static bool tap_device_category(
    ui_lab_context_t *context,
    furnace_hmi_ui_string_id_t text
)
{
    lv_obj_t *rail = context->explorer.content == NULL
        ? NULL : lv_obj_get_child(context->explorer.content, 0);
    lv_obj_t *button = rail == NULL ? NULL : find_button(rail, ui_string(text));
    if (button == NULL) {
        fprintf(stderr, "Missing Device category: %s\n", ui_string(text));
        return false;
    }
    if (lv_obj_send_event(button, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        return false;
    }
    handle_timers(2U);
    return true;
}

/* Reading `index` has the given label and, when non-NULL, the given value. */
static bool reading_is(const ui_lab_context_t *context, size_t index, const char *name, const char *value)
{
    const furnace_hmi_ui_lab_explorer_t *explorer = &context->explorer;
    return index < FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT &&
           explorer->running_reading_names[index] != NULL &&
           strcmp(lv_label_get_text(explorer->running_reading_names[index]), name) == 0 &&
           (value == NULL || strcmp(lv_label_get_text(explorer->running_reading_values[index]), value) == 0);
}

static int run_capture(lv_display_t *display, ui_lab_context_t *context, const char *directory)
{
    /* "manual-setup" avoids colliding with the "manual" simulator-scenario capture. */
    static const char *names[] = { "home", "manual-setup", "loading", "running", "programs", "program-detail",
        "stages", "program-editor", "stage-detail", "editor", "device", "settings" };
    for (int page = 0; page < FURNACE_HMI_UI_LAB_PAGE_COUNT; ++page) {
        furnace_hmi_simulator_set_scenario(&context->simulator,
            page == FURNACE_HMI_UI_LAB_PAGE_RUNNING ? FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL : FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        context->explorer.running_view_mode = 0;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) { return 3; }
        if (capture_frame(display, context, directory, names[page]) != 0) { return 6; }
        if (page == FURNACE_HMI_UI_LAB_PAGE_MANUAL &&
            find_label(context->explorer.content, "Manual mode") == NULL) { return 3; }
        if (page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
            if (!tap_text(context, "120 C") || context->explorer.modal == NULL ||
                capture_frame(display, context, directory, "keyboard-numeric") != 0 ||
                !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) {
                fprintf(stderr, "Numeric keypad capture failed\n"); return 3;
            }
        }
        if (page == FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR) {
            const char *name = context->explorer.program_editor_name[0] != '\0'
                ? context->explorer.program_editor_name : "Enter program name";
            if (!tap_text(context, name) || context->explorer.modal == NULL ||
                capture_frame(display, context, directory, "keyboard-text") != 0 ||
                !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) {
                fprintf(stderr, "Text keyboard capture failed\n"); return 3;
            }
        }
        if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
            if (!reading_is(context, 0U, "Elapsed", NULL) || !reading_is(context, 1U, "Remaining", NULL) ||
                !reading_is(context, 4U, "Est. power", NULL)) {
                fprintf(stderr, "Running reading line is incomplete\n"); return 3;
            }
            if (capture_frame(display, context, directory, "running-time-both")) { return 6; }
            const furnace_hmi_dashboard_state_t original = context->explorer.observation;
            furnace_hmi_dashboard_state_t updated = original;
            updated.elapsed_seconds.value += 60;
            updated.remaining_seconds.value -= 60;
            updated.current_temperature_c.value = 777;
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &updated) ||
                !reading_is(context, 0U, "Elapsed", NULL) ||
                !reading_is(context, 2U, "Current", "777 C")) { return 3; }
            updated.remaining_seconds.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &updated) ||
                !reading_is(context, 1U, "Remaining", "--:--")) { return 3; }
            if (capture_frame(display, context, directory, "running-time-unknown")) { return 6; }
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &original)) { return 3; }
            /* Automatic cooling keeps curing Elapsed and adds only cooling data. */
            furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_COOLING);
            if (!render_scenario(context) ||
                !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_RUNNING) ||
                !reading_is(context, 0U, "Elapsed", "1:08") || !reading_is(context, 1U, "Cooling", "0:17") ||
                !reading_is(context, 2U, "Est. remaining", "0:25") || !reading_is(context, 3U, "Current", NULL) ||
                !reading_is(context, 4U, "Target", "30 C") ||
                context->explorer.cooling_overlay == NULL ||
                lv_obj_has_flag(context->explorer.cooling_overlay, LV_OBJ_FLAG_HIDDEN) ||
                find_label(context->explorer.content, "Automatic cooling") == NULL) {
                fprintf(stderr, "Running cooling presentation failed\n"); return 3;
            }
            if (capture_frame(display, context, directory, "running-cooling")) { return 6; }
            furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
            if (!render_scenario(context)) { return 3; }
        }
        if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING || page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            for (uint8_t variant = 1; variant < 3; ++variant) {
                context->explorer.running_view_mode = variant;
                if (page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
                    if (!render_scenario(context)) { return 3; }
                    context->explorer.selected_stage = (uint8_t)(variant + 1U);
                    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGE)) { return 3; }
                } else if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) { return 3; }
                if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
                    /* Split and Furnace views keep the same reading line. */
                    if (!reading_is(context, 0U, "Elapsed", NULL) || !reading_is(context, 3U, "Target", NULL)) {
                        return 3;
                    }
                }
                char name[64]; snprintf(name, sizeof(name), "%s-%u", names[page], (unsigned)variant);
                if (capture_frame(display, context, directory, name) != 0) { return 6; }
            }
        }
    }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) {
        fprintf(stderr, "Manual geometry check: could not render idle fixture\n");
        return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_MANUAL)) {
        fprintf(stderr, "Manual geometry check: could not enter Manual\n");
        return 3;
    }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START)) {
        fprintf(stderr, "Manual geometry check: could not start Manual\n");
        return 3;
    }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_MANUAL);
    if (!furnace_hmi_ui_lab_explorer_update(
            &context->explorer, furnace_hmi_simulator_state(&context->simulator))) {
        fprintf(stderr, "Manual geometry check: could not apply elapsed fixture\n");
        return 3;
    }
    lv_obj_t *manual_elapsed = find_label(context->explorer.content, "Curing elapsed: 0h 42m");
    if (manual_elapsed == NULL) {
        fprintf(stderr, "Manual geometry check: missing elapsed label\n");
        return 3;
    }
    lv_obj_t *target_label = find_label(context->explorer.content, "Target temperature");
    lv_obj_t *target_values = target_label == NULL ? NULL :
        lv_obj_get_child(lv_obj_get_parent(target_label), 1);
    lv_obj_t *target_decrement = target_values == NULL ? NULL : lv_obj_get_child(target_values, 0);
    lv_obj_t *target_increment = target_values == NULL ? NULL : lv_obj_get_child(target_values, 2);
    lv_obj_t *manual_running_stop = find_button(
        context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
    lv_obj_t *manual_columns = lv_obj_get_child(context->explorer.content, 0);
    lv_obj_t *manual_graph = manual_columns == NULL ? NULL : lv_obj_get_child(manual_columns, 0);
    lv_obj_t *manual_controls = manual_columns == NULL ? NULL : lv_obj_get_child(manual_columns, 1);
    lv_obj_t *manual_context = lv_obj_get_parent(manual_elapsed);
    lv_obj_t *manual_control_stack = target_label == NULL ? NULL :
        lv_obj_get_parent(lv_obj_get_parent(target_label));
    lv_obj_t *manual_actions = manual_running_stop == NULL ? NULL :
        lv_obj_get_parent(manual_running_stop);
    if (target_label == NULL || target_decrement == NULL || target_increment == NULL ||
        manual_running_stop == NULL ||
        manual_graph == NULL || manual_controls == NULL || manual_context == NULL ||
        manual_control_stack == NULL || manual_actions == NULL) {
        fprintf(stderr, "Manual geometry check: missing control widgets\n");
        return 3;
    }
    lv_area_t target_label_area, target_decrement_area, manual_stop_area, manual_content_area,
        manual_controls_area, manual_context_area, manual_control_stack_area, manual_actions_area;
    lv_obj_get_coords(target_label, &target_label_area);
    lv_obj_get_coords(target_decrement, &target_decrement_area);
    lv_obj_get_coords(manual_running_stop, &manual_stop_area);
    lv_obj_get_coords(context->explorer.content, &manual_content_area);
    lv_obj_get_coords(manual_controls, &manual_controls_area);
    lv_obj_get_coords(manual_context, &manual_context_area);
    lv_obj_get_coords(manual_control_stack, &manual_control_stack_area);
    lv_obj_get_coords(manual_actions, &manual_actions_area);
    if (target_label_area.y2 >= target_decrement_area.y1 ||
        lv_obj_get_width(target_decrement) < 48 || lv_obj_get_height(target_decrement) < 52 ||
        lv_obj_get_height(manual_graph) <= lv_obj_get_height(manual_controls) * 20 / 10 ||
        manual_stop_area.y2 < manual_content_area.y2 - 20 ||
        manual_context_area.y2 >= target_label_area.y1 ||
        manual_actions_area.x2 >= manual_control_stack_area.x1) {
        fprintf(stderr,
                "Manual geometry check: label %ld/%ld button %ld/%ld graph %ld controls %ld context %ld/%ld actions %ld stack %ld stop %ld content %ld\n",
                (long)target_label_area.y2, (long)target_decrement_area.y1,
                (long)lv_obj_get_width(target_decrement), (long)lv_obj_get_height(target_decrement),
                (long)lv_obj_get_height(manual_graph), (long)lv_obj_get_height(manual_controls),
                (long)manual_context_area.y2, (long)target_label_area.y1,
                (long)manual_actions_area.x2, (long)manual_control_stack_area.x1,
                (long)manual_stop_area.y2, (long)manual_content_area.y2);
        return 3;
    }
    lv_obj_t *const manual_trajectory = context->explorer.trajectory;
    lv_obj_send_event(target_increment, LV_EVENT_CLICKED, NULL);
    handle_timers(2U);
    if (context->explorer.trajectory != manual_trajectory ||
        context->explorer.manual_target_c != 121) {
        fprintf(stderr, "Manual geometry check: target adjustment recreated the trajectory\n");
        return 3;
    }
    if (capture_frame(display, context, directory, "manual-running")) { return 6; }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW)) { return 3; }
    static const uint8_t settings_tabs[] = { 0U, 1U, 2U, 3U };
    static const char *const settings_names[] = {
        "settings-display",
        "settings-time",
        "settings-connections",
        "settings-maintenance",
    };
    for (size_t index = 0U; index < sizeof(settings_tabs) / sizeof(settings_tabs[0]); ++index) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        context->explorer.settings_tab = settings_tabs[index];
        context->explorer.settings_display_page = index == 0U ? 1U : 0U;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
            capture_frame(display, context, directory, settings_names[index])) { return 6; }
    }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    context->explorer.device_tab = 1U;
    context->explorer.device_filter = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-components")) { return 6; }
    context->explorer.device_selected_component = 0U;
    context->explorer.device_component_selected = true;
    context->explorer.device_page = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-component-detail")) { return 6; }
    context->explorer.device_component_selected = false;
    context->explorer.device_tab = 2U;
    context->explorer.device_filter = 2U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-notifications-service")) { return 6; }
    context->explorer.device_tab = 3U;
    context->explorer.device_filter = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-history")) { return 6; }
    context->explorer.device_tab = 4U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-info")) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    furnace_hmi_dashboard_state_t stale_state =
        *furnace_hmi_simulator_state(&context->simulator);
    stale_state.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    if (!furnace_hmi_ui_lab_explorer_update(
            &context->explorer, &stale_state)) {
        return 3;
    }
    context->explorer.device_tab = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE)) { return 3; }
    if (capture_frame(display, context, directory, "device-stale")) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) { return 3; }
    if (capture_frame(display, context, directory, "service-dialog") != 0) { return 6; }
    for (unsigned variant = 0; variant < 10; ++variant) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        furnace_hmi_ui_lab_page_t page = FURNACE_HMI_UI_LAB_PAGE_PROGRAMS;
        const char *name = "programs-last";
        if (variant == 0) { context->explorer.program_page = 1; }
        if (variant == 1) { context->explorer.program_page = 1; context->explorer.library_count = 8; name = "programs-partial"; }
        if (variant == 2) { context->explorer.library_count = 0; name = "programs-empty"; }
        if (variant == 3) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES; context->explorer.program_stage_page = 0; name = "stages-last"; }
        if (variant == 4) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES; context->explorer.selected_program = 11; context->explorer.program_stage_page = 0; name = "stages-maximum"; }
        if (variant == 5) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL; context->explorer.favourite_count = 1; name = "remove-favourite"; }
        if (variant >= 6 && variant <= 8) {
            page = FURNACE_HMI_UI_LAB_PAGE_HOME; context->explorer.recent_count = 0;
            context->explorer.favourite_count = (uint8_t)(variant - 6);
            if (variant == 8) { context->explorer.favourite_count = 3; }
            for (uint8_t k = 0; k < context->explorer.favourite_count; ++k) { context->explorer.quick_launch_favourites[k] = k; }
            name = variant == 6 ? "home-empty" : variant == 7 ? "home-few" : "home-full";
        }
        if (variant == 9) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING; context->explorer.selected_program = 11; name = "loading-long"; }
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, page) || capture_frame(display, context, directory, name)) { return 6; }
    }
    const furnace_hmi_ui_string_id_t dialog_actions[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE, FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT,
        FURNACE_HMI_UI_STRING_UI_LAB_EDIT_NAME, FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT,
        FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES, FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW,
        FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW
    };
    const char *dialog_names[] = { "notices", "delete", "rename-preview", "save", "discard", "pause-preview", "stop-preview" };
    for (unsigned i = 0; i < 7; ++i) {
        furnace_hmi_simulator_set_scenario(&context->simulator, i >= 5 ? FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL : FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        furnace_hmi_ui_lab_page_t page = i >= 5 ? FURNACE_HMI_UI_LAB_PAGE_RUNNING : i >= 3 ? FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR : FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, page)) { return 3; }
        const bool opened = i == 0U
            ? furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)
            : (i == 2U ? tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) : tap(context, dialog_actions[i]));
        if (!opened) { return 3; }
        if (capture_frame(display, context, directory, dialog_names[i])) { return 6; }
        if (i == 1) {
            if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE) || capture_frame(display, context, directory, "request-result")) { return 6; }
        }
    }
    for (furnace_hmi_sim_scenario_t scenario = 0; scenario < FURNACE_HMI_SIM_SCENARIO_COUNT; ++scenario) {
        furnace_hmi_simulator_set_scenario(&context->simulator, scenario);
        if (!render_scenario(context)) { return 3; }
        if (scenario == FURNACE_HMI_SIM_SCENARIO_MANUAL && !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_RUNNING)) { return 3; }
        if (capture_frame(display, context, directory, furnace_hmi_sim_scenario_name(scenario))) { return 6; }
    }
    for (unsigned i = 0; i < 8; ++i) {
        furnace_hmi_simulator_set_scenario(&context->simulator, i == 2 ? FURNACE_HMI_SIM_SCENARIO_PAUSED : i >= 3 && i <= 5 ? FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL : FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        const char *name = "new-program";
        if (i == 0) {
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) || !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT)) { return 3; }
        } else if (i == 1) {
            name = "add-stage";
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES) || !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE)) { return 3; }
        } else if (i == 2) {
            name = "resume-preview"; if (!tap(context,FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW)) { return 3; }
        } else if (i == 3) {
            name = "run-details";
            lv_obj_t *button = find_button(context->explorer.root, context->explorer.state->program_name);
            if (!button) { return 3; } lv_obj_send_event(button,LV_EVENT_CLICKED,NULL); handle_timers(2U);
        } else if (i == 4 || i == 5) {
            furnace_hmi_dashboard_state_t state = context->explorer.observation;
            name = i == 4 ? "zero-invalid" : "long-negative-gap";
            state.current_temperature_c.value = i == 4 ? 0 : -1234;
            state.fan_rpm.value = 0; state.door_closed.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
            state.estimated_power_kw_x100.value = 0;
            if (i == 5) {
                snprintf(state.program_name,sizeof(state.program_name),"%s","Long ceramic programme name for review");
                state.graph_samples[2].measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
                state.graph_samples[0].measured_temperature_c = -1234;
                ++state.program_id;
            }
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer,&state)) { return 3; }
        } else if (i == 6) {
            name = "automatic-cooling";
            context->explorer.selected_stage = 3;
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL) || !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGE)) { return 3; }
        } else {
            name = "favourite-limit"; context->explorer.favourite_count = 3;
            for (unsigned k = 0; k < 3; ++k) { context->explorer.quick_launch_favourites[k] = (uint8_t)k; }
            context->explorer.selected_program = 3;
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) || !tap_program_favourite(context)) { return 3; }
        }
        if (capture_frame(display, context,directory,name)) { return 6; }
    }
    furnace_hmi_simulator_set_scenario(&context->simulator,FURNACE_HMI_SIM_SCENARIO_DISCONNECTED);
    if (!render_scenario(context) || !furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_DEVICE) || capture_frame(display, context,directory,"device-unavailable") ||
        !furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer) || capture_frame(display, context,directory,"service-unavailable")) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator,FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    context->explorer.selected_program = 11;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING) ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW) || capture_frame(display,context,directory,"start-preview") ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) || !furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_HOME) || capture_frame(display,context,directory,"home-changed-recents")) { return 6; }
    context->explorer.draft_target_c = -1;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) || capture_frame(display, context, directory, "editor-validation")) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_MANUAL);
    if (!render_scenario(context) || !furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_RUNNING) || !furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer) || capture_frame(display,context,directory,"manual-maintenance")) { return 6; }
    lv_obj_t *manual_stop = find_button(context->explorer.content,ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
    lv_area_t stop_area, modal_area;
    if (!manual_stop) { return 3; }
    lv_obj_get_coords(manual_stop,&stop_area); lv_obj_get_coords(context->explorer.modal,&modal_area);
    if (modal_area.y2 >= stop_area.y1) { fprintf(stderr,"Manual modal obstructed Stop\n"); return 3; }
    return 0;
}

static int run_smoke_test(lv_display_t *display, ui_lab_context_t *context)
{
    lv_display_set_resolution(display, 640, 360); lv_obj_update_layout(lv_screen_active());
    for (int page = 0; page < FURNACE_HMI_UI_LAB_PAGE_COUNT; ++page) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) {
            fprintf(stderr, "Structural route failed: %d\n", page); return 3;
        }
        handle_timers(1U);
    }
    lv_display_set_resolution(display, FURNACE_HMI_DESKTOP_WIDTH, FURNACE_HMI_DESKTOP_HEIGHT);
    lv_obj_update_layout(lv_screen_active());
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context) || context->explorer.state_strip == NULL) {
        fprintf(stderr, "Top-line light setup failed\n"); return 3;
    }
    lv_area_t light_target;
    lv_obj_get_coords(context->explorer.state_strip, &light_target);
    if (lv_obj_get_height(context->explorer.state_strip) < FURNACE_HMI_UI_LAB_LIGHT_TARGET_HEIGHT ||
        light_target.x1 > 0 ||
        light_target.x2 < lv_display_get_horizontal_resolution(display) - 1) {
        fprintf(stderr, "Top-line light hit target is too small\n"); return 3;
    }
    if (lv_obj_send_event(context->explorer.state_strip, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        fprintf(stderr, "Top-line light-on event failed\n"); return 3;
    }
    handle_timers(1U);
    if (
        !context->explorer.manual_light_on) {
        fprintf(stderr, "Top-line light-on action failed\n"); return 3;
    }
    if (lv_obj_send_event(context->explorer.state_strip, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        fprintf(stderr, "Top-line light-off event failed\n"); return 3;
    }
    handle_timers(1U);
    if (
        context->explorer.manual_light_on) {
        fprintf(stderr, "Top-line light-off action failed\n"); return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_text(context, "80%  >") ||
        find_button(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR) ||
        !tap_text(context, "8") || !tap_text(context, "1") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY) ||
        context->explorer.settings_candidate_numeric[3] != 81 ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD) ||
        context->explorer.settings_candidate_numeric[3] != 80) {
        fprintf(stderr, "Settings numeric editor failed\n"); return 3;
    }
    if (!tap_text(context, "80%  >") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR) ||
        !tap_text(context, "8") || !tap_text(context, "1") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY) ||
        !tap_text(context, "More >") ||
        !tap_text(context, "< Back") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Settings display/save flow failed\n"); return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) ||
        !tap_row_text(context, "Ceramic test program") ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
         !tap_label_button(context, "3 min") ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
         /* Unchanged draft: Back leaves without asking. */
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_BACK) || context->explorer.modal != NULL ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL ||
         /* A new stage makes the draft dirty: Back asks before discarding it. */
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) ||
         !tap_text(context, "+ Add stage") ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_BACK) || context->explorer.modal == NULL ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) {
        fprintf(stderr, "Program edit flow failed\\n"); return 3;
    }
    lv_obj_t *favourite_button = find_program_favourite_button(context);
    if (favourite_button == NULL || lv_obj_get_child_count(favourite_button) == 0U ||
        !lv_obj_check_type(lv_obj_get_child(favourite_button, 0), &lv_line_class) ||
        !tap_program_favourite(context)) {
        fprintf(stderr, "Program favourite star flow failed\\n"); return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS)) {
        fprintf(stderr, "Program refinement smoke return failed\\n");
        return 3;
    }
    if (!tap_content_text(context, "Maintenance") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REVIEW) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_PREFERENCES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_PREFERENCES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Settings maintenance flow failed\n"); return 3;
    }
    if (!tap_text(context, "Display & interaction") ||
        !tap_text(context, "80%  >") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR) ||
        !tap_text(context, "8") || !tap_text(context, "1") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY) ||
        find_button(context->explorer.nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DEVICE)) == NULL) {
        fprintf(stderr, "Settings dirty navigation guard failed\n"); return 3;
    }
    lv_obj_t *device_nav = find_button(
        context->explorer.nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DEVICE));
    if (lv_obj_send_event(device_nav, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        fprintf(stderr, "Settings dirty navigation guard failed\n"); return 3;
    }
    handle_timers(2U);
    if (context->explorer.modal == NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD)) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_DEVICE) {
        fprintf(stderr, "Settings dirty navigation guard failed\n"); return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        context->explorer.device_tab != 0U ||
        context->device_catalogue.component_count == 0U) {
        fprintf(stderr, "Device overview route failed\n"); return 3;
    }
    if (!tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG) ||
        !tap_row_text(context, "Program start") ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_OVERDUE))) {
        fprintf(stderr, "Device log/maintenance navigation failed\n"); return 3;
    }
    if (!tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_HISTORY) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INSPECTION)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INFO) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DETAILS)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE))) {
        fprintf(stderr, "Device notification/history/info flow failed\n"); return 3;
    }
    clear_views(context);
    printf("UI lab: Device overview, log, maintenance, history and info navigation passed.\n");
    if (!render_scenario(context) ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
        !tap_text(context, "+ Add stage") ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) { return 3; }
    int32_t before = context->explorer.draft_target_c;
    lv_obj_t *increase = find_button(context->explorer.root, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE));
    if (!increase) { return 3; }
    lv_obj_send_event(increase, LV_EVENT_CLICKED, NULL); lv_obj_send_event(increase, LV_EVENT_CLICKED, NULL);
    handle_timers(2U);
    if (context->explorer.draft_target_c != before + 1) { return 3; }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DECREASE) || context->explorer.draft_target_c != before ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING)) { return 3; }
    if (furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME)) { return 3; }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES)) { return 3; }
    lv_obj_t *modal = context->explorer.modal;
    int32_t target = context->explorer.draft_target_c;
    furnace_hmi_simulator_tick(&context->simulator, 1);
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) || context->explorer.modal != modal) { fprintf(stderr,"Telemetry lost modal\n"); return 3; }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) || context->explorer.draft_target_c != target ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR) { return 3; }
    for (furnace_hmi_sim_scenario_t scenario = 0; scenario < FURNACE_HMI_SIM_SCENARIO_COUNT; ++scenario) {
        furnace_hmi_simulator_set_scenario(&context->simulator, scenario);
        if (!render_scenario(context)) { return 3; }
        handle_timers(2U);
        if (context->explorer.page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
            if (furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME)) { return 3; }
            int64_t domain = context->explorer.trajectory_domain_maximum_seconds;
            if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_FULL_VISUAL) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_FULL_GRAPH) || domain != context->explorer.trajectory_domain_maximum_seconds) { return 3; }
            lv_obj_t *content = context->explorer.content;
            furnace_hmi_simulator_tick(&context->simulator, 1);
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) || context->explorer.content != content) { return 3; }
            if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) { return 3; }
            lv_obj_t *stop = find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
            lv_area_t stop_area, overlay_area;
            if (!stop) { return 3; }
            lv_obj_get_coords(stop, &stop_area); lv_obj_get_coords(context->explorer.modal, &overlay_area);
            if (overlay_area.y2 >= stop_area.y1) { return 3; }
            lv_obj_send_event(stop, LV_EVENT_CLICKED, NULL); handle_timers(2U);
        }
        if (scenario == FURNACE_HMI_SIM_SCENARIO_MANUAL) {
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_RUNNING) ||
                furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE)) { return 3; }
        }
    }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
    if (!render_scenario(context)) { return 3; }
    furnace_hmi_dashboard_state_t original = *furnace_hmi_simulator_state(&context->simulator);
    furnace_hmi_dashboard_state_t changed = original;
    int32_t original_domain = context->explorer.trajectory_domain_maximum_temperature_c;
    changed.graph_samples[1].measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    changed.door_closed.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    changed.fan_rpm.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &changed) ||
        !lv_obj_has_flag(context->explorer.measured_segments[0], LV_OBJ_FLAG_HIDDEN) ||
        !lv_obj_has_flag(context->explorer.measured_segments[1], LV_OBJ_FLAG_HIDDEN) ||
        strcmp(lv_label_get_text(context->explorer.live_door), ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN))) { fprintf(stderr,"Invalid observation became valid\n"); return 3; }
    changed.graph_samples[0].measured_temperature_c = -1234;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &changed) || context->explorer.trajectory_domain_minimum_temperature_c >= -1234) { fprintf(stderr,"Negative measurement escaped domain\n"); return 3; }
    changed.graph_samples[0].measured_temperature_c = 3000;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &changed) || context->explorer.trajectory_domain_maximum_temperature_c <= original_domain) { fprintf(stderr,"Domain did not expand\n"); return 3; }
    int32_t expanded = context->explorer.trajectory_domain_maximum_temperature_c;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &original) || context->explorer.trajectory_domain_maximum_temperature_c != expanded) { fprintf(stderr,"Domain contracted\n"); return 3; }
    changed = original; ++changed.controller_session_epoch;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &changed) || context->explorer.trajectory_domain_maximum_temperature_c != original_domain) { fprintf(stderr,"New session retained old domain\n"); return 3; }
    for (unsigned i = 0; i < 20; ++i) {
        if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer) ||
            !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) { return 3; }
    }
    clear_views(context); printf("UI lab: routes, editor containment/discard, modal retention, run lock, Manual navigation, domain retention and Stop passed.\n");
    return 0;
}

static bool load_device_catalogue(ui_lab_context_t *context)
{
    static const char *const paths[] = {
        "simulator/ui_lab/data/device_service_catalogue.txt",
        "device_service_catalogue.txt",
        "simulator/ui_lab/device_service_catalogue.txt",
        "simulator/ui_lab/Debug/device_service_catalogue.txt",
    };
    for (size_t index = 0U; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        if (furnace_hmi_ui_lab_device_catalogue_load(&context->device_catalogue, paths[index])) {
            (void)printf("Device catalogue loaded: %s (%u components, revision %s)\n",
                         paths[index], (unsigned)context->device_catalogue.component_count,
                         context->device_catalogue.revision);
            return true;
        }
    }
    (void)fprintf(stderr, "Device catalogue could not be loaded from the known paths.\n");
    return false;
}

int main(int argc, char **argv)
{
    furnace_hmi_sim_scenario_t initial_scenario;
    bool smoke_test;
    bool list_scenarios;
    const char *capture_directory;
    if (!parse_arguments(argc, argv, &initial_scenario, &smoke_test, &list_scenarios, &capture_directory)) {
        print_usage(argv[0]);
        return 64;
    }
    if (list_scenarios) {
        for (furnace_hmi_sim_scenario_t scenario = 0;
             scenario < FURNACE_HMI_SIM_SCENARIO_COUNT;
             ++scenario) {
            (void)printf("%s\n", furnace_hmi_sim_scenario_name(scenario));
        }
        return 0;
    }
    if (furnace_hmi_foundation_probe_version() != FURNACE_HMI_FOUNDATION_PROBE_VERSION) {
        (void)fprintf(stderr, "Portable foundation build probe mismatch.\n");
        return 1;
    }

    ui_lab_context_t context = {0};
    context.requested_scenario = -1;
    context.requested_page = -1;
    context.requested_service_dialog = false;
    context.quit_requested = false;
    furnace_hmi_simulator_initialize(&context.simulator, initial_scenario);
    if (!load_device_catalogue(&context)) {
        return 1;
    }

    lv_init();
    lv_display_t *display = lv_sdl_window_create(
        FURNACE_HMI_DESKTOP_WIDTH,
        FURNACE_HMI_DESKTOP_HEIGHT
    );
    if (display == NULL) {
        (void)fprintf(stderr, "SDL display creation failed.\n");
        return 2;
    }
    lv_sdl_window_set_title(display, ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE));
    lv_sdl_window_set_resizeable(display, false);
    (void)lv_sdl_mouse_create();
    SDL_AddEventWatch(event_watch, &context);

    int result;
    if (capture_directory != NULL) {
        result = run_capture(display, &context, capture_directory);
        if (capture_geometry_errors != 0U) { result = 6; }
        clear_views(&context);
        SDL_DelEventWatch(event_watch, &context);
        return result;
    }
    if (smoke_test) {
        result = run_smoke_test(display, &context);
        SDL_DelEventWatch(event_watch, &context);
        return result;
    }
    if (!render_scenario(&context)) {
        (void)fprintf(stderr, "Initial UI laboratory scenario could not be rendered.\n");
        SDL_DelEventWatch(event_watch, &context);
        return 3;
    }

    uint32_t last_tick = lv_tick_get();
    while (!context.quit_requested) {
        const int requested = context.requested_scenario;
        context.requested_scenario = -1;
        if (requested >= 0 && requested < FURNACE_HMI_SIM_SCENARIO_COUNT) {
            furnace_hmi_simulator_set_scenario(
                &context.simulator,
                (furnace_hmi_sim_scenario_t)requested
            );
            if (!render_scenario(&context)) {
                (void)fprintf(stderr, "Requested UI laboratory scenario could not be rendered.\n");
                result = 4;
                goto cleanup;
            }
        }
        const int requested_page = context.requested_page;
        context.requested_page = -1;
        if (requested_page >= 0 && requested_page < FURNACE_HMI_UI_LAB_PAGE_COUNT &&
            !(context.explorer.page == FURNACE_HMI_UI_LAB_PAGE_RUNNING &&
              requested_page != FURNACE_HMI_UI_LAB_PAGE_RUNNING) &&
            !furnace_hmi_ui_lab_explorer_navigate(
                &context.explorer,
                (furnace_hmi_ui_lab_page_t)requested_page
            )) {
            (void)fprintf(stderr, "Requested UI laboratory page could not be rendered.\n");
            /* A contained Program/editor/modal rejects ordinary shortcuts. */
        }
        if (context.requested_service_dialog) {
            context.requested_service_dialog = false;
            if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context.explorer)) {
                (void)fprintf(stderr, "UI laboratory service dialog could not be rendered.\n");
                result = 4;
                goto cleanup;
            }
        }

        const uint32_t now = lv_tick_get();
        if (now - last_tick >= 1000U) {
            furnace_hmi_simulator_tick(&context.simulator, (now - last_tick) / 1000U);
            last_tick = now;
            if (context.explorer.root != NULL &&
                !furnace_hmi_ui_lab_explorer_update(
                    &context.explorer, furnace_hmi_simulator_state(&context.simulator))) {
                (void)fprintf(stderr, "UI laboratory explorer update failed.\n");
                result = 5;
                goto cleanup;
            }
        }
        handle_timers(1U);
    }
    result = 0;

cleanup:
    clear_views(&context);
    SDL_DelEventWatch(event_watch, &context);
    return result;
}
