/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/hmi_fonts.h>
#include <furnace_hmi/hmi_theme.h>
#include <furnace_hmi/simulated_controller.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>
#include <SDL.h>

#include "ui_lab_explorer.h"
#include <furnace_hmi/support_qr.h>
#include "ui_lab_theme.h"

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

/* Programs started from the lab run 60 times faster than real time (one
 * simulated minute per second) so a whole run can be watched; scenarios keep
 * real time. Simulator behaviour only. */
#define UI_LAB_PROGRAM_RUN_SPEED 60U

/* The host side of the operator's controller requests: hand each to the
 * simulated controller, then show the controller's answer. */
static bool pump_controller_requests(ui_lab_context_t *context)
{
    furnace_hmi_ui_lab_request_t request;
    if (!furnace_hmi_ui_lab_explorer_take_request(&context->explorer, &request)) {
        return true;
    }
    switch (request.kind) {
    case FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM: {
        furnace_hmi_sim_program_t program;
        memset(&program, 0, sizeof(program));
        (void)snprintf(program.name, sizeof(program.name), "%s", request.program_name);
        program.stage_count = request.program.stage_count;
        for (uint8_t index = 0U; index < program.stage_count && index < FURNACE_HMI_SIM_MAX_STAGES; ++index) {
            const furnace_hmi_ui_lab_program_stage_t *stage = &request.program.stages[index];
            program.stages[index] = (furnace_hmi_sim_stage_t) {
                .hold = stage->kind == FURNACE_HMI_UI_LAB_STAGE_HOLD,
                .target_c = stage->target_c,
                .rate_tenths_c_per_minute = stage->rate_tenths_c_per_minute,
                .minutes = stage->minutes > 0 ? (uint32_t)stage->minutes : 0U,
            };
        }
        (void)furnace_hmi_simulator_start_program(&context->simulator, &program);
        break;
    }
    case FURNACE_HMI_UI_LAB_REQUEST_PAUSE:
        (void)furnace_hmi_simulator_set_paused(&context->simulator, true);
        break;
    case FURNACE_HMI_UI_LAB_REQUEST_RESUME:
        (void)furnace_hmi_simulator_set_paused(&context->simulator, false);
        break;
    case FURNACE_HMI_UI_LAB_REQUEST_STOP:
        (void)furnace_hmi_simulator_stop(&context->simulator);
        break;
    case FURNACE_HMI_UI_LAB_REQUEST_CLOSE_RUN:
        (void)furnace_hmi_simulator_close_run(&context->simulator);
        break;
    case FURNACE_HMI_UI_LAB_REQUEST_NONE:
        break;
    }
    return furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator));
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
    lv_obj_set_style_text_font(context->simulation_marker, &furnace_hmi_font_regular_14, LV_PART_MAIN);
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
    (void)fprintf(stderr, "Scenarios: disconnected, idle, running-normal, manual, paused, fault, stale, running-overrun,\n"
                          "  cooling, overtemperature, heating-slow, sensor-fault, power-loss\n");
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

/* Stage dialog top row: 0 delete, 1 pager (its heading holds the position),
 * 2 add. */
static lv_obj_t *stage_dialog_top(ui_lab_context_t *context, uint32_t index)
{
    lv_obj_t *dialog = context->explorer.stage_dialog;
    lv_obj_t *card = dialog == NULL ? NULL : lv_obj_get_child(dialog, 0);
    lv_obj_t *top = card == NULL ? NULL : lv_obj_get_child(card, 0);
    return top == NULL ? NULL : lv_obj_get_child(top, (int32_t)index);
}

static bool tap_object(lv_obj_t *object)
{
    if (object == NULL || lv_obj_has_state(object, LV_STATE_DISABLED) ||
        lv_obj_send_event(object, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
        return false;
    }
    handle_timers(2U);
    return true;
}

/* Open the stage editor from the stage's node on the Program details graph. */
static bool open_stage_editor(ui_lab_context_t *context, uint8_t stage)
{
    if (stage >= FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) ||
        !tap_object(context->explorer.trajectory_stage_nodes[stage]) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
        context->explorer.selected_stage != stage) {
        fprintf(stderr, "Stage %u editor did not open\n", (unsigned)stage + 1U);
        return false;
    }
    return true;
}

static bool tap(ui_lab_context_t *context, furnace_hmi_ui_string_id_t text)
{
    return tap_text(context, ui_string(text));
}

/* Brightness gauge step `index` (0 = 10 %): body > grid > card > gauge > steps. */
static lv_obj_t *brightness_step(ui_lab_context_t *context, uint32_t index)
{
    lv_obj_t *label = find_label(context->explorer.content,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BRIGHTNESS));
    lv_obj_t *card = label == NULL ? NULL : lv_obj_get_parent(lv_obj_get_parent(label));
    lv_obj_t *gauge = card == NULL ? NULL : lv_obj_get_child(card, 1);
    lv_obj_t *steps = gauge == NULL ? NULL : lv_obj_get_child(gauge, 1);
    return steps == NULL ? NULL : lv_obj_get_child(steps, (int32_t)index);
}

/* Set date & time: modal > dialog > picker (title, hidden detail, picker). */
static lv_obj_t *clock_picker(ui_lab_context_t *context)
{
    lv_obj_t *dialog = context->explorer.modal == NULL ? NULL : lv_obj_get_child(context->explorer.modal, 0);
    return dialog == NULL ? NULL : lv_obj_get_child(dialog, 2);
}

static uint32_t clock_column_count(ui_lab_context_t *context)
{
    lv_obj_t *picker = clock_picker(context);
    return picker == NULL ? 0U : lv_obj_get_child_count(picker);
}

/* Column children: name, +, value, -. */
static lv_obj_t *clock_step_button(ui_lab_context_t *context, uint32_t field, bool up)
{
    lv_obj_t *picker = clock_picker(context);
    lv_obj_t *column = picker == NULL ? NULL : lv_obj_get_child(picker, (int32_t)field);
    return column == NULL ? NULL : lv_obj_get_child(column, up ? 1 : 3);
}

