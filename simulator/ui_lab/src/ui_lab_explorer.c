/* SPDX-License-Identifier: Apache-2.0 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "ui_lab_explorer.h"

#include <furnace_hmi/ui_strings.h>

#define UI_LAB_BACKGROUND lv_color_hex(0x111315)
#define UI_LAB_SURFACE lv_color_hex(0x1C2023)
#define UI_LAB_SURFACE_RAISED lv_color_hex(0x272D31)
#define UI_LAB_BORDER lv_color_hex(0x3A4348)
#define UI_LAB_TEXT lv_color_hex(0xF0EDE5)
#define UI_LAB_MUTED lv_color_hex(0xA6AAA8)
#define UI_LAB_DEEP_BLUE lv_color_hex(0x35627B)
#define UI_LAB_NAV_GLOW lv_color_hex(0x4B8199)
#define UI_LAB_PLANNED lv_color_hex(0x778087)
#define UI_LAB_RUNNING lv_color_hex(0xD68031)
#define UI_LAB_PAUSED lv_color_hex(0x9D6BC4)
#define UI_LAB_MANUAL lv_color_hex(0x2E9689)
#define UI_LAB_IDLE lv_color_hex(0x60737E)
#define UI_LAB_FAULT lv_color_hex(0xD9534F)
#define UI_LAB_STALE lv_color_hex(0xB58A3D)
#define UI_LAB_UNAVAILABLE lv_color_hex(0x73787B)
#define UI_LAB_GRAPH_LEFT 46
#define UI_LAB_GRAPH_RIGHT 8
#define UI_LAB_GRAPH_TOP 20
#define UI_LAB_GRAPH_BOTTOM 30

typedef enum {
    UI_LAB_ACTION_HOME = 0,
    UI_LAB_ACTION_PROGRAMS,
    UI_LAB_ACTION_PROGRAM_LOADING,
    UI_LAB_ACTION_RUNNING,
    UI_LAB_ACTION_PROGRAM_DETAIL,
    UI_LAB_ACTION_PROGRAM_STAGES,
    UI_LAB_ACTION_STAGE_DETAIL,
    UI_LAB_ACTION_STAGE_EDITOR,
    UI_LAB_ACTION_DEVICE,
    UI_LAB_ACTION_SETTINGS,
    UI_LAB_ACTION_BACK_PROGRAMS,
    UI_LAB_ACTION_BACK_PROGRAM_DETAIL,
    UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE,
    UI_LAB_ACTION_NEXT_PROGRAM_PAGE,
    UI_LAB_ACTION_PREVIOUS_STAGE_PAGE,
    UI_LAB_ACTION_NEXT_STAGE_PAGE,
    UI_LAB_ACTION_PREVIOUS_STAGE,
    UI_LAB_ACTION_NEXT_STAGE,
    UI_LAB_ACTION_SHOW_NOTICES,
    UI_LAB_ACTION_SHOW_SERVICE,
    UI_LAB_ACTION_SHOW_DELETE,
    UI_LAB_ACTION_SHOW_SAVE,
    UI_LAB_ACTION_SHOW_DISCARD,
    UI_LAB_ACTION_SHOW_RUN_DETAILS,
    UI_LAB_ACTION_ADD_STAGE,
    UI_LAB_ACTION_NEW_DRAFT,
    UI_LAB_ACTION_PAUSE_PREVIEW,
    UI_LAB_ACTION_RESUME_PREVIEW,
    UI_LAB_ACTION_STOP_PREVIEW,
    UI_LAB_ACTION_DECREMENT_TARGET,
    UI_LAB_ACTION_INCREMENT_TARGET,
    UI_LAB_ACTION_DECREMENT_RATE,
    UI_LAB_ACTION_INCREMENT_RATE,
    UI_LAB_ACTION_DECREMENT_DURATION,
    UI_LAB_ACTION_INCREMENT_DURATION,
    UI_LAB_ACTION_SET_DRAFT_HEATING,
    UI_LAB_ACTION_SET_DRAFT_HOLD,
    UI_LAB_ACTION_SET_DRAFT_COOL,
    UI_LAB_ACTION_SET_COOL_FASTEST,
    UI_LAB_ACTION_SET_COOL_CONTROLLED,
    UI_LAB_ACTION_TOGGLE_FAVOURITE,
    UI_LAB_ACTION_SHOW_GRAPH_VIEW,
    UI_LAB_ACTION_SHOW_SPLIT_VIEW,
    UI_LAB_ACTION_SHOW_VISUAL_VIEW,
    UI_LAB_ACTION_CLOSE_MODAL,
    UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
} ui_lab_action_t;

typedef struct {
    furnace_hmi_ui_string_id_t name;
    furnace_hmi_ui_string_id_t material;
    furnace_hmi_ui_string_id_t duration;
    furnace_hmi_ui_string_id_t maximum_temperature;
    furnace_hmi_ui_string_id_t estimated_energy;
    uint8_t stage_count;
    uint16_t duration_minutes;
    int32_t maximum_temperature_c;
} ui_lab_program_fixture_t;

typedef struct {
    furnace_hmi_ui_string_id_t name;
    furnace_hmi_ui_string_id_t kind;
    int32_t target_c;
    int32_t delta_c_per_minute;
    int32_t duration_minutes;
} ui_lab_stage_fixture_t;

static const ui_lab_program_fixture_t program_fixtures[] = {
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_CERAMIC_TEST,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_STONEWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_1,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_1,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_1, 5U, 64U, 850 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_BISQUE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_EARTHENWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_2, 6U, 340U, 980 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_COOLING_TEST,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 4U, 135U, 620 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_GLAZE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLAZED_CERAMIC,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 7U, 290U, 1220 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ANNEAL,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLASS,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_5, 4U, 210U, 560 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_QUICK_BISQUE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_EARTHENWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_1,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_1, 5U, 180U, 900 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_HIGH_FIRE_GLAZE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLAZED_CERAMIC,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 6U, 360U, 1250 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_KILN_DRY,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 3U, 90U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_STONEWARE_TEST,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_STONEWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 5U, 220U, 1180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_GLASS_ANNEAL,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLASS,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_5, 4U, 300U, 540 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_LONG_COOLING,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 3U, 180U, 450 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_PRODUCTION,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_STONEWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 8U, 420U, 1200 },
};

static const ui_lab_stage_fixture_t stage_fixtures[] = {
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_WARM_UP,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 180, 25, 8 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_DRYING_HOLD,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD, 180, 0, 12 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING_850,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 850, 35, 24 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_PEAK_HOLD,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD, 850, 0, 15 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CONTROLLED_COOLING,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COOLING, 250, -20, 30 },
};

_Static_assert(
    sizeof(program_fixtures) / sizeof(program_fixtures[0]) == FURNACE_HMI_UI_LAB_MAX_PROGRAMS,
    "UI-lab quick-launch storage must cover every program fixture"
);

typedef enum {
    UI_LAB_DRAFT_STAGE_HEATING = 0,
    UI_LAB_DRAFT_STAGE_HOLD,
    UI_LAB_DRAFT_STAGE_COOL,
} ui_lab_draft_stage_kind_t;

static ui_lab_draft_stage_kind_t fixture_stage_kind(const ui_lab_stage_fixture_t *stage)
{
    if (stage == NULL || stage->kind == FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING) {
        return UI_LAB_DRAFT_STAGE_HEATING;
    }
    if (stage->kind == FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) {
        return UI_LAB_DRAFT_STAGE_HOLD;
    }
    return UI_LAB_DRAFT_STAGE_COOL;
}

static lv_color_t stage_kind_color(ui_lab_draft_stage_kind_t kind)
{
    switch (kind) {
    case UI_LAB_DRAFT_STAGE_HEATING:
        return UI_LAB_RUNNING;
    case UI_LAB_DRAFT_STAGE_HOLD:
        return UI_LAB_PAUSED;
    case UI_LAB_DRAFT_STAGE_COOL:
        return UI_LAB_MANUAL;
    }
    return UI_LAB_MUTED;
}

static size_t program_fixture_count(void)
{
    return sizeof(program_fixtures) / sizeof(program_fixtures[0]);
}

static bool program_is_favourite(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t program_index
)
{
    if (explorer == NULL) {
        return false;
    }
    for (size_t index = 0; index < explorer->favourite_count; ++index) {
        if (explorer->quick_launch_favourites[index] == program_index) {
            return true;
        }
    }
    return false;
}

static void record_recent_program(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t program_index)
{
    if (explorer == NULL || program_index >= program_fixture_count()) {
        return;
    }
    uint8_t values[FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS];
    size_t count = 0U;
    values[count++] = program_index;
    for (size_t index = 0; index < explorer->recent_count &&
                           count < FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS; ++index) {
        if (explorer->quick_launch_recent[index] != program_index) {
            values[count++] = explorer->quick_launch_recent[index];
        }
    }
    memcpy(explorer->quick_launch_recent, values, sizeof(values));
    explorer->recent_count = (uint8_t)count;
}

static bool toggle_program_favourite(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t program_index
)
{
    if (explorer == NULL || program_index >= program_fixture_count()) {
        return false;
    }
    for (size_t index = 0; index < explorer->favourite_count; ++index) {
        if (explorer->quick_launch_favourites[index] == program_index) {
            for (size_t move = index + 1U; move < explorer->favourite_count; ++move) {
                explorer->quick_launch_favourites[move - 1U] = explorer->quick_launch_favourites[move];
            }
            explorer->favourite_count--;
            return true;
        }
    }
    if (explorer->favourite_count >= FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS) {
        return false;
    }
    explorer->quick_launch_favourites[explorer->favourite_count++] = program_index;
    return true;
}

static void update_heating_relationship(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->draft_stage_kind != UI_LAB_DRAFT_STAGE_HEATING) {
        return;
    }
    int32_t rate = explorer->draft_rate_tenths_c_per_minute;
    if (rate <= 0) {
        rate = 1;
    }
    const int32_t start_temperature = 25;
    const int32_t change = explorer->draft_target_c > start_temperature
        ? explorer->draft_target_c - start_temperature : start_temperature - explorer->draft_target_c;
    if (explorer->draft_heating_duration_drives_rate) {
        const int32_t duration = explorer->draft_duration_minutes > 0
            ? explorer->draft_duration_minutes : 1;
        explorer->draft_rate_tenths_c_per_minute = (change * 10 + duration - 1) / duration;
    }
    else {
        explorer->draft_duration_minutes = (change * 10 + rate - 1) / rate;
    }
}

static void prepare_program_preview(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const ui_lab_program_fixture_t *program
)
{
    if (explorer == NULL || program == NULL) {
        return;
    }
    furnace_hmi_dashboard_state_initialize(&explorer->preview_graph_state);
    explorer->preview_graph_state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    const size_t stage_count = sizeof(stage_fixtures) / sizeof(stage_fixtures[0]);
    uint32_t fixture_minutes = 0U;
    for (size_t index = 0; index < stage_count; ++index) {
        fixture_minutes += (uint32_t)stage_fixtures[index].duration_minutes;
    }
    explorer->preview_graph_state.graph_sample_count = stage_count + 1U;
    explorer->preview_graph_state.graph_samples[0] = (furnace_hmi_graph_sample_t) {
        .elapsed_seconds = 0U,
        .planned_temperature_c = 25,
        .measured_temperature_c = 0,
        .measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
    };
    uint32_t accumulated_minutes = 0U;
    for (size_t index = 0; index < stage_count; ++index) {
        accumulated_minutes += (uint32_t)stage_fixtures[index].duration_minutes;
        explorer->preview_graph_state.graph_samples[index + 1U] = (furnace_hmi_graph_sample_t) {
            .elapsed_seconds = (uint32_t)program->duration_minutes * 60U * accumulated_minutes /
                fixture_minutes,
            .planned_temperature_c = stage_fixtures[index].target_c,
            .measured_temperature_c = 0,
            .measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
        };
    }
    explorer->preview_graph_state.graph_samples[stage_count].elapsed_seconds =
        (uint32_t)program->duration_minutes * 60U;
    explorer->trajectory_uses_program_preview = true;
    explorer->trajectory_domain_initialized = false;
}

static const char *ui_string(furnace_hmi_ui_string_id_t id)
{
    const char *text = furnace_hmi_ui_string(id);
    return text == NULL ? "" : text;
}

static void format_rate_tenths(char *buffer, size_t capacity, int32_t rate_tenths)
{
    const int32_t magnitude = rate_tenths < 0 ? -rate_tenths : rate_tenths;
    (void)snprintf(buffer, capacity,
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_RATE_TENTHS),
                   rate_tenths < 0 ? "-" : "+", (long)(magnitude / 10),
                   (long)(magnitude % 10));
}

static void format_stage_card_summary(
    char *buffer,
    size_t capacity,
    const ui_lab_stage_fixture_t *stage
)
{
    char rate[24];
    if (stage == NULL) {
        (void)snprintf(buffer, capacity, "%s", "");
        return;
    }
    format_rate_tenths(rate, sizeof(rate), stage->delta_c_per_minute * 10);
    (void)snprintf(buffer, capacity,
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CARD_SUMMARY_FORMAT),
                   (long)stage->target_c, rate, (long)stage->duration_minutes);
}

static void remove_scroll(lv_obj_t *object)
{
    if (object == NULL) {
        return;
    }
    lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(object, LV_SCROLLBAR_MODE_OFF);
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

static lv_obj_t *create_panel(lv_obj_t *parent, lv_color_t background)
{
    lv_obj_t *panel = lv_obj_create(parent);
    if (panel == NULL) {
        return NULL;
    }
    lv_obj_set_style_bg_color(panel, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, UI_LAB_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 8, LV_PART_MAIN);
    remove_scroll(panel);
    return panel;
}

static lv_obj_t *create_transparent_container(lv_obj_t *parent)
{
    lv_obj_t *container = lv_obj_create(parent);
    if (container == NULL) {
        return NULL;
    }
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    remove_scroll(container);
    return container;
}

static bool state_is_current(const furnace_hmi_dashboard_state_t *state)
{
    return state != NULL && state->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
}

static lv_color_t state_color(const furnace_hmi_dashboard_state_t *state)
{
    if (state == NULL || state->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return UI_LAB_UNAVAILABLE;
    }
    if (state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE) {
        return UI_LAB_STALE;
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_FAULT) {
        return UI_LAB_FAULT;
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_PAUSED) {
        return UI_LAB_PAUSED;
    }
    if (state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL &&
        state->run_state == FURNACE_HMI_RUN_STATE_RUNNING) {
        return UI_LAB_MANUAL;
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_RUNNING) {
        return UI_LAB_RUNNING;
    }
    return UI_LAB_IDLE;
}

static const char *state_text(const furnace_hmi_dashboard_state_t *state)
{
    if (state == NULL || state->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return ui_string(FURNACE_HMI_UI_STRING_STATE_UNAVAILABLE_BADGE);
    }
    if (state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE) {
        return ui_string(FURNACE_HMI_UI_STRING_STATE_STALE_BADGE);
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_FAULT) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAULT);
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_PAUSED) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PAUSED);
    }
    if (state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL &&
        state->run_state == FURNACE_HMI_RUN_STATE_RUNNING) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_MANUAL);
    }
    if (state->run_state == FURNACE_HMI_RUN_STATE_RUNNING) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_RUNNING);
    }
    return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_IDLE);
}

static const char *page_title(furnace_hmi_ui_lab_page_t page)
{
    switch (page) {
    case FURNACE_HMI_UI_LAB_PAGE_HOME:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HOME_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_LOADING_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_RUNNING:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUNNING_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAMS:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_LIBRARY_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DETAIL_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGES_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_DETAIL_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_EDITOR_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_DEVICE:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_SETTINGS:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_TITLE);
    case FURNACE_HMI_UI_LAB_PAGE_COUNT:
        break;
    }
    return "";
}

static void format_observation_value(
    char *buffer,
    size_t capacity,
    const furnace_hmi_i32_value_t *value,
    furnace_hmi_ui_string_id_t format_id
)
{
    if (value == NULL || value->validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
    }
    else if (value->validity == FURNACE_HMI_VALUE_VALIDITY_STALE) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_STALE_VALUE));
    }
    else {
        (void)snprintf(buffer, capacity, ui_string(format_id), (long)value->value);
    }
}

static void format_remaining(
    char *buffer,
    size_t capacity,
    const furnace_hmi_i32_value_t *value
)
{
    if (value == NULL || value->validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT ||
        value->value < 0) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
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

static void action_callback(lv_event_t *event);
static bool rebuild_page(furnace_hmi_ui_lab_explorer_t *explorer);

/* This is deliberately cosmetic. It executes only on the LVGL owner context,
 * does not delay navigation, and is never used to animate controller state. */
