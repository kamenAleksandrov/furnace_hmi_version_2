/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>

#include <furnace_hmi/development_marker.h>
#include <furnace_hmi/hmi_fonts.h>

bool furnace_hmi_development_marker_create(lv_obj_t *parent, furnace_hmi_ui_string_id_t text)
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

    lv_label_set_text_static(footer_text, furnace_hmi_ui_string(text));
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

lv_obj_t *furnace_hmi_development_tag_create(furnace_hmi_ui_string_id_t text)
{
    lv_obj_t *tag = lv_label_create(lv_layer_top());

    if (tag == NULL) {
        return NULL;
    }
    lv_label_set_text_static(tag, furnace_hmi_ui_string(text));
    lv_obj_set_style_text_font(tag, &furnace_hmi_font_regular_14, LV_PART_MAIN);
    /* The UI lab's simulation tag: amber text on a dark tag over the top line. */
    lv_obj_set_style_text_color(tag, lv_color_hex(UINT32_C(0xE8A04B)), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tag, lv_color_hex(UINT32_C(0x171B1E)), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tag, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(tag, 6, LV_PART_MAIN);
    lv_obj_align(tag, LV_ALIGN_TOP_RIGHT, -10, -4);
    lv_obj_remove_flag(tag, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    return tag;
}