/* Cards in the Device Maintenance grid (body > grid). */
static uint32_t maintenance_card_count(ui_lab_context_t *context)
{
    lv_obj_t *body = context->explorer.content == NULL ? NULL : lv_obj_get_child(context->explorer.content, 1);
    lv_obj_t *grid = body == NULL ? NULL : lv_obj_get_child(body, 1);
    return grid == NULL ? 0U : lv_obj_get_child_count(grid);
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

/* Review fixture: every free library slot holds an operator program (copies of
 * the delivered ones) and the last one has eight stages and a long name. */
static void fill_program_library(ui_lab_context_t *context)
{
    furnace_hmi_ui_lab_explorer_t *explorer = &context->explorer;
    for (size_t index = 3U; index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++index) {
        explorer->programs[index] = explorer->programs[index % 3U];
        explorer->programs[index].library = false;
        (void)snprintf(explorer->program_name_overrides[index], FURNACE_HMI_UI_LAB_NAME_CAPACITY,
                       "Operator program %u", (unsigned)index - 2U);
    }
    furnace_hmi_ui_lab_program_t *longest = &explorer->programs[FURNACE_HMI_UI_LAB_MAX_PROGRAMS - 1U];
    const furnace_hmi_ui_lab_program_stage_t stages[FURNACE_HMI_UI_LAB_MAX_STAGES] = {
        { FURNACE_HMI_UI_LAB_STAGE_HEATING, 50, 10, 25 }, { FURNACE_HMI_UI_LAB_STAGE_HOLD, 50, 0, 30 },
        { FURNACE_HMI_UI_LAB_STAGE_HEATING, 80, 10, 30 }, { FURNACE_HMI_UI_LAB_STAGE_HOLD, 80, 0, 30 },
        { FURNACE_HMI_UI_LAB_STAGE_HEATING, 120, 10, 40 }, { FURNACE_HMI_UI_LAB_STAGE_HOLD, 120, 0, 60 },
        { FURNACE_HMI_UI_LAB_STAGE_HEATING, 160, 10, 40 }, { FURNACE_HMI_UI_LAB_STAGE_HOLD, 160, 0, 90 },
    };
    longest->stage_count = FURNACE_HMI_UI_LAB_MAX_STAGES;
    memcpy(longest->stages, stages, sizeof(stages));
    (void)snprintf(explorer->program_name_overrides[FURNACE_HMI_UI_LAB_MAX_PROGRAMS - 1U],
                   FURNACE_HMI_UI_LAB_NAME_CAPACITY, "%s", "Long post-cure schedule review");
}

static int run_capture(lv_display_t *display, ui_lab_context_t *context, const char *directory)
{
    /* "manual-setup" avoids colliding with the "manual" simulator-scenario capture. */
    static const char *names[] = { "home", "manual-setup", "loading", "running", "programs", "program-detail",
        "stages", "program-editor", "editor", "device", "settings" };
    for (int page = 0; page < FURNACE_HMI_UI_LAB_PAGE_COUNT; ++page) {
        furnace_hmi_simulator_set_scenario(&context->simulator,
            page == FURNACE_HMI_UI_LAB_PAGE_RUNNING ? FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL : FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        context->explorer.running_view_mode = 0;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) { return 3; }
        if (capture_frame(display, context, directory, names[page]) != 0) { return 6; }
        /* Manual setup opens with the readings strip in its Ready state. */
        if (page == FURNACE_HMI_UI_LAB_PAGE_MANUAL &&
            (context->explorer.manual_readings == NULL ||
             find_label(context->explorer.manual_readings, "Ready") == NULL)) {
            fprintf(stderr, "Manual setup readings missing\n"); return 3;
        }
        if (page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
            if (!tap_text(context, "120") || context->explorer.modal == NULL ||
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
                !reading_is(context, 4U, "Est. power kW", NULL)) {
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
                !reading_is(context, 2U, "Current \xC2\xB0" "C", "777")) { return 3; }
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
                !reading_is(context, 2U, "Est. remaining", "0:25") || !reading_is(context, 3U, "Current \xC2\xB0" "C", NULL) ||
                !reading_is(context, 4U, "Target \xC2\xB0" "C", "30") ||
                context->explorer.cooling_overlay == NULL ||
                lv_obj_has_flag(context->explorer.cooling_overlay, LV_OBJ_FLAG_HIDDEN) ||
                find_label(context->explorer.content, "Automatic cooling") == NULL) {
                fprintf(stderr, "Running cooling presentation failed\n"); return 3;
            }
            if (capture_frame(display, context, directory, "running-cooling")) { return 6; }
            /* Once measured cooling passes the start-of-cooling window the
             * reading line offers Show cooling, which widens the graph. */
            furnace_hmi_dashboard_state_t longer = context->explorer.observation;
            longer.cooling_elapsed_seconds.value = 3 * 3600;
            const int64_t short_window = context->explorer.trajectory_domain_maximum_seconds;
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &longer) ||
                context->explorer.cooling_toggle == NULL ||
                !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SHOW_COOLING) ||
                !context->explorer.running_cooling_full ||
                context->explorer.trajectory_domain_maximum_seconds <= short_window ||
                find_button(context->explorer.content,
                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HIDE_COOLING)) == NULL) {
                fprintf(stderr, "Running cooling toggle failed\n"); return 3;
            }
            if (capture_frame(display, context, directory, "running-cooling-full")) { return 6; }
            furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
            if (!render_scenario(context)) { return 3; }
        }
        if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING || page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            for (uint8_t variant = 1; variant < 3; ++variant) {
                context->explorer.running_view_mode = variant;
                if (page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
                    if (!render_scenario(context)) { return 3; }
                    context->explorer.selected_stage = (uint8_t)(variant + 1U);
                    if (!open_stage_editor(context, (uint8_t)(variant + 1U))) { return 3; }
                } else if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) { return 3; }
                if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
                    /* Split and Furnace views keep the same reading line. */
                    if (!reading_is(context, 0U, "Elapsed", NULL) || !reading_is(context, 3U, "Target \xC2\xB0" "C", NULL)) {
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
    /* Manual running (web): readings strip with the observed elapsed time
     * above the forecast; Pause/Stop left of the one-piece steppers. */
    lv_obj_t *manual_elapsed = find_label(context->explorer.content, "0:42");
    lv_obj_t *target_value = find_button(context->explorer.content, "120");
    lv_obj_t *manual_stepper = target_value == NULL ? NULL : lv_obj_get_parent(target_value);
    lv_obj_t *target_decrement = manual_stepper == NULL ? NULL : lv_obj_get_child(manual_stepper, 0);
    lv_obj_t *target_increment = manual_stepper == NULL ? NULL : lv_obj_get_child(manual_stepper, 2);
    lv_obj_t *manual_running_stop = find_button(
        context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
    lv_obj_t *manual_readings = context->explorer.manual_readings;
    lv_obj_t *manual_graph = context->explorer.trajectory;
    lv_obj_t *manual_control_stack = manual_stepper == NULL ? NULL : lv_obj_get_parent(manual_stepper);
    lv_obj_t *manual_controls = manual_control_stack == NULL ? NULL : lv_obj_get_parent(manual_control_stack);
    lv_obj_t *manual_actions = manual_running_stop == NULL ? NULL : lv_obj_get_parent(manual_running_stop);
    if (manual_elapsed == NULL || target_decrement == NULL || target_increment == NULL ||
        manual_running_stop == NULL || manual_readings == NULL || manual_graph == NULL ||
        manual_controls == NULL || manual_control_stack == NULL || manual_actions == NULL ||
        find_label(manual_readings, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_SETPOINT)) == NULL) {
        fprintf(stderr, "Manual geometry check: missing control widgets\n");
        return 3;
    }
    lv_area_t readings_area, graph_area, target_decrement_area, manual_stop_area, manual_content_area,
        manual_controls_area, manual_control_stack_area, manual_actions_area;
    lv_obj_get_coords(manual_readings, &readings_area);
    lv_obj_get_coords(manual_graph, &graph_area);
    lv_obj_get_coords(target_decrement, &target_decrement_area);
    lv_obj_get_coords(manual_running_stop, &manual_stop_area);
    lv_obj_get_coords(context->explorer.content, &manual_content_area);
    lv_obj_get_coords(manual_controls, &manual_controls_area);
    lv_obj_get_coords(manual_control_stack, &manual_control_stack_area);
    lv_obj_get_coords(manual_actions, &manual_actions_area);
    if (readings_area.y2 >= graph_area.y1 || graph_area.y2 >= manual_controls_area.y1 ||
        lv_obj_get_width(target_decrement) < 48 || lv_obj_get_height(target_decrement) < 56 ||
        lv_obj_get_height(manual_graph) <= lv_obj_get_height(manual_controls) * 2 ||
        manual_stop_area.y2 < manual_content_area.y2 - 20 ||
        manual_actions_area.x2 >= manual_control_stack_area.x1) {
        fprintf(stderr,
                "Manual geometry check: readings %ld graph %ld-%ld controls %ld button %ld/%ld actions %ld stack %ld stop %ld content %ld\n",
                (long)readings_area.y2, (long)graph_area.y1, (long)graph_area.y2, (long)manual_controls_area.y1,
                (long)lv_obj_get_width(target_decrement), (long)lv_obj_get_height(target_decrement),
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
        "settings-system",
    };
    for (size_t index = 0U; index < sizeof(settings_tabs) / sizeof(settings_tabs[0]); ++index) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        context->explorer.settings_tab = settings_tabs[index];
        context->explorer.settings_connections_enabled = settings_tabs[index] == 2U;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
            capture_frame(display, context, directory, settings_names[index])) { return 6; }
    }
    context->explorer.settings_connections_enabled = false;
    /* Set date & time over the Date & time page, then the Dim when idle choice. */
    context->explorer.settings_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_AND_TIME)) ||
        context->explorer.modal == NULL ||
        capture_frame(display, context, directory, "settings-set-date-time") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) { return 6; }
    context->explorer.settings_tab = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_WHEN_IDLE)) ||
        context->explorer.modal == NULL ||
        capture_frame(display, context, directory, "settings-dim-choice") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    context->explorer.device_tab = 0U;
    context->explorer.device_filter = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-overview")) { return 6; }
    context->explorer.device_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-log")) { return 6; }
    context->explorer.device_tab = 2U;
    context->explorer.device_page = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-maintenance")) { return 6; }
    context->explorer.device_page = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        capture_frame(display, context, directory, "device-maintenance-2")) { return 6; }
    context->explorer.device_tab = 3U;
    context->explorer.device_page = 0U;
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
    /* Losing the link raises the Connection lost alert; capture it, then close it. */
    if (context->explorer.modal == NULL || capture_frame(display, context, directory, "alert-link-lost") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) { return 6; }
    context->explorer.device_tab = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE)) { return 3; }
    if (capture_frame(display, context, directory, "device-stale")) { return 6; }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) { return 3; }
    if (capture_frame(display, context, directory, "service-dialog") != 0) { return 6; }
    /* Maintenance task dialog (corner X, Maintenance done) and its result. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context) || !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_OVERDUE)) ||
        capture_frame(display, context, directory, "maintenance-task") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_DONE) ||
        capture_frame(display, context, directory, "maintenance-recorded")) { return 6; }
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG) ||
        capture_frame(display, context, directory, "device-log") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_ALARMS) ||
        capture_frame(display, context, directory, "device-log-alarms") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ALL) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INFO) ||
        capture_frame(display, context, directory, "device-info") ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_VALUE)) ||
        capture_frame(display, context, directory, "device-support")) { return 6; }
    /* Manual readings at the review sizes, while heating and after Stop (cooling). */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context) || !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_MANUAL) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START) || capture_frame(display, context, directory, "manual-heating") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) || capture_frame(display, context, directory, "manual-cooling")) { return 6; }
    /* Date & time with the date format card; Delete programs with one chosen. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    context->explorer.settings_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        capture_frame(display, context, directory, "settings-time")) { return 6; }
    context->explorer.settings_tab = 3U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)) ||
        capture_frame(display, context, directory, "settings-delete") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)) { return 6; }
    context->explorer.settings_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT)) ||
        capture_frame(display, context, directory, "settings-date-format")) { return 6; }
    for (unsigned variant = 0; variant < 10; ++variant) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context)) { return 3; }
        furnace_hmi_ui_lab_page_t page = FURNACE_HMI_UI_LAB_PAGE_PROGRAMS;
        const char *name = "programs-last";
        if (variant == 0) { fill_program_library(context); context->explorer.program_page = 1; }
        if (variant == 1) { fill_program_library(context); context->explorer.program_page = 1; context->explorer.library_count = 0; name = "programs-partial"; }
        if (variant == 2) { context->explorer.library_count = 0; name = "programs-empty"; }
        if (variant == 3) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES; context->explorer.program_stage_page = 0; name = "stages-last"; }
        if (variant == 4) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES; fill_program_library(context); context->explorer.selected_program = 11; context->explorer.program_stage_page = 0; name = "stages-maximum"; }
        if (variant == 5) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL; context->explorer.favourite_count = 1; name = "remove-favourite"; }
        if (variant >= 6 && variant <= 8) {
            page = FURNACE_HMI_UI_LAB_PAGE_HOME; context->explorer.recent_count = 0;
            context->explorer.favourite_count = (uint8_t)(variant - 6);
            if (variant == 8) { context->explorer.favourite_count = 3; }
            for (uint8_t k = 0; k < context->explorer.favourite_count; ++k) { context->explorer.quick_launch_favourites[k] = k; }
            name = variant == 6 ? "home-empty" : variant == 7 ? "home-few" : "home-full";
        }
        if (variant == 9) { page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING; fill_program_library(context); context->explorer.selected_program = 11; name = "loading-long"; }
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
        if (!opened || !pump_controller_requests(context)) { return 3; }
        if (capture_frame(display, context, directory, dialog_names[i])) { return 6; }
        if (i == 1) {
            if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT) || capture_frame(display, context, directory, "request-result")) { return 6; }
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
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES) ||
                !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) || !tap_text(context, "+ Add stage")) { return 3; }
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
            if (!open_stage_editor(context, 3U)) { return 3; }
        } else {
            name = "favourite-limit"; context->explorer.favourite_count = 3;
            fill_program_library(context);
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
    /* Start asks on Program details; Load program starts at once. The run
     * follows the controller's reports; Stop returns Home. */
    fill_program_library(context);
    context->explorer.selected_program = 11;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) || capture_frame(display,context,directory,"start-confirm") ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer,FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING) ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_RUNNING || capture_frame(display,context,directory,"start-running")) { return 6; }
    furnace_hmi_simulator_tick(&context->simulator, 150U * 60U);
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) ||
        capture_frame(display,context,directory,"program-run") ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) || !pump_controller_requests(context) ||
        capture_frame(display,context,directory,"program-stopped") ||
        !tap(context,FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_HOME || capture_frame(display,context,directory,"home-changed-recents")) { return 6; }
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