static void set_nav_glow_width(void *target, int32_t width)
{
    lv_obj_set_style_shadow_width((lv_obj_t *)target, width, LV_PART_MAIN);
}

static uint8_t nav_destination_for_page(furnace_hmi_ui_lab_page_t page)
{
    switch (page) {
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAMS:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES:
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL:
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR:
        return 1U;
    case FURNACE_HMI_UI_LAB_PAGE_DEVICE:
        return 2U;
    case FURNACE_HMI_UI_LAB_PAGE_SETTINGS:
        return 3U;
    case FURNACE_HMI_UI_LAB_PAGE_HOME:
    case FURNACE_HMI_UI_LAB_PAGE_RUNNING:
    case FURNACE_HMI_UI_LAB_PAGE_COUNT:
        return 0U;
    }
    return 0U;
}

static void configure_nav_button(lv_obj_t *button, bool active, bool animate)
{
    if (button == NULL) {
        return;
    }
    const int32_t final_glow_width = active ? 10 : 0;
    lv_obj_set_style_bg_color(button, active ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE_RAISED,
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(button, active ? 7 : 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(button, UI_LAB_NAV_GLOW, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(button, active ? LV_OPA_70 : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_x(button, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(button, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_spread(button, 0, LV_PART_MAIN);
    const lv_style_selector_t pressed_main =
        (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED;
    lv_obj_set_style_bg_color(button, UI_LAB_NAV_GLOW, pressed_main);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, pressed_main);
    lv_obj_set_style_shadow_opa(button, LV_OPA_90, pressed_main);
    lv_anim_delete(button, set_nav_glow_width);
    if (!animate) {
        set_nav_glow_width(button, final_glow_width);
        return;
    }
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, button);
    lv_anim_set_exec_cb(&animation, set_nav_glow_width);
    lv_anim_set_values(&animation, active ? 0 : 10, final_glow_width);
    lv_anim_set_duration(&animation, 140);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_start(&animation);
}

static void update_nav_selection(furnace_hmi_ui_lab_explorer_t *explorer, bool animate)
{
    if (explorer == NULL || explorer->nav == NULL) {
        return;
    }
    const uint8_t selected = nav_destination_for_page(explorer->page);
    for (uint8_t index = 0U; index < 4U; ++index) {
        configure_nav_button(lv_obj_get_child(explorer->nav, index), index == selected, animate);
    }
}

static void configure_button(lv_obj_t *button, lv_color_t background)
{
    lv_obj_set_style_bg_color(button, background, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, 6, LV_PART_MAIN);
    remove_scroll(button);
}

static furnace_hmi_ui_lab_explorer_t *action_owner(lv_event_t *event)
{
    const furnace_hmi_ui_lab_action_slot_t *slot = lv_event_get_user_data(event);
    return slot == NULL ? NULL : slot->explorer;
}

static lv_obj_t *create_action_button(
    lv_obj_t *parent,
    const char *text,
    lv_color_t background,
    furnace_hmi_ui_lab_action_slot_t *slot,
    furnace_hmi_ui_lab_explorer_t *explorer,
    ui_lab_action_t action,
    int value
)
{
    if (parent == NULL || slot == NULL || explorer == NULL) {
        return NULL;
    }
    lv_obj_t *button = lv_button_create(parent);
    if (button == NULL) {
        return NULL;
    }
    slot->explorer = explorer;
    slot->action = (int)action;
    slot->value = value;
    configure_button(button, background);
    lv_obj_t *label = create_label(button, text, &lv_font_montserrat_14, UI_LAB_TEXT);
    if (label == NULL) {
        lv_obj_delete(button);
        return NULL;
    }
    lv_obj_center(label);
    lv_obj_add_event_cb(button, action_callback, LV_EVENT_CLICKED, slot);
    return button;
}

static lv_obj_t *create_page_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    lv_color_t background,
    ui_lab_action_t action,
    int value
)
{
    if (explorer == NULL || explorer->page_action_count >=
        sizeof(explorer->page_actions) / sizeof(explorer->page_actions[0])) {
        return NULL;
    }
    return create_action_button(
        parent,
        text,
        background,
        &explorer->page_actions[explorer->page_action_count++],
        explorer,
        action,
        value
    );
}

static lv_obj_t *create_modal_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    lv_color_t background,
    ui_lab_action_t action,
    int value
)
{
    if (explorer == NULL || explorer->modal_action_count >=
        sizeof(explorer->modal_actions) / sizeof(explorer->modal_actions[0])) {
        return NULL;
    }
    return create_action_button(
        parent,
        text,
        background,
        &explorer->modal_actions[explorer->modal_action_count++],
        explorer,
        action,
        value
    );
}

static bool make_metric(
    lv_obj_t *parent,
    const char *label_text,
    const char *value_text,
    lv_obj_t **value_out
)
{
    lv_obj_t *metric = create_panel(parent, UI_LAB_SURFACE_RAISED);
    if (metric == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(metric, 1);
    lv_obj_set_height(metric, 64);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metric, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *label = create_label(metric, label_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *value = create_label(metric, value_text, &lv_font_montserrat_20, UI_LAB_TEXT);
    if (label == NULL || value == NULL) {
        return false;
    }
    if (value_out != NULL) {
        *value_out = value;
    }
    return true;
}

static bool make_detail_fact(
    lv_obj_t *parent,
    const char *heading,
    const char *value,
    int32_t width_percent
)
{
    lv_obj_t *fact = create_transparent_container(parent);
    if (fact == NULL) {
        return false;
    }
    lv_obj_set_width(fact, lv_pct(width_percent));
    lv_obj_set_height(fact, 42);
    lv_obj_set_flex_flow(fact, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fact, 0, LV_PART_MAIN);
    return create_label(fact, heading, &lv_font_montserrat_14, UI_LAB_MUTED) != NULL &&
           create_label(fact, value, &lv_font_montserrat_16, UI_LAB_TEXT) != NULL;
}

static lv_obj_t *make_graph_line(
    lv_obj_t *parent,
    const lv_point_precise_t *points,
    uint32_t point_count,
    lv_color_t color,
    int32_t width,
    bool dashed
)
{
    lv_obj_t *line = lv_line_create(parent);
    if (line == NULL) {
        return NULL;
    }
    lv_line_set_points(line, points, point_count);
    lv_obj_set_size(line, lv_pct(100), lv_pct(100));
    lv_obj_set_style_line_color(line, color, LV_PART_MAIN);
    lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN);
    if (dashed) {
        lv_obj_set_style_line_dash_width(line, 6, LV_PART_MAIN);
        lv_obj_set_style_line_dash_gap(line, 5, LV_PART_MAIN);
    }
    remove_scroll(line);
    return line;
}

static void trajectory_domain(
    const furnace_hmi_dashboard_state_t *state,
    int32_t *maximum_seconds,
    int32_t *minimum_temperature_c,
    int32_t *maximum_temperature_c
)
{
    int32_t latest_seconds = 0;
    int32_t minimum = 0;
    int32_t maximum = 100;
    bool have_temperature = false;
    if (state != NULL) {
        for (size_t index = 0; index < state->graph_sample_count; ++index) {
            const furnace_hmi_graph_sample_t *sample = &state->graph_samples[index];
            if ((int32_t)sample->elapsed_seconds > latest_seconds) {
                latest_seconds = (int32_t)sample->elapsed_seconds;
            }
            const int32_t values[] = {
                sample->planned_temperature_c,
                sample->measured_temperature_c,
            };
            const size_t value_count = sample->measured_validity ==
                FURNACE_HMI_VALUE_VALIDITY_CURRENT ? 2U : 1U;
            for (size_t value_index = 0; value_index < value_count; ++value_index) {
                if (!have_temperature) {
                    minimum = values[value_index];
                    maximum = values[value_index];
                    have_temperature = true;
                }
                else {
                    if (values[value_index] < minimum) {
                        minimum = values[value_index];
                    }
                    if (values[value_index] > maximum) {
                        maximum = values[value_index];
                    }
                }
            }
        }
        if (state->elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            state->elapsed_seconds.value > latest_seconds) {
            latest_seconds = state->elapsed_seconds.value;
        }
        if (state->current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
            const int32_t value = state->current_temperature_c.value;
            if (!have_temperature || value < minimum) {
                minimum = value;
            }
            if (!have_temperature || value > maximum) {
                maximum = value;
            }
            have_temperature = true;
        }
    }
    if (!have_temperature) {
        minimum = 0;
        maximum = 100;
    }
    const int32_t temperature_span = maximum - minimum < 100 ? 100 : maximum - minimum;
    *minimum_temperature_c = minimum > temperature_span / 10 ?
        minimum - temperature_span / 10 : 0;
    *maximum_temperature_c = maximum + temperature_span / 8;
    *maximum_seconds = latest_seconds < 60 ? 60 : latest_seconds + latest_seconds / 8;
}

static bool trajectory_measurement_outside_domain(
    const furnace_hmi_dashboard_state_t *state,
    const furnace_hmi_ui_lab_explorer_t *explorer
)
{
    if (state == NULL || explorer == NULL || !explorer->trajectory_domain_initialized) {
        return true;
    }
    if (state->elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
        state->elapsed_seconds.value > explorer->trajectory_domain_maximum_seconds) {
        return true;
    }
    if (state->current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
        (state->current_temperature_c.value < explorer->trajectory_domain_minimum_temperature_c ||
         state->current_temperature_c.value > explorer->trajectory_domain_maximum_temperature_c)) {
        return true;
    }
    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        const furnace_hmi_graph_sample_t *sample = &state->graph_samples[index];
        if (sample->measured_validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            (sample->elapsed_seconds > (uint32_t)explorer->trajectory_domain_maximum_seconds ||
             sample->measured_temperature_c < explorer->trajectory_domain_minimum_temperature_c ||
             sample->measured_temperature_c > explorer->trajectory_domain_maximum_temperature_c)) {
            return true;
        }
    }
    return false;
}

static bool update_trajectory(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->trajectory == NULL || explorer->planned_line == NULL ||
        explorer->state == NULL) {
        return false;
    }
    const furnace_hmi_dashboard_state_t *graph_state = explorer->trajectory_uses_program_preview
        ? &explorer->preview_graph_state : explorer->state;
    if (!explorer->trajectory_domain_initialized ||
        (!explorer->trajectory_uses_program_preview &&
         trajectory_measurement_outside_domain(graph_state, explorer))) {
        trajectory_domain(graph_state, &explorer->trajectory_domain_maximum_seconds,
                          &explorer->trajectory_domain_minimum_temperature_c,
                          &explorer->trajectory_domain_maximum_temperature_c);
        explorer->trajectory_domain_initialized = true;
    }
    const int32_t maximum_seconds = explorer->trajectory_domain_maximum_seconds;
    const int32_t minimum_temperature_c = explorer->trajectory_domain_minimum_temperature_c;
    const int32_t maximum_temperature_c = explorer->trajectory_domain_maximum_temperature_c;
    const int32_t graph_width = (int32_t)lv_obj_get_width(explorer->trajectory);
    const int32_t graph_height = (int32_t)lv_obj_get_height(explorer->trajectory);
    if (graph_width < 160 || graph_height < 72) {
        return false;
    }
    const int32_t left = graph_width > 360 ? UI_LAB_GRAPH_LEFT : 36;
    const int32_t right = UI_LAB_GRAPH_RIGHT;
    const int32_t top = UI_LAB_GRAPH_TOP;
    const int32_t bottom = UI_LAB_GRAPH_BOTTOM;
    const int32_t width = graph_width - left - right;
    const int32_t height = graph_height - top - bottom;
    if (width <= 0 || height <= 0 || maximum_temperature_c <= minimum_temperature_c) {
        return false;
    }
    char minimum_text[24];
    char middle_text[24];
    char maximum_text[24];
    char middle_time_text[24];
    char end_time_text[24];
    (void)snprintf(minimum_text, sizeof(minimum_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)minimum_temperature_c);
    (void)snprintf(maximum_text, sizeof(maximum_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)maximum_temperature_c);
    (void)snprintf(middle_text, sizeof(middle_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)(minimum_temperature_c +
                          (maximum_temperature_c - minimum_temperature_c) / 2));
    (void)snprintf(middle_time_text, sizeof(middle_time_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                   (long)(maximum_seconds / 120));
    (void)snprintf(end_time_text, sizeof(end_time_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                   (long)(maximum_seconds / 60));
    if (explorer->trajectory_minimum_label != NULL) {
        lv_label_set_text(explorer->trajectory_minimum_label, minimum_text);
    }
    if (explorer->trajectory_maximum_label != NULL) {
        lv_label_set_text(explorer->trajectory_maximum_label, maximum_text);
    }
    if (explorer->trajectory_middle_temperature_label != NULL) {
        lv_label_set_text(explorer->trajectory_middle_temperature_label, middle_text);
    }
    if (explorer->trajectory_middle_time_label != NULL) {
        lv_label_set_text(explorer->trajectory_middle_time_label, middle_time_text);
    }
    if (explorer->trajectory_end_time_label != NULL) {
        lv_label_set_text(explorer->trajectory_end_time_label, end_time_text);
    }
    for (size_t index = 0;
         index < FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT; ++index) {
        const int32_t y = top + height * (int32_t)index /
            (int32_t)(FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT - 1U);
        explorer->grid_line_points[index][0] = (lv_point_precise_t) { left, y };
        explorer->grid_line_points[index][1] = (lv_point_precise_t) { graph_width - right, y };
        if (explorer->grid_lines[index] != NULL) {
            lv_line_set_points(explorer->grid_lines[index], explorer->grid_line_points[index], 2U);
        }
    }
    for (size_t index = 0;
         index < FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT; ++index) {
        const size_t line_index = FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT + index;
        const int32_t x = left + width * (int32_t)index /
            (int32_t)(FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT - 1U);
        explorer->grid_line_points[line_index][0] = (lv_point_precise_t) { x, top };
        explorer->grid_line_points[line_index][1] = (lv_point_precise_t) { x, top + height };
        if (explorer->grid_lines[line_index] != NULL) {
            lv_line_set_points(explorer->grid_lines[line_index], explorer->grid_line_points[line_index], 2U);
        }
    }
    if (explorer->trajectory_maximum_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_maximum_label, 2, top - 2);
    }
    if (explorer->trajectory_middle_temperature_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_middle_temperature_label, 2, top + height / 2 - 7);
    }
    if (explorer->trajectory_minimum_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_minimum_label, 2, top + height - 8);
    }
    if (explorer->trajectory_middle_time_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_middle_time_label, graph_width / 2 - 16,
                       graph_height - 17);
    }
    if (explorer->trajectory_time_axis_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_time_axis_label, left, graph_height - 17);
    }
    if (explorer->trajectory_end_time_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_end_time_label, graph_width - right - 40,
                       graph_height - 17);
    }
    const size_t count = graph_state->graph_sample_count;
    for (size_t index = 0; index < count; ++index) {
        const furnace_hmi_graph_sample_t *sample = &graph_state->graph_samples[index];
        const int32_t x = left + (int32_t)((int64_t)sample->elapsed_seconds * width /
                                                        maximum_seconds);
        const int32_t planned_y = top + height - (int32_t)((int64_t)
            (sample->planned_temperature_c - minimum_temperature_c) * height /
            (maximum_temperature_c - minimum_temperature_c));
        explorer->planned_points[index] = (lv_point_precise_t) { x, planned_y };
    }
    if (count < 2U) {
        lv_obj_add_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN);
        for (size_t index = 0; index + 1U < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
            if (explorer->measured_segments[index] != NULL) {
                lv_obj_add_flag(explorer->measured_segments[index], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return true;
    }
    lv_obj_remove_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN);
    lv_line_set_points(explorer->planned_line, explorer->planned_points, (uint32_t)count);
    for (size_t index = 0; index + 1U < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
        lv_obj_t *segment = explorer->measured_segments[index];
        if (segment == NULL || index + 1U >= count ||
            graph_state->graph_samples[index].measured_validity !=
                FURNACE_HMI_VALUE_VALIDITY_CURRENT ||
            graph_state->graph_samples[index + 1U].measured_validity !=
                FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
            if (segment != NULL) {
                lv_obj_add_flag(segment, LV_OBJ_FLAG_HIDDEN);
            }
            continue;
        }
        const furnace_hmi_graph_sample_t *first = &graph_state->graph_samples[index];
        const furnace_hmi_graph_sample_t *second = &graph_state->graph_samples[index + 1U];
        const int32_t first_x = left + (int32_t)((int64_t)first->elapsed_seconds * width /
                                                               maximum_seconds);
        const int32_t second_x = left + (int32_t)((int64_t)second->elapsed_seconds * width /
                                                                maximum_seconds);
        const int32_t first_y = top + height - (int32_t)((int64_t)
            (first->measured_temperature_c - minimum_temperature_c) * height /
            (maximum_temperature_c - minimum_temperature_c));
        const int32_t second_y = top + height - (int32_t)((int64_t)
            (second->measured_temperature_c - minimum_temperature_c) * height /
            (maximum_temperature_c - minimum_temperature_c));
        explorer->measured_segment_points[index][0] = (lv_point_precise_t) { first_x, first_y };
        explorer->measured_segment_points[index][1] = (lv_point_precise_t) { second_x, second_y };
        lv_line_set_points(segment, explorer->measured_segment_points[index], 2U);
        lv_obj_set_style_line_color(segment, state_color(explorer->state), LV_PART_MAIN);
        lv_obj_remove_flag(segment, LV_OBJ_FLAG_HIDDEN);
    }
    for (size_t index = 0; index < FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT; ++index) {
        lv_obj_t *node = explorer->trajectory_stage_nodes[index];
        if (node == NULL || count < 2U) {
            continue;
        }
        const size_t point_index = index < count ? index : count - 1U;
        lv_obj_set_pos(node, explorer->planned_points[point_index].x - 7,
                       explorer->planned_points[point_index].y - 7);
    }
    lv_obj_invalidate(explorer->trajectory);
    return true;
}

