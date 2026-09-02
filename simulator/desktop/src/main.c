/* SPDX-License-Identifier: Apache-2.0 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/state_availability_view.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>
#include <SDL.h>

#if !defined(FURNACE_HMI_DESKTOP_WIDTH) || !defined(FURNACE_HMI_DESKTOP_HEIGHT)
#error "Desktop window dimensions must be supplied by the target build."
#endif

typedef struct {
    lv_obj_t *button;
    lv_obj_t *status_label;
    lv_obj_t *text_area;
    uint32_t input_event_count;
} input_diagnostic_view_t;

static void input_event_callback(lv_event_t *event)
{
    char status_text[64];
    input_diagnostic_view_t *view = lv_event_get_user_data(event);

    ++view->input_event_count;
    (void)snprintf(
        status_text,
        sizeof(status_text),
        "%s: %" PRIu32,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_INPUT_EVENTS_LABEL),
        view->input_event_count
    );
    lv_label_set_text(view->status_label, status_text);
}

static void create_input_diagnostic_view(input_diagnostic_view_t *view)
{
    char probe_text[64];
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *title = lv_label_create(screen);
    lv_obj_t *description = lv_label_create(screen);
    lv_obj_t *probe_label = lv_label_create(screen);
    lv_obj_t *button_label;

    view->status_label = lv_label_create(screen);
    view->button = lv_button_create(screen);
    button_label = lv_label_create(view->button);
    view->text_area = lv_textarea_create(screen);

    lv_obj_set_style_pad_all(screen, 32, LV_PART_MAIN);
    lv_obj_set_style_pad_row(screen, 14, LV_PART_MAIN);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        screen,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    lv_label_set_text(
        title,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_FOUNDATION_VIEW_TITLE)
    );
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_label_set_text(
        description,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_FOUNDATION_DESCRIPTION)
    );
    lv_obj_set_width(description, 680);
    lv_label_set_long_mode(description, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(description, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    (void)snprintf(
        probe_text,
        sizeof(probe_text),
        "%s: %" PRIu32,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_BUILD_PROBE_LABEL),
        furnace_hmi_foundation_probe_version()
    );
    lv_label_set_text(probe_label, probe_text);

    (void)snprintf(
        probe_text,
        sizeof(probe_text),
        "%s: 0",
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_INPUT_EVENTS_LABEL)
    );
    lv_label_set_text(view->status_label, probe_text);

    lv_label_set_text(
        button_label,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_INPUT_ACTION_LABEL)
    );
    lv_obj_center(button_label);
    lv_obj_add_event_cb(view->button, input_event_callback, LV_EVENT_CLICKED, view);

    lv_textarea_set_one_line(view->text_area, true);
    lv_textarea_set_placeholder_text(
        view->text_area,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_TEXT_INPUT_PLACEHOLDER)
    );
    lv_obj_set_width(view->text_area, 420);

    lv_group_t *input_group = lv_group_create();
    lv_group_set_default(input_group);
    lv_group_add_obj(input_group, view->button);
    lv_group_add_obj(input_group, view->text_area);

    lv_indev_t *keyboard = lv_sdl_keyboard_create();
    lv_indev_t *mouse_wheel = lv_sdl_mousewheel_create();
    (void)lv_sdl_mouse_create();
    lv_indev_set_group(keyboard, input_group);
    lv_indev_set_group(mouse_wheel, input_group);
}

static void handle_timers(uint32_t iterations)
{
    for(uint32_t iteration = 0; iteration < iterations; ++iteration) {
        uint32_t delay_ms = lv_timer_handler();
        if(delay_ms == LV_NO_TIMER_READY || delay_ms > UINT32_C(20)) {
            delay_ms = UINT32_C(5);
        }
        lv_delay_ms(delay_ms);
    }
}

static bool push_mouse_event(
    uint32_t type,
    uint32_t window_id,
    int32_t x,
    int32_t y
)
{
    SDL_Event event = {0};

    event.type = type;
    if(type == SDL_MOUSEMOTION) {
        event.motion.windowID = window_id;
        event.motion.x = x;
        event.motion.y = y;
    }
    else {
        event.button.windowID = window_id;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
        event.button.x = x;
        event.button.y = y;
    }

    return SDL_PushEvent(&event) == 1;
}

static int run_input_smoke_test(lv_display_t *display, input_diagnostic_view_t *view)
{
    SDL_Window *window = lv_sdl_window_get_window(display);
    SDL_Event text_event = {0};
    lv_area_t button_area;
    uint32_t window_id;
    int32_t button_x;
    int32_t button_y;

    if(window == NULL) {
        (void)fprintf(stderr, "SDL window lookup failed.\n");
        return 3;
    }

    lv_obj_update_layout(lv_screen_active());
    lv_obj_get_coords(view->button, &button_area);
    button_x = button_area.x1 + (button_area.x2 - button_area.x1) / 2;
    button_y = button_area.y1 + (button_area.y2 - button_area.y1) / 2;
    window_id = SDL_GetWindowID(window);

    if(
        !push_mouse_event(SDL_MOUSEMOTION, window_id, button_x, button_y)
        || !push_mouse_event(SDL_MOUSEBUTTONDOWN, window_id, button_x, button_y)
        || !push_mouse_event(SDL_MOUSEBUTTONUP, window_id, button_x, button_y)
    ) {
        (void)fprintf(stderr, "SDL pointer event injection failed.\n");
        return 4;
    }
    handle_timers(UINT32_C(30));
    if(view->input_event_count != UINT32_C(1)) {
        (void)fprintf(stderr, "SDL pointer input did not reach the LVGL widget.\n");
        return 5;
    }

    lv_group_focus_obj(view->text_area);
    text_event.type = SDL_TEXTINPUT;
    text_event.text.windowID = window_id;
    (void)snprintf(text_event.text.text, sizeof(text_event.text.text), "%s", "input");
    if(SDL_PushEvent(&text_event) != 1) {
        (void)fprintf(stderr, "SDL text event injection failed.\n");
        return 6;
    }
    handle_timers(UINT32_C(30));
    if(strcmp(lv_textarea_get_text(view->text_area), "input") != 0) {
        (void)fprintf(stderr, "SDL keyboard input did not reach the LVGL widget.\n");
        return 7;
    }

    return 0;
}

static bool object_tree_contains_label_text(const lv_obj_t *object, const char *expected)
{
    if (lv_obj_check_type(object, &lv_label_class) &&
        strcmp(lv_label_get_text(object), expected) == 0) {
        return true;
    }

    const uint32_t child_count = lv_obj_get_child_count(object);
    for (uint32_t index = 0; index < child_count; ++index) {
        if (object_tree_contains_label_text(lv_obj_get_child(object, (int32_t)index), expected)) {
            return true;
        }
    }

    return false;
}

static bool object_tree_is_read_only_presentation(const lv_obj_t *object)
{
    const lv_obj_class_t *object_class = lv_obj_get_class(object);
    if ((object_class != &lv_obj_class && object_class != &lv_label_class) ||
        lv_obj_has_flag_any(
            object,
            LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE
        )) {
        return false;
    }

    const uint32_t child_count = lv_obj_get_child_count(object);
    for (uint32_t index = 0; index < child_count; ++index) {
        if (!object_tree_is_read_only_presentation(
                lv_obj_get_child(object, (int32_t)index)
            )) {
            return false;
        }
    }

    return true;
}

static bool object_tree_fits_area(const lv_obj_t *object, const lv_area_t *bounds)
{
    lv_area_t area;

    lv_obj_get_coords(object, &area);
    if (area.x1 < bounds->x1 || area.y1 < bounds->y1 ||
        area.x2 > bounds->x2 || area.y2 > bounds->y2) {
        return false;
    }

    const uint32_t child_count = lv_obj_get_child_count(object);
    for (uint32_t index = 0; index < child_count; ++index) {
        if (!object_tree_fits_area(lv_obj_get_child(object, (int32_t)index), bounds)) {
            return false;
        }
    }

    return true;
}

static int verify_availability_presentation(
    const furnace_hmi_state_availability_view_t *view,
    furnace_hmi_state_availability_t expected_availability,
    furnace_hmi_ui_string_id_t expected_badge,
    furnace_hmi_ui_string_id_t expected_heading,
    furnace_hmi_ui_string_id_t expected_detail
)
{
    lv_area_t screen_bounds;

    if (view->rendered_availability != expected_availability) {
        (void)fprintf(stderr, "Availability presentation mapped to the wrong state.\n");
        return 8;
    }
    if (!object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected_badge)
        ) ||
        !object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected_heading)
        ) ||
        !object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected_detail)
        )) {
        (void)fprintf(stderr, "Availability-state copy was not rendered.\n");
        return 9;
    }
    if (!object_tree_is_read_only_presentation(view->root)) {
        (void)fprintf(stderr, "Availability view exposed an interactive object.\n");
        return 10;
    }

    lv_obj_update_layout(lv_screen_active());
    lv_obj_get_coords(lv_screen_active(), &screen_bounds);
    if (!object_tree_fits_area(view->root, &screen_bounds)) {
        (void)fprintf(stderr, "Availability view exceeded the supported viewport.\n");
        return 11;
    }

    return 0;
}

typedef struct {
    furnace_hmi_state_availability_t availability;
    furnace_hmi_ui_string_id_t badge;
    furnace_hmi_ui_string_id_t heading;
    furnace_hmi_ui_string_id_t detail;
} availability_test_case_t;

static int create_verify_destroy_presentation(
    const furnace_hmi_state_availability_view_state_t *state,
    const availability_test_case_t *expected
)
{
    furnace_hmi_state_availability_view_t view = {0};
    int result;

    if (!furnace_hmi_state_availability_view_create(
            &view,
            lv_screen_active(),
            state
        )) {
        (void)fprintf(stderr, "Availability-state view creation failed.\n");
        return 12;
    }
    handle_timers(UINT32_C(2));
    result = verify_availability_presentation(
        &view,
        expected->availability,
        expected->badge,
        expected->heading,
        expected->detail
    );

    if (furnace_hmi_state_availability_view_create(
            &view,
            lv_screen_active(),
            state
        )) {
        (void)fprintf(stderr, "Active availability view was recreated without destroy.\n");
        return 13;
    }
    furnace_hmi_state_availability_view_destroy(&view);
    if (view.root != NULL ||
        view.rendered_availability != FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        (void)fprintf(stderr, "Availability view destroy did not clear its handle.\n");
        return 14;
    }

    return result;
}

static int run_presentation_smoke_test(lv_display_t *display)
{
    static const availability_test_case_t valid_cases[] = {
        {
            .availability = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE,
            .badge = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_DETAIL,
        },
        {
            .availability = FURNACE_HMI_STATE_AVAILABILITY_STALE,
            .badge = FURNACE_HMI_UI_STRING_STATE_STALE_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_STALE_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_STALE_DETAIL,
        },
        {
            .availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT,
            .badge = FURNACE_HMI_UI_STRING_STATE_CURRENT_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_CURRENT_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_CURRENT_DETAIL,
        },
    };
    const availability_test_case_t *unavailable = &valid_cases[0];
    furnace_hmi_state_availability_view_state_t state = {0};
    furnace_hmi_state_availability_view_t pointer_test_view = {0};
    int result;

    lv_display_set_resolution(display, 640, 360);

    for (size_t index = 0; index < sizeof(valid_cases) / sizeof(valid_cases[0]); ++index) {
        state.availability = valid_cases[index].availability;
        result = create_verify_destroy_presentation(&state, &valid_cases[index]);
        if (result != 0) {
            return result;
        }
    }

    state = (furnace_hmi_state_availability_view_state_t) {0};
    result = create_verify_destroy_presentation(&state, unavailable);
    if (result != 0) {
        return result;
    }

    state.availability = (furnace_hmi_state_availability_t)-1;
    result = create_verify_destroy_presentation(&state, unavailable);
    if (result != 0) {
        return result;
    }

    state.availability = FURNACE_HMI_STATE_AVAILABILITY_COUNT;
    result = create_verify_destroy_presentation(&state, unavailable);
    if (result != 0) {
        return result;
    }

    result = create_verify_destroy_presentation(NULL, unavailable);
    if (result != 0) {
        return result;
    }

    if (furnace_hmi_state_availability_view_create(NULL, lv_screen_active(), &state) ||
        furnace_hmi_state_availability_view_create(&pointer_test_view, NULL, &state)) {
        (void)fprintf(stderr, "Availability view accepted a NULL required pointer.\n");
        return 15;
    }

    lv_obj_t *temporary_parent = lv_obj_create(lv_screen_active());
    if (temporary_parent == NULL ||
        !furnace_hmi_state_availability_view_create(
            &pointer_test_view,
            temporary_parent,
            &state
        )) {
        if (temporary_parent != NULL) {
            lv_obj_delete(temporary_parent);
        }
        (void)fprintf(stderr, "Availability parent-lifetime setup failed.\n");
        return 16;
    }
    lv_obj_delete(temporary_parent);
    if (pointer_test_view.root != NULL ||
        pointer_test_view.status_bar != NULL ||
        pointer_test_view.status_badge != NULL ||
        pointer_test_view.heading != NULL ||
        pointer_test_view.detail != NULL ||
        pointer_test_view.rendered_availability !=
            FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        (void)fprintf(stderr, "Parent deletion did not clear the availability handle.\n");
        return 17;
    }

    lv_obj_t *unrelated_object = lv_obj_create(lv_screen_active());
    if (unrelated_object == NULL) {
        (void)fprintf(stderr, "Availability reuse-safety setup failed.\n");
        return 18;
    }
    furnace_hmi_state_availability_view_destroy(&pointer_test_view);
    if (!lv_obj_is_valid(unrelated_object)) {
        (void)fprintf(stderr, "Availability destroy removed an unrelated object.\n");
        return 19;
    }
    lv_obj_delete(unrelated_object);

    return 0;
}

static bool create_desktop_development_marker(lv_obj_t *parent)
{
    lv_obj_t *footer = lv_obj_create(parent);
    lv_obj_t *footer_text;

    if (footer == NULL) {
        return false;
    }
    footer_text = lv_label_create(footer);
    if (footer_text == NULL) {
        lv_obj_delete(footer);
        return false;
    }

    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_height(footer, 42);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(footer, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_right(footer, 28, LV_PART_MAIN);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        footer,
        LV_FLEX_ALIGN_END,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(
        footer,
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE
    );

    lv_label_set_text_static(
        footer_text,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DESKTOP_DEVELOPMENT_LABEL)
    );
    lv_obj_set_style_text_color(
        footer_text,
        lv_color_hex(UINT32_C(0x657586)),
        LV_PART_MAIN
    );
    lv_obj_remove_flag(
        footer_text,
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE
    );

    return true;
}

int main(int argc, char **argv)
{
    const bool input_smoke_test = argc == 2 && strcmp(argv[1], "--smoke-test") == 0;
    const bool presentation_smoke_test =
        argc == 2 && strcmp(argv[1], "--presentation-smoke-test") == 0;
    input_diagnostic_view_t diagnostic_view = {0};
    furnace_hmi_state_availability_view_state_t initial_state = {0};
    furnace_hmi_state_availability_view_t availability_view = {0};

    if (argc > 2 || (argc == 2 && !input_smoke_test && !presentation_smoke_test)) {
        (void)fprintf(
            stderr,
            "Usage: furnace_hmi_desktop [--smoke-test|--presentation-smoke-test]\n"
        );
        return 64;
    }

    if (furnace_hmi_foundation_probe_version() != FURNACE_HMI_FOUNDATION_PROBE_VERSION) {
        (void)fprintf(stderr, "Portable foundation build probe mismatch.\n");
        return 1;
    }

    lv_init();
    lv_display_t *display = lv_sdl_window_create(
        FURNACE_HMI_DESKTOP_WIDTH,
        FURNACE_HMI_DESKTOP_HEIGHT
    );
    if(display == NULL) {
        (void)fprintf(stderr, "SDL display creation failed.\n");
        return 2;
    }
    lv_sdl_window_set_title(
        display,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE)
    );
    lv_sdl_window_set_resizeable(display, false);

    if (input_smoke_test) {
        create_input_diagnostic_view(&diagnostic_view);
        return run_input_smoke_test(display, &diagnostic_view);
    }
    if (presentation_smoke_test) {
        return run_presentation_smoke_test(display);
    }
    if (!furnace_hmi_state_availability_view_create(
            &availability_view,
            lv_screen_active(),
            &initial_state
        )) {
        (void)fprintf(stderr, "State-availability view creation failed.\n");
        return 13;
    }
    if (!create_desktop_development_marker(availability_view.root)) {
        furnace_hmi_state_availability_view_destroy(&availability_view);
        (void)fprintf(stderr, "Desktop development marker creation failed.\n");
        return 16;
    }
    (void)lv_sdl_mouse_create();

    for(;;) {
        handle_timers(UINT32_C(1));
    }
}