static bool modal_shows(const ui_lab_context_t *context, furnace_hmi_ui_string_id_t title)
{
    return context->explorer.modal != NULL && find_label(context->explorer.modal, ui_string(title)) != NULL;
}

/* Controller alerts: each fault kind opens its alert over Running (Home for a
 * power interruption), Close only dismisses it, Return Home leaves the ended
 * run, and link loss/restoration raise their own alerts with commands disabled. */
static int check_controller_alerts(ui_lab_context_t *context)
{
    static const struct {
        furnace_hmi_sim_scenario_t scenario;
        furnace_hmi_ui_string_id_t title;
        furnace_hmi_ui_lab_page_t page;
    } faults[] = {
        { FURNACE_HMI_SIM_SCENARIO_FAULT, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_DOOR_OPEN_TITLE, FURNACE_HMI_UI_LAB_PAGE_RUNNING },
        { FURNACE_HMI_SIM_SCENARIO_OVER_TEMPERATURE, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_OVER_TEMPERATURE_TITLE, FURNACE_HMI_UI_LAB_PAGE_RUNNING },
        { FURNACE_HMI_SIM_SCENARIO_HEATING_SLOW, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_TARGET_NOT_REACHED_TITLE, FURNACE_HMI_UI_LAB_PAGE_RUNNING },
        { FURNACE_HMI_SIM_SCENARIO_SENSOR_FAULT, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_SENSOR_TITLE, FURNACE_HMI_UI_LAB_PAGE_RUNNING },
        { FURNACE_HMI_SIM_SCENARIO_POWER_LOSS, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_POWER_INTERRUPTED_TITLE, FURNACE_HMI_UI_LAB_PAGE_HOME },
    };
    for (size_t i = 0; i < sizeof(faults) / sizeof(faults[0]); ++i) {
        furnace_hmi_simulator_set_scenario(&context->simulator, faults[i].scenario);
        if (!render_scenario(context) || context->explorer.page != faults[i].page ||
            !modal_shows(context, faults[i].title)) {
            fprintf(stderr, "Alert missing for %s\n", furnace_hmi_sim_scenario_name(faults[i].scenario));
            return 3;
        }
        if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) || context->explorer.modal != NULL) {
            fprintf(stderr, "Alert did not close for %s\n", furnace_hmi_sim_scenario_name(faults[i].scenario));
            return 3;
        }
        if (faults[i].page != FURNACE_HMI_UI_LAB_PAGE_RUNNING) { continue; }
        /* The run has ended: the fault replaces the stage label, Pause is gone and Stop is Return Home. */
        if (find_label(context->explorer.content, ui_string(faults[i].title)) == NULL ||
            find_button(context->explorer.root, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW)) != NULL ||
            find_button(context->explorer.root, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW)) != NULL ||
            !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME) ||
            context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_HOME) {
            fprintf(stderr, "Faulted Running did not end the run for %s\n", furnace_hmi_sim_scenario_name(faults[i].scenario));
            return 3;
        }
    }

    /* A fault reported during a run keeps Running and opens the alert. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
    if (!render_scenario(context) || context->explorer.modal != NULL) { return 3; }
    furnace_hmi_dashboard_state_t state = *furnace_hmi_simulator_state(&context->simulator);
    state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
    state.fault_kind = FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &state) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
        !modal_shows(context, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_OVER_TEMPERATURE_TITLE)) {
        fprintf(stderr, "Live fault did not raise its alert\n");
        return 3;
    }
    /* The same report again does not reopen a closed alert. */
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !furnace_hmi_ui_lab_explorer_update(&context->explorer, &state) || context->explorer.modal != NULL) {
        fprintf(stderr, "Repeated fault reopened its alert\n");
        return 3;
    }

    /* Link lost: alert, Pause and Stop unavailable; restored while open: restored alert. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
    if (!render_scenario(context)) { return 3; }
    const furnace_hmi_dashboard_state_t current = *furnace_hmi_simulator_state(&context->simulator);
    furnace_hmi_dashboard_state_t stale = current;
    stale.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    lv_obj_t *stop = NULL;
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &stale) ||
        !modal_shows(context, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE) ||
        (stop = find_button(context->explorer.root, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW))) == NULL ||
        !lv_obj_has_state(stop, LV_STATE_DISABLED)) {
        fprintf(stderr, "Link loss did not raise its alert or disable Stop\n");
        return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &current) ||
        !modal_shows(context, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_RESTORED_TITLE) ||
        (stop = find_button(context->explorer.root, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW))) == NULL ||
        lv_obj_has_state(stop, LV_STATE_DISABLED) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Link restoration did not show its alert or re-enable Stop\n");
        return 3;
    }
    /* Restored after the operator closed the link-lost alert: nothing reopens. */
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, &stale) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !furnace_hmi_ui_lab_explorer_update(&context->explorer, &current) || context->explorer.modal != NULL) {
        fprintf(stderr, "Restored link reopened a closed alert\n");
        return 3;
    }
    printf("UI lab: controller alerts, faulted Running and link loss passed.\n");
    return 0;
}