static bool make_trajectory(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    int32_t height
)
{
    explorer->trajectory = lv_obj_create(parent);
    if (explorer->trajectory == NULL) {
        return false;
    }
    lv_obj_set_width(explorer->trajectory, lv_pct(100));
    if (height > 0) {
        lv_obj_set_height(explorer->trajectory, height);
        lv_obj_set_style_min_height(explorer->trajectory, height, LV_PART_MAIN);
        lv_obj_set_flex_grow(explorer->trajectory, 1);
    }
    else {
        lv_obj_set_height(explorer->trajectory, lv_pct(100));
        lv_obj_set_flex_grow(explorer->trajectory, 0);
    }
    lv_obj_set_style_bg_color(explorer->trajectory, UI_LAB_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->trajectory, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->trajectory, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->trajectory, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->trajectory, 0, LV_PART_MAIN);
    remove_scroll(explorer->trajectory);
    for (size_t index = 0; index < sizeof(explorer->grid_lines) /
                           sizeof(explorer->grid_lines[0]); ++index) {
        explorer->grid_lines[index] = make_graph_line(
            explorer->trajectory, explorer->grid_line_points[index], 2U, UI_LAB_BORDER, 1, false
        );
        if (explorer->grid_lines[index] == NULL) {
            return false;
        }
    }
    explorer->planned_line = make_graph_line(explorer->trajectory, explorer->planned_points, 0U,
                                              UI_LAB_PLANNED, 2, false);
    for (size_t index = 0; index + 1U < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
        explorer->measured_segments[index] = make_graph_line(
            explorer->trajectory, explorer->measured_segment_points[index], 0U,
            state_color(explorer->state), 3, false
        );
        if (explorer->measured_segments[index] == NULL) {
            return false;
        }
    }
    lv_obj_t *axis = create_label(explorer->trajectory,
                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_TEMPERATURE_AXIS),
                                  &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->trajectory_minimum_label = create_label(explorer->trajectory, "",
                                                       &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->trajectory_maximum_label = create_label(explorer->trajectory, "",
                                                       &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->trajectory_middle_temperature_label = create_label(explorer->trajectory, "",
                                                                  &lv_font_montserrat_14,
                                                                  UI_LAB_MUTED);
    explorer->trajectory_time_axis_label = create_label(
        explorer->trajectory, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_TIME_AXIS),
        &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->trajectory_end_time_label = create_label(explorer->trajectory, "",
                                                        &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->trajectory_middle_time_label = create_label(explorer->trajectory, "",
                                                           &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *planned = create_label(explorer->trajectory,
                                     ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_PLANNED),
                                     &lv_font_montserrat_14, UI_LAB_PLANNED);
    lv_obj_t *measured = create_label(explorer->trajectory,
                                      ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_MEASURED),
                                      &lv_font_montserrat_14, state_color(explorer->state));
    if (explorer->planned_line == NULL || axis == NULL ||
        explorer->trajectory_minimum_label == NULL || explorer->trajectory_maximum_label == NULL ||
        explorer->trajectory_middle_temperature_label == NULL || explorer->trajectory_time_axis_label == NULL ||
        explorer->trajectory_end_time_label == NULL || explorer->trajectory_middle_time_label == NULL || planned == NULL ||
        measured == NULL) {
        return false;
    }
    lv_obj_align(axis, LV_ALIGN_TOP_LEFT, 4, 1);
    lv_obj_set_pos(planned, UI_LAB_GRAPH_LEFT + 104, 1);
    lv_obj_align(measured, LV_ALIGN_TOP_RIGHT, -3, 1);
    if (explorer->trajectory_uses_program_preview) {
        size_t node_count = program_fixtures[explorer->selected_program % program_fixture_count()].stage_count;
        if (node_count > FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT) {
            node_count = FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT;
        }
        for (size_t index = 0; index < node_count; ++index) {
            const size_t stage_index = index < sizeof(stage_fixtures) / sizeof(stage_fixtures[0])
                ? index : sizeof(stage_fixtures) / sizeof(stage_fixtures[0]) - 1U;
            explorer->trajectory_stage_nodes[index] = create_page_button(
                explorer, explorer->trajectory, "", stage_kind_color(fixture_stage_kind(&stage_fixtures[stage_index])),
                UI_LAB_ACTION_STAGE_EDITOR, (int)index
            );
            if (explorer->trajectory_stage_nodes[index] == NULL) {
                return false;
            }
            lv_obj_set_size(explorer->trajectory_stage_nodes[index], 14, 14);
            lv_obj_add_flag(explorer->trajectory_stage_nodes[index], LV_OBJ_FLAG_FLOATING);
        }
    }
    lv_obj_update_layout(explorer->root);
    lv_obj_update_layout(explorer->trajectory);
    return update_trajectory(explorer);
}

static void clear_modal(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer != NULL && explorer->modal != NULL && lv_obj_is_valid(explorer->modal)) {
        lv_obj_delete(explorer->modal);
    }
    if (explorer != NULL) {
        explorer->modal = NULL;
        explorer->modal_action_count = 0U;
    }
}

static bool open_dialog(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title_text,
    const char *detail_text,
    ui_lab_action_t primary_action,
    const char *primary_text,
    lv_color_t primary_color
)
{
    clear_modal(explorer);
    lv_obj_t *overlay = lv_obj_create(explorer->root);
    if (overlay == NULL) {
        return false;
    }
    explorer->modal = overlay;
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *dialog = create_panel(overlay, UI_LAB_SURFACE_RAISED);
    if (dialog == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_size(dialog, lv_pct(70), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dialog, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_row(dialog, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(dialog);
    lv_obj_t *title = create_label(dialog, title_text, &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *detail = create_label(dialog, detail_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *actions = create_transparent_container(dialog);
    if (title == NULL || detail == NULL || actions == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(detail, lv_pct(100));
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 46);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    lv_obj_t *cancel = create_modal_button(
        explorer,
        actions,
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
        UI_LAB_DEEP_BLUE,
        UI_LAB_ACTION_CLOSE_MODAL,
        0
    );
    if (cancel == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_flex_grow(cancel, 1);
    if (primary_action != UI_LAB_ACTION_CLOSE_MODAL && primary_text != NULL) {
        lv_obj_t *primary = create_modal_button(
            explorer,
            actions,
            primary_text,
            primary_color,
            primary_action,
            0
        );
        if (primary == NULL) {
            clear_modal(explorer);
            return false;
        }
        lv_obj_set_flex_grow(primary, 1);
    }
    return true;
}

static bool create_furnace_visual(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    bool expansive
)
{
    lv_obj_t *visual = create_panel(parent, UI_LAB_SURFACE);
    if (visual == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(visual, 1);
    lv_obj_set_flex_flow(visual, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(visual, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *title = create_label(visual, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FURNACE_VISUAL),
                                   &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *furnace = lv_obj_create(visual);
    lv_obj_t *top = lv_obj_create(visual);
    lv_obj_t *side = lv_obj_create(visual);
    lv_obj_t *door = lv_obj_create(visual);
    if (title == NULL || furnace == NULL || top == NULL || side == NULL || door == NULL) {
        return false;
    }
    const lv_color_t accent = state_color(explorer->state);
    const int32_t furnace_width = expansive ? 360 : 270;
    const int32_t furnace_height = expansive ? 210 : 160;
    const int32_t top_width = expansive ? 304 : 228;
    const int32_t top_height = expansive ? 34 : 28;
    const int32_t side_width = expansive ? 48 : 38;
    const int32_t side_height = expansive ? 192 : 144;
    const int32_t door_size = expansive ? 138 : 106;
    lv_obj_set_size(furnace, furnace_width, furnace_height);
    lv_obj_set_style_bg_color(furnace, lv_color_hex(0x465055), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(furnace, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(furnace, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(furnace, accent, LV_PART_MAIN);
    lv_obj_set_style_radius(furnace, 5, LV_PART_MAIN);
    remove_scroll(furnace);
    lv_obj_set_size(top, top_width, top_height);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x68747A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(top, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(top, UI_LAB_TEXT, LV_PART_MAIN);
    lv_obj_set_style_transform_skew_x(top, -20, LV_PART_MAIN);
    lv_obj_align_to(top, furnace, LV_ALIGN_OUT_TOP_MID, 6, 0);
    remove_scroll(top);
    lv_obj_set_size(side, side_width, side_height);
    lv_obj_set_style_bg_color(side, lv_color_hex(0x30383D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(side, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(side, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(side, UI_LAB_TEXT, LV_PART_MAIN);
    lv_obj_align_to(side, furnace, LV_ALIGN_OUT_RIGHT_MID, -1, 2);
    remove_scroll(side);
    lv_obj_set_size(door, door_size, door_size);
    lv_obj_set_style_bg_color(door, lv_color_hex(0x111315), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(door, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(door, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(door, accent, LV_PART_MAIN);
    lv_obj_set_style_radius(door, door_size / 2, LV_PART_MAIN);
    lv_obj_center(door);
    remove_scroll(door);
    lv_obj_t *door_label = create_label(door, "", &lv_font_montserrat_14, accent);
    if (door_label == NULL) {
        return false;
    }
    lv_label_set_text(door_label, explorer->state != NULL && explorer->state->door_closed.value
                                      ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED)
                                      : ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OPEN));
    lv_obj_center(door_label);
    lv_obj_t *bubbles = create_transparent_container(visual);
    if (bubbles == NULL) {
        return false;
    }
    lv_obj_set_width(bubbles, lv_pct(100));
    lv_obj_set_height(bubbles, 62);
    lv_obj_set_flex_flow(bubbles, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bubbles, 6, LV_PART_MAIN);
    char temperature[32];
    char rpm[32];
    format_observation_value(temperature, sizeof(temperature),
                             &explorer->state->current_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    format_observation_value(rpm, sizeof(rpm), &explorer->state->fan_rpm,
                             FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT);
    if (!make_metric(bubbles, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_TEMPERATURE),
                     temperature, NULL) ||
        !make_metric(bubbles, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAN), rpm, NULL) ||
        !make_metric(bubbles, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DOOR),
                     explorer->state->door_closed.value
                         ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED)
                         : ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OPEN), NULL)) {
        return false;
    }
    return true;
}

static bool build_home(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (!state_is_current(explorer->state)) {
        lv_obj_t *panel = create_panel(explorer->content, UI_LAB_SURFACE);
        if (panel == NULL) {
            return false;
        }
        lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *heading = create_label(panel, state_text(explorer->state), &lv_font_montserrat_20,
                                         state_color(explorer->state));
        lv_obj_t *detail = create_label(panel,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NO_CURRENT_SNAPSHOT),
                                        &lv_font_montserrat_16, UI_LAB_MUTED);
        if (heading == NULL || detail == NULL) {
            return false;
        }
        lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(detail, lv_pct(75));
        return true;
    }

    lv_obj_t *columns = create_transparent_container(explorer->content);
    lv_obj_t *quick_launch = create_panel(columns, UI_LAB_SURFACE);
    lv_obj_t *visual_column = create_transparent_container(columns);
    if (columns == NULL || visual_column == NULL || quick_launch == NULL) {
        return false;
    }
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    lv_obj_set_width(quick_launch, lv_pct(32));
    lv_obj_set_height(quick_launch, lv_pct(100));
    lv_obj_set_flex_flow(quick_launch, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(quick_launch, 6, LV_PART_MAIN);
    lv_obj_set_width(visual_column, lv_pct(68));
    lv_obj_set_height(visual_column, lv_pct(100));
    lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *title = create_label(quick_launch,
                                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_QUICK_LAUNCH),
                                   &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *favourites = create_label(quick_launch,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAVOURITES),
                                        &lv_font_montserrat_14, UI_LAB_MUTED);
    if (title == NULL || favourites == NULL) {
        return false;
    }
    if (explorer->favourite_count == 0U) {
        lv_obj_t *empty = create_label(quick_launch,
                                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NO_FAVOURITES),
                                       &lv_font_montserrat_14, UI_LAB_MUTED);
        if (empty == NULL) {
            return false;
        }
    }
    for (size_t index = 0; index < explorer->favourite_count; ++index) {
        const uint8_t program_index = explorer->quick_launch_favourites[index];
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                ui_string(program_fixtures[program_index].name),
                                                UI_LAB_SURFACE_RAISED,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 42);
    }
    lv_obj_t *recent = create_label(quick_launch,
                                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RECENT_PROGRAMS),
                                    &lv_font_montserrat_14, UI_LAB_MUTED);
    if (recent == NULL) {
        return false;
    }
    for (size_t index = 0; index < explorer->recent_count; ++index) {
        const uint8_t program_index = explorer->quick_launch_recent[index];
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                ui_string(program_fixtures[program_index].name),
                                                UI_LAB_SURFACE_RAISED,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 42);
    }
    lv_obj_t *spacer = create_transparent_container(quick_launch);
    if (spacer == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_t *choose = create_page_button(explorer, quick_launch,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CHOOSE_PROGRAM),
                                           UI_LAB_DEEP_BLUE, UI_LAB_ACTION_PROGRAMS, 0);
    if (choose == NULL) {
        return false;
    }
    lv_obj_set_width(choose, lv_pct(100));
    lv_obj_set_height(choose, 46);
    return create_furnace_visual(explorer, visual_column, true);
}

static bool make_program_card(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const ui_lab_program_fixture_t *program,
    int program_index
)
{
    lv_obj_t *card = create_page_button(explorer, parent, "", UI_LAB_SURFACE_RAISED,
                                        UI_LAB_ACTION_PROGRAM_DETAIL, program_index);
    if (card == NULL) {
        return false;
    }
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, 88);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *name = create_label(card, ui_string(program->name), &lv_font_montserrat_16, UI_LAB_TEXT);
    char details[96];
    (void)snprintf(details, sizeof(details),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_SUMMARY_FORMAT),
                   (unsigned int)program->stage_count, ui_string(program->duration),
                   ui_string(program->maximum_temperature));
    lv_obj_t *summary = create_label(card, details, &lv_font_montserrat_14, UI_LAB_MUTED);
    if (name == NULL || summary == NULL) {
        return false;
    }
    return true;
}

static bool build_programs(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t program_count = program_fixture_count();
    const size_t per_page = 6U;
    const size_t page_count = (program_count + per_page - 1U) / per_page;
    const size_t start = (size_t)explorer->program_page * per_page;
    const size_t end = start + per_page < program_count ? start + per_page : program_count;
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *page_header = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    if (toolbar == NULL || page_header == NULL || list == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 42);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(toolbar, 8, LV_PART_MAIN);
    lv_obj_set_width(page_header, lv_pct(100));
    lv_obj_set_height(page_header, 22);
    lv_obj_set_flex_flow(page_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page_header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(list, 8, LV_PART_MAIN);
    lv_obj_t *new_draft = create_page_button(explorer, toolbar,
                                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT),
                                             UI_LAB_DEEP_BLUE, UI_LAB_ACTION_NEW_DRAFT, 0);
    lv_obj_t *spacer = create_transparent_container(toolbar);
    char page_text[32];
    (void)snprintf(page_text, sizeof(page_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAGE_FORMAT),
                   (unsigned int)explorer->program_page + 1U, (unsigned int)page_count);
    lv_obj_t *page = create_label(page_header, page_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *previous = create_page_button(explorer, toolbar,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS),
                                            UI_LAB_SURFACE_RAISED,
                                            UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE, 0);
    lv_obj_t *next = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_NEXT_PROGRAM_PAGE, 0);
    if (new_draft == NULL || spacer == NULL || page == NULL || previous == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_width(new_draft, 112);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_width(previous, 82);
    lv_obj_set_width(next, 82);
    lv_obj_set_height(previous, 42);
    lv_obj_set_height(next, 42);
    lv_obj_t *columns[2] = {
        create_transparent_container(list),
        create_transparent_container(list),
    };
    if (columns[0] == NULL || columns[1] == NULL) {
        return false;
    }
    for (size_t column = 0; column < 2U; ++column) {
        lv_obj_set_width(columns[column], lv_pct(50));
        lv_obj_set_height(columns[column], lv_pct(100));
        lv_obj_set_flex_flow(columns[column], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(columns[column], 8, LV_PART_MAIN);
    }
    for (size_t index = start; index < end; ++index) {
        if (!make_program_card(explorer, columns[(index - start) % 2U],
                               &program_fixtures[index], (int)index)) {
            return false;
        }
    }
    return true;
}

static bool build_program_loading(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const ui_lab_program_fixture_t *program =
        &program_fixtures[explorer->selected_program %
                          (sizeof(program_fixtures) / sizeof(program_fixtures[0]))];
    prepare_program_preview(explorer, program);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *columns = create_transparent_container(explorer->content);
    if (toolbar == NULL || columns == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 40);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_HOME, 0);
    lv_obj_t *title = create_label(toolbar, ui_string(program->name), &lv_font_montserrat_20,
                                   UI_LAB_TEXT);
    if (back == NULL || title == NULL) {
        return false;
    }
    lv_obj_set_width(back, 70);
    lv_obj_set_flex_grow(title, 1);
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    lv_obj_update_layout(explorer->root);
    const int32_t columns_height = lv_obj_get_content_height(explorer->content) -
        lv_obj_get_height(toolbar) -
        lv_obj_get_style_pad_row(explorer->content, LV_PART_MAIN);
    if (columns_height < 72) {
        return false;
    }
    /* Percentage height would include the toolbar and push graph labels below
     * the visible content. Keep this row inside the content area instead. */
    lv_obj_set_flex_grow(columns, 0);
    lv_obj_set_height(columns, columns_height);
    lv_obj_t *graph_column = create_transparent_container(columns);
    lv_obj_t *summary = create_panel(columns, UI_LAB_SURFACE);
    if (graph_column == NULL || summary == NULL) {
        return false;
    }
    lv_obj_set_width(graph_column, lv_pct(62));
    lv_obj_set_height(graph_column, lv_pct(100));
    lv_obj_set_flex_flow(graph_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(summary, lv_pct(38));
    lv_obj_set_height(summary, lv_pct(100));
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(summary, 7, LV_PART_MAIN);
    if (!make_trajectory(explorer, graph_column, 0)) {
        return false;
    }
    char details[96];
    (void)snprintf(details, sizeof(details),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_SUMMARY_FORMAT),
                   (unsigned int)program->stage_count, ui_string(program->duration),
                   ui_string(program->maximum_temperature));
    lv_obj_t *facts = create_label(summary, details, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *energy = create_label(summary, ui_string(program->estimated_energy),
                                    &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *start = create_page_button(explorer, summary,
                                         ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW),
                                         state_color(explorer->state), UI_LAB_ACTION_RUNNING, 0);
    lv_obj_t *details_button = create_page_button(explorer, summary,
                                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VIEW_DETAILS),
                                                  UI_LAB_DEEP_BLUE,
                                                  UI_LAB_ACTION_PROGRAM_DETAIL,
                                                  (int)explorer->selected_program);
    if (facts == NULL || energy == NULL || start == NULL || details_button == NULL) {
        return false;
    }
    lv_obj_set_width(start, lv_pct(100));
    lv_obj_set_height(start, 46);
    lv_obj_set_width(details_button, lv_pct(100));
    lv_obj_set_height(details_button, 42);
    return true;
}

static bool build_running(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (!state_is_current(explorer->state)) {
        return build_home(explorer);
    }
    char elapsed[32];
    char remaining[32];
    char current[32];
    char target[32];
    char power[32];
    format_remaining(elapsed, sizeof(elapsed), &explorer->state->elapsed_seconds);
    format_remaining(remaining, sizeof(remaining), &explorer->state->remaining_seconds);
    format_observation_value(current, sizeof(current), &explorer->state->current_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    format_observation_value(target, sizeof(target), &explorer->state->target_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    if (explorer->state->estimated_power_kw_x100.validity ==
        FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        const int32_t value = explorer->state->estimated_power_kw_x100.value;
        (void)snprintf(power, sizeof(power),
                       ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_POWER_FORMAT),
                       value / 100, value >= 0 ? value % 100 : -(value % 100));
    }
    else {
        (void)snprintf(power, sizeof(power), "%s",
                       ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
    }
    lv_obj_t *metrics = create_transparent_container(explorer->content);
    lv_obj_t *phase = create_panel(explorer->content, UI_LAB_SURFACE);
    lv_obj_t *middle = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (metrics == NULL || phase == NULL || middle == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_width(metrics, lv_pct(100));
    lv_obj_set_height(metrics, 64);
    lv_obj_set_flex_flow(metrics, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(metrics, 5, LV_PART_MAIN);
    if (!make_metric(metrics, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ELAPSED), elapsed, NULL) ||
        !make_metric(metrics, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REMAINING), remaining, NULL) ||
        !make_metric(metrics, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_TEMPERATURE), current, NULL) ||
        !make_metric(metrics, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET), target, NULL) ||
        !make_metric(metrics, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_ESTIMATED_POWER), power, NULL)) {
        return false;
    }
    lv_obj_set_width(phase, lv_pct(100));
    lv_obj_set_height(phase, 34);
    lv_obj_set_flex_flow(phase, LV_FLEX_FLOW_ROW);
    char phase_text[112];
    (void)snprintf(phase_text, sizeof(phase_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUNNING_PHASE_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PHASE), state_text(explorer->state),
                   explorer->state->stage_name, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_STAGE));
    lv_obj_t *phase_label = create_label(phase, phase_text, &lv_font_montserrat_14,
                                         state_color(explorer->state));
    if (phase_label == NULL) {
        return false;
    }
    lv_obj_set_width(middle, lv_pct(100));
    lv_obj_set_flex_grow(middle, 1);
    lv_obj_set_flex_flow(middle, explorer->running_view_mode == 1U ? LV_FLEX_FLOW_ROW :
                         LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_column(middle, 8, LV_PART_MAIN);
    if (explorer->running_view_mode == 0U) {
        if (!make_trajectory(explorer, middle, 236)) {
            return false;
        }
    }
    else if (explorer->running_view_mode == 1U) {
        lv_obj_t *graph_column = create_transparent_container(middle);
        lv_obj_t *visual_column = create_transparent_container(middle);
        if (graph_column == NULL || visual_column == NULL) {
            return false;
        }
        lv_obj_set_width(graph_column, lv_pct(57));
        lv_obj_set_height(graph_column, lv_pct(100));
        lv_obj_set_flex_flow(graph_column, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(visual_column, lv_pct(43));
        lv_obj_set_height(visual_column, lv_pct(100));
        lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
        if (!make_trajectory(explorer, graph_column, 212) ||
            !create_furnace_visual(explorer, visual_column, false)) {
            return false;
        }
    }
    else if (!create_furnace_visual(explorer, middle, true)) {
        return false;
    }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 48);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 6, LV_PART_MAIN);
    const bool paused = explorer->state->run_state == FURNACE_HMI_RUN_STATE_PAUSED;
    const ui_lab_action_t primary_action = paused ? UI_LAB_ACTION_RESUME_PREVIEW :
                                                   UI_LAB_ACTION_PAUSE_PREVIEW;
    const furnace_hmi_ui_string_id_t primary_text = paused ?
        FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW : FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW;
    const ui_lab_action_t mode_actions[] = {
        primary_action, UI_LAB_ACTION_STOP_PREVIEW, UI_LAB_ACTION_SHOW_GRAPH_VIEW,
        UI_LAB_ACTION_SHOW_SPLIT_VIEW, UI_LAB_ACTION_SHOW_VISUAL_VIEW,
    };
    const furnace_hmi_ui_string_id_t mode_texts[] = {
        primary_text, FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW,
        FURNACE_HMI_UI_STRING_UI_LAB_FULL_GRAPH, FURNACE_HMI_UI_STRING_UI_LAB_SPLIT_VIEW,
        FURNACE_HMI_UI_STRING_UI_LAB_FULL_VISUAL,
    };
    for (size_t index = 0; index < sizeof(mode_actions) / sizeof(mode_actions[0]); ++index) {
        lv_obj_t *button = create_page_button(explorer, actions, ui_string(mode_texts[index]),
                                              index == 1U ? UI_LAB_FAULT :
                                              (index == 0U ? state_color(explorer->state) : UI_LAB_DEEP_BLUE),
                                              mode_actions[index], 0);
        if (button == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, 46);
    }
    return true;
}

static bool build_program_stages(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t stage_count = sizeof(stage_fixtures) / sizeof(stage_fixtures[0]);
    const size_t per_page = 9U;
    const size_t page_count = (stage_count + per_page - 1U) / per_page;
    const size_t start = (size_t)explorer->program_stage_page * per_page;
    const size_t end = start + per_page < stage_count ? start + per_page : stage_count;
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *page_header = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    if (toolbar == NULL || page_header == NULL || list == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 36);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED,
                                        UI_LAB_ACTION_BACK_PROGRAM_DETAIL, 0);
    lv_obj_t *add_stage = create_page_button(explorer, toolbar,
                                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE),
                                             UI_LAB_DEEP_BLUE, UI_LAB_ACTION_ADD_STAGE, 0);
    char page_text[32];
    (void)snprintf(page_text, sizeof(page_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAGE_FORMAT),
                   (unsigned int)explorer->program_stage_page + 1U, (unsigned int)page_count);
    lv_obj_t *spacer = create_transparent_container(toolbar);
    lv_obj_t *page = create_label(page_header, page_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *previous = create_page_button(explorer, toolbar,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS),
                                            UI_LAB_SURFACE_RAISED,
                                            UI_LAB_ACTION_PREVIOUS_STAGE_PAGE, 0);
    lv_obj_t *next = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT),
                                        UI_LAB_SURFACE_RAISED,
                                        UI_LAB_ACTION_NEXT_STAGE_PAGE, 0);
    if (back == NULL || add_stage == NULL || spacer == NULL || page == NULL || previous == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_width(back, 62);
    lv_obj_set_width(add_stage, 112);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_width(previous, 72);
    lv_obj_set_width(next, 72);
    lv_obj_set_height(add_stage, 34);
    lv_obj_set_height(previous, 34);
    lv_obj_set_height(next, 34);
    lv_obj_set_width(page_header, lv_pct(100));
    lv_obj_set_height(page_header, 18);
    lv_obj_set_flex_flow(page_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page_header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(list, 6, LV_PART_MAIN);
    for (size_t column = 0; column < 3U; ++column) {
        lv_obj_t *column_parent = create_transparent_container(list);
        if (column_parent == NULL) {
            return false;
        }
        lv_obj_set_width(column_parent, lv_pct(33));
        lv_obj_set_height(column_parent, lv_pct(100));
        lv_obj_set_flex_flow(column_parent, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(column_parent, 6, LV_PART_MAIN);
        for (size_t index = start + column; index < end; index += 3U) {
            const ui_lab_stage_fixture_t *stage = &stage_fixtures[index];
            lv_obj_t *card = create_page_button(explorer, column_parent, "",
                                                UI_LAB_SURFACE_RAISED,
                                                UI_LAB_ACTION_STAGE_EDITOR, (int)index);
            if (card == NULL) {
                return false;
            }
            lv_obj_set_width(card, lv_pct(100));
            lv_obj_set_height(card, 78);
            lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_pad_row(card, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(card, 4, LV_PART_MAIN);
            char title[56];
            char summary_text[72];
            (void)snprintf(title, sizeof(title), "%u. %s", (unsigned int)index + 1U,
                           ui_string(stage->name));
            format_stage_card_summary(summary_text, sizeof(summary_text), stage);
            const lv_color_t kind_color = stage_kind_color(fixture_stage_kind(stage));
            lv_obj_t *name = create_label(card, title, &lv_font_montserrat_16, UI_LAB_TEXT);
            lv_obj_t *kind = create_label(card, ui_string(stage->kind), &lv_font_montserrat_14, kind_color);
            lv_obj_t *summary_label = create_label(card, summary_text, &lv_font_montserrat_14, UI_LAB_MUTED);
            if (name == NULL || kind == NULL || summary_label == NULL) {
                return false;
            }
        }
    }
    return true;
}

static bool build_program_detail(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const ui_lab_program_fixture_t *program =
        &program_fixtures[explorer->selected_program % program_fixture_count()];
    prepare_program_preview(explorer, program);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *columns = create_transparent_container(explorer->content);
    if (toolbar == NULL || columns == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 40);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_BACK_PROGRAMS, 0);
    lv_obj_t *title = create_label(toolbar, ui_string(program->name), &lv_font_montserrat_20, UI_LAB_TEXT);
    const bool favourite_saved = program_is_favourite(explorer, explorer->selected_program);
    lv_obj_t *favourite = create_page_button(
        explorer, toolbar,
        ui_string(favourite_saved
                      ? FURNACE_HMI_UI_STRING_UI_LAB_REMOVE_FAVOURITE
                      : FURNACE_HMI_UI_STRING_UI_LAB_ADD_FAVOURITE),
        favourite_saved ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE_RAISED,
        UI_LAB_ACTION_TOGGLE_FAVOURITE, explorer->selected_program
    );
    lv_obj_t *edit_name = create_page_button(explorer, toolbar,
                                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_NAME),
                                             UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SHOW_SAVE, 0);
    if (back == NULL || title == NULL || favourite == NULL || edit_name == NULL) {
        return false;
    }
    lv_obj_set_width(back, 68);
    lv_obj_set_flex_grow(title, 1);
    lv_obj_set_width(favourite, 134);
    lv_obj_set_width(edit_name, 84);
    lv_obj_set_height(favourite, 32);
    lv_obj_set_height(edit_name, 32);
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    lv_obj_t *graph_column = create_transparent_container(columns);
    lv_obj_t *details = create_panel(columns, UI_LAB_SURFACE);
    if (graph_column == NULL || details == NULL) {
        return false;
    }
    lv_obj_set_width(graph_column, lv_pct(64));
    lv_obj_set_height(graph_column, lv_pct(100));
    lv_obj_set_flex_flow(graph_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(details, lv_pct(36));
    lv_obj_set_height(details, lv_pct(100));
    lv_obj_set_flex_flow(details, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(details, 5, LV_PART_MAIN);
    if (!make_trajectory(explorer, graph_column, 180)) {
        return false;
    }
    char stage_count[24];
    (void)snprintf(stage_count, sizeof(stage_count),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COUNT_FORMAT),
                   (unsigned int)program->stage_count);
    lv_obj_t *facts = create_transparent_container(details);
    if (facts == NULL) {
        return false;
    }
    lv_obj_set_width(facts, lv_pct(100));
    lv_obj_set_height(facts, 88);
    lv_obj_set_flex_flow(facts, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(facts, 2, LV_PART_MAIN);
    if (!make_detail_fact(facts, stage_count, ui_string(program->duration), 50) ||
        !make_detail_fact(facts, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MAX_TEMPERATURE),
                          ui_string(program->maximum_temperature), 50) ||
        !make_detail_fact(facts, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ESTIMATED_ENERGY),
                          ui_string(program->estimated_energy), 50) ||
        !make_detail_fact(facts, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL),
                          ui_string(program->material), 50)) {
        return false;
    }
    const furnace_hmi_ui_string_id_t actions[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_VIEW_STAGES,
        FURNACE_HMI_UI_STRING_UI_LAB_LOAD_PROGRAM,
        FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT,
        FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW,
    };
    const ui_lab_action_t action_ids[] = {
        UI_LAB_ACTION_PROGRAM_STAGES,
        UI_LAB_ACTION_PROGRAM_LOADING,
        UI_LAB_ACTION_SHOW_DELETE,
        UI_LAB_ACTION_RUNNING,
    };
    for (size_t index = 0; index < sizeof(actions) / sizeof(actions[0]); ++index) {
        lv_obj_t *button = create_page_button(
            explorer, details, ui_string(actions[index]),
            index == 2U ? UI_LAB_FAULT :
            (index == 3U ? state_color(explorer->state) : UI_LAB_SURFACE_RAISED),
            action_ids[index], index == 1U ? (int)explorer->selected_program : 0
        );
        if (button == NULL) {
            return false;
        }
        lv_obj_set_width(button, lv_pct(100));
        lv_obj_set_height(button, 38);
    }
    return true;
}

static bool build_stage_detail(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const ui_lab_stage_fixture_t *stage = &stage_fixtures[explorer->selected_stage %
        (sizeof(stage_fixtures) / sizeof(stage_fixtures[0]))];
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *summary = create_panel(explorer->content, UI_LAB_SURFACE);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (toolbar == NULL || summary == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 42);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED,
                                        UI_LAB_ACTION_BACK_PROGRAM_DETAIL, 0);
    lv_obj_t *title = create_label(toolbar, ui_string(stage->name), &lv_font_montserrat_20, UI_LAB_TEXT);
    if (back == NULL || title == NULL) {
        return false;
    }
    lv_obj_set_width(back, 68);
    lv_obj_set_flex_grow(title, 1);
    lv_obj_set_width(summary, lv_pct(100));
    lv_obj_set_flex_grow(summary, 1);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(summary, 8, LV_PART_MAIN);
    char row[72];
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_TYPE), ui_string(stage->kind));
    lv_obj_t *kind = create_label(summary, row, &lv_font_montserrat_16, UI_LAB_TEXT);
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TARGET_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET), (long)stage->target_c);
    lv_obj_t *target = create_label(summary, row, &lv_font_montserrat_16, UI_LAB_TEXT);
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_RATE_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE), (long)stage->delta_c_per_minute);
    lv_obj_t *rate = create_label(summary, row, &lv_font_montserrat_16, UI_LAB_TEXT);
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_DURATION_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION), (long)stage->duration_minutes);
    lv_obj_t *duration = create_label(summary, row, &lv_font_montserrat_16, UI_LAB_TEXT);
    if (kind == NULL || target == NULL || rate == NULL || duration == NULL) {
        return false;
    }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 44);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 7, LV_PART_MAIN);
    lv_obj_t *previous = create_page_button(explorer, actions,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS),
                                            UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_PREVIOUS_STAGE, 0);
    lv_obj_t *edit = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGE),
                                        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_STAGE_EDITOR,
                                        (int)explorer->selected_stage);
    lv_obj_t *next = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_NEXT_STAGE, 0);
    if (previous == NULL || edit == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(previous, 1);
    lv_obj_set_flex_grow(edit, 1);
    lv_obj_set_flex_grow(next, 1);
    return true;
}

