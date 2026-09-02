/* SPDX-License-Identifier: Apache-2.0 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <furnace_hmi/state_availability_view.h>
#include <furnace_hmi/ui_strings.h>

typedef struct {
    furnace_hmi_ui_string_id_t badge;
    furnace_hmi_ui_string_id_t heading;
    furnace_hmi_ui_string_id_t detail;
    uint32_t accent_color;
} availability_presentation_t;

static furnace_hmi_state_availability_t normalize_availability(
    const furnace_hmi_state_availability_view_state_t *state
)
{
    if (state == NULL) {
        return FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    }

    switch (state->availability) {
    case FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE:
    case FURNACE_HMI_STATE_AVAILABILITY_STALE:
    case FURNACE_HMI_STATE_AVAILABILITY_CURRENT:
        return state->availability;
    default:
        return FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    }
}

static availability_presentation_t presentation_for(
    furnace_hmi_state_availability_t availability
)
{
    switch (availability) {
    case FURNACE_HMI_STATE_AVAILABILITY_CURRENT:
        return (availability_presentation_t) {
            .badge = FURNACE_HMI_UI_STRING_STATE_CURRENT_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_CURRENT_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_CURRENT_DETAIL,
            .accent_color = UINT32_C(0x35C88A),
        };
    case FURNACE_HMI_STATE_AVAILABILITY_STALE:
        return (availability_presentation_t) {
            .badge = FURNACE_HMI_UI_STRING_STATE_STALE_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_STALE_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_STALE_DETAIL,
            .accent_color = UINT32_C(0xF5A524),
        };
    case FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE:
    default:
        return (availability_presentation_t) {
            .badge = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_BADGE,
            .heading = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_HEADING,
            .detail = FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_DETAIL,
            .accent_color = UINT32_C(0xFF5C6C),
        };
    }
}

static void make_container_transparent(lv_obj_t *object)
{
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(object, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(object, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(object, 0, LV_PART_MAIN);
}

static void make_tree_non_interactive(lv_obj_t *object)
{
    const lv_obj_flag_t interaction_flags =
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_SCROLLABLE;
    const uint32_t child_count = lv_obj_get_child_count(object);

    lv_obj_remove_flag(object, interaction_flags);
    for (uint32_t index = 0; index < child_count; ++index) {
        make_tree_non_interactive(lv_obj_get_child(object, (int32_t)index));
    }
}

static void clear_view_on_root_delete(lv_event_t *event)
{
    furnace_hmi_state_availability_view_t *view = lv_event_get_user_data(event);

    if (view != NULL) {
        (void)memset(view, 0, sizeof(*view));
    }
}

void furnace_hmi_state_availability_view_destroy(
    furnace_hmi_state_availability_view_t *view
)
{
    if (view == NULL) {
        return;
    }

    if (view->root != NULL) {
        lv_obj_delete(view->root);
    }
    (void)memset(view, 0, sizeof(*view));
}

bool furnace_hmi_state_availability_view_create(
    furnace_hmi_state_availability_view_t *view,
    lv_obj_t *parent,
    const furnace_hmi_state_availability_view_state_t *state
)
{
    const furnace_hmi_state_availability_t availability = normalize_availability(state);
    const availability_presentation_t presentation = presentation_for(availability);
    lv_obj_t *header;
    lv_obj_t *identity;
    lv_obj_t *application_title;
    lv_obj_t *application_role;
    lv_obj_t *content;
    lv_obj_t *card;

    if (view == NULL || parent == NULL) {
        return false;
    }
    if (view->root != NULL) {
        return false;
    }

    view->rendered_availability = availability;

    view->root = lv_obj_create(parent);
    if (view->root == NULL) {
        goto creation_failed;
    }
    (void)lv_obj_add_event_cb(
        view->root,
        clear_view_on_root_delete,
        LV_EVENT_DELETE,
        view
    );
    lv_obj_set_size(view->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(view->root, lv_color_hex(UINT32_C(0x0B1118)), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(view->root, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(view->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(view->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(view->root, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(view->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(view->root, LV_OBJ_FLAG_SCROLLABLE);

    header = lv_obj_create(view->root);
    if (header == NULL) {
        goto creation_failed;
    }
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 72);
    lv_obj_set_style_bg_color(header, lv_color_hex(UINT32_C(0x121C26)), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(header, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_right(header, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_top(header, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(header, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        header,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    identity = lv_obj_create(header);
    if (identity == NULL) {
        goto creation_failed;
    }
    make_container_transparent(identity);
    lv_obj_set_height(identity, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(identity, 1);
    lv_obj_set_flex_flow(identity, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(identity, 2, LV_PART_MAIN);

    application_title = lv_label_create(identity);
    if (application_title == NULL) {
        goto creation_failed;
    }
    lv_label_set_text_static(
        application_title,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE)
    );
    lv_obj_set_style_text_color(
        application_title,
        lv_color_hex(UINT32_C(0xF4F7FA)),
        LV_PART_MAIN
    );
    lv_obj_set_style_text_font(application_title, &lv_font_montserrat_24, LV_PART_MAIN);

    application_role = lv_label_create(identity);
    if (application_role == NULL) {
        goto creation_failed;
    }
    lv_label_set_text_static(
        application_role,
        furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_REMOTE_INTERFACE_LABEL)
    );
    lv_obj_set_style_text_color(
        application_role,
        lv_color_hex(UINT32_C(0x91A0AE)),
        LV_PART_MAIN
    );

    view->status_badge = lv_label_create(header);
    if (view->status_badge == NULL) {
        goto creation_failed;
    }
    lv_label_set_text_static(
        view->status_badge,
        furnace_hmi_ui_string(presentation.badge)
    );
    lv_obj_set_style_bg_color(
        view->status_badge,
        lv_color_hex(presentation.accent_color),
        LV_PART_MAIN
    );
    lv_obj_set_style_bg_opa(view->status_badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(
        view->status_badge,
        lv_color_hex(UINT32_C(0x0B1118)),
        LV_PART_MAIN
    );
    lv_obj_set_style_text_letter_space(view->status_badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(view->status_badge, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_left(view->status_badge, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(view->status_badge, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(view->status_badge, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(view->status_badge, 7, LV_PART_MAIN);

    content = lv_obj_create(view->root);
    if (content == NULL) {
        goto creation_failed;
    }
    make_container_transparent(content);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_pad_left(content, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_right(content, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_top(content, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(content, 24, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(
        content,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    card = lv_obj_create(content);
    if (card == NULL) {
        goto creation_failed;
    }
    lv_obj_set_width(card, lv_pct(88));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(UINT32_C(0x17232E)), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(
        card,
        lv_color_hex(UINT32_C(0x2A3A48)),
        LV_PART_MAIN
    );
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 28, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 16, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    view->status_bar = lv_obj_create(card);
    if (view->status_bar == NULL) {
        goto creation_failed;
    }
    lv_obj_set_width(view->status_bar, 72);
    lv_obj_set_height(view->status_bar, 5);
    lv_obj_set_style_bg_color(
        view->status_bar,
        lv_color_hex(presentation.accent_color),
        LV_PART_MAIN
    );
    lv_obj_set_style_bg_opa(view->status_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(view->status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(view->status_bar, 3, LV_PART_MAIN);

    view->heading = lv_label_create(card);
    if (view->heading == NULL) {
        goto creation_failed;
    }
    lv_label_set_text_static(
        view->heading,
        furnace_hmi_ui_string(presentation.heading)
    );
    lv_obj_set_width(view->heading, lv_pct(100));
    lv_label_set_long_mode(view->heading, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_color(
        view->heading,
        lv_color_hex(UINT32_C(0xF4F7FA)),
        LV_PART_MAIN
    );
    lv_obj_set_style_text_font(view->heading, &lv_font_montserrat_24, LV_PART_MAIN);

    view->detail = lv_label_create(card);
    if (view->detail == NULL) {
        goto creation_failed;
    }
    lv_label_set_text_static(
        view->detail,
        furnace_hmi_ui_string(presentation.detail)
    );
    lv_obj_set_width(view->detail, lv_pct(100));
    lv_label_set_long_mode(view->detail, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_color(
        view->detail,
        lv_color_hex(UINT32_C(0xAAB6C2)),
        LV_PART_MAIN
    );

    make_tree_non_interactive(view->root);
    return true;

creation_failed:
    furnace_hmi_state_availability_view_destroy(view);
    return false;
}