/* Theme switch check: a probe palette whose every colour differs from every
 * colour of every generated theme. After switching, no visible style may still
 * hold a theme colour; one that does was set from a literal or kept from
 * before the switch. Opacities follow the default theme. */
static furnace_hmi_color_t probe_palette[FURNACE_HMI_COLOR_COUNT];
static unsigned probe_checked;

static bool rgb_in_themes(uint32_t rgb)
{
    for (int id = 0; id < FURNACE_HMI_THEME_COUNT; ++id) {
        const furnace_hmi_theme_t *theme = furnace_hmi_theme((furnace_hmi_theme_id_t)id);
        for (int role = 0; role < FURNACE_HMI_COLOR_COUNT; ++role) {
            if (theme->colors[role].rgb == rgb) {
                return true;
            }
        }
    }
    return false;
}

static void build_probe_palette(void)
{
    const furnace_hmi_theme_t *base = furnace_hmi_theme((furnace_hmi_theme_id_t)0);
    uint32_t seed = 0x2468ACU;
    for (int role = 0; role < FURNACE_HMI_COLOR_COUNT; ++role) {
        bool unique = false;
        uint32_t rgb = 0U;
        while (!unique) {
            seed = seed * 1103515245U + 12345U;
            rgb = (seed >> 8) & 0xFFFFFFU;
            unique = !rgb_in_themes(rgb);
            for (int other = 0; unique && other < role; ++other) {
                unique = probe_palette[other].rgb != rgb;
            }
        }
        probe_palette[role] = (furnace_hmi_color_t){ rgb, base->colors[role].opa };
    }
}

static bool stale_color(lv_color_t color, const char *property, lv_obj_t *object, const char *where)
{
    const uint32_t rgb = lv_color_to_u32(color) & 0xFFFFFFU;
    ++probe_checked;
    if (!rgb_in_themes(rgb)) {
        return false;
    }
    const char *text = lv_obj_check_type(object, &lv_label_class) ? lv_label_get_text(object) : "";
    fprintf(stderr, "Theme switch: %s keeps theme colour %06X in %s (label \"%s\")\n",
            where, (unsigned)rgb, property, text);
    return true;
}

/* Visible style colours of an object tree; returns the number still holding a theme colour. */
static unsigned count_stale_colors(lv_obj_t *object, const char *where)
{
    if (object == NULL || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) {
        return 0U;
    }
    const lv_part_t main = LV_PART_MAIN;
    unsigned stale = 0U;
    /* A gradient descriptor (the light strip) replaces the background colour. */
    if (lv_obj_get_style_bg_opa(object, main) > LV_OPA_TRANSP && lv_obj_get_style_bg_grad(object, main) == NULL) {
        stale += stale_color(lv_obj_get_style_bg_color(object, main), "background", object, where);
        if (lv_obj_get_style_bg_grad_dir(object, main) != LV_GRAD_DIR_NONE) {
            stale += stale_color(lv_obj_get_style_bg_grad_color(object, main), "gradient", object, where);
        }
    }
    if (lv_obj_get_style_border_width(object, main) > 0 && lv_obj_get_style_border_opa(object, main) > LV_OPA_TRANSP) {
        stale += stale_color(lv_obj_get_style_border_color(object, main), "border", object, where);
    }
    if (lv_obj_get_style_shadow_width(object, main) > 0 && lv_obj_get_style_shadow_opa(object, main) > LV_OPA_TRANSP) {
        stale += stale_color(lv_obj_get_style_shadow_color(object, main), "shadow", object, where);
    }
    if (lv_obj_check_type(object, &lv_label_class)) {
        stale += stale_color(lv_obj_get_style_text_color(object, main), "text", object, where);
    }
    if (lv_obj_check_type(object, &lv_image_class) && lv_obj_get_style_image_recolor_opa(object, main) > LV_OPA_TRANSP) {
        stale += stale_color(lv_obj_get_style_image_recolor(object, main), "icon", object, where);
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count && stale < 5U; ++index) {
        stale += count_stale_colors(lv_obj_get_child(object, (int32_t)index), where);
    }
    return stale;
}