static bool make_editor_row(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *title,
    int32_t value,
    const char *format,
    ui_lab_action_t decrement,
    ui_lab_action_t increment
)
{
    lv_obj_t *row = create_panel(parent, UI_LAB_SURFACE_RAISED);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 54);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_t *label = create_label(row, title, &lv_font_montserrat_16, UI_LAB_MUTED);
    char formatted[32];
    if (format == ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_RATE_TENTHS)) {
        format_rate_tenths(formatted, sizeof(formatted), value);
    }
    else {
        (void)snprintf(formatted, sizeof(formatted), format, (long)value);
    }
    lv_obj_t *shown_value = create_label(row, formatted, &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *minus = create_page_button(
        explorer, row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE), UI_LAB_DEEP_BLUE,
        decrement, 0
    );
    lv_obj_t *plus = create_page_button(
        explorer, row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE), UI_LAB_DEEP_BLUE,
        increment, 0
    );
    if (label == NULL || shown_value == NULL || minus == NULL || plus == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(label, 1);
    lv_obj_set_width(shown_value, 86);
    lv_obj_set_size(minus, 48, 38);
    lv_obj_set_size(plus, 48, 38);
    return true;
}

static bool build_stage_editor(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *heading = create_panel(explorer->content, UI_LAB_SURFACE);
    if (heading == NULL) {
        return false;
    }
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 42);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_t *draft = create_label(heading, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOCAL_DRAFT),
                                   &lv_font_montserrat_16, UI_LAB_DEEP_BLUE);
    lv_obj_t *title = create_label(heading, ui_string(stage_fixtures[explorer->selected_stage %
                                     (sizeof(stage_fixtures) / sizeof(stage_fixtures[0]))].name),
                                   &lv_font_montserrat_16, UI_LAB_TEXT);
    if (draft == NULL || title == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(title, 1);
    lv_obj_t *kind_actions = create_transparent_container(explorer->content);
    if (kind_actions == NULL) {
        return false;
    }
    lv_obj_set_width(kind_actions, lv_pct(100));
    lv_obj_set_height(kind_actions, 40);
    lv_obj_set_flex_flow(kind_actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(kind_actions, 7, LV_PART_MAIN);
    const furnace_hmi_ui_string_id_t kind_ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING,
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD,
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COOLING,
    };
    const ui_lab_action_t kind_actions_ids[] = {
        UI_LAB_ACTION_SET_DRAFT_HEATING,
        UI_LAB_ACTION_SET_DRAFT_HOLD,
        UI_LAB_ACTION_SET_DRAFT_COOL,
    };
    for (size_t index = 0; index < sizeof(kind_ids) / sizeof(kind_ids[0]); ++index) {
        lv_obj_t *button = create_page_button(
            explorer, kind_actions, ui_string(kind_ids[index]),
            explorer->draft_stage_kind == index
                ? stage_kind_color((ui_lab_draft_stage_kind_t)index) : UI_LAB_SURFACE_RAISED,
            kind_actions_ids[index], 0
        );
        if (button == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, 38);
    }
    if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HOLD) {
        lv_obj_t *inherited = create_panel(explorer->content, UI_LAB_SURFACE_RAISED);
        if (inherited == NULL) {
            return false;
        }
        lv_obj_set_width(inherited, lv_pct(100));
        lv_obj_set_height(inherited, 48);
        lv_obj_set_flex_flow(inherited, LV_FLEX_FLOW_ROW);
        char inherited_text[64];
        (void)snprintf(inherited_text, sizeof(inherited_text),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TARGET_FORMAT),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INHERITED_TARGET),
                       (long)explorer->draft_target_c);
        lv_obj_t *inherited_label = create_label(inherited, inherited_text,
                                                 &lv_font_montserrat_16, UI_LAB_MUTED);
        if (inherited_label == NULL ||
            !make_editor_row(explorer, explorer->content,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                             explorer->draft_duration_minutes,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                             UI_LAB_ACTION_DECREMENT_DURATION,
                             UI_LAB_ACTION_INCREMENT_DURATION)) {
            return false;
        }
    }
    else {
        if (!make_editor_row(explorer, explorer->content,
                             ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET),
                             explorer->draft_target_c,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                             UI_LAB_ACTION_DECREMENT_TARGET,
                             UI_LAB_ACTION_INCREMENT_TARGET)) {
            return false;
        }
        if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL) {
            lv_obj_t *cooling_mode = create_transparent_container(explorer->content);
            if (cooling_mode == NULL) {
                return false;
            }
            lv_obj_set_width(cooling_mode, lv_pct(100));
            lv_obj_set_height(cooling_mode, 42);
            lv_obj_set_flex_flow(cooling_mode, LV_FLEX_FLOW_ROW);
            lv_obj_set_style_pad_column(cooling_mode, 7, LV_PART_MAIN);
            lv_obj_t *fastest = create_page_button(
                explorer, cooling_mode, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOL_FASTEST),
                explorer->draft_cooling_rate_enabled ? UI_LAB_SURFACE_RAISED : UI_LAB_DEEP_BLUE,
                UI_LAB_ACTION_SET_COOL_FASTEST, 0
            );
            lv_obj_t *controlled = create_page_button(
                explorer, cooling_mode, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOL_CONTROLLED),
                explorer->draft_cooling_rate_enabled ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE_RAISED,
                UI_LAB_ACTION_SET_COOL_CONTROLLED, 0
            );
            if (fastest == NULL || controlled == NULL) {
                return false;
            }
            lv_obj_set_flex_grow(fastest, 1);
            lv_obj_set_flex_grow(controlled, 1);
            lv_obj_set_height(fastest, 40);
            lv_obj_set_height(controlled, 40);
        }
        if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HEATING ||
            (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL &&
             explorer->draft_cooling_rate_enabled)) {
            if (!make_editor_row(explorer, explorer->content,
                                 explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL
                                     ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOL_CONTROLLED)
                                     : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE),
                                 explorer->draft_rate_tenths_c_per_minute,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_RATE_TENTHS),
                                 UI_LAB_ACTION_DECREMENT_RATE,
                                 UI_LAB_ACTION_INCREMENT_RATE)) {
                return false;
            }
        }
        if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HEATING) {
            if (!make_editor_row(explorer, explorer->content,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                                 explorer->draft_duration_minutes,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                                 UI_LAB_ACTION_DECREMENT_DURATION,
                                 UI_LAB_ACTION_INCREMENT_DURATION)) {
                return false;
            }
            lv_obj_t *relationship = create_transparent_container(explorer->content);
            if (relationship == NULL) {
                return false;
            }
            lv_obj_set_width(relationship, lv_pct(100));
            lv_obj_set_height(relationship, 24);
            char calculated[64];
            if (explorer->draft_heating_duration_drives_rate) {
                char rate[24];
                format_rate_tenths(rate, sizeof(rate), explorer->draft_rate_tenths_c_per_minute);
                (void)snprintf(calculated, sizeof(calculated),
                               ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                               ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CALCULATED_RATE), rate);
            }
            else {
                (void)snprintf(calculated, sizeof(calculated),
                               ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_DURATION_FORMAT),
                               ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CALCULATED_DURATION),
                               (long)explorer->draft_duration_minutes);
            }
            if (create_label(relationship, calculated, &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) {
                return false;
            }
        }
    }
    lv_obj_t *action_spacer = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (action_spacer == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_width(action_spacer, lv_pct(100));
    lv_obj_set_flex_grow(action_spacer, 1);
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 48);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    lv_obj_t *discard = create_page_button(explorer, actions,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                                           UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SHOW_DISCARD, 0);
    lv_obj_t *save = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT),
                                        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_SHOW_SAVE, 0);
    if (discard == NULL || save == NULL) {
        return false;
    }
    lv_obj_set_height(discard, 46);
    lv_obj_set_height(save, 46);
    lv_obj_set_flex_grow(discard, 1);
    lv_obj_set_flex_grow(save, 1);
    return true;
}

