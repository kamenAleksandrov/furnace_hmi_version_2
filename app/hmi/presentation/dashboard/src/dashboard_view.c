/* SPDX-License-Identifier: Apache-2.0 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/dashboard_view.h>
#include <furnace_hmi/ui_strings.h>

#define background_color lv_color_hex(0x0B1118)
#define panel_color lv_color_hex(0x17232E)
#define panel_highlight_color lv_color_hex(0x1E2E3C)
#define text_color lv_color_hex(0xEBF2F7)
#define muted_color lv_color_hex(0x9CADBB)
#define accent_color lv_color_hex(0x46C8BE)
#define planned_color lv_color_hex(0x7D8B97)
#define fault_color lv_color_hex(0xF0695A)

static const char *ui_string(furnace_hmi_ui_string_id_t id)
{
    const char *text = furnace_hmi_ui_string(id);
    return text == NULL ? "" : text;
}

static lv_obj_t *create_label(
    lv_obj_t *parent,
    const char *text,
    const lv_font_t *font,
    lv_color_t color
)
{
    lv_obj_t *label = lv_label_create(parent);
    if (label == NULL) {
        return NULL;
    }
    lv_label_set_text(label, text == NULL ? "" : text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    return label;
}

static lv_obj_t *create_panel(lv_obj_t *parent, lv_color_t color)
{
    lv_obj_t *panel = lv_obj_create(parent);
    if (panel == NULL) {
        return NULL;
    }
    lv_obj_set_style_bg_color(panel, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x344654), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 10, LV_PART_MAIN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    return panel;
}

static const char *run_state_text(
    furnace_hmi_machine_mode_t mode,
    furnace_hmi_run_state_t state
)
{
    if (mode == FURNACE_HMI_MACHINE_MODE_MANUAL &&
        state == FURNACE_HMI_RUN_STATE_RUNNING) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_MANUAL);
    }
    switch (state) {
    case FURNACE_HMI_RUN_STATE_RUNNING:
        return furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_RUNNING);
    case FURNACE_HMI_RUN_STATE_PAUSED:
        return furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PAUSED);
    case FURNACE_HMI_RUN_STATE_FAULT:
        return furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAULT);
    case FURNACE_HMI_RUN_STATE_IDLE:
        return furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_IDLE);
    case FURNACE_HMI_RUN_STATE_COMPLETE:
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_COMPLETE);
    case FURNACE_HMI_RUN_STATE_COUNT:
        return NULL;
    }
    return NULL;
}

static bool set_label_text(lv_obj_t *label, const char *text)
{
    if (label == NULL || text == NULL) {
        return false;
    }
    lv_label_set_text(label, text);
    return true;
}

static void format_duration(
    char *buffer,
    size_t capacity,
    const furnace_hmi_i32_value_t *value
)
{
    if (value == NULL || value->validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
        return;
    }
    if (value->validity == FURNACE_HMI_VALUE_VALIDITY_STALE || value->value < 0) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_STALE_VALUE));
        return;
    }
    (void)snprintf(
        buffer,
        capacity,
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DURATION_FORMAT),
        value->value / 3600,
        (value->value / 60) % 60
    );
}

static void format_value(
    char *buffer,
    size_t capacity,
    const furnace_hmi_i32_value_t *value,
    const char *unit
)
{
    if (value == NULL || value->validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
        return;
    }
    if (value->validity == FURNACE_HMI_VALUE_VALIDITY_STALE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_STALE_VALUE));
        return;
    }
    (void)snprintf(buffer, capacity, "%" PRId32 " %s", value->value, unit);
}

static void format_power(char *buffer, size_t capacity, const furnace_hmi_i32_value_t *value)
{
    if (value == NULL || value->validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
        return;
    }
    (void)snprintf(buffer, capacity, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_POWER_FORMAT),
                   value->value / 100, value->value % 100);
}

static void format_bool(char *buffer, size_t capacity, const furnace_hmi_bool_value_t *value,
                        const char *true_text, const char *false_text)
{
    if (value == NULL || value->validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
    }
    else if (value->validity == FURNACE_HMI_VALUE_VALIDITY_STALE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_STALE_VALUE));
    }
    else {
        (void)snprintf(buffer, capacity, "%s", value->value ? true_text : false_text);
    }
}

static bool update_chart(
    furnace_hmi_dashboard_view_t *view,
    const furnace_hmi_dashboard_state_t *state
)
{
    if (view == NULL || state == NULL || view->chart == NULL ||
        view->planned_series == NULL || view->measured_series == NULL ||
        state->graph_sample_count > FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS) {
        return false;
    }

    for (size_t index = 0; index < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
        if (index < state->graph_sample_count) {
            view->planned_values[index] = state->graph_samples[index].planned_temperature_c;
            view->measured_values[index] =
                state->graph_samples[index].measured_validity ==
                        FURNACE_HMI_VALUE_VALIDITY_CURRENT
                    ? state->graph_samples[index].measured_temperature_c
                    : LV_CHART_POINT_NONE;
        }
        else {
            view->planned_values[index] = LV_CHART_POINT_NONE;
            view->measured_values[index] = LV_CHART_POINT_NONE;
        }
    }

    lv_chart_set_series_ext_y_array(view->chart, view->planned_series, view->planned_values);
    lv_chart_set_series_ext_y_array(view->chart, view->measured_series, view->measured_values);
    lv_chart_refresh(view->chart);
    return true;
}

static void nav_button_callback(lv_event_t *event)
{
    furnace_hmi_dashboard_view_t *view = lv_event_get_user_data(event);
    if (view != NULL && view->navigation_feedback != NULL) {
        lv_label_set_text_static(
            view->navigation_feedback,
            ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_NAV_PREVIEW)
        );
    }
}

static bool create_nav_button(
    lv_obj_t *parent,
    const char *text,
    furnace_hmi_dashboard_view_t *view
)
{
    lv_obj_t *button = lv_button_create(parent);
    if (button == NULL) {
        return false;
    }
    lv_obj_set_height(button, 48);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_bg_color(button, panel_highlight_color, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    lv_obj_t *label = create_label(button, text, &lv_font_montserrat_16, text_color);
    if (label == NULL) {
        lv_obj_delete(button);
        return false;
    }
    lv_obj_center(label);
    lv_obj_add_event_cb(button, nav_button_callback, LV_EVENT_CLICKED, view);
    return true;
}

static bool create_furnace_visual(lv_obj_t *parent, furnace_hmi_dashboard_view_t *view)
{
    lv_obj_t *visual = create_panel(parent, lv_color_hex(0x0F181F));
    if (visual == NULL) {
        return false;
    }
    lv_obj_set_width(visual, lv_pct(100));
    lv_obj_set_flex_grow(visual, 1);
    lv_obj_set_flex_flow(visual, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(visual, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = create_label(
        visual,
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_VISUAL),
        &lv_font_montserrat_16,
        muted_color
    );
    lv_obj_t *furnace = lv_obj_create(visual);
    if (title == NULL || furnace == NULL) {
        return false;
    }
    lv_obj_set_size(furnace, lv_pct(70), 92);
    lv_obj_set_style_bg_color(furnace, lv_color_hex(0x46525B), LV_PART_MAIN);
    lv_obj_set_style_border_width(furnace, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(furnace, lv_color_hex(0x929DA4), LV_PART_MAIN);
    lv_obj_set_style_radius(furnace, 8, LV_PART_MAIN);
    lv_obj_remove_flag(furnace, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_obj_t *furnace_label = create_label(
        furnace,
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FURNACE_LABEL),
        &lv_font_montserrat_20,
        text_color
    );
    if (furnace_label == NULL) {
        return false;
    }
    lv_obj_center(furnace_label);

    view->visual_status = create_label(visual, "", &lv_font_montserrat_16, accent_color);
    if (view->visual_status == NULL) {
        return false;
    }
    return true;
}

static bool create_summary_row(
    lv_obj_t *parent,
    const char *label_text,
    lv_obj_t **value_out,
    furnace_hmi_dashboard_view_t *view
)
{
    lv_obj_t *row = lv_obj_create(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_obj_t *label = create_label(row, label_text, &lv_font_montserrat_16, muted_color);
    lv_obj_t *value = create_label(row, "--", &lv_font_montserrat_16, text_color);
    if (label == NULL || value == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(label, 1);
    *value_out = value;
    (void)view;
    return true;
}

bool furnace_hmi_dashboard_view_create(
    furnace_hmi_dashboard_view_t *view,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state
)
{
    if (view == NULL || parent == NULL || state == NULL || view->root != NULL ||
        !furnace_hmi_dashboard_state_is_well_formed(state) ||
        state->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT) {
        return false;
    }

    view->root = lv_obj_create(parent);
    if (view->root == NULL) {
        return false;
    }
    lv_obj_set_size(view->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(view->root, background_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(view->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(view->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(view->root, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(view->root, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(view->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(view->root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_obj_t *header = create_panel(view->root, panel_color);
    if (header == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_t *header_title = create_label(header, ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
                                          &lv_font_montserrat_20, text_color);
    view->session_label = create_label(header, "", &lv_font_montserrat_16, muted_color);
    view->run_badge = create_label(header, "", &lv_font_montserrat_16, accent_color);
    if (header_title == NULL || view->session_label == NULL || view->run_badge == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 48);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(header_title, 1);
    lv_obj_set_style_pad_all(header, 8, LV_PART_MAIN);

    lv_obj_t *content = lv_obj_create(view->root);
    if (content == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_t *summary = create_panel(content, panel_color);
    lv_obj_t *right = lv_obj_create(content);
    if (summary == NULL || right == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(content, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);
    lv_obj_set_width(summary, lv_pct(39));
    lv_obj_set_height(summary, lv_pct(100));
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(summary, 5, LV_PART_MAIN);
    lv_obj_set_width(right, lv_pct(61));
    lv_obj_set_height(right, lv_pct(100));
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(right, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(right, 8, LV_PART_MAIN);

    lv_obj_t *summary_title = create_label(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PROGRAM),
                                           &lv_font_montserrat_16, muted_color);
    view->program_value = create_label(summary, "", &lv_font_montserrat_20, text_color);
    view->stage_value = create_label(summary, "", &lv_font_montserrat_16, text_color);
    if (summary_title == NULL || view->program_value == NULL || view->stage_value == NULL ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TEMPERATURE),
                            &view->temperature_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET),
                            &view->target_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TIMING),
                            &view->timing_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DEMAND),
                            &view->demand_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_ESTIMATED_POWER),
                            &view->power_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAN),
                            &view->fan_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DOOR),
                            &view->door_value, view) ||
        !create_summary_row(summary, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SERVICE),
                            &view->service_value, view)) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_style_text_color(view->program_value, accent_color, LV_PART_MAIN);

    if (!create_furnace_visual(right, view)) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }

    lv_obj_t *chart_panel = create_panel(right, panel_color);
    if (chart_panel == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_t *chart_title = create_label(
        chart_panel,
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CHART_TITLE),
        &lv_font_montserrat_16,
        muted_color
    );
    view->chart = lv_chart_create(chart_panel);
    lv_obj_t *legend = lv_obj_create(chart_panel);
    if (chart_title == NULL || view->chart == NULL || legend == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(chart_panel, lv_pct(100));
    lv_obj_set_flex_grow(chart_panel, 1);
    lv_obj_set_style_pad_row(chart_panel, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(chart_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(view->chart, 1);
    lv_obj_set_width(view->chart, lv_pct(100));
    lv_chart_set_type(view->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(view->chart, FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS);
    lv_chart_set_axis_range(view->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
    lv_chart_set_div_line_count(view->chart, 4, 6);
    lv_obj_set_style_bg_color(view->chart, lv_color_hex(0x0F181F), LV_PART_MAIN);
    lv_obj_set_style_line_width(view->chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(view->chart, 0, 0, LV_PART_INDICATOR);
    view->planned_series = lv_chart_add_series(view->chart, planned_color, LV_CHART_AXIS_PRIMARY_Y);
    view->measured_series = lv_chart_add_series(view->chart, accent_color, LV_CHART_AXIS_PRIMARY_Y);
    if (view->planned_series == NULL || view->measured_series == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(legend, lv_pct(100));
    lv_obj_set_height(legend, 22);
    lv_obj_set_style_bg_opa(legend, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(legend, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(legend, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *planned_label = create_label(legend, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PLANNED),
                                           &lv_font_montserrat_16, planned_color);
    lv_obj_t *measured_label = create_label(legend, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_MEASURED),
                                            &lv_font_montserrat_16, accent_color);
    if (planned_label == NULL || measured_label == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_style_pad_column(legend, 16, LV_PART_MAIN);

    lv_obj_t *footer = lv_obj_create(view->root);
    if (footer == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    view->navigation_feedback = create_label(footer, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SIM_HINT),
                                             &lv_font_montserrat_14, muted_color);
    if (view->navigation_feedback == NULL) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_height(footer, 66);
    lv_obj_set_style_bg_opa(footer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(footer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(footer, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(footer, 4, LV_PART_MAIN);
    lv_obj_set_flex_grow(view->navigation_feedback, 1);
    lv_obj_set_width(view->navigation_feedback, lv_pct(100));

    lv_obj_t *nav = lv_obj_create(footer);
    if (nav == NULL ||
        !create_nav_button(nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_HOME), view) ||
        !create_nav_button(nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PROGRAMS), view) ||
        !create_nav_button(nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DEVICE), view) ||
        !create_nav_button(nav, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SETTINGS), view)) {
        furnace_hmi_dashboard_view_destroy(view);
        return false;
    }
    lv_obj_set_width(nav, lv_pct(100));
    lv_obj_set_height(nav, 48);
    lv_obj_set_style_bg_opa(nav, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(nav, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(nav, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(nav, 5, LV_PART_MAIN);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);

    return furnace_hmi_dashboard_view_update(view, state);
}

bool furnace_hmi_dashboard_view_update(
    furnace_hmi_dashboard_view_t *view,
    const furnace_hmi_dashboard_state_t *state
)
{
    char buffer[96];
    if (view == NULL || view->root == NULL || state == NULL ||
        !furnace_hmi_dashboard_state_is_well_formed(state) ||
        state->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT) {
        return false;
    }

    (void)snprintf(buffer, sizeof(buffer),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SESSION_FORMAT),
                   state->controller_session_epoch, state->state_revision);
    set_label_text(view->session_label, buffer);
    set_label_text(view->run_badge, run_state_text(state->mode, state->run_state));
    set_label_text(view->program_value,
                   state->program_name[0] == '\0'
                       ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_MANUAL_SESSION)
                       : state->program_name);
    if (state->stage_count == 0U) {
        (void)snprintf(buffer, sizeof(buffer), "%s", state->stage_name);
    }
    else {
        (void)snprintf(buffer, sizeof(buffer),
                       ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_STAGE_FORMAT),
                       state->stage_name,
                       (unsigned int)state->stage_index + 1U,
                       (unsigned int)state->stage_count);
    }
    set_label_text(view->stage_value, buffer);

    format_value(buffer, sizeof(buffer), &state->current_temperature_c,
                 ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNIT_C));
    set_label_text(view->temperature_value, buffer);
    format_value(buffer, sizeof(buffer), &state->target_temperature_c,
                 ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNIT_C));
    set_label_text(view->target_value, buffer);
    format_duration(buffer, sizeof(buffer), &state->remaining_seconds);
    set_label_text(view->timing_value, buffer);
    (void)snprintf(buffer, sizeof(buffer),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DEMAND_FORMAT),
                   state->heater_demand_percent.value);
    set_label_text(view->demand_value, buffer);
    format_power(buffer, sizeof(buffer), &state->estimated_power_kw_x100);
    set_label_text(view->power_value, buffer);
    (void)snprintf(buffer, sizeof(buffer),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT),
                   state->fan_rpm.value);
    set_label_text(view->fan_value, buffer);
    format_bool(
        buffer,
        sizeof(buffer),
        &state->door_closed,
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED),
        ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OPEN)
    );
    set_label_text(view->door_value, buffer);
    (void)snprintf(buffer, sizeof(buffer),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SERVICE_FORMAT),
                   state->service_time_hours.value);
    set_label_text(view->service_value, buffer);
    (void)snprintf(buffer, sizeof(buffer),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_VISUAL_STATUS_FORMAT),
                   run_state_text(state->mode, state->run_state),
                   state->fan_running.value
                       ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_ON)
                       : ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OFF),
                   state->door_closed.value
                       ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED)
                       : ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OPEN));
    set_label_text(view->visual_status, buffer);
    if (state->run_state == FURNACE_HMI_RUN_STATE_FAULT) {
        lv_obj_set_style_text_color(view->run_badge, fault_color, LV_PART_MAIN);
        lv_obj_set_style_text_color(view->visual_status, fault_color, LV_PART_MAIN);
    }
    else {
        lv_obj_set_style_text_color(view->run_badge, accent_color, LV_PART_MAIN);
        lv_obj_set_style_text_color(view->visual_status, accent_color, LV_PART_MAIN);
    }
    return update_chart(view, state);
}

void furnace_hmi_dashboard_view_destroy(furnace_hmi_dashboard_view_t *view)
{
    if (view == NULL) {
        return;
    }
    if (view->root != NULL && lv_obj_is_valid(view->root)) {
        lv_obj_delete(view->root);
    }
    memset(view, 0, sizeof(*view));
}