static int check_theme_switch(ui_lab_context_t *context)
{
    build_probe_palette();
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) { return 3; }
    /* Switching in place restyles the chrome built at creation and the page. */
    ui_lab_theme_use_palette(probe_palette);
    if (!furnace_hmi_ui_lab_explorer_refresh_presentation(&context->explorer)) {
        fprintf(stderr, "Theme switch: refresh failed\n");
        ui_lab_theme_use_palette(NULL);
        return 3;
    }
    if (count_stale_colors(context->explorer.root, "Home after refresh") != 0U) {
        ui_lab_theme_use_palette(NULL);
        return 3;
    }
    unsigned stale = 0U;
    for (int page = 0; page < FURNACE_HMI_UI_LAB_PAGE_COUNT && stale == 0U; ++page) {
        /* Each route from a fresh screen, as in the structural route check. */
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context) ||
            !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) {
            fprintf(stderr, "Theme switch: page %d failed to open\n", page);
            ui_lab_theme_use_palette(NULL);
            return 3;
        }
        char where[32];
        (void)snprintf(where, sizeof(where), "page %d", page);
        stale += count_stale_colors(context->explorer.root, where);
    }
    context->explorer.settings_connections_enabled = true;
    for (uint8_t tab = 0U; tab < 4U && stale == 0U; ++tab) {
        context->explorer.settings_tab = tab;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS)) {
            ui_lab_theme_use_palette(NULL);
            return 3;
        }
        stale += count_stale_colors(context->explorer.root, "Settings tab");
    }
    context->explorer.settings_connections_enabled = false;
    context->explorer.settings_tab = 0U;
    for (uint8_t tab = 0U; tab < 5U && stale == 0U; ++tab) {
        context->explorer.device_tab = tab;
        if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE)) {
            ui_lab_theme_use_palette(NULL);
            return 3;
        }
        stale += count_stale_colors(context->explorer.root, "Device tab");
    }
    context->explorer.device_tab = 0U;
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL);
    if (!render_scenario(context) ||
        !furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) {
        ui_lab_theme_use_palette(NULL);
        return 3;
    }
    stale += count_stale_colors(context->explorer.root, "Running with a dialog");
    /* Back to the default theme: the root takes the theme's page colour again. */
    if (!ui_lab_theme_select((furnace_hmi_theme_id_t)0) ||
        !furnace_hmi_ui_lab_explorer_refresh_presentation(&context->explorer) ||
        (lv_color_to_u32(lv_obj_get_style_bg_color(context->explorer.root, LV_PART_MAIN)) & 0xFFFFFFU) !=
            furnace_hmi_theme((furnace_hmi_theme_id_t)0)->colors[FURNACE_HMI_COLOR_PAGE].rgb) {
        fprintf(stderr, "Theme switch back to the default theme failed\n");
        return 3;
    }
    /* Guard against a vacuous pass: the pages hold thousands of styled colours. */
    if (stale != 0U || probe_checked < 1000U) {
        fprintf(stderr, "Theme switch: %u stale of %u colours checked\n", stale, probe_checked);
        return 3;
    }
    printf("UI lab: theme switch restyled every page, tab and dialog.\n");
    return 0;
}

/* Pseudo-language check: every catalogue text with letters becomes
 * "[text~~~]" (about 30 % longer, conversions and {fields} kept). A visible
 * label with letters but no "[" did not come from the catalogue. Single
 * characters (keyboard keys) and catalogue data loaded from files are allowed. */
static char pseudo_pool[96000];
static const char *pseudo_table[FURNACE_HMI_UI_STRING_COUNT];

static bool has_letter(const char *text)
{
    for (; *text != '\0'; ++text) {
        if ((*text >= 'A' && *text <= 'Z') || (*text >= 'a' && *text <= 'z')) {
            return true;
        }
    }
    return false;
}

static bool build_pseudo_table(void)
{
    size_t used = 0U;
    for (int id = 0; id < FURNACE_HMI_UI_STRING_COUNT; ++id) {
        const char *text = furnace_hmi_ui_string((furnace_hmi_ui_string_id_t)id);
        pseudo_table[id] = NULL;
        if (text == NULL || !has_letter(text) || id == FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_SEPARATOR) {
            continue;
        }
        const size_t length = strlen(text);
        const size_t padding = (length * 3U + 9U) / 10U;
        if (used + length + padding + 3U > sizeof(pseudo_pool)) {
            return false;
        }
        char *out = pseudo_pool + used;
        out[0] = '[';
        memcpy(out + 1, text, length);
        memset(out + 1 + length, '~', padding);
        out[1 + length + padding] = ']';
        out[2 + length + padding] = '\0';
        pseudo_table[id] = out;
        used += length + padding + 3U;
    }
    return true;
}

/* Text from data, not from the HMI: the device catalogue file (English text
 * fields today; see the readiness plan) and names the controller reports. */
static bool contains_field(const char *text, const char *field)
{
    return field[0] != '\0' && strstr(text, field) != NULL;
}

static bool is_catalogue_data(const ui_lab_context_t *context, const char *text)
{
    const furnace_hmi_ui_lab_device_catalogue_t *catalogue = &context->device_catalogue;
    if (contains_field(text, catalogue->revision)) {
        return true;
    }
    for (uint8_t part = 0U; part < catalogue->component_count; ++part) {
        const furnace_hmi_ui_lab_device_component_t *component = &catalogue->components[part];
        if (contains_field(text, component->name) || contains_field(text, component->part) ||
            contains_field(text, component->location) || contains_field(text, component->usage) ||
            contains_field(text, component->source)) {
            return true;
        }
        for (uint8_t task = 0U; task < component->task_count; ++task) {
            const furnace_hmi_ui_lab_device_task_t *item = &component->tasks[task];
            if (contains_field(text, item->name) || contains_field(text, item->basis) ||
                contains_field(text, item->interval) || contains_field(text, item->remaining) ||
                contains_field(text, item->last) || contains_field(text, item->procedure)) {
                return true;
            }
        }
    }
    /* Language names are always shown in their own language. */
    for (int language = 0; language < FURNACE_HMI_UI_LANGUAGE_COUNT; ++language) {
        if (contains_field(text, furnace_hmi_ui_language_name((furnace_hmi_ui_language_t)language))) {
            return true;
        }
    }
    const furnace_hmi_dashboard_state_t *state = context->explorer.state;
    return state != NULL && (contains_field(text, state->program_name) || contains_field(text, state->stage_name));
}

static unsigned count_hard_coded(const ui_lab_context_t *context, lv_obj_t *object, const char *where)
{
    if (object == NULL || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) {
        return 0U;
    }
    unsigned found = 0U;
    if (lv_obj_check_type(object, &lv_label_class)) {
        const char *text = lv_label_get_text(object);
        if (text != NULL && strlen(text) > 1U && has_letter(text) && strchr(text, '[') == NULL &&
            !is_catalogue_data(context, text)) {
            fprintf(stderr, "Pseudo language: %s shows \"%s\" from outside the catalogue\n", where, text);
            found = 1U;
        }
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        found += count_hard_coded(context, lv_obj_get_child(object, (int32_t)index), where);
    }
    return found;
}