static bool build_device(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *columns = create_transparent_container(explorer->content);
    if (columns == NULL) {
        return false;
    }
    lv_obj_set_size(columns, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    lv_obj_t *visual_column = create_transparent_container(columns);
    lv_obj_t *status_column = create_transparent_container(columns);
    if (visual_column == NULL || status_column == NULL) {
        return false;
    }
    lv_obj_set_width(visual_column, lv_pct(52));
    lv_obj_set_height(visual_column, lv_pct(100));
    lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(status_column, lv_pct(48));
    lv_obj_set_height(status_column, lv_pct(100));
    lv_obj_set_flex_flow(status_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(status_column, 8, LV_PART_MAIN);
    if (!create_furnace_visual(explorer, visual_column, false)) {
        return false;
    }
    lv_obj_t *status = create_panel(status_column, UI_LAB_SURFACE);
    lv_obj_t *service = create_panel(status_column, UI_LAB_SURFACE_RAISED);
    if (status == NULL || service == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(status, 1);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(status, 5, LV_PART_MAIN);
    char current[32];
    char demand[32];
    char fan[32];
    format_observation_value(current, sizeof(current), explorer->state == NULL ? NULL :
                             &explorer->state->current_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    format_observation_value(demand, sizeof(demand), explorer->state == NULL ? NULL :
                             &explorer->state->heater_demand_percent,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_PERCENT);
    format_observation_value(fan, sizeof(fan), explorer->state == NULL ? NULL :
                             &explorer->state->fan_rpm,
                             FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT);
    char line[72];
    (void)snprintf(line, sizeof(line), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_TEMPERATURE), current);
    lv_obj_t *temperature = create_label(status, line, &lv_font_montserrat_14, UI_LAB_TEXT);
    (void)snprintf(line, sizeof(line), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HEATER_DEMAND), demand);
    lv_obj_t *heater = create_label(status, line, &lv_font_montserrat_14, UI_LAB_TEXT);
    (void)snprintf(line, sizeof(line), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAN_STATUS), fan);
    lv_obj_t *fan_status = create_label(status, line, &lv_font_montserrat_14, UI_LAB_TEXT);
    const char *door = explorer->state != NULL && explorer->state->door_closed.value
        ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED)
        : ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_OPEN);
    (void)snprintf(line, sizeof(line), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DOOR_STATUS), door);
    lv_obj_t *door_status = create_label(status, line, &lv_font_montserrat_14, UI_LAB_TEXT);
    if (temperature == NULL || heater == NULL || fan_status == NULL || door_status == NULL) {
        return false;
    }
    lv_obj_set_width(service, lv_pct(100));
    lv_obj_set_height(service, 112);
    lv_obj_set_flex_flow(service, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(service, 4, LV_PART_MAIN);
    lv_obj_t *service_title = create_label(service,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_WARNING),
                                           &lv_font_montserrat_16, UI_LAB_STALE);
    lv_obj_t *service_detail = create_label(service,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGEMENT_REQUIRED),
                                            &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *service_button = create_page_button(explorer, service,
                                                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_DETAILS),
                                                   UI_LAB_STALE, UI_LAB_ACTION_SHOW_SERVICE, 0);
    if (service_title == NULL || service_detail == NULL || service_button == NULL) {
        return false;
    }
    lv_obj_set_width(service_button, lv_pct(100));
    lv_obj_set_height(service_button, 38);
    return true;
}

