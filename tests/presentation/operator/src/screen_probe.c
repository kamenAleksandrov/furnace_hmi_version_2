/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <string.h>

#include <furnace_hmi/screen_probe.h>

bool furnace_hmi_probe_tree_contains(const lv_obj_t *object, const char *text)
{
    if (lv_obj_check_type(object, &lv_label_class) && strstr(lv_label_get_text(object), text) != NULL) {
        return true;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        if (furnace_hmi_probe_tree_contains(lv_obj_get_child(object, (int32_t)index), text)) {
            return true;
        }
    }
    return false;
}

lv_obj_t *furnace_hmi_probe_first_button(lv_obj_t *object)
{
    if (object == NULL) {
        return NULL;
    }
    if (lv_obj_check_type(object, &lv_button_class)) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = furnace_hmi_probe_first_button(lv_obj_get_child(object, (int32_t)index));
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

lv_obj_t *furnace_hmi_probe_button_with_text(lv_obj_t *object, const char *text)
{
    if (object == NULL) {
        return NULL;
    }
    if (lv_obj_check_type(object, &lv_button_class) && furnace_hmi_probe_tree_contains(object, text)) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = furnace_hmi_probe_button_with_text(lv_obj_get_child(object, (int32_t)index), text);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

lv_obj_t *furnace_hmi_probe_label_with_text(lv_obj_t *object, const char *text)
{
    if (object == NULL) {
        return NULL;
    }
    if (lv_obj_check_type(object, &lv_label_class) && strcmp(lv_label_get_text(object), text) == 0) {
        return object;
    }
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *found = furnace_hmi_probe_label_with_text(lv_obj_get_child(object, (int32_t)index), text);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

lv_obj_t *furnace_hmi_probe_card_with_text(lv_obj_t *root, const char *text)
{
    lv_obj_t *object = furnace_hmi_probe_label_with_text(root, text);
    while (object != NULL && object != root && !lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE)) {
        object = lv_obj_get_parent(object);
    }
    return object == root ? NULL : object;
}

void furnace_hmi_probe_settle(void)
{
    for (int pass = 0; pass < 3; ++pass) {
        (void)lv_timer_handler();
    }
}

void furnace_hmi_probe_click(lv_obj_t *button)
{
    if (button != NULL) {
        lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
        furnace_hmi_probe_settle();
    }
}

bool furnace_hmi_probe_enabled(const lv_obj_t *button)
{
    return button != NULL && !lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN) &&
           !lv_obj_has_state(button, LV_STATE_DISABLED);
}