static int check_pseudo_language(ui_lab_context_t *context, bool report_only)
{
    if (!build_pseudo_table()) {
        fprintf(stderr, "Pseudo language: pool too small\n");
        return 3;
    }
    furnace_hmi_ui_strings_override(pseudo_table);
    unsigned found = 0U;
    char where[48];
    /* A language change in place relabels the navigation built at creation. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    furnace_hmi_ui_strings_override(NULL);
    if (!render_scenario(context)) {
        return 3;
    }
    furnace_hmi_ui_strings_override(pseudo_table);
    if (!furnace_hmi_ui_lab_explorer_refresh_presentation(&context->explorer) ||
        find_button(context->explorer.nav, pseudo_table[FURNACE_HMI_UI_STRING_DASHBOARD_HOME]) == NULL) {
        fprintf(stderr, "Pseudo language: navigation kept its old labels after a refresh\n");
        furnace_hmi_ui_strings_override(NULL);
        return 3;
    }
    for (int page = 0; page < FURNACE_HMI_UI_LAB_PAGE_COUNT; ++page) {
        furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
        if (!render_scenario(context) ||
            !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, (furnace_hmi_ui_lab_page_t)page)) {
            furnace_hmi_ui_strings_override(NULL);
            return 3;
        }
        (void)snprintf(where, sizeof(where), "page %d", page);
        found += count_hard_coded(context, context->explorer.root, where);
    }
    context->explorer.settings_connections_enabled = true;
    for (uint8_t tab = 0U; tab < 4U; ++tab) {
        context->explorer.settings_tab = tab;
        (void)furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS);
        (void)snprintf(where, sizeof(where), "Settings tab %u", (unsigned)tab);
        found += count_hard_coded(context, context->explorer.root, where);
    }
    context->explorer.settings_connections_enabled = false;
    context->explorer.settings_tab = 0U;
    for (uint8_t tab = 0U; tab < 5U; ++tab) {
        context->explorer.device_tab = tab;
        (void)furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        (void)snprintf(where, sizeof(where), "Device tab %u", (unsigned)tab);
        found += count_hard_coded(context, context->explorer.root, where);
    }
    context->explorer.device_tab = 0U;
    static const furnace_hmi_sim_scenario_t scenarios[] = {
        FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL, FURNACE_HMI_SIM_SCENARIO_COOLING,
        FURNACE_HMI_SIM_SCENARIO_FAULT, FURNACE_HMI_SIM_SCENARIO_STALE, FURNACE_HMI_SIM_SCENARIO_MANUAL,
    };
    for (size_t index = 0U; index < sizeof(scenarios) / sizeof(scenarios[0]); ++index) {
        furnace_hmi_simulator_set_scenario(&context->simulator, scenarios[index]);
        if (!render_scenario(context)) {
            furnace_hmi_ui_strings_override(NULL);
            return 3;
        }
        (void)snprintf(where, sizeof(where), "scenario %d", (int)scenarios[index]);
        found += count_hard_coded(context, context->explorer.root, where);
    }
    if (furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) {
        found += count_hard_coded(context, context->explorer.root, "service dialog");
    }
    furnace_hmi_ui_strings_override(NULL);
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context)) {
        return 3;
    }
    if (found != 0U && !report_only) {
        return 3;
    }
    printf("UI lab: pseudo language found %u labels outside the catalogue.\n", found);
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
    /* Display: the brightness gauge steps and -/+ (no number shown), the dim
     * choice (Off by default), Discard and Save. */
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MORE)) != NULL ||
        find_label(context->explorer.content, "80%") != NULL ||
        context->explorer.settings_saved_numeric[4] != 0 ||
        !tap_object(brightness_step(context, 2U)) ||
        context->explorer.settings_candidate_numeric[3] != 30 ||
        !tap_content_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE)) ||
        context->explorer.settings_candidate_numeric[3] != 40 ||
        !tap_object(brightness_step(context, 0U)) ||
        !lv_obj_has_state(find_button(context->explorer.content,
                                      ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE)), LV_STATE_DISABLED) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD) ||
        context->explorer.settings_candidate_numeric[3] != 80) {
        fprintf(stderr, "Settings brightness gauge failed\n"); return 3;
    }
    if (!tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_WHEN_IDLE)) ||
        context->explorer.modal == NULL ||
        find_button(context->explorer.modal, "30 min") == NULL ||
        !tap_object(find_button(context->explorer.modal, "5 min")) ||
        context->explorer.modal != NULL ||
        context->explorer.settings_candidate_numeric[4] != 5 ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DEFAULT_RUNNING_VIEW)) ||
        !tap_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_FURNACE)) ||
        context->explorer.settings_candidate_running_view != 2U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        context->explorer.settings_saved_numeric[4] != 5) {
        fprintf(stderr, "Settings display/save flow failed\n"); return 3;
    }
    /* Rail: Connections is listed only when the controller enables it;
     * System holds reset/delete/restart. */
    const char *connections = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONNECTIONS);
    if (find_button(context->explorer.content, connections) != NULL ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SYSTEM)) == NULL) {
        fprintf(stderr, "Settings rail without Connections failed\n"); return 3;
    }
    context->explorer.settings_connections_enabled = true;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_content_text(context, connections) ||
        context->explorer.settings_tab != 2U ||
        find_label(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SERVER)) == NULL) {
        fprintf(stderr, "Settings Connections when enabled failed\n"); return 3;
    }
    context->explorer.settings_connections_enabled = false;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        context->explorer.settings_tab != 0U ||
        find_button(context->explorer.content, connections) != NULL) {
        fprintf(stderr, "Settings Connections fallback failed\n"); return 3;
    }
    /* Set date & time: 31 Jan -> Feb clamps the day; Cancel keeps the clock;
     * Set changes it; the 12-hour format adds an AM/PM column. */
    context->explorer.settings_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        find_label(context->explorer.content, "Clock source") != NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_AND_TIME)) ||
        clock_column_count(context) != 5U) {
        fprintf(stderr, "Set date & time dialog failed to open\n"); return 3;
    }
    for (unsigned step = 0U; step < 8U; ++step) {
        if (!tap_object(clock_step_button(context, 1U, false))) { return 3; }
    }
    for (unsigned step = 0U; step < 17U; ++step) {
        if (!tap_object(clock_step_button(context, 0U, true))) { return 3; }
    }
    if (context->explorer.clock_draft.month != 1U || context->explorer.clock_draft.day != 31U ||
        !tap_object(clock_step_button(context, 1U, true)) ||
        context->explorer.clock_draft.month != 2U || context->explorer.clock_draft.day != 28U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) ||
        context->explorer.clock.month != 9U || context->explorer.clock.day != 14U ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_AND_TIME)) ||
        !tap_object(clock_step_button(context, 2U, true)) ||
        !tap_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET_DATE_TIME)) ||
        context->explorer.clock.year != 2027U ||
        find_label(context->explorer.modal,
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_TIME_SET_TITLE)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_TIME_FORMAT)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_AND_TIME)) ||
        clock_column_count(context) != 6U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD) ||
        !context->explorer.settings_candidate_24_hour) {
        fprintf(stderr, "Set date & time flow failed\n"); return 3;
    }
    context->explorer.clock = (furnace_hmi_ui_lab_clock_t){ 2026U, 9U, 14U, 14U, 32U };
    context->explorer.settings_tab = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_1)) ||
         !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) ||
         context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
         /* First stage card: its detail line is "50 min  ·  0.3 C/min". */
         !tap_label_button(context, "50 min  \xC2\xB7  0.3 \xC2\xB0" "C/min") ||
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
        !lv_obj_check_type(lv_obj_get_child(favourite_button, 0), &lv_image_class) ||
        !tap_program_favourite(context)) {
        fprintf(stderr, "Program favourite star flow failed\\n"); return 3;
    }
    /* Program details: Start program asks first. Cancel changes nothing;
     * confirming records the program as recent and requests the start. The
     * controller's run opens Running; Pause, Resume and Stop are requests too. */
    context->explorer.selected_program = 2U;
    const uint8_t recent_before = context->explorer.quick_launch_recent[0];
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) || context->explorer.modal == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) || context->explorer.modal != NULL ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL ||
        context->explorer.quick_launch_recent[0] != recent_before ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) || context->explorer.modal != NULL ||
        context->explorer.quick_launch_recent[0] != 2U ||
        context->explorer.request.kind != FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM ||
        !pump_controller_requests(context) || context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
        strcmp(context->explorer.state->program_name, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_3)) != 0 ||
        context->explorer.state->stage_count != 6U) {
        fprintf(stderr, "Program details start confirmation failed\n"); return 3;
    }
    /* 70 minutes in: the first Hold (stage 2 of 6). */
    furnace_hmi_simulator_tick(&context->simulator, 70U * 60U);
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) ||
        context->explorer.state->stage_index != 1U || context->explorer.state->current_temperature_c.value != 90 ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW) || !pump_controller_requests(context) ||
        context->explorer.state->run_state != FURNACE_HMI_RUN_STATE_PAUSED ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW) || !pump_controller_requests(context) ||
        context->explorer.state->run_state != FURNACE_HMI_RUN_STATE_RUNNING ||
        /* Stop: the run is cut at 70 min and cools on Running, with Start again
         * and Return Home; Start again runs the same program from the start. */
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
        context->explorer.state->run_phase != FURNACE_HMI_RUN_PHASE_COOLING ||
        context->explorer.state->elapsed_seconds.value != 70 * 60 ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW)) != NULL ||
        find_label(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_STOPPED_COOLING)) == NULL ||
        furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_AGAIN) || !pump_controller_requests(context) ||
        context->explorer.state->run_phase != FURNACE_HMI_RUN_PHASE_CURING ||
        context->explorer.state->elapsed_seconds.value != 0 ||
        strcmp(context->explorer.state->program_name, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_3)) != 0 ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) || !pump_controller_requests(context) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_HOME ||
        context->explorer.state->mode != FURNACE_HMI_MACHINE_MODE_IDLE) {
        fprintf(stderr, "Program run pause/resume/stop failed\n"); return 3;
    }
    /* Stage dialog over the program editor: add after, move by position,
     * delete with confirmation; Discard restores every stage, Save keeps them
     * and marks the draft changed; a Hold locks Target and Rate. */
    context->explorer.selected_program = 0U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
        !tap_label_button(context, "50 min  \xC2\xB7  0.3 \xC2\xB0" "C/min") ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
        context->explorer.stage_dialog == NULL || context->explorer.staged_stage_count != 4U ||
        !tap_object(stage_dialog_top(context, 2U)) ||
        context->explorer.staged_stage_count != 5U || context->explorer.selected_stage != 1U ||
        context->explorer.staged_targets[1] != 60 ||
        !tap_object(lv_obj_get_child(lv_obj_get_child(stage_dialog_top(context, 1U), 1), 0)) ||
        context->explorer.modal == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR) || !tap_text(context, "4") ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY) ||
        context->explorer.selected_stage != 3U || context->explorer.staged_targets[3] != 60 ||
        !tap_object(stage_dialog_top(context, 0U)) || context->explorer.modal == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DELETE_STAGE) ||
        context->explorer.staged_stage_count != 4U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
        context->explorer.stage_dialog != NULL || context->explorer.program_editor_dirty ||
        context->explorer.staged_stage_count != 4U ||
        /* The editor shows the original stages again: no added 20-minute 60 C ramp. */
        find_label(context->explorer.content, "50 min  \xC2\xB7  0.3 \xC2\xB0" "C/min") == NULL ||
        find_label(context->explorer.content, "20 min  \xC2\xB7  1 \xC2\xB0" "C/min") != NULL) {
        fprintf(stderr, "Stage dialog add/move/delete/discard failed\n"); return 3;
    }
    if (!tap_label_button(context, "240 min") || context->explorer.draft_stage_kind != 1U) {
        fprintf(stderr, "Stage dialog hold entry failed\n"); return 3;
    }
    lv_obj_t *hold_plus = find_button(context->explorer.stage_dialog,
                                      ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE));
    if (hold_plus == NULL || !lv_obj_has_state(hold_plus, LV_STATE_DISABLED) ||
        !tap_object(stage_dialog_top(context, 2U)) || context->explorer.staged_stage_count != 5U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
        !context->explorer.program_editor_dirty || context->explorer.staged_stage_count != 5U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_BACK) || context->explorer.modal == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) {
        fprintf(stderr, "Stage dialog hold lock/save failed\n"); return 3;
    }
    /* A new program: Save waits for a stage; one added stage is enough. Saved, it
     * opens in the library, starts, runs through automatic cooling to complete,
     * returns Home, and can be deleted. */
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)) == NULL ||
        !lv_obj_has_state(find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM)),
                          LV_STATE_DISABLED) ||
        !tap_text(context, "+ Add stage") || context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR) {
        fprintf(stderr, "New program stage entry failed\n"); return 3;
    }
    /* The operator's name (the keyboard flow has its own checks). */
    (void)snprintf(context->explorer.program_editor_name, sizeof(context->explorer.program_editor_name), "%s", "Customer cure A");
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL || context->explorer.selected_program != 3U ||
        !context->explorer.programs[3].present || context->explorer.programs[3].library ||
        context->explorer.programs[3].stage_count != 1U || context->explorer.programs[3].stages[0].target_c != 120 ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) ||
        strcmp(context->explorer.program_name_overrides[3], "Customer cure A") != 0 ||
        !tap_row_text(context, "Customer cure A") ||
        context->explorer.selected_program != 3U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
        strcmp(context->explorer.state->program_name, "Customer cure A") != 0) {
        fprintf(stderr, "New program save/list/start failed\n"); return 3;
    }
    /* 95 minutes of heating to 120 C, then 75 minutes of automatic cooling. */
    furnace_hmi_simulator_tick(&context->simulator, 95U * 60U);
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) ||
        context->explorer.state->run_phase != FURNACE_HMI_RUN_PHASE_COOLING) {
        fprintf(stderr, "New program did not reach automatic cooling\n"); return 3;
    }
    furnace_hmi_simulator_tick(&context->simulator, 75U * 60U);
    if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) ||
        context->explorer.state->run_state != FURNACE_HMI_RUN_STATE_COMPLETE ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW)) != NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME) || !pump_controller_requests(context) ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_HOME ||
        context->explorer.quick_launch_recent[0] != 3U) {
        fprintf(stderr, "New program completion/Return Home failed\n"); return 3;
    }
    context->explorer.selected_program = 3U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT) || context->explorer.modal == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT) ||
        context->explorer.programs[3].present || context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAMS ||
        context->explorer.recent_count == 0U || context->explorer.quick_launch_recent[0] == 3U ||
        find_label(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DELETED)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Program delete failed\n"); return 3;
    }
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS)) {
        fprintf(stderr, "Program refinement smoke return failed\\n");
        return 3;
    }
    if (!tap_content_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SYSTEM)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SETTINGS)) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_PREFERENCES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_PREFERENCES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Settings System flow failed\n"); return 3;
    }
    if (!tap_text(context, "Display & interaction") ||
        !tap_content_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE)) ||
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
    /* Device log: a month of controller events, newest first, this display's
     * start on top; filter tabs; an event opens its details. */
    /* Manual Stop: the controller cools automatically and the session stays on
     * screen with cooling readings, the steppers off and Exit / Start new
     * (navigation stays locked); Start new keeps target and rate. */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context) ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_MANUAL)) {
        fprintf(stderr, "Manual cooling setup failed\n"); return 3;
    }
    const int32_t manual_target = context->explorer.manual_target_c;
    if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START) || !context->explorer.manual_running ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) || !context->explorer.manual_cooling ||
        context->explorer.manual_running ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW)) != NULL ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_EXIT)) == NULL ||
        find_label(context->explorer.manual_readings, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_READING_EST_REMAINING)) == NULL ||
        !lv_obj_has_state(lv_obj_get_child(lv_obj_get_child(context->explorer.manual_steppers, 0), 1), LV_STATE_DISABLED) ||
        furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_START_NEW) || context->explorer.manual_cooling ||
        context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_MANUAL || context->explorer.manual_target_c != manual_target ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_START) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_EXIT) || context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_HOME) {
        fprintf(stderr, "Manual stop/cooling/Exit/Start new failed\n"); return 3;
    }
    /* Date format: a choice of five; dates on the page follow the edited one,
     * Save keeps it, Reset (date & time formats) restores "6 Oct 2026". */
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    context->explorer.settings_tab = 1U;
    if (!render_scenario(context)) { return 3; }
    context->explorer.settings_tab = 1U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        find_label(context->explorer.content, "14 Sep 2026") == NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT)) ||
        !tap_text(context, "14.09.2026") || context->explorer.settings_candidate_date_format != 1U ||
        find_label(context->explorer.content, "14.09.2026") == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE) || context->explorer.settings_saved_date_format != 1U ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Date format choice failed\n"); return 3;
    }
    /* Settings > System > Delete programs removes every program after one
     * confirmation that names them; then there is nothing to delete. Reset of
     * the formats restores the date format. */
    context->explorer.settings_tab = 3U;
    if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)) ||
        find_label(context->explorer.modal, "Delete all 3 programs?") == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CANCEL) || !context->explorer.programs[1].present ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ALL) ||
        context->explorer.programs[0].present || context->explorer.programs[1].present ||
        context->explorer.programs[2].present ||
        find_label(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAMS_DELETED)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS)) ||
        find_label(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NO_PROGRAMS)) == NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SETTINGS)) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_FORMAT_PREFERENCES) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_PREFERENCES) ||
        context->explorer.settings_saved_date_format != 0U || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) {
        fprintf(stderr, "Settings Delete programs / Reset formats failed\n"); return 3;
    }
    furnace_hmi_simulator_set_scenario(&context->simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    if (!render_scenario(context) ||
        !furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE)) {
        fprintf(stderr, "Device log setup failed\n"); return 3;
    }
    const furnace_hmi_ui_lab_explorer_t *log_explorer = &context->explorer;
    bool log_sorted = log_explorer->log_count > 40U;
    for (uint16_t index = 1U; index < log_explorer->log_count; ++index) {
        log_sorted = log_sorted && log_explorer->log_events[index].minute <= log_explorer->log_events[index - 1U].minute;
    }
    if (!log_sorted || log_explorer->log_events[0].minute - log_explorer->log_events[log_explorer->log_count - 1U].minute >= 30 * 1440) {
        fprintf(stderr, "Device log history failed\n"); return 3;
    }
    if (!tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG) ||
        find_button(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_MAINTENANCE)) == NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_DISPLAY_STARTED)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_ALARMS) || context->explorer.log_filter != 2U ||
        find_label(context->explorer.content, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_DISPLAY_STARTED)) != NULL ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ALL) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE) ||
        maintenance_card_count(context) != 6U ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_OVERDUE))) {
        fprintf(stderr, "Device log/maintenance navigation failed\n"); return 3;
    }
    /* A task dialog: the corner X closes it; Maintenance done is recorded by the
     * controller, resets the task and leads Service history. */
    lv_obj_t *task_dialog = context->explorer.modal != NULL ? lv_obj_get_child(context->explorer.modal, 0) : NULL;
    lv_obj_t *corner = NULL;
    for (uint32_t child = 0U; task_dialog != NULL && child < lv_obj_get_child_count(task_dialog); ++child) {
        lv_obj_t *item = lv_obj_get_child(task_dialog, (int32_t)child);
        if (lv_obj_has_flag(item, LV_OBJ_FLAG_FLOATING)) { corner = item; }
    }
    const uint8_t task_component = context->explorer.device_selected_component;
    const uint8_t task_index = context->explorer.device_selected_task;
    if (corner == NULL || find_button(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) != NULL ||
        !tap_object(corner) || context->explorer.modal != NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_OVERDUE)) ||
        !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_DONE) ||
        find_label(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_RECORDED)) == NULL ||
        context->explorer.service_record_count != 1U ||
        context->explorer.device_catalogue.components[task_component].tasks[task_index].task_state !=
            FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE ||
        strcmp(context->explorer.device_catalogue.components[task_component].tasks[task_index].last, "14 Sep 2026") != 0) {
        fprintf(stderr, "Maintenance done flow failed\n"); return 3;
    }
    if (!tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_HISTORY) ||
        !tap_row_text(context, context->explorer.service_records[0].title) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INSPECTION)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) ||
        !tap_device_category(context, FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INFO) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MODEL)) ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE))) {
        fprintf(stderr, "Device notification/history/info flow failed\n"); return 3;
    }
    /* Device info: five cards, no Controller link; Support shows the QR code
     * (an A8 image in qr-ink) with the corner X as its only way out. */
    lv_obj_t *support_code = NULL;
    if (find_label(context->explorer.content, "Controller link") != NULL ||
        !tap_row_text(context, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_VALUE)) ||
        context->explorer.modal == NULL ||
        find_button(context->explorer.modal, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) != NULL) {
        fprintf(stderr, "Device info Support dialog failed\n"); return 3;
    }
    lv_obj_t *support_dialog = lv_obj_get_child(context->explorer.modal, 0);
    lv_obj_t *support_corner = NULL;
    for (uint32_t child = 0U; child < lv_obj_get_child_count(support_dialog); ++child) {
        lv_obj_t *item = lv_obj_get_child(support_dialog, (int32_t)child);
        if (lv_obj_has_flag(item, LV_OBJ_FLAG_FLOATING)) { support_corner = item; }
        lv_obj_t *plate = lv_obj_get_child_count(item) == 1U ? lv_obj_get_child(item, 0) : NULL;
        lv_obj_t *image = plate != NULL && lv_obj_get_child_count(plate) == 1U ? lv_obj_get_child(plate, 0) : NULL;
        if (image != NULL && lv_obj_check_type(image, &lv_image_class) &&
            lv_image_get_src(image) == &furnace_hmi_support_qr) { support_code = image; }
    }
    if (support_code == NULL || support_corner == NULL || !tap_object(support_corner) ||
        context->explorer.modal != NULL) {
        fprintf(stderr, "Device info Support QR code failed\n"); return 3;
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
        /* Fault scenarios open with their alert; close it as an operator would. */
        if (context->explorer.alert_open != 0U && !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_CLOSE)) { return 3; }
        if (context->explorer.page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
            if (furnace_hmi_ui_lab_explorer_navigate(&context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME)) { return 3; }
            int64_t domain = context->explorer.trajectory_domain_maximum_seconds;
            if (!tap(context, FURNACE_HMI_UI_STRING_UI_LAB_FULL_VISUAL) || !tap(context, FURNACE_HMI_UI_STRING_UI_LAB_FULL_GRAPH) || domain != context->explorer.trajectory_domain_maximum_seconds) { return 3; }
            lv_obj_t *content = context->explorer.content;
            furnace_hmi_simulator_tick(&context->simulator, 1);
            if (!furnace_hmi_ui_lab_explorer_update(&context->explorer, furnace_hmi_simulator_state(&context->simulator)) || context->explorer.content != content) { return 3; }
            if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) { return 3; }
            /* A run the controller ended offers Return Home where Stop was. */
            const bool ended = context->explorer.state->run_state == FURNACE_HMI_RUN_STATE_FAULT;
            lv_obj_t *stop = find_button(context->explorer.content, ui_string(ended ? FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME
                                                                                    : FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW));
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
    const int alerts = check_controller_alerts(context);
    if (alerts != 0) { return alerts; }
    const int theme = check_theme_switch(context);
    if (theme != 0) { return theme; }
    const int pseudo = check_pseudo_language(context, false);
    if (pseudo != 0) { return pseudo; }
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

        if (context.explorer.root != NULL && !pump_controller_requests(&context)) {
            (void)fprintf(stderr, "UI laboratory controller request failed.\n");
            result = 5;
            goto cleanup;
        }
        const uint32_t now = lv_tick_get();
        if (now - last_tick >= 1000U) {
            furnace_hmi_simulator_tick(&context.simulator, (now - last_tick) / 1000U *
                (context.simulator.program_run ? UI_LAB_PROGRAM_RUN_SPEED : 1U));
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