static bool build_settings(furnace_hmi_ui_lab_explorer_t *explorer)
{
    (void)explorer;
    static const furnace_hmi_ui_string_id_t setting_ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_DISPLAY_THEME,
        FURNACE_HMI_UI_STRING_UI_LAB_UNITS_CELSIUS,
        FURNACE_HMI_UI_STRING_UI_LAB_LANGUAGE_ENGLISH,
        FURNACE_HMI_UI_STRING_UI_LAB_CONTROLLER_SETTINGS_PENDING,
    };
    for (size_t index = 0; index < sizeof(setting_ids) / sizeof(setting_ids[0]); ++index) {
        lv_obj_t *setting = create_panel(explorer->content,
                                         index < 3U ? UI_LAB_SURFACE : UI_LAB_SURFACE_RAISED);
        if (setting == NULL) {
            return false;
        }
        lv_obj_set_width(setting, lv_pct(100));
        lv_obj_set_height(setting, index < 3U ? 58 : 82);
        lv_obj_t *label = create_label(setting, ui_string(setting_ids[index]),
                                       &lv_font_montserrat_16, index == 3U ? UI_LAB_MUTED : UI_LAB_TEXT);
        if (label == NULL) {
            return false;
        }
        if (index == 3U) {
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, lv_pct(100));
        }
    }
    return true;
}

