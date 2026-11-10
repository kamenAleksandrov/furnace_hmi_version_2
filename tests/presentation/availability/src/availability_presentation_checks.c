/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <furnace_hmi/availability_presentation_checks.h>
#include <furnace_hmi/state_availability_view.h>
#include <furnace_hmi/ui_strings.h>

typedef struct {
    furnace_hmi_state_availability_t availability;
    furnace_hmi_ui_string_id_t badge;
    furnace_hmi_ui_string_id_t heading;
    furnace_hmi_ui_string_id_t detail;
} availability_test_case_t;

static int failed(const char **failure, const char *description, int code)
{
    *failure = description;
    return code;
}

static void service_timers(uint32_t iterations)
{
    for (uint32_t iteration = 0; iteration < iterations; ++iteration) {
        (void)lv_timer_handler();
    }
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
    const availability_test_case_t *expected,
    const char **failure
)
{
    lv_area_t screen_bounds;

    if (view->rendered_availability != expected->availability) {
        return failed(failure, "Availability presentation mapped to the wrong state.", 8);
    }
    if (!object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected->badge)
        ) ||
        !object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected->heading)
        ) ||
        !object_tree_contains_label_text(
            view->root,
            furnace_hmi_ui_string(expected->detail)
        )) {
        return failed(failure, "Availability-state copy was not rendered.", 9);
    }
    if (!object_tree_is_read_only_presentation(view->root)) {
        return failed(failure, "Availability view exposed an interactive object.", 10);
    }

    lv_obj_update_layout(lv_screen_active());
    lv_obj_get_coords(lv_screen_active(), &screen_bounds);
    if (!object_tree_fits_area(view->root, &screen_bounds)) {
        return failed(failure, "Availability view exceeded the supported viewport.", 11);
    }

    return 0;
}

static int create_verify_destroy_presentation(
    const furnace_hmi_state_availability_view_state_t *state,
    const availability_test_case_t *expected,
    const char **failure
)
{
    furnace_hmi_state_availability_view_t view = {0};
    int result;

    if (!furnace_hmi_state_availability_view_create(
            &view,
            lv_screen_active(),
            state
        )) {
        return failed(failure, "Availability-state view creation failed.", 12);
    }
    service_timers(UINT32_C(2));
    result = verify_availability_presentation(&view, expected, failure);

    if (furnace_hmi_state_availability_view_create(
            &view,
            lv_screen_active(),
            state
        )) {
        return failed(failure, "Active availability view was recreated without destroy.", 13);
    }
    furnace_hmi_state_availability_view_destroy(&view);
    if (view.root != NULL ||
        view.rendered_availability != FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return failed(failure, "Availability view destroy did not clear its handle.", 14);
    }

    return result;
}

int furnace_hmi_availability_presentation_check(lv_display_t *display, const char **failure)
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
        result = create_verify_destroy_presentation(&state, &valid_cases[index], failure);
        if (result != 0) {
            return result;
        }
    }

    state = (furnace_hmi_state_availability_view_state_t) {0};
    result = create_verify_destroy_presentation(&state, unavailable, failure);
    if (result != 0) {
        return result;
    }

    state.availability = (furnace_hmi_state_availability_t)-1;
    result = create_verify_destroy_presentation(&state, unavailable, failure);
    if (result != 0) {
        return result;
    }

    state.availability = FURNACE_HMI_STATE_AVAILABILITY_COUNT;
    result = create_verify_destroy_presentation(&state, unavailable, failure);
    if (result != 0) {
        return result;
    }

    result = create_verify_destroy_presentation(NULL, unavailable, failure);
    if (result != 0) {
        return result;
    }

    if (furnace_hmi_state_availability_view_create(NULL, lv_screen_active(), &state) ||
        furnace_hmi_state_availability_view_create(&pointer_test_view, NULL, &state)) {
        return failed(failure, "Availability view accepted a NULL required pointer.", 15);
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
        return failed(failure, "Availability parent-lifetime setup failed.", 16);
    }
    lv_obj_delete(temporary_parent);
    if (pointer_test_view.root != NULL ||
        pointer_test_view.status_bar != NULL ||
        pointer_test_view.status_badge != NULL ||
        pointer_test_view.heading != NULL ||
        pointer_test_view.detail != NULL ||
        pointer_test_view.rendered_availability !=
            FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return failed(failure, "Parent deletion did not clear the availability handle.", 17);
    }

    lv_obj_t *unrelated_object = lv_obj_create(lv_screen_active());
    if (unrelated_object == NULL) {
        return failed(failure, "Availability reuse-safety setup failed.", 18);
    }
    furnace_hmi_state_availability_view_destroy(&pointer_test_view);
    if (!lv_obj_is_valid(unrelated_object)) {
        return failed(failure, "Availability destroy removed an unrelated object.", 19);
    }
    lv_obj_delete(unrelated_object);

    return 0;
}