static bool rebuild_page(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->root == NULL) {
        return false;
    }
    clear_modal(explorer);
    if (explorer->content != NULL && lv_obj_is_valid(explorer->content)) {
        lv_obj_delete(explorer->content);
    }
    explorer->content = create_transparent_container(explorer->root);
    explorer->page_action_count = 0U;
    explorer->trajectory = NULL;
    explorer->planned_line = NULL;
    explorer->trajectory_minimum_label = NULL;
    explorer->trajectory_maximum_label = NULL;
    explorer->trajectory_time_axis_label = NULL;
    explorer->trajectory_end_time_label = NULL;
    explorer->trajectory_middle_temperature_label = NULL;
    explorer->trajectory_middle_time_label = NULL;
    memset(explorer->grid_lines, 0, sizeof(explorer->grid_lines));
    memset(explorer->trajectory_stage_nodes, 0, sizeof(explorer->trajectory_stage_nodes));
    memset(explorer->measured_segments, 0, sizeof(explorer->measured_segments));
    explorer->trajectory_uses_program_preview = false;
    explorer->home_temperature = NULL;
    explorer->home_target = NULL;
    explorer->home_stage = NULL;
    explorer->home_remaining = NULL;
    explorer->home_state_detail = NULL;
    if (explorer->content == NULL) {
        return false;
    }
    lv_obj_set_width(explorer->content, lv_pct(100));
    lv_obj_set_flex_grow(explorer->content, 1);
    lv_obj_set_flex_flow(explorer->content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(explorer->content, 7, LV_PART_MAIN);
    if (explorer->nav != NULL) {
        update_nav_selection(explorer, explorer->nav_selection_animation_pending);
        explorer->nav_selection_animation_pending = false;
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
            explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            lv_obj_add_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(explorer->nav, -1);
        }
    }

    if (explorer->header_title != NULL) {
        lv_label_set_text(explorer->header_title, page_title(explorer->page));
    }
    switch (explorer->page) {
    case FURNACE_HMI_UI_LAB_PAGE_HOME:
        return build_home(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING:
        return build_program_loading(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_RUNNING:
        return build_running(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAMS:
        return build_programs(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL:
        return build_program_detail(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES:
        return build_program_stages(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL:
        return build_stage_detail(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR:
        return build_stage_editor(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_DEVICE:
        return build_device(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_SETTINGS:
        return build_settings(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_COUNT:
        break;
    }
    return false;
}

static bool update_header(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->state_badge == NULL || explorer->service_badge == NULL) {
        return false;
    }
    const lv_color_t accent = state_color(explorer->state);
    lv_label_set_text(explorer->state_badge, state_text(explorer->state));
    lv_obj_set_style_text_color(explorer->state_badge, accent, LV_PART_MAIN);
    lv_label_set_text(explorer->service_badge,
                      ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_CONNECTION_CURRENT));
    lv_obj_set_style_text_color(explorer->service_badge, UI_LAB_MUTED, LV_PART_MAIN);
    return true;
}

static void apply_action(
    furnace_hmi_ui_lab_explorer_t *explorer,
    ui_lab_action_t action,
    int value
)
{
    if (explorer == NULL) {
        return;
    }
    switch (action) {
    case UI_LAB_ACTION_HOME:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_HOME);
        break;
    case UI_LAB_ACTION_PROGRAMS:
    case UI_LAB_ACTION_BACK_PROGRAMS:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAMS);
        break;
    case UI_LAB_ACTION_PROGRAM_LOADING:
        if (value >= 0 && value < (int)(sizeof(program_fixtures) / sizeof(program_fixtures[0]))) {
            explorer->selected_program = (uint8_t)value;
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING);
        break;
    case UI_LAB_ACTION_RUNNING:
        explorer->running_view_mode = 1U;
        record_recent_program(explorer, explorer->selected_program);
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_RUNNING);
        break;
    case UI_LAB_ACTION_PROGRAM_DETAIL:
        if (value >= 0 && value < (int)(sizeof(program_fixtures) / sizeof(program_fixtures[0]))) {
            explorer->selected_program = (uint8_t)value;
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL);
        break;
    case UI_LAB_ACTION_PROGRAM_STAGES:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES);
        break;
    case UI_LAB_ACTION_STAGE_DETAIL:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL);
        break;
    case UI_LAB_ACTION_STAGE_EDITOR:
        if (value >= 0 && value < (int)(sizeof(stage_fixtures) / sizeof(stage_fixtures[0]))) {
            explorer->selected_stage = (uint8_t)value;
            explorer->draft_target_c = stage_fixtures[value].target_c;
            explorer->draft_rate_tenths_c_per_minute =
                stage_fixtures[value].delta_c_per_minute * 10;
            explorer->draft_duration_minutes = stage_fixtures[value].duration_minutes;
            explorer->draft_stage_kind = (uint8_t)fixture_stage_kind(&stage_fixtures[value]);
            explorer->draft_cooling_rate_enabled = stage_fixtures[value].delta_c_per_minute != 0;
            explorer->draft_heating_duration_drives_rate = false;
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR);
        break;
    case UI_LAB_ACTION_DEVICE:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        break;
    case UI_LAB_ACTION_SETTINGS:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS);
        break;
    case UI_LAB_ACTION_BACK_PROGRAM_DETAIL:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL);
        break;
    case UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE:
        if (explorer->program_page > 0U) {
            explorer->program_page--;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_NEXT_PROGRAM_PAGE:
        if ((size_t)explorer->program_page + 1U <
            (program_fixture_count() + 5U) / 6U) {
            explorer->program_page++;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_PREVIOUS_STAGE_PAGE:
        if (explorer->program_stage_page > 0U) {
            explorer->program_stage_page--;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_NEXT_STAGE_PAGE:
        if ((size_t)explorer->program_stage_page + 1U <
            ((sizeof(stage_fixtures) / sizeof(stage_fixtures[0])) + 8U) / 9U) {
            explorer->program_stage_page++;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_PREVIOUS_STAGE:
        if (explorer->selected_stage > 0U) {
            explorer->selected_stage--;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_NEXT_STAGE:
        if (explorer->selected_stage + 1U < sizeof(stage_fixtures) / sizeof(stage_fixtures[0])) {
            explorer->selected_stage++;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SHOW_NOTICES:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NOTIFICATIONS),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        break;
    case UI_LAB_ACTION_SHOW_SERVICE:
        (void)furnace_hmi_ui_lab_explorer_show_service_warning(explorer);
        break;
    case UI_LAB_ACTION_SHOW_DELETE:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE), UI_LAB_FAULT);
        break;
    case UI_LAB_ACTION_SHOW_SAVE:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT), UI_LAB_DEEP_BLUE);
        break;
    case UI_LAB_ACTION_SHOW_DISCARD:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_FAULT);
        break;
    case UI_LAB_ACTION_SHOW_RUN_DETAILS:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_DETAILS),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        break;
    case UI_LAB_ACTION_ADD_STAGE:
    case UI_LAB_ACTION_NEW_DRAFT:
        explorer->selected_stage = 0U;
        explorer->draft_target_c = stage_fixtures[0].target_c;
        explorer->draft_rate_tenths_c_per_minute = stage_fixtures[0].delta_c_per_minute * 10;
        explorer->draft_duration_minutes = stage_fixtures[0].duration_minutes;
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
        explorer->draft_cooling_rate_enabled = false;
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR);
        break;
    case UI_LAB_ACTION_PAUSE_PREVIEW:
    case UI_LAB_ACTION_RESUME_PREVIEW:
    case UI_LAB_ACTION_STOP_PREVIEW:
        (void)open_dialog(explorer, state_text(explorer->state),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, state_color(explorer->state));
        break;
    case UI_LAB_ACTION_DECREMENT_TARGET:
        explorer->draft_target_c -= 1;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_INCREMENT_TARGET:
        explorer->draft_target_c += 1;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DECREMENT_RATE:
        explorer->draft_rate_tenths_c_per_minute -= 1;
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_INCREMENT_RATE:
        explorer->draft_rate_tenths_c_per_minute += 1;
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DECREMENT_DURATION:
        if (explorer->draft_duration_minutes > 1) {
            explorer->draft_duration_minutes -= 1;
        }
        explorer->draft_heating_duration_drives_rate = true;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_INCREMENT_DURATION:
        explorer->draft_duration_minutes += 1;
        explorer->draft_heating_duration_drives_rate = true;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SET_DRAFT_HEATING:
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
        if (explorer->draft_rate_tenths_c_per_minute <= 0) {
            explorer->draft_rate_tenths_c_per_minute = 10;
        }
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SET_DRAFT_HOLD:
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HOLD;
        explorer->draft_rate_tenths_c_per_minute = 0;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SET_DRAFT_COOL:
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_COOL;
        explorer->draft_cooling_rate_enabled = false;
        if (explorer->draft_rate_tenths_c_per_minute >= 0) {
            explorer->draft_rate_tenths_c_per_minute = -10;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SET_COOL_FASTEST:
        explorer->draft_cooling_rate_enabled = false;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SET_COOL_CONTROLLED:
        explorer->draft_cooling_rate_enabled = true;
        if (explorer->draft_rate_tenths_c_per_minute >= 0) {
            explorer->draft_rate_tenths_c_per_minute = -10;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_TOGGLE_FAVOURITE:
        if (!toggle_program_favourite(explorer, (uint8_t)value)) {
            (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAVOURITES),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAVOURITE_LIMIT),
                              UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        }
        else {
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SHOW_GRAPH_VIEW:
        explorer->running_view_mode = 0U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SHOW_SPLIT_VIEW:
        explorer->running_view_mode = 1U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SHOW_VISUAL_VIEW:
        explorer->running_view_mode = 2U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_CLOSE_MODAL:
        clear_modal(explorer);
        break;
    case UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONTROLLER_OWNED),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REQUEST_PREVIEW_DETAIL),
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        break;
    }
}

static void deferred_action(void *user_data)
{
    furnace_hmi_ui_lab_explorer_t *explorer = user_data;
    if (explorer == NULL || explorer->root == NULL) {
        return;
    }
    explorer->action_pending = false;
    apply_action(explorer, (ui_lab_action_t)explorer->pending_action,
                 explorer->pending_action_value);
}

static void action_callback(lv_event_t *event)
{
    const furnace_hmi_ui_lab_action_slot_t *slot = lv_event_get_user_data(event);
    furnace_hmi_ui_lab_explorer_t *explorer = action_owner(event);
    if (slot == NULL || explorer == NULL) {
        return;
    }
    explorer->pending_action = slot->action;
    explorer->pending_action_value = slot->value;
    if (!explorer->action_pending) {
        explorer->action_pending = true;
        lv_async_call(deferred_action, explorer);
    }
}

bool furnace_hmi_ui_lab_explorer_create(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state
)
{
    if (explorer == NULL || parent == NULL || explorer->root != NULL || state == NULL ||
        !furnace_hmi_dashboard_state_is_well_formed(state)) {
        return false;
    }
    memset(explorer, 0, sizeof(*explorer));
    explorer->state = state;
    explorer->page = FURNACE_HMI_UI_LAB_PAGE_HOME;
    explorer->selected_program = 0U;
    explorer->selected_stage = 2U;
    explorer->draft_target_c = stage_fixtures[2].target_c;
    explorer->draft_rate_tenths_c_per_minute = stage_fixtures[2].delta_c_per_minute * 10;
    explorer->draft_duration_minutes = stage_fixtures[2].duration_minutes;
    explorer->draft_stage_kind = (uint8_t)fixture_stage_kind(&stage_fixtures[2]);
    explorer->quick_launch_recent[0] = 3U;
    explorer->quick_launch_recent[1] = 1U;
    explorer->quick_launch_recent[2] = 2U;
    explorer->recent_count = FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS;

    explorer->root = lv_obj_create(parent);
    if (explorer->root == NULL) {
        return false;
    }
    lv_obj_set_size(explorer->root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(explorer->root, UI_LAB_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->root, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->root, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(explorer->root, LV_FLEX_FLOW_COLUMN);
    remove_scroll(explorer->root);

    lv_obj_t *header = create_panel(explorer->root, UI_LAB_SURFACE);
    if (header == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, 42);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(header, 6, LV_PART_MAIN);
    explorer->header_title = create_label(header, page_title(explorer->page),
                                          &lv_font_montserrat_20, UI_LAB_TEXT);
    explorer->state_badge = create_label(header, "", &lv_font_montserrat_14, UI_LAB_RUNNING);
    explorer->service_badge = create_label(header, "", &lv_font_montserrat_14, UI_LAB_STALE);
    lv_obj_t *notices = create_action_button(
        header,
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NOTIFICATIONS),
        UI_LAB_DEEP_BLUE,
        &explorer->header_actions[0], explorer, UI_LAB_ACTION_SHOW_NOTICES, 0
    );
    if (explorer->header_title == NULL || explorer->state_badge == NULL ||
        explorer->service_badge == NULL || notices == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_flex_grow(explorer->header_title, 1);
    lv_obj_set_width(notices, 70);
    lv_obj_set_height(notices, 28);

    explorer->nav = create_panel(explorer->root, UI_LAB_SURFACE_RAISED);
    if (explorer->nav == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_width(explorer->nav, lv_pct(100));
    lv_obj_set_height(explorer->nav, 48);
    lv_obj_set_flex_flow(explorer->nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_width(explorer->nav, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->nav, 9, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->nav, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_column(explorer->nav, 0, LV_PART_MAIN);
    const furnace_hmi_ui_string_id_t nav_texts[] = {
        FURNACE_HMI_UI_STRING_DASHBOARD_HOME,
        FURNACE_HMI_UI_STRING_DASHBOARD_PROGRAMS,
        FURNACE_HMI_UI_STRING_DASHBOARD_DEVICE,
        FURNACE_HMI_UI_STRING_DASHBOARD_SETTINGS,
    };
    const ui_lab_action_t nav_actions[] = {
        UI_LAB_ACTION_HOME, UI_LAB_ACTION_PROGRAMS, UI_LAB_ACTION_DEVICE, UI_LAB_ACTION_SETTINGS,
    };
    for (size_t index = 0; index < sizeof(nav_texts) / sizeof(nav_texts[0]); ++index) {
        lv_obj_t *button = create_action_button(
            explorer->nav, ui_string(nav_texts[index]), UI_LAB_DEEP_BLUE, &explorer->nav_actions[index], explorer,
            nav_actions[index], 0
        );
        if (button == NULL) {
            furnace_hmi_ui_lab_explorer_destroy(explorer);
            return false;
        }
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, 44);
    }
    update_nav_selection(explorer, false);
    if (!update_header(explorer) || !rebuild_page(explorer)) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    return true;
}

bool furnace_hmi_ui_lab_explorer_update(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const furnace_hmi_dashboard_state_t *state
)
{
    if (explorer == NULL || explorer->root == NULL || state == NULL ||
        !furnace_hmi_dashboard_state_is_well_formed(state)) {
        return false;
    }
    const bool availability_changed = explorer->state == NULL ||
        explorer->state->availability != state->availability;
    explorer->state = state;
    if (!update_header(explorer)) {
        return false;
    }
    if (explorer->modal != NULL) {
        return true;
    }
    if (availability_changed && (explorer->page == FURNACE_HMI_UI_LAB_PAGE_HOME ||
                                 explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING)) {
        return rebuild_page(explorer);
    }
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_HOME ||
        explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
        return rebuild_page(explorer);
    }
    return explorer->trajectory == NULL || update_trajectory(explorer);
}

bool furnace_hmi_ui_lab_explorer_navigate(
    furnace_hmi_ui_lab_explorer_t *explorer,
    furnace_hmi_ui_lab_page_t page
)
{
    if (explorer == NULL || explorer->root == NULL || page >= FURNACE_HMI_UI_LAB_PAGE_COUNT) {
        return false;
    }
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING &&
        page != FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
        return false;
    }
    if (explorer->page != page) {
        explorer->trajectory_domain_initialized = false;
        explorer->nav_selection_animation_pending =
            nav_destination_for_page(explorer->page) != nav_destination_for_page(page);
    }
    explorer->page = page;
    return rebuild_page(explorer);
}

bool furnace_hmi_ui_lab_explorer_show_service_warning(
    furnace_hmi_ui_lab_explorer_t *explorer
)
{
    if (explorer == NULL || explorer->root == NULL) {
        return false;
    }
    char detail[256];
    if (state_is_current(explorer->state)) {
        char current[32];
        char due[32];
        char forecast[32];
        format_observation_value(current, sizeof(current), &explorer->state->service_time_hours,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_HOURS);
        format_observation_value(due, sizeof(due), &explorer->state->service_due_at_hours,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_HOURS);
        format_observation_value(forecast, sizeof(forecast),
                                 &explorer->state->service_forecast_completion_hours,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_HOURS);
        (void)snprintf(detail, sizeof(detail),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_DIALOG_FORMAT),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_DIALOG_DETAIL),
                       ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_SERVICE), current,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_DUE), due,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FORECAST_COMPLETION), forecast,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGEMENT_REQUIRED));
    }
    else {
        (void)snprintf(detail, sizeof(detail), "%s",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NO_CURRENT_SNAPSHOT));
    }
    return open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_WARNING), detail,
                       UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGE_PREVIEW), UI_LAB_STALE);
}

void furnace_hmi_ui_lab_explorer_destroy(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL) {
        return;
    }
    if (explorer->root != NULL && lv_obj_is_valid(explorer->root)) {
        lv_obj_delete(explorer->root);
    }
    memset(explorer, 0, sizeof(*explorer));
}
