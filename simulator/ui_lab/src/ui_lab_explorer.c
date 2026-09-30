/* SPDX-License-Identifier: Apache-2.0 */

#include <inttypes.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_lab_explorer.h"

#include <furnace_hmi/ui_strings.h>

#define UI_LAB_BACKGROUND lv_color_hex(0x171B1E)
#define UI_LAB_SURFACE lv_color_hex(0x242A2E)
#define UI_LAB_SURFACE_RAISED lv_color_hex(0x30383D)
#define UI_LAB_BORDER lv_color_hex(0x465159)
#define UI_LAB_DIVIDER lv_color_hex(0x3A454C)
#define UI_LAB_TEXT lv_color_hex(0xF1F3F2)
#define UI_LAB_MUTED lv_color_hex(0xBBC3C7)
#define UI_LAB_DEEP_BLUE lv_color_hex(0xB87922)
#define UI_LAB_PRIMARY UI_LAB_DEEP_BLUE
#define UI_LAB_TIME_BORDER lv_color_hex(0x467A98)
#define UI_LAB_NAV_GLOW lv_color_hex(0x4A86A8)
#define UI_LAB_PLANNED lv_color_hex(0x9BA5AB)
#define UI_LAB_COOLING_ESTIMATE lv_color_hex(0x9CC9A9)
#define UI_LAB_COOLING_DASH 6.0
#define UI_LAB_COOLING_GAP 4.0
#define UI_LAB_MEASURED lv_color_hex(0xF6C453)
#define UI_LAB_RUNNING lv_color_hex(0xE8A04B)
#define UI_LAB_PAUSED lv_color_hex(0xA77BD1)
#define UI_LAB_MANUAL lv_color_hex(0x42B6A3)
#define UI_LAB_IDLE lv_color_hex(0x7D94A0)
#define UI_LAB_FAULT lv_color_hex(0xE76562)
#define UI_LAB_STALE lv_color_hex(0xD1A45B)
#define UI_LAB_UNAVAILABLE lv_color_hex(0x7B858A)
#define UI_LAB_BUTTON_MIN_HEIGHT 56
#define UI_LAB_RADIUS 6
#define UI_LAB_SPACE 8
#define UI_LAB_PANEL_PADDING 12
#define UI_LAB_GRAPH lv_color_hex(0x20272B)
#define UI_LAB_PROGRAM_BORDER lv_color_hex(0x3B454B)
#define UI_LAB_PROGRAM_DELETE_TEXT lv_color_hex(0xFFC2B9)
#define UI_LAB_PROGRAM_DELETE_BACKGROUND lv_color_hex(0x4D302E)
#define UI_LAB_PROGRAM_DELETE_BORDER lv_color_hex(0x78504B)
#define UI_LAB_PROGRAM_STAGE_ACTION_TEXT lv_color_hex(0xF3D298)
#define UI_LAB_GRAPH_LEFT 68
#define UI_LAB_GRAPH_RIGHT 8
#define UI_LAB_GRAPH_TOP 40
#define UI_LAB_GRAPH_BOTTOM 30
#define UI_LAB_MAX_PROGRAM_TEMPERATURE_C 180
#define UI_LAB_MAX_PROGRAM_RATE_TENTHS 30
/* Touch keyboards: keys fill their row height but never drop below the touch minimum. */
#define UI_LAB_KEY_MIN_HEIGHT 48
/* Numeric keypad panel width: narrower, taller keys instead of wide slabs. */
#define UI_LAB_NUMERIC_KEYPAD_WIDTH 460

static const char keyboard_letters[] = {
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0
};
static const char keyboard_letters_middle[] = {
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '-', 0
};
static const char keyboard_letters_bottom[] = {
    'z', 'x', 'c', 'v', 'b', 'n', 'm', 0
};
static const char keyboard_numbers[] = {
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0
};

typedef enum {
    UI_LAB_ACTION_HOME = 0,
    UI_LAB_ACTION_MANUAL,
    UI_LAB_ACTION_PROGRAM_EDITOR,
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
    UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR,
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
    UI_LAB_ACTION_CONFIRM_DISCARD,
    UI_LAB_ACTION_SHOW_RENAME,
    UI_LAB_ACTION_SHOW_STAGE_RENAME,
    UI_LAB_ACTION_CONFIRM_SAVE_ALL,
    UI_LAB_ACTION_SETTINGS_TAB,
    UI_LAB_ACTION_SETTINGS_DISPLAY_MORE,
    UI_LAB_ACTION_SETTINGS_EDIT_VALUE,
    UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION,
    UI_LAB_ACTION_SETTINGS_CYCLE_VIEW,
    UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT,
    UI_LAB_ACTION_SETTINGS_SHOW_DATE_TIME,
    UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP,
    UI_LAB_ACTION_SETTINGS_SAVE,
    UI_LAB_ACTION_SETTINGS_DISCARD,
    UI_LAB_ACTION_SETTINGS_UNSAVED_STAY,
    UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB,
    UI_LAB_ACTION_SETTINGS_SAVE_TO_TAB,
    UI_LAB_ACTION_SETTINGS_EDITOR_CANCEL,
    UI_LAB_ACTION_SETTINGS_EDITOR_APPLY,
    UI_LAB_ACTION_SETTINGS_EDITOR_DECREMENT,
    UI_LAB_ACTION_SETTINGS_EDITOR_INCREMENT,
    UI_LAB_ACTION_SETTINGS_SHOW_RESET,
    UI_LAB_ACTION_SETTINGS_RESET_SCOPE,
    UI_LAB_ACTION_SETTINGS_RESET_CONFIRM,
    UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS,
    UI_LAB_ACTION_SETTINGS_DELETE_SCOPE,
    UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM,
    UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY,
    UI_LAB_ACTION_SETTINGS_RESTART_DISPLAY_CONFIRM,
    UI_LAB_ACTION_SETTINGS_ASSESS_RESTART,
    UI_LAB_ACTION_SETTINGS_RESTART_CONFIRM,
    UI_LAB_ACTION_STAGE_EDIT_VALUE,
    UI_LAB_ACTION_DEVICE_TAB,
    UI_LAB_ACTION_DEVICE_FILTER,
    UI_LAB_ACTION_DEVICE_PREVIOUS,
    UI_LAB_ACTION_DEVICE_NEXT,
    UI_LAB_ACTION_DEVICE_COMPONENT,
    UI_LAB_ACTION_DEVICE_BACK,
    UI_LAB_ACTION_DEVICE_PART_INFO,
    UI_LAB_ACTION_DEVICE_TASK,
    UI_LAB_ACTION_DEVICE_NOTICE,
    UI_LAB_ACTION_DEVICE_INFO,
    UI_LAB_ACTION_DEVICE_HISTORY,
    UI_LAB_ACTION_DEVICE_LOG_EVENT,
    UI_LAB_ACTION_MANUAL_START,
    UI_LAB_ACTION_MANUAL_STOP,
    UI_LAB_ACTION_MANUAL_PAUSE,
    UI_LAB_ACTION_MANUAL_EDIT_TARGET,
    UI_LAB_ACTION_MANUAL_EDIT_RATE,
    UI_LAB_ACTION_MANUAL_DECREMENT_TARGET,
    UI_LAB_ACTION_MANUAL_INCREMENT_TARGET,
    UI_LAB_ACTION_MANUAL_DECREMENT_RATE,
    UI_LAB_ACTION_MANUAL_INCREMENT_RATE,
    UI_LAB_ACTION_TOGGLE_LIGHT,
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
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_1, 5U, 65U, 160 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_BISQUE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_EARTHENWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_2,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_2, 6U, 214U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_COOLING_TEST,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 4U, 95U, 140 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_GLAZE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLAZED_CERAMIC,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 7U, 280U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ANNEAL,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLASS,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_5,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_5, 4U, 103U, 120 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_QUICK_BISQUE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_EARTHENWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_1,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_2,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_1, 5U, 163U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_HIGH_FIRE_GLAZE,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLAZED_CERAMIC,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 6U, 237U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_KILN_DRY,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
       FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 3U, 84U, 140 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_STONEWARE_TEST,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_STONEWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 5U, 178U, 180 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_GLASS_ANNEAL,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_GLASS,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_5,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_5, 4U, 159U, 120 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_LONG_COOLING,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_TEST_COUPON,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_3,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_3, 3U, 92U, 120 },
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_PRODUCTION,
      FURNACE_HMI_UI_STRING_UI_LAB_MATERIAL_STONEWARE,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DURATION_2,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_MAXIMUM_4,
      FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ENERGY_4, 8U, 337U, 180 },
};

static const ui_lab_stage_fixture_t stage_fixtures[] = {
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_WARM_UP,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 34, 30, 3 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_DRYING_HOLD,
       FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD, 34, 0, 9 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING_850,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 160, 30, 42 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_PEAK_HOLD,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD, 160, 0, 11 },
    { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CONTROLLED_COOLING,
      FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COOLING, 47, -30, 35 },
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

static const char *ui_string(furnace_hmi_ui_string_id_t id);

static void copy_text_bounded(char *destination, size_t capacity, const char *source)
{
    if (destination == NULL || capacity == 0U) {
        return;
    }
    const char *value = source == NULL ? "" : source;
    size_t length = strlen(value);
    if (length >= capacity) {
        length = capacity - 1U;
    }
    memcpy(destination, value, length);
    destination[length] = 0;
}

static const char *program_name_text(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    size_t index
)
{
    if (explorer != NULL && index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS &&
        explorer->program_name_overrides[index][0] != 0) {
        return explorer->program_name_overrides[index];
    }
    return ui_string(program_fixtures[index % program_fixture_count()].name);
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

static ui_lab_stage_fixture_t program_stage(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index);
static size_t program_authored_stage_count(const ui_lab_program_fixture_t *program);

static void clear_staged_session(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memset(explorer->staged_valid, 0, sizeof(explorer->staged_valid));
    memset(explorer->staged_name_valid, 0, sizeof(explorer->staged_name_valid));
    memset(explorer->staged_names, 0, sizeof(explorer->staged_names));
    explorer->staged_program = UINT8_MAX;
    explorer->staged_stage_count = 0U;
    explorer->staged_session_active = false;
    explorer->program_editor_active = false;
    explorer->program_editor_new = false;
    explorer->program_editor_dirty = false;
    explorer->program_editor_name[0] = '\0';
}

static void store_selected_stage_draft(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->selected_stage >= FURNACE_HMI_UI_LAB_MAX_STAGES) {
        return;
    }
    const uint8_t index = explorer->selected_stage;
    explorer->staged_targets[index] = explorer->draft_target_c;
    explorer->staged_rates[index] = explorer->draft_rate_tenths_c_per_minute;
    explorer->staged_durations[index] = explorer->draft_duration_minutes;
    explorer->staged_kinds[index] = explorer->draft_stage_kind;
    explorer->staged_cooling_rate_enabled[index] = explorer->draft_cooling_rate_enabled;
    explorer->staged_program = explorer->selected_program;
    explorer->staged_valid[index] = true;
}

static void load_stage_draft(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t index)
{
    const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
    explorer->selected_stage = index;
    if (explorer->staged_valid[index]) {
        explorer->draft_target_c = explorer->staged_targets[index];
        explorer->draft_rate_tenths_c_per_minute = explorer->staged_rates[index];
        explorer->draft_duration_minutes = explorer->staged_durations[index];
        explorer->draft_stage_kind = explorer->staged_kinds[index];
        explorer->draft_cooling_rate_enabled = explorer->staged_cooling_rate_enabled[index];
    } else {
        explorer->draft_target_c = stage.target_c;
        explorer->draft_rate_tenths_c_per_minute = stage.delta_c_per_minute;
        explorer->draft_duration_minutes = stage.duration_minutes;
        explorer->draft_stage_kind = (uint8_t)fixture_stage_kind(&stage);
        explorer->draft_cooling_rate_enabled = stage.delta_c_per_minute != 0;
    }
    /* Cooling is controller-owned and is never an authored stage. Normalize
     * legacy fixture/draft data before it reaches the editor. */
    if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL) {
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
        explorer->draft_cooling_rate_enabled = false;
        explorer->draft_rate_tenths_c_per_minute = 10;
    }
    explorer->draft_heating_duration_drives_rate = false;
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
    const int32_t start_temperature = explorer->selected_stage == 0 ? 25 : program_stage(explorer, explorer->selected_stage - 1U).target_c;
    const int64_t change = explorer->draft_target_c > start_temperature
        ? (int64_t)explorer->draft_target_c - start_temperature : (int64_t)start_temperature - explorer->draft_target_c;
    if (explorer->draft_heating_duration_drives_rate) {
        const int32_t duration = explorer->draft_duration_minutes > 0
            ? explorer->draft_duration_minutes : 1;
        int64_t result = (change * 10 + duration - 1) / duration;
        explorer->draft_rate_tenths_c_per_minute = result > INT32_MAX ? INT32_MAX : (int32_t)(result > 0 ? result : 1);
    }
    else {
        int64_t result = (change * 10 + rate - 1) / rate;
        explorer->draft_duration_minutes = result > INT32_MAX ? INT32_MAX : (int32_t)(result > 0 ? result : 1);
    }
}

/* Bounded host illustration: one source for summaries, nodes and stage cards.
 * These local estimates neither validate nor persist a controller program. */
static ui_lab_stage_fixture_t program_stage(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    const ui_lab_program_fixture_t *program = &program_fixtures[explorer->selected_program % program_fixture_count()];
    size_t count = program_authored_stage_count(program);
    if (explorer->staged_session_active &&
        explorer->staged_program == explorer->selected_program &&
        explorer->staged_stage_count > 0U) {
        count = explorer->staged_stage_count;
    }
    if (count == 0U) {
        return (ui_lab_stage_fixture_t) {
            FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING,
            FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 120, 10, 1
        };
    }
    if (index >= count) { index = count - 1U; }
    ui_lab_stage_fixture_t stage = stage_fixtures[index % 4U];
    if (explorer->selected_program == 0U && index < 4U) {
        stage = stage_fixtures[index];
    } else {
        const int32_t maximum = program->maximum_temperature_c > UI_LAB_MAX_PROGRAM_TEMPERATURE_C
            ? UI_LAB_MAX_PROGRAM_TEMPERATURE_C : program->maximum_temperature_c;
        stage.target_c = index < 2U && count > 3U ? maximum / 4 : maximum;
        stage.duration_minutes = program->duration_minutes / (int32_t)count +
            (index < (size_t)(program->duration_minutes % (int32_t)count) ? 1 : 0);
        const int32_t previous = index == 0 ? 25 :
            (index <= 2U && count > 3U ? maximum / 4 : maximum);
        stage.kind = stage.target_c == previous ? FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD :
            FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING;
        stage.name = stage.kind;
        stage.delta_c_per_minute = (stage.target_c - previous) * 10 / stage.duration_minutes;
        if (stage.delta_c_per_minute > UI_LAB_MAX_PROGRAM_RATE_TENTHS) {
            stage.delta_c_per_minute = UI_LAB_MAX_PROGRAM_RATE_TENTHS;
        }
    }
    if (explorer->staged_program == explorer->selected_program &&
        index < FURNACE_HMI_UI_LAB_MAX_STAGES && explorer->staged_valid[index]) {
        stage.target_c = explorer->staged_targets[index];
        stage.delta_c_per_minute = explorer->staged_rates[index];
        stage.duration_minutes = explorer->staged_durations[index];
        stage.kind = explorer->staged_kinds[index] == UI_LAB_DRAFT_STAGE_HOLD ? FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD :
            FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING;
        stage.name = stage.kind;
    }
    return stage;
}

static const char *stage_name_text(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    size_t index
)
{
    if (explorer != NULL && index < FURNACE_HMI_UI_LAB_MAX_STAGES &&
        explorer->staged_program == explorer->selected_program &&
        explorer->staged_name_valid[index]) {
        return explorer->staged_names[index];
    }
    const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
    return ui_string(stage.name);
}
static const char *program_duration(const ui_lab_program_fixture_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES), (long)program->duration_minutes);
    return text;
}

static const char *program_maximum(const ui_lab_program_fixture_t *program)
{
    static char text[32];
    const int32_t maximum = program != NULL && program->maximum_temperature_c < UI_LAB_MAX_PROGRAM_TEMPERATURE_C
        ? program->maximum_temperature_c : UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C), (long)program->maximum_temperature_c);
    if (program != NULL) {
        snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C), (long)maximum);
    }
    return text;
}

static size_t program_authored_stage_count(const ui_lab_program_fixture_t *program)
{
    return program != NULL && program->stage_count > 0U ? program->stage_count - 1U : 0U;
}

static size_t editor_stage_count(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer != NULL && explorer->program_editor_active &&
        explorer->staged_program == explorer->selected_program) {
        return explorer->staged_stage_count;
    }
    return program_authored_stage_count(
        &program_fixtures[explorer->selected_program % program_fixture_count()]);
}

static int32_t program_cooling_minutes(const ui_lab_program_fixture_t *program)
{
    const int32_t maximum = program == NULL
        ? 30
        : (program->maximum_temperature_c > UI_LAB_MAX_PROGRAM_TEMPERATURE_C
            ? UI_LAB_MAX_PROGRAM_TEMPERATURE_C
            : program->maximum_temperature_c);
    const int32_t delta = maximum > 30 ? maximum - 30 : 0;
    const int32_t estimate = (delta * 5 + 5) / 6;
    return estimate < 20 ? 20 : estimate;
}

static const char *program_cooling(const ui_lab_program_fixture_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
             (long)program_cooling_minutes(program));
    return text;
}

static const char *program_total(const ui_lab_program_fixture_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
             (long)(program->duration_minutes + program_cooling_minutes(program)));
    return text;
}

static void prepare_program_preview(furnace_hmi_ui_lab_explorer_t *explorer, const ui_lab_program_fixture_t *program)
{
    furnace_hmi_dashboard_state_initialize(&explorer->preview_graph_state);
    explorer->preview_graph_state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    const size_t authored = program_authored_stage_count(program);
    explorer->preview_graph_state.graph_sample_count = authored + 2U;
    explorer->preview_graph_state.graph_samples[0].planned_temperature_c = 25;
    uint32_t minutes = 0;
    for (size_t i = 0; i < authored; ++i) {
        ui_lab_stage_fixture_t stage = program_stage(explorer, i);
        minutes += (uint32_t)stage.duration_minutes;
        explorer->preview_graph_state.graph_samples[i + 1U].elapsed_seconds = minutes * 60U;
        explorer->preview_graph_state.graph_samples[i + 1U].planned_temperature_c = stage.target_c;
    }
    /* A preview shows only the start of automatic cooling (at most a quarter
     * of the curing time) so the authored stages keep most of the width. The
     * segment follows the estimated cooling slope toward 30 C. */
    const uint32_t cooling = (uint32_t)program_cooling_minutes(program);
    uint32_t shown = minutes / 4U;
    if (shown < 1U) { shown = 1U; }
    if (shown > cooling) { shown = cooling; }
    const int32_t cooling_from = explorer->preview_graph_state.graph_samples[authored].planned_temperature_c;
    minutes += shown;
    explorer->preview_graph_state.graph_samples[authored + 1U].elapsed_seconds = minutes * 60U;
    explorer->preview_graph_state.graph_samples[authored + 1U].planned_temperature_c =
        cooling_from + (int32_t)(((int64_t)30 - cooling_from) * (int64_t)shown / (int64_t)cooling);
    explorer->trajectory_uses_program_preview = true;
    explorer->preview_graph_state.graph_samples[authored + 1U].planned_cooling = true;
    explorer->trajectory_domain_initialized = false;
}

static const char *ui_string(furnace_hmi_ui_string_id_t id)
{
    const char *text = furnace_hmi_ui_string(id);
    return text == NULL ? "" : text;
}

#define UI_LAB_DEVICE_CATALOGUE_MAX_LINES 64U
#define UI_LAB_DEVICE_CATALOGUE_LINE_CAPACITY 1024U

static void trim_device_catalogue_field(char *text)
{
    char *start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1U);
    }
    size_t length = strlen(text);
    while (length > 0U && isspace((unsigned char)text[length - 1U])) {
        text[--length] = '\0';
    }
}

static bool next_device_catalogue_field(char **cursor, char *target, size_t capacity)
{
    if (cursor == NULL || *cursor == NULL || target == NULL || capacity == 0U) {
        return false;
    }
    char *field = *cursor;
    char *separator = strchr(field, '|');
    if (separator == NULL) {
        *cursor = NULL;
    }
    else {
        *separator = '\0';
        *cursor = separator + 1;
    }
    trim_device_catalogue_field(field);
    const size_t length = strlen(field);
    if (length == 0U || length >= capacity) {
        return false;
    }
    memcpy(target, field, length + 1U);
    return true;
}

static bool parse_device_task_state(
    const char *text,
    furnace_hmi_ui_lab_device_task_state_t *state
)
{
    if (text == NULL || state == NULL) {
        return false;
    }
    if (strcmp(text, "ok") == 0) {
        *state = FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE;
    }
    else if (strcmp(text, "soon") == 0) {
        *state = FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON;
    }
    else if (strcmp(text, "due") == 0) {
        *state = FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE;
    }
    else if (strcmp(text, "condition") == 0) {
        *state = FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION;
    }
    else if (strcmp(text, "unknown") == 0) {
        *state = FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN;
    }
    else {
        return false;
    }
    return true;
}

bool furnace_hmi_ui_lab_device_catalogue_load(
    furnace_hmi_ui_lab_device_catalogue_t *catalogue,
    const char *path
)
{
    if (catalogue == NULL || path == NULL || path[0] == '\0') {
        return false;
    }
    memset(catalogue, 0, sizeof(*catalogue));
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    bool valid = false;
    bool revision_seen = false;
    char line[UI_LAB_DEVICE_CATALOGUE_LINE_CAPACITY];
    unsigned line_count = 0U;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (++line_count > UI_LAB_DEVICE_CATALOGUE_MAX_LINES ||
            (strchr(line, '\n') == NULL && !feof(file))) {
            goto close_file;
        }
        trim_device_catalogue_field(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        char *cursor = line;
        char record[16];
        if (!next_device_catalogue_field(&cursor, record, sizeof(record))) {
            goto close_file;
        }
        if (strcmp(record, "revision") == 0) {
            if (revision_seen ||
                !next_device_catalogue_field(&cursor, catalogue->revision,
                                             sizeof(catalogue->revision)) ||
                cursor != NULL) {
                goto close_file;
            }
            revision_seen = true;
            continue;
        }
        if (strcmp(record, "component") != 0) {
            goto close_file;
        }

        furnace_hmi_ui_lab_device_component_t component = {0};
        furnace_hmi_ui_lab_device_task_t task = {0};
        if (!next_device_catalogue_field(&cursor, component.id, sizeof(component.id)) ||
            !next_device_catalogue_field(&cursor, component.name, sizeof(component.name)) ||
            !next_device_catalogue_field(&cursor, component.part, sizeof(component.part)) ||
            !next_device_catalogue_field(&cursor, component.location, sizeof(component.location)) ||
            !next_device_catalogue_field(&cursor, component.usage, sizeof(component.usage)) ||
            !next_device_catalogue_field(&cursor, component.source, sizeof(component.source)) ||
            !next_device_catalogue_field(&cursor, component.instance, sizeof(component.instance)) ||
            !next_device_catalogue_field(&cursor, task.id, sizeof(task.id)) ||
            !next_device_catalogue_field(&cursor, task.name, sizeof(task.name)) ||
            !next_device_catalogue_field(&cursor, task.basis, sizeof(task.basis)) ||
            !next_device_catalogue_field(&cursor, task.interval, sizeof(task.interval)) ||
            !next_device_catalogue_field(&cursor, task.remaining, sizeof(task.remaining)) ||
            !next_device_catalogue_field(&cursor, task.state, sizeof(task.state)) ||
            !next_device_catalogue_field(&cursor, task.last, sizeof(task.last)) ||
            !next_device_catalogue_field(&cursor, task.procedure, sizeof(task.procedure)) ||
            cursor != NULL ||
            !parse_device_task_state(task.state, &task.task_state)) {
            goto close_file;
        }

        size_t component_index = catalogue->component_count;
        for (size_t index = 0U; index < catalogue->component_count; ++index) {
            if (strcmp(catalogue->components[index].id, component.id) == 0) {
                component_index = index;
                break;
            }
        }
        if (component_index == catalogue->component_count) {
            if (catalogue->component_count >= FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS) {
                goto close_file;
            }
            catalogue->components[component_index] = component;
            ++catalogue->component_count;
        }
        else if (strcmp(catalogue->components[component_index].name, component.name) != 0 ||
                 strcmp(catalogue->components[component_index].instance, component.instance) != 0) {
            goto close_file;
        }
        furnace_hmi_ui_lab_device_component_t *stored = &catalogue->components[component_index];
        if (stored->task_count >= FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS) {
            goto close_file;
        }
        stored->tasks[stored->task_count++] = task;
    }
    valid = revision_seen;

close_file:
    (void)fclose(file);
    if (!valid) {
        memset(catalogue, 0, sizeof(*catalogue));
    }
    return valid;
}

static void format_rate_tenths(char *buffer, size_t capacity, int32_t rate_tenths)
{
    const int64_t magnitude = rate_tenths < 0 ? -(int64_t)rate_tenths : rate_tenths;
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
    if (stage->kind == FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) {
        snprintf(rate, sizeof(rate), "%s", ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INHERITED_TARGET));
    } else { format_rate_tenths(rate, sizeof(rate), stage->delta_c_per_minute); }
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
    lv_obj_set_style_radius(panel, UI_LAB_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, UI_LAB_PANEL_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, UI_LAB_SPACE, LV_PART_MAIN);
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
    case FURNACE_HMI_UI_LAB_PAGE_MANUAL:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_MODE);
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
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM);
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
static bool update_manual_prediction(furnace_hmi_ui_lab_explorer_t *explorer);
static bool update_manual_run_state(furnace_hmi_ui_lab_explorer_t *explorer);
static bool settings_show_message(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title_text,
    const char *detail_text
);

/* This is deliberately cosmetic. It executes only on the LVGL owner context,
 * does not delay navigation, and is never used to animate controller state. */
static void set_nav_glow_width(void *target, int32_t width)
{
    lv_obj_set_style_shadow_width((lv_obj_t *)target, width, LV_PART_MAIN);
}

static uint8_t nav_destination_for_page(furnace_hmi_ui_lab_page_t page)
{
    switch (page) {
    case FURNACE_HMI_UI_LAB_PAGE_MANUAL:
        return 0U;
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAMS:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES:
    case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR:
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
    lv_obj_set_style_bg_color(button, active ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE,
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(button, active ? UI_LAB_RADIUS : 0, LV_PART_MAIN);
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
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_style_text_color(label, active ? UI_LAB_TEXT : UI_LAB_MUTED, LV_PART_MAIN);
    }
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
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, UI_LAB_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(button, UI_LAB_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_min_height(button, 48, LV_PART_MAIN);
    lv_obj_set_height(button, 48);
    lv_obj_set_width(button, 0);
    const lv_style_selector_t pressed_main =
        (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED;
    lv_obj_set_style_bg_color(button, UI_LAB_NAV_GLOW, pressed_main);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, pressed_main);
    lv_obj_set_style_border_color(button, UI_LAB_NAV_GLOW, pressed_main);
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
    if (text != NULL && text[0] != '\0') {
        lv_obj_t *label = create_label(button, text, &lv_font_montserrat_14, UI_LAB_TEXT);
        if (label == NULL) { lv_obj_delete(button); return NULL; }
        lv_obj_center(label);
    }
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
    lv_obj_t *metric = create_transparent_container(parent);
    if (metric == NULL) {
        return false;
    }
    lv_obj_set_width(metric, 0);
    lv_obj_set_flex_grow(metric, 1);
    lv_obj_set_height(metric, 72);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metric, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *label = create_label(metric, label_text, &lv_font_montserrat_16, UI_LAB_MUTED);
    lv_obj_t *value = create_label(metric, value_text, &lv_font_montserrat_32, UI_LAB_TEXT);
    if (label == NULL || value == NULL) {
        return false;
    }
    if (value_out != NULL) {
        *value_out = value;
    }
    return true;
}

static bool make_primary_metric(
    lv_obj_t *parent,
    const char *label_text,
    const char *value_text,
    lv_obj_t **value_out
)
{
    lv_obj_t *metric = create_transparent_container(parent);
    if (metric == NULL) {
        return false;
    }
    lv_obj_set_width(metric, 0);
    lv_obj_set_flex_grow(metric, 1);
    lv_obj_set_height(metric, 78);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metric, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *label = create_label(metric, label_text, &lv_font_montserrat_16, UI_LAB_MUTED);
    lv_obj_t *value = create_label(metric, value_text, &lv_font_montserrat_40, UI_LAB_TEXT);
    if (value_out != NULL) { *value_out = value; }
    return label != NULL && value != NULL;
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
    if (width_percent == 100) {
        lv_obj_set_width(fact, lv_pct(100));
    } else {
        lv_obj_set_width(fact, 0);
        lv_obj_set_flex_grow(fact, 1);
    }
    lv_obj_set_height(fact, lv_pct(100));
    lv_obj_set_flex_flow(fact, LV_FLEX_FLOW_COLUMN);
    /* Bottom-aligned so values share a baseline when a heading wraps. */
    lv_obj_set_flex_align(fact, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(fact, 0, LV_PART_MAIN);
    lv_obj_t *heading_label = create_label(fact, heading, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *value_label = create_label(fact, value, &lv_font_montserrat_20, UI_LAB_TEXT);
    if (heading_label == NULL || value_label == NULL) {
        return false;
    }
    lv_obj_set_width(heading_label, lv_pct(100));
    lv_label_set_long_mode(heading_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(value_label, lv_pct(100));
    return true;
}

/* One label-left, value-right row for a narrow facts column. */
static bool make_detail_row(lv_obj_t *parent, const char *heading, const char *value)
{
    lv_obj_t *row = create_transparent_container(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return create_label(row, heading, &lv_font_montserrat_14, UI_LAB_MUTED) != NULL &&
           create_label(row, value, &lv_font_montserrat_20, UI_LAB_TEXT) != NULL;
}

/* Secondary action: industrial-yellow outline, no fill. */
static void style_outline_action(lv_obj_t *button)
{
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, UI_LAB_DEEP_BLUE, LV_PART_MAIN);
    lv_obj_set_style_text_color(button, UI_LAB_TEXT, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_style_text_color(label, UI_LAB_TEXT, LV_PART_MAIN);
    }
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

static lv_obj_t *make_favourite_star(lv_obj_t *parent, lv_color_t color, bool selected)
{
    static const lv_point_precise_t points[] = {
        { 15, 1 }, { 19, 11 }, { 29, 11 }, { 21, 17 }, { 24, 28 },
        { 15, 22 }, { 6, 28 }, { 9, 17 }, { 1, 11 }, { 11, 11 }, { 15, 1 }
    };
    lv_obj_t *star = lv_line_create(parent);
    if (star == NULL) {
        return NULL;
    }
    lv_line_set_points(star, points, sizeof(points) / sizeof(points[0]));
    lv_obj_set_size(star, 30, 30);
    lv_obj_set_style_line_color(star, color, LV_PART_MAIN);
    lv_obj_set_style_line_width(star, selected ? 3 : 2, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(star, true, LV_PART_MAIN);
    lv_obj_center(star);
    lv_obj_add_flag(star, LV_OBJ_FLAG_EVENT_BUBBLE);
    remove_scroll(star);
    return star;
}

static void trajectory_domain(
    const furnace_hmi_dashboard_state_t *state,
    int64_t *maximum_seconds,
    int32_t *minimum_temperature_c,
    int32_t *maximum_temperature_c
)
{
    int64_t latest_seconds = 0;
    int32_t minimum = 0;
    int32_t maximum = 100;
    bool have_temperature = false;
    if (state != NULL) {
        for (size_t index = 0; index < state->graph_sample_count; ++index) {
            const furnace_hmi_graph_sample_t *sample = &state->graph_samples[index];
            if ((int64_t)sample->elapsed_seconds > latest_seconds) {
                latest_seconds = sample->elapsed_seconds;
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
    const int64_t difference = (int64_t)maximum - minimum;
    const int64_t temperature_span = difference < 100 ? 100 : difference;
    const int64_t lower = minimum < 0 || minimum > temperature_span / 10 ? (int64_t)minimum - temperature_span / 10 : 0;
    const int64_t upper = (int64_t)maximum + temperature_span / 8;
    *minimum_temperature_c = lower < INT32_MIN ? INT32_MIN : (int32_t)lower;
    *maximum_temperature_c = upper > INT32_MAX ? INT32_MAX : (int32_t)upper;
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
        const int64_t old_time = explorer->trajectory_domain_maximum_seconds;
        const int32_t old_min = explorer->trajectory_domain_minimum_temperature_c;
        const int32_t old_max = explorer->trajectory_domain_maximum_temperature_c;
        trajectory_domain(graph_state, &explorer->trajectory_domain_maximum_seconds,
                          &explorer->trajectory_domain_minimum_temperature_c,
                          &explorer->trajectory_domain_maximum_temperature_c);
        if (explorer->trajectory_domain_initialized) {
            if (old_time > explorer->trajectory_domain_maximum_seconds) { explorer->trajectory_domain_maximum_seconds = old_time; }
            if (old_min < explorer->trajectory_domain_minimum_temperature_c) { explorer->trajectory_domain_minimum_temperature_c = old_min; }
            if (old_max > explorer->trajectory_domain_maximum_temperature_c) { explorer->trajectory_domain_maximum_temperature_c = old_max; }
        }
        explorer->trajectory_domain_initialized = true;
    }
    const int64_t maximum_seconds = explorer->trajectory_domain_maximum_seconds;
    const int32_t minimum_temperature_c = explorer->trajectory_domain_minimum_temperature_c;
    const int32_t maximum_temperature_c = explorer->trajectory_domain_maximum_temperature_c;
    const int32_t graph_width = (int32_t)lv_obj_get_width(explorer->trajectory);
    const int32_t graph_height = (int32_t)lv_obj_get_height(explorer->trajectory);
    if (graph_width < 160 || graph_height <= UI_LAB_GRAPH_TOP + UI_LAB_GRAPH_BOTTOM) {
        /* Compact structural floor: no drawable plot until layout supplies room. */
        return true;
    }
    const int32_t left = UI_LAB_GRAPH_LEFT;
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
                          ((int64_t)maximum_temperature_c - minimum_temperature_c) / 2));
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
        lv_obj_set_pos(explorer->trajectory_maximum_label, 8, top - 2);
    }
    if (explorer->trajectory_middle_temperature_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_middle_temperature_label, 8, top + height / 2 - 7);
    }
    if (explorer->trajectory_minimum_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_minimum_label, 8, top + height - 8);
    }
    if (explorer->trajectory_middle_time_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_middle_time_label, graph_width / 2 - 16,
                       graph_height - 25);
    }
    if (explorer->trajectory_time_axis_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_time_axis_label, left, graph_height - 25);
    }
    if (explorer->trajectory_end_time_label != NULL) {
        lv_obj_align(explorer->trajectory_end_time_label, LV_ALIGN_TOP_RIGHT, -8, graph_height - 25);
    }
    const size_t count = graph_state->graph_sample_count;
    for (size_t index = 0; index < count; ++index) {
        const furnace_hmi_graph_sample_t *sample = &graph_state->graph_samples[index];
        const int32_t x = left + (int32_t)((int64_t)sample->elapsed_seconds * width /
                                                        maximum_seconds);
        const int32_t planned_y = top + height - (int32_t)(((int64_t)sample->planned_temperature_c - minimum_temperature_c) * height /
            ((int64_t)maximum_temperature_c - minimum_temperature_c));
        explorer->planned_points[index] = (lv_point_precise_t) { x, planned_y };
    }
    /* Planned samples flagged as automatic cooling form a dashed estimate that
     * continues from the last curing sample; curing stays a solid line. */
    size_t curing_count = count;
    for (size_t index = 1U; index < count; ++index) {
        if (graph_state->graph_samples[index].planned_cooling) {
            curing_count = index;
            break;
        }
    }
    const bool planned_visible = explorer->trajectory_uses_program_preview ||
                                 explorer->state->mode != FURNACE_HMI_MACHINE_MODE_MANUAL;
    const bool show_cooling = planned_visible && curing_count < count;
    if (explorer->cooling_overlay != NULL) {
        if (show_cooling) {
            explorer->cooling_point_count = 0U;
            for (size_t index = curing_count - 1U; index < count; ++index) {
                explorer->cooling_points[explorer->cooling_point_count++] = explorer->planned_points[index];
            }
            lv_obj_remove_flag(explorer->cooling_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(explorer->cooling_overlay);
        }
        else {
            lv_obj_add_flag(explorer->cooling_overlay, LV_OBJ_FLAG_HIDDEN);
        }
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
    /* With a cooling segment the solid planned line ends where curing ends. */
    lv_line_set_points(explorer->planned_line, explorer->planned_points,
                       (uint32_t)(show_cooling ? curing_count : count));
    if (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL && !explorer->trajectory_uses_program_preview) { lv_obj_add_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN); }
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
        const int32_t first_y = top + height - (int32_t)(((int64_t)first->measured_temperature_c - minimum_temperature_c) * height /
            ((int64_t)maximum_temperature_c - minimum_temperature_c));
        const int32_t second_y = top + height - (int32_t)(((int64_t)second->measured_temperature_c - minimum_temperature_c) * height /
            ((int64_t)maximum_temperature_c - minimum_temperature_c));
        explorer->measured_segment_points[index][0] = (lv_point_precise_t) { first_x, first_y };
        explorer->measured_segment_points[index][1] = (lv_point_precise_t) { second_x, second_y };
        lv_line_set_points(segment, explorer->measured_segment_points[index], 2U);
        /* The measured trace keeps one readable visual identity when the
         * controller moves from running to paused; state is shown by the
         * persistent status accent instead. */
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

/* LVGL's software renderer dashes only horizontal and vertical lines, so the
 * diagonal cooling estimate is drawn as a bounded run of short segments. */
static void cooling_overlay_draw(lv_event_t *event)
{
    const furnace_hmi_ui_lab_explorer_t *explorer = lv_event_get_user_data(event);
    lv_obj_t *overlay = lv_event_get_current_target(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    if (explorer == NULL || overlay == NULL || layer == NULL) {
        return;
    }
    lv_area_t coords;
    lv_obj_get_coords(overlay, &coords);
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = UI_LAB_COOLING_ESTIMATE;
    line.width = 3;
    const double cycle = UI_LAB_COOLING_DASH + UI_LAB_COOLING_GAP;
    double travelled = 0.0;
    for (uint32_t index = 0U; index + 1U < explorer->cooling_point_count; ++index) {
        const double x0 = (double)coords.x1 + (double)explorer->cooling_points[index].x;
        const double y0 = (double)coords.y1 + (double)explorer->cooling_points[index].y;
        const double dx = (double)explorer->cooling_points[index + 1U].x - (double)explorer->cooling_points[index].x;
        const double dy = (double)explorer->cooling_points[index + 1U].y - (double)explorer->cooling_points[index].y;
        const double length = sqrt(dx * dx + dy * dy);
        double at = 0.0;
        while (at < length) {
            const double phase = fmod(travelled + at, cycle);
            const double step = phase < UI_LAB_COOLING_DASH ? UI_LAB_COOLING_DASH - phase : cycle - phase;
            const double end = at + step < length ? at + step : length;
            if (phase < UI_LAB_COOLING_DASH) {
                line.p1 = (lv_point_precise_t) { (lv_value_precise_t)lround(x0 + dx * at / length),
                                                 (lv_value_precise_t)lround(y0 + dy * at / length) };
                line.p2 = (lv_point_precise_t) { (lv_value_precise_t)lround(x0 + dx * end / length),
                                                 (lv_value_precise_t)lround(y0 + dy * end / length) };
                lv_draw_line(layer, &line);
            }
            at = end;
        }
        travelled += length;
    }
}

static void trajectory_size_changed(lv_event_t *event)
{
    furnace_hmi_ui_lab_explorer_t *explorer = lv_event_get_user_data(event);
    if (explorer->planned_line != NULL) { (void)update_trajectory(explorer); }
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
    lv_obj_set_style_bg_color(explorer->trajectory, UI_LAB_GRAPH, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->trajectory, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->trajectory, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(explorer->trajectory, UI_LAB_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->trajectory, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->trajectory, 0, LV_PART_MAIN);
    remove_scroll(explorer->trajectory);
    for (size_t index = 0; index < sizeof(explorer->grid_lines) /
                           sizeof(explorer->grid_lines[0]); ++index) {
        explorer->grid_lines[index] = make_graph_line(
            explorer->trajectory, explorer->grid_line_points[index], 2U, UI_LAB_DIVIDER, 1, false
        );
        if (explorer->grid_lines[index] == NULL) {
            return false;
        }
    }
    explorer->planned_line = make_graph_line(explorer->trajectory, explorer->planned_points, 0U,
                                              UI_LAB_PLANNED, 2, false);
    /* Created after the planned line and before stage nodes: above the grid,
     * below the node markers. Hidden unless a preview has a cooling tail. */
    explorer->cooling_overlay = lv_obj_create(explorer->trajectory);
    if (explorer->cooling_overlay == NULL) {
        return false;
    }
    lv_obj_remove_style_all(explorer->cooling_overlay);
    lv_obj_set_size(explorer->cooling_overlay, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(explorer->cooling_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(explorer->cooling_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(explorer->cooling_overlay, cooling_overlay_draw, LV_EVENT_DRAW_MAIN, explorer);
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
    lv_obj_align(axis, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_align(planned, LV_ALIGN_TOP_RIGHT, -100, 8);
    lv_obj_align(measured, LV_ALIGN_TOP_RIGHT, -8, 8);
    if (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL && !explorer->trajectory_uses_program_preview) { lv_obj_add_flag(planned, LV_OBJ_FLAG_HIDDEN); }
    if (explorer->trajectory_uses_program_preview) {
        lv_obj_add_flag(measured, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(planned, LV_ALIGN_TOP_RIGHT, -8, 8);
        if (explorer->page != FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
            size_t node_count = program_fixtures[explorer->selected_program % program_fixture_count()].stage_count;
            if (node_count > FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT) {
                node_count = FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT;
            }
            for (size_t index = 0; index < node_count; ++index) {
                const ui_lab_stage_fixture_t node_stage = program_stage(explorer, index);
                explorer->trajectory_stage_nodes[index] = create_page_button(
                    explorer, explorer->trajectory, "", stage_kind_color(fixture_stage_kind(&node_stage)),
                    UI_LAB_ACTION_STAGE_EDITOR, (int)index
                );
                if (explorer->trajectory_stage_nodes[index] == NULL) {
                    return false;
                }
                lv_obj_set_style_min_height(explorer->trajectory_stage_nodes[index], 14, LV_PART_MAIN);
                lv_obj_set_size(explorer->trajectory_stage_nodes[index], 14, 14);
                lv_obj_set_ext_click_area(explorer->trajectory_stage_nodes[index], 5);
                lv_obj_add_flag(explorer->trajectory_stage_nodes[index], LV_OBJ_FLAG_FLOATING);
            }
        }
    }
    lv_obj_update_layout(explorer->root);
    lv_obj_update_layout(explorer->trajectory);
    lv_obj_add_event_cb(explorer->trajectory, trajectory_size_changed, LV_EVENT_SIZE_CHANGED, explorer);
    return true;
}

static void clear_modal(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer != NULL && explorer->modal != NULL && lv_obj_is_valid(explorer->modal)) {
        lv_obj_delete(explorer->modal);
    }
    if (explorer != NULL) {
        explorer->modal = NULL;
        explorer->modal_action_count = 0U;
        explorer->settings_editor_spinbox = NULL;
        explorer->keyboard_input = NULL;
        explorer->keyboard_hint = NULL;
        explorer->keyboard_apply = NULL;
        explorer->keyboard_layout = NULL;
        explorer->keyboard_button_count = 0U;
        explorer->keyboard_mode = 0U;
    }
}

enum {
    UI_LAB_KEYBOARD_MODE_TEXT = 1,
    UI_LAB_KEYBOARD_MODE_NUMBER,
};

enum {
    UI_LAB_KEYBOARD_TARGET_PROGRAM_NAME = 1,
    UI_LAB_KEYBOARD_TARGET_STAGE_NAME,
    UI_LAB_KEYBOARD_TARGET_SETTINGS_NUMBER,
    UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET,
    UI_LAB_KEYBOARD_TARGET_DRAFT_RATE,
    UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION,
    UI_LAB_KEYBOARD_TARGET_MANUAL_TARGET,
    UI_LAB_KEYBOARD_TARGET_MANUAL_RATE,
};

enum {
    UI_LAB_KEYBOARD_BUTTON_INSERT = 1,
    UI_LAB_KEYBOARD_BUTTON_BACKSPACE,
    UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT,
    UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT,
    UI_LAB_KEYBOARD_BUTTON_CLEAR,
    UI_LAB_KEYBOARD_BUTTON_SIGN,
    UI_LAB_KEYBOARD_BUTTON_SHIFT,
    UI_LAB_KEYBOARD_BUTTON_CANCEL,
    UI_LAB_KEYBOARD_BUTTON_APPLY,
};

static void keyboard_button_event(lv_event_t *event);

static size_t keyboard_utf8_sequence_length(unsigned char first)
{
    if ((first & 0xE0U) == 0xC0U) {
        return 2U;
    }
    if ((first & 0xF0U) == 0xE0U) {
        return 3U;
    }
    if ((first & 0xF8U) == 0xF0U) {
        return 4U;
    }
    return 1U;
}

static size_t keyboard_utf8_character_count(const char *text, size_t length)
{
    size_t count = 0U;
    size_t position = 0U;
    while (text != NULL && position < length && text[position] != 0) {
        size_t sequence = keyboard_utf8_sequence_length((unsigned char)text[position]);
        if (sequence > length - position) {
            sequence = 1U;
        }
        position += sequence;
        ++count;
    }
    return count;
}

static size_t keyboard_utf8_previous(const char *text, size_t position)
{
    if (text == NULL || position == 0U) {
        return 0U;
    }
    --position;
    while (position > 0U && ((unsigned char)text[position] & 0xC0U) == 0x80U) {
        --position;
    }
    return position;
}

static size_t keyboard_utf8_next(const char *text, size_t position, size_t length)
{
    if (text == NULL || position >= length) {
        return length;
    }
    size_t sequence = keyboard_utf8_sequence_length((unsigned char)text[position]);
    if (sequence > length - position) {
        sequence = 1U;
    }
    return position + sequence;
}

static void keyboard_sync_input(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->keyboard_input == NULL ||
        !lv_obj_is_valid(explorer->keyboard_input)) {
        return;
    }
    lv_textarea_set_text(explorer->keyboard_input, explorer->keyboard_value);
    lv_textarea_set_cursor_pos(
        explorer->keyboard_input,
        (int32_t)keyboard_utf8_character_count(explorer->keyboard_value,
                                               explorer->keyboard_cursor));
}

static void keyboard_remove_selection(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->keyboard_selected == 0U) {
        return;
    }
    const size_t start = explorer->keyboard_cursor < explorer->keyboard_anchor
        ? explorer->keyboard_cursor : explorer->keyboard_anchor;
    const size_t end = explorer->keyboard_cursor > explorer->keyboard_anchor
        ? explorer->keyboard_cursor : explorer->keyboard_anchor;
    memmove(explorer->keyboard_value + start,
            explorer->keyboard_value + end,
            explorer->keyboard_length - end + 1U);
    explorer->keyboard_length -= end - start;
    explorer->keyboard_cursor = start;
    explorer->keyboard_anchor = start;
    explorer->keyboard_selected = 0U;
}

static void keyboard_update_feedback(furnace_hmi_ui_lab_explorer_t *explorer);

static void keyboard_insert(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *text,
    size_t length
)
{
    if (explorer == NULL || text == NULL || length == 0U ||
        explorer->keyboard_mode == 0U) {
        return;
    }
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_NUMBER) {
        for (size_t index = 0U; index < length; ++index) {
            if (!isdigit((unsigned char)text[index]) && text[index] != '.') {
                return;
            }
        }
    }
    const size_t old_length = explorer->keyboard_length;
    const size_t selection_start = explorer->keyboard_cursor < explorer->keyboard_anchor
        ? explorer->keyboard_cursor : explorer->keyboard_anchor;
    const size_t selected_bytes = explorer->keyboard_selected != 0U
        ? (explorer->keyboard_cursor > explorer->keyboard_anchor
            ? explorer->keyboard_cursor - explorer->keyboard_anchor
            : explorer->keyboard_anchor - explorer->keyboard_cursor) : 0U;
    const size_t selected_length = keyboard_utf8_character_count(
        explorer->keyboard_value + selection_start, selected_bytes);
    const size_t max_length = explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT ? 32U : 24U;
    const size_t character_count = keyboard_utf8_character_count(text, length);
    const size_t existing_count = keyboard_utf8_character_count(
        explorer->keyboard_value, old_length);
    if (existing_count < selected_length ||
        existing_count - selected_length + character_count > max_length ||
        old_length - selected_bytes + length >= sizeof(explorer->keyboard_value)) {
        if (explorer->keyboard_hint != NULL) {
            lv_label_set_text(explorer->keyboard_hint,
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_MAXIMUM_LENGTH));
        }
        return;
    }
    keyboard_remove_selection(explorer);
    memmove(explorer->keyboard_value + explorer->keyboard_cursor + length,
            explorer->keyboard_value + explorer->keyboard_cursor,
            explorer->keyboard_length - explorer->keyboard_cursor + 1U);
    memcpy(explorer->keyboard_value + explorer->keyboard_cursor, text, length);
    explorer->keyboard_length += length;
    explorer->keyboard_cursor += length;
    explorer->keyboard_anchor = explorer->keyboard_cursor;
    keyboard_sync_input(explorer);
    keyboard_update_feedback(explorer);
}

static void keyboard_backspace(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL) {
        return;
    }
    if (explorer->keyboard_selected != 0U) {
        keyboard_remove_selection(explorer);
    }
    else if (explorer->keyboard_cursor > 0U) {
        const size_t start = keyboard_utf8_previous(explorer->keyboard_value,
                                                    explorer->keyboard_cursor);
        memmove(explorer->keyboard_value + start,
                explorer->keyboard_value + explorer->keyboard_cursor,
                explorer->keyboard_length - explorer->keyboard_cursor + 1U);
        explorer->keyboard_length -= explorer->keyboard_cursor - start;
        explorer->keyboard_cursor = start;
        explorer->keyboard_anchor = start;
    }
    keyboard_sync_input(explorer);
    keyboard_update_feedback(explorer);
}

static void keyboard_move(furnace_hmi_ui_lab_explorer_t *explorer, int direction)
{
    if (explorer == NULL) {
        return;
    }
    if (explorer->keyboard_selected != 0U) {
        explorer->keyboard_cursor = direction < 0
            ? (explorer->keyboard_cursor < explorer->keyboard_anchor
                ? explorer->keyboard_cursor : explorer->keyboard_anchor)
            : (explorer->keyboard_cursor > explorer->keyboard_anchor
                ? explorer->keyboard_cursor : explorer->keyboard_anchor);
        explorer->keyboard_anchor = explorer->keyboard_cursor;
        explorer->keyboard_selected = 0U;
    }
    else if (direction < 0) {
        explorer->keyboard_cursor = keyboard_utf8_previous(
            explorer->keyboard_value, explorer->keyboard_cursor);
    }
    else {
        explorer->keyboard_cursor = keyboard_utf8_next(
            explorer->keyboard_value, explorer->keyboard_cursor,
            explorer->keyboard_length);
    }
    explorer->keyboard_anchor = explorer->keyboard_cursor;
    keyboard_sync_input(explorer);
}

static bool keyboard_parse_number(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    int32_t *value_out
)
{
    if (explorer == NULL || value_out == NULL || explorer->keyboard_length == 0U) {
        return false;
    }
    size_t position = 0U;
    bool negative = false;
    if (explorer->keyboard_value[position] == '-' ||
        explorer->keyboard_value[position] == '+') {
        negative = explorer->keyboard_value[position] == '-';
        ++position;
    }
    bool decimal = false;
    bool digits = false;
    size_t fractional_digits = 0U;
    int64_t integer_part = 0;
    int64_t fractional_part = 0;
    while (position < explorer->keyboard_length) {
        const unsigned char character = (unsigned char)explorer->keyboard_value[position++];
        if (isdigit(character)) {
            digits = true;
            if (decimal) {
                if (++fractional_digits > 1U) {
                    return false;
                }
                fractional_part = character - '0';
            }
            else {
                integer_part = integer_part * 10 + character - '0';
                if (integer_part > INT32_MAX) {
                    return false;
                }
            }
        }
        else if (character == '.' && !decimal) {
            decimal = true;
        }
        else {
            return false;
        }
    }
    if (!digits || (!explorer->keyboard_rate_tenths && fractional_part != 0)) {
        return false;
    }
    int64_t value = explorer->keyboard_rate_tenths
        ? integer_part * 10 + fractional_part : integer_part;
    if (negative) {
        value = -value;
    }
    if (value < explorer->keyboard_minimum || value > explorer->keyboard_maximum ||
        (explorer->keyboard_step > 0 &&
         (value - explorer->keyboard_minimum) % explorer->keyboard_step != 0)) {
        return false;
    }
    *value_out = (int32_t)value;
    return true;
}

static bool keyboard_value_is_valid(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->keyboard_mode == 0U) {
        return false;
    }
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_NUMBER) {
        int32_t value;
        return keyboard_parse_number(explorer, &value);
    }
    bool has_non_space = false;
    for (size_t index = 0U; index < explorer->keyboard_length; ++index) {
        const unsigned char character = (unsigned char)explorer->keyboard_value[index];
        if (character < 0x20U || character == 0x7FU) {
            return false;
        }
        if (!isspace(character)) {
            has_non_space = true;
        }
    }
    return has_non_space && keyboard_utf8_character_count(
        explorer->keyboard_value, explorer->keyboard_length) <= 32U;
}

static void keyboard_update_feedback(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->keyboard_hint == NULL ||
        !lv_obj_is_valid(explorer->keyboard_hint)) {
        return;
    }
    const bool valid = keyboard_value_is_valid(explorer);
    if (!valid) {
        lv_label_set_text(
            explorer->keyboard_hint,
            explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT
                ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ENTER_NAME)
                : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_INVALID_NUMBER));
        lv_obj_set_style_text_color(explorer->keyboard_hint, UI_LAB_PROGRAM_DELETE_TEXT, LV_PART_MAIN);
    }
    else if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT) {
        lv_label_set_text(explorer->keyboard_hint,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_APPLY_HINT));
        lv_obj_set_style_text_color(explorer->keyboard_hint, UI_LAB_MUTED, LV_PART_MAIN);
    }
    else {
        char range[96];
        (void)snprintf(range, sizeof(range),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_RANGE_FORMAT),
                       (long)explorer->keyboard_minimum,
                       (long)explorer->keyboard_maximum,
                       (long)explorer->keyboard_step);
        lv_label_set_text(explorer->keyboard_hint, range);
        lv_obj_set_style_text_color(explorer->keyboard_hint, UI_LAB_MUTED, LV_PART_MAIN);
    }
    if (explorer->keyboard_apply != NULL && lv_obj_is_valid(explorer->keyboard_apply)) {
        if (valid) {
            lv_obj_clear_state(explorer->keyboard_apply, LV_STATE_DISABLED);
        }
        else {
            lv_obj_add_state(explorer->keyboard_apply, LV_STATE_DISABLED);
        }
    }
}

static lv_obj_t *keyboard_add_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    size_t length,
    uint8_t action,
    lv_color_t background,
    bool function_key,
    bool disabled
)
{
    if (explorer == NULL || parent == NULL || text == NULL ||
        explorer->keyboard_button_count >= FURNACE_HMI_UI_LAB_KEYBOARD_BUTTON_COUNT ||
        length >= sizeof(explorer->keyboard_buttons[0].text)) {
        return NULL;
    }
    lv_obj_t *button = lv_button_create(parent);
    if (button == NULL) {
        return NULL;
    }
    configure_button(button, background);
    lv_obj_set_width(button, 0);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_height(button, lv_pct(100));
    lv_obj_set_style_min_height(button, UI_LAB_KEY_MIN_HEIGHT, LV_PART_MAIN);
    furnace_hmi_ui_lab_keyboard_button_t *slot =
        &explorer->keyboard_buttons[explorer->keyboard_button_count++];
    slot->explorer = explorer;
    slot->action = action;
    memcpy(slot->text, text, length);
    slot->text[length] = 0;
    lv_obj_t *label = create_label(button, slot->text,
                                   function_key ? &lv_font_montserrat_14 : &lv_font_montserrat_20,
                                   UI_LAB_TEXT);
    if (label == NULL) {
        --explorer->keyboard_button_count;
        lv_obj_delete(button);
        return NULL;
    }
    if (function_key && strcmp(text, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE)) == 0) {
        lv_obj_set_style_text_letter_space(label, -1, LV_PART_MAIN);
    }
    lv_obj_center(label);
    if (disabled) {
        /* Unavailable keys recede rather than looking highlighted: override the
         * default theme's disabled recolor, which lightens the key. */
        const lv_style_selector_t disabled_main =
            (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED;
        lv_obj_add_state(button, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(button, UI_LAB_GRAPH, disabled_main);
        lv_obj_set_style_recolor_opa(button, LV_OPA_TRANSP, disabled_main);
        lv_obj_set_style_text_color(label, UI_LAB_MUTED, LV_PART_MAIN);
        lv_obj_set_style_text_opa(label, LV_OPA_50, LV_PART_MAIN);
    }
    lv_obj_add_event_cb(button, keyboard_button_event, LV_EVENT_CLICKED, slot);
    return button;
}

/* A keyboard row that shares the layout height equally with its siblings. */
static lv_obj_t *keyboard_add_row(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *row = create_transparent_container(explorer->keyboard_layout);
    if (row == NULL) {
        return NULL;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 1);
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_style_min_height(row, UI_LAB_KEY_MIN_HEIGHT, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
    return row;
}

static bool keyboard_add_text_row(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *characters,
    uint8_t row_index
)
{
    lv_obj_t *row = keyboard_add_row(explorer);
    if (row == NULL) {
        return false;
    }
    if (row_index == 3U) {
        const char *function = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CAPS);
        const lv_color_t color = explorer->keyboard_shift != 0U
            ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE_RAISED;
        lv_obj_t *function_button = keyboard_add_button(
            explorer, row, function, strlen(function),
            UI_LAB_KEYBOARD_BUTTON_SHIFT,
            color, true, false);
        if (function_button == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(function_button, 3);
        if (explorer->keyboard_shift != 0U) {
            lv_obj_t *label = lv_obj_get_child(function_button, 0);
            if (label != NULL) {
                lv_obj_set_style_text_color(label, lv_color_hex(0xB8DCF0), LV_PART_MAIN);
            }
            lv_obj_set_style_border_color(function_button, UI_LAB_NAV_GLOW, LV_PART_MAIN);
        }
    }
    const size_t character_length = strlen(characters);
    size_t position = 0U;
    while (position < character_length) {
        size_t length = keyboard_utf8_sequence_length((unsigned char)characters[position]);
        if (length > character_length - position) {
            length = 1U;
        }
        char label[8];
        memcpy(label, characters + position, length);
        label[length] = 0;
        if (explorer->keyboard_shift != 0U &&
            length == 1U && isalpha((unsigned char)label[0])) {
            label[0] = (char)toupper((unsigned char)label[0]);
        }
        lv_obj_t *key = keyboard_add_button(explorer, row, label, length,
                                            UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_SURFACE_RAISED,
                                            false, false);
        if (key == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(key, 2);
        position += length;
    }
    if (row_index == 3U) {
        /* Delete sits after m, mirroring Caps, so the row has no empty slots. */
        lv_obj_t *delete_key = keyboard_add_button(
            explorer, row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE),
            strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE)),
            UI_LAB_KEYBOARD_BUTTON_BACKSPACE, UI_LAB_SURFACE_RAISED, true, false);
        if (delete_key == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(delete_key, 3);
    }
    return true;
}

static bool keyboard_render(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->keyboard_layout == NULL) {
        return false;
    }
    if (lv_obj_is_valid(explorer->keyboard_layout)) {
        lv_obj_clean(explorer->keyboard_layout);
    }
    explorer->keyboard_button_count = 0U;
    explorer->keyboard_apply = NULL;
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT) {
        const char *rows[] = {
            keyboard_numbers,
            keyboard_letters,
            keyboard_letters_middle,
            keyboard_letters_bottom,
        };
        for (uint8_t row = 0U; row < sizeof(rows) / sizeof(rows[0]); ++row) {
            if (!keyboard_add_text_row(explorer, rows[row], row)) {
                return false;
            }
        }
        lv_obj_t *bottom = keyboard_add_row(explorer);
        if (bottom == NULL) {
            return false;
        }
        const char space[] = { ' ', 0 };
        lv_obj_t *space_key = keyboard_add_button(explorer, bottom, space, 1U,
                                                  UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_SURFACE_RAISED,
                                                  true, false);
        if (space_key == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(space_key, 5);
        if (
            keyboard_add_button(explorer, bottom, LV_SYMBOL_LEFT, strlen(LV_SYMBOL_LEFT),
                                UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT, UI_LAB_SURFACE_RAISED,
                                true, false) == NULL ||
            keyboard_add_button(explorer, bottom, LV_SYMBOL_RIGHT, strlen(LV_SYMBOL_RIGHT),
                                UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT, UI_LAB_SURFACE_RAISED,
                                true, false) == NULL ||
            keyboard_add_button(explorer, bottom, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR),
                                strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR)),
                                UI_LAB_KEYBOARD_BUTTON_CLEAR, UI_LAB_SURFACE_RAISED,
                                true, false) == NULL ||
            keyboard_add_button(explorer, bottom, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
                                strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)),
                                UI_LAB_KEYBOARD_BUTTON_CANCEL, UI_LAB_SURFACE_RAISED,
                                true, false) == NULL) {
            return false;
        }
        lv_obj_t *apply = keyboard_add_button(
            explorer, bottom, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY),
            strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY)),
            UI_LAB_KEYBOARD_BUTTON_APPLY, UI_LAB_DEEP_BLUE, true, false);
        if (apply == NULL) {
            return false;
        }
        explorer->keyboard_apply = apply;
    }
    else {
        lv_obj_t *grid = NULL;
        const char *keys[] = {
            "7", "8", "9", "Backspace", "4", "5", "6", "Clear",
            "1", "2", "3", "+/-", "0", ".", "left", "right"
        };
        for (size_t index = 0U; index < sizeof(keys) / sizeof(keys[0]); ++index) {
            if (index % 4U == 0U) {
                grid = keyboard_add_row(explorer);
                if (grid == NULL) {
                    return false;
                }
            }
            const char *key = keys[index];
            uint8_t action = UI_LAB_KEYBOARD_BUTTON_INSERT;
            bool function = false;
            bool disabled = false;
            const char *label = key;
            if (index == 3U) {
                label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE);
                action = UI_LAB_KEYBOARD_BUTTON_BACKSPACE;
                function = true;
            }
            else if (index == 7U) {
                label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR);
                action = UI_LAB_KEYBOARD_BUTTON_CLEAR;
                function = true;
            }
            else if (index == 11U) {
                label = "+/-";
                action = UI_LAB_KEYBOARD_BUTTON_SIGN;
                function = true;
                disabled = explorer->keyboard_minimum >= 0;
            }
            else if (index == 14U) {
                label = LV_SYMBOL_LEFT;
                action = UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT;
                function = true;
            }
            else if (index == 15U) {
                label = LV_SYMBOL_RIGHT;
                action = UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT;
                function = true;
            }
            else if (index == 13U) {
                disabled = explorer->keyboard_rate_tenths == 0U;
            }
            lv_obj_t *button = keyboard_add_button(
                explorer, grid, label, strlen(label), action, UI_LAB_SURFACE_RAISED,
                function, disabled);
            if (button == NULL) {
                return false;
            }
        }
        lv_obj_t *actions = keyboard_add_row(explorer);
        if (actions == NULL) {
            return false;
        }
        if (keyboard_add_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
                                strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL)),
                                UI_LAB_KEYBOARD_BUTTON_CANCEL, UI_LAB_SURFACE_RAISED,
                                true, false) == NULL) {
            return false;
        }
        lv_obj_t *apply = keyboard_add_button(
            explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY),
            strlen(ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY)),
            UI_LAB_KEYBOARD_BUTTON_APPLY, UI_LAB_DEEP_BLUE, true, false);
        if (apply == NULL) {
            return false;
        }
        explorer->keyboard_apply = apply;
    }
    return true;
}

static bool keyboard_open(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t mode,
    const char *title,
    const char *value,
    uint8_t target,
    uint8_t stage,
    int32_t minimum,
    int32_t maximum,
    int32_t step,
    bool rate_tenths
)
{
    if (explorer == NULL || title == NULL || value == NULL) {
        return false;
    }
    clear_modal(explorer);
    explorer->keyboard_mode = mode;
    explorer->keyboard_target = target;
    explorer->keyboard_stage = stage;
    explorer->keyboard_shift = 0U;
    explorer->keyboard_selected = 1U;
    explorer->keyboard_minimum = minimum;
    explorer->keyboard_maximum = maximum;
    explorer->keyboard_step = step;
    explorer->keyboard_rate_tenths = rate_tenths ? 1U : 0U;
    copy_text_bounded(explorer->keyboard_value, sizeof(explorer->keyboard_value), value);
    explorer->keyboard_length = strlen(explorer->keyboard_value);
    explorer->keyboard_cursor = explorer->keyboard_length;
    explorer->keyboard_anchor = 0U;

    lv_obj_t *overlay = lv_obj_create(explorer->root);
    if (overlay == NULL) {
        clear_modal(explorer);
        return false;
    }
    explorer->modal = overlay;
    lv_obj_set_width(overlay, lv_pct(100));
    const int32_t root_height = lv_obj_get_height(explorer->root);
    lv_obj_set_height(overlay, root_height);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 12, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *panel = create_panel(overlay, UI_LAB_GRAPH);
    if (panel == NULL) {
        clear_modal(explorer);
        return false;
    }
    const int32_t root_width = lv_obj_get_width(explorer->root);
    const int32_t full_width = root_width > 24 ? root_width - 24 : root_width;
    lv_obj_set_width(panel, mode == UI_LAB_KEYBOARD_MODE_TEXT || full_width < UI_LAB_NUMERIC_KEYPAD_WIDTH
                                ? full_width : UI_LAB_NUMERIC_KEYPAD_WIDTH);
    lv_obj_set_height(panel, lv_pct(100));
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(panel);
    lv_obj_t *heading = create_transparent_container(panel);
    lv_obj_t *input = lv_textarea_create(panel);
    explorer->keyboard_input = input;
    lv_obj_t *hint = create_label(panel, "", &lv_font_montserrat_14, UI_LAB_MUTED);
    explorer->keyboard_hint = hint;
    if (heading == NULL || input == NULL || hint == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 22);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_t *heading_title = create_label(
        heading, title, &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *heading_count = create_label(
        heading,
        ui_string(mode == UI_LAB_KEYBOARD_MODE_TEXT
            ? FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_TEXT_ENTRY
            : FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_NUMERIC_ENTRY),
        &lv_font_montserrat_14, UI_LAB_MUTED);
    if (heading_title == NULL || heading_count == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_flex_grow(heading_title, 1);
    lv_obj_set_style_text_align(heading_count, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_width(input, lv_pct(100));
    lv_obj_set_height(input, 48);
    lv_obj_set_style_bg_color(input, UI_LAB_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(input, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(input, lv_color_hex(0x7895A7), LV_PART_MAIN);
    lv_obj_set_style_border_width(input, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(input, 4, LV_PART_MAIN);
    lv_obj_set_style_text_color(input, UI_LAB_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(input, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_textarea_set_one_line(input, true);
    lv_textarea_set_cursor_click_pos(input, false);
    lv_textarea_set_text_selection(input, false);
    lv_textarea_set_max_length(input, mode == UI_LAB_KEYBOARD_MODE_TEXT ? 32U : 24U);
    keyboard_sync_input(explorer);
    lv_obj_set_width(hint, lv_pct(100));
    lv_obj_set_height(hint, 16);
    explorer->keyboard_layout = create_transparent_container(panel);
    if (explorer->keyboard_layout == NULL) {
        clear_modal(explorer);
        return false;
    }
    /* Keys take all remaining panel height instead of leaving an empty band. */
    lv_obj_set_width(explorer->keyboard_layout, lv_pct(100));
    lv_obj_set_height(explorer->keyboard_layout, 1);
    lv_obj_set_flex_grow(explorer->keyboard_layout, 1);
    lv_obj_set_style_pad_top(explorer->keyboard_layout, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(explorer->keyboard_layout, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(explorer->keyboard_layout, 6, LV_PART_MAIN);
    if (!keyboard_render(explorer)) {
        clear_modal(explorer);
        return false;
    }
    keyboard_update_feedback(explorer);
    return true;
}

static void format_keyboard_value(char *buffer, size_t capacity, int32_t value, bool rate_tenths)
{
    if (rate_tenths) {
        (void)snprintf(buffer, capacity, "%ld.%ld", (long)(value / 10),
                       (long)(value < 0 ? -(value % 10) : value % 10));
    }
    else {
        (void)snprintf(buffer, capacity, "%ld", (long)value);
    }
}

static bool keyboard_open_text(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title,
    const char *value,
    uint8_t target,
    uint8_t stage
)
{
    return keyboard_open(explorer, UI_LAB_KEYBOARD_MODE_TEXT, title, value,
                          target, stage, 0, 0, 0, false);
}

static bool keyboard_open_number(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title,
    int32_t value,
    uint8_t target,
    uint8_t stage,
    int32_t minimum,
    int32_t maximum,
    int32_t step,
    bool rate_tenths
)
{
    char formatted[32];
    format_keyboard_value(formatted, sizeof(formatted), value, rate_tenths);
    return keyboard_open(explorer, UI_LAB_KEYBOARD_MODE_NUMBER, title, formatted,
                         target, stage, minimum, maximum, step, rate_tenths);
}

static void keyboard_apply_value(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || !keyboard_value_is_valid(explorer)) {
        return;
    }
    int32_t numeric_value = 0;
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_NUMBER &&
        !keyboard_parse_number(explorer, &numeric_value)) {
        return;
    }
    const uint8_t target = explorer->keyboard_target;
    const uint8_t stage = explorer->keyboard_stage;
    if (target == UI_LAB_KEYBOARD_TARGET_PROGRAM_NAME) {
        if (explorer->program_editor_active) {
            if (strcmp(explorer->program_editor_name, explorer->keyboard_value) != 0) {
                explorer->program_editor_dirty = true;
            }
            copy_text_bounded(explorer->program_editor_name,
                              sizeof(explorer->program_editor_name), explorer->keyboard_value);
        } else {
            copy_text_bounded(explorer->program_name_overrides[explorer->selected_program],
                              FURNACE_HMI_UI_LAB_NAME_CAPACITY, explorer->keyboard_value);
        }
    }
    else if (target == UI_LAB_KEYBOARD_TARGET_STAGE_NAME &&
             stage < FURNACE_HMI_UI_LAB_MAX_STAGES) {
        store_selected_stage_draft(explorer);
        copy_text_bounded(explorer->staged_names[stage], FURNACE_HMI_UI_LAB_NAME_CAPACITY,
                          explorer->keyboard_value);
        explorer->staged_name_valid[stage] = true;
    }
    else if (target == UI_LAB_KEYBOARD_TARGET_SETTINGS_NUMBER &&
             explorer->settings_editor_key < 5U) {
        explorer->settings_candidate_numeric[explorer->settings_editor_key] = numeric_value;
    }
    else if (target == UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET) {
        explorer->draft_target_c = numeric_value;
        update_heating_relationship(explorer);
    }
    else if (target == UI_LAB_KEYBOARD_TARGET_DRAFT_RATE) {
        explorer->draft_rate_tenths_c_per_minute = explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL
            ? -numeric_value : numeric_value;
        update_heating_relationship(explorer);
    }
   else if (target == UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION) {
       explorer->draft_duration_minutes = numeric_value;
       update_heating_relationship(explorer);
   }
    else if (target == UI_LAB_KEYBOARD_TARGET_MANUAL_TARGET) {
        explorer->manual_target_c = numeric_value;
    }
    else if (target == UI_LAB_KEYBOARD_TARGET_MANUAL_RATE) {
        explorer->manual_rate_tenths_c_per_minute = numeric_value;
    }
    const bool manual_value = target == UI_LAB_KEYBOARD_TARGET_MANUAL_TARGET ||
        target == UI_LAB_KEYBOARD_TARGET_MANUAL_RATE;
    clear_modal(explorer);
    if (manual_value) {
        (void)update_manual_prediction(explorer);
    }
    else {
        (void)rebuild_page(explorer);
    }
}

static void keyboard_handle_button(const furnace_hmi_ui_lab_keyboard_button_t *button)
{
    if (button == NULL || button->explorer == NULL || button->explorer->modal == NULL) {
        return;
    }
    furnace_hmi_ui_lab_explorer_t *explorer = button->explorer;
    switch (button->action) {
    case UI_LAB_KEYBOARD_BUTTON_INSERT:
        keyboard_insert(explorer, button->text, strlen(button->text));
        if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT &&
            explorer->keyboard_shift != 0U) {
            explorer->keyboard_shift = 0U;
            (void)keyboard_render(explorer);
            keyboard_sync_input(explorer);
            keyboard_update_feedback(explorer);
        }
        break;
    case UI_LAB_KEYBOARD_BUTTON_BACKSPACE:
        keyboard_backspace(explorer);
        break;
    case UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT:
        keyboard_move(explorer, -1);
        break;
    case UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT:
        keyboard_move(explorer, 1);
        break;
    case UI_LAB_KEYBOARD_BUTTON_CLEAR:
        explorer->keyboard_value[0] = 0;
        explorer->keyboard_length = 0U;
        explorer->keyboard_cursor = 0U;
        explorer->keyboard_anchor = 0U;
        explorer->keyboard_selected = 0U;
        keyboard_sync_input(explorer);
        keyboard_update_feedback(explorer);
        break;
    case UI_LAB_KEYBOARD_BUTTON_SIGN:
        if (explorer->keyboard_minimum < 0) {
            if (explorer->keyboard_length > 0U && explorer->keyboard_value[0] == '-') {
                memmove(explorer->keyboard_value, explorer->keyboard_value + 1,
                        explorer->keyboard_length);
                --explorer->keyboard_length;
                if (explorer->keyboard_cursor > 0U) {
                    --explorer->keyboard_cursor;
                }
            }
            else {
                memmove(explorer->keyboard_value + 1, explorer->keyboard_value,
                        explorer->keyboard_length + 1U);
                explorer->keyboard_value[0] = '-';
                ++explorer->keyboard_length;
                ++explorer->keyboard_cursor;
            }
            explorer->keyboard_anchor = explorer->keyboard_cursor;
            keyboard_sync_input(explorer);
            keyboard_update_feedback(explorer);
        }
        break;
    case UI_LAB_KEYBOARD_BUTTON_SHIFT:
        explorer->keyboard_shift = explorer->keyboard_shift == 0U ? 1U : 0U;
        (void)keyboard_render(explorer);
        keyboard_sync_input(explorer);
        keyboard_update_feedback(explorer);
        break;
    case UI_LAB_KEYBOARD_BUTTON_CANCEL:
        clear_modal(explorer);
        break;
    case UI_LAB_KEYBOARD_BUTTON_APPLY:
        keyboard_apply_value(explorer);
        break;
    default:
        break;
    }
}

static void keyboard_deferred_button(void *user_data)
{
    keyboard_handle_button((const furnace_hmi_ui_lab_keyboard_button_t *)user_data);
}

static void keyboard_button_event(lv_event_t *event)
{
    const furnace_hmi_ui_lab_keyboard_button_t *button = lv_event_get_user_data(event);
    if (button == NULL || button->explorer == NULL || button->explorer->modal == NULL) {
        return;
    }
    lv_async_call(keyboard_deferred_button, (void *)button);
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
    lv_obj_set_width(overlay, lv_pct(100));
    lv_obj_set_height(overlay, lv_obj_get_height(explorer->root) - (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING ? (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL ? 152 : 92) : 0));
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
    lv_obj_set_size(dialog, lv_pct(86), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dialog, UI_LAB_PANEL_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_row(dialog, UI_LAB_SPACE, LV_PART_MAIN);
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
    if (detail_text[0] == '\0') {
        lv_obj_add_flag(detail, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    const char *dismiss_text = primary_action == UI_LAB_ACTION_CLOSE_MODAL && primary_text != NULL
        ? primary_text : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL);
    lv_obj_t *cancel = create_modal_button(
        explorer,
        actions,
        dismiss_text,
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

static const char *door_text(const furnace_hmi_dashboard_state_t *state)
{
    if (!state_is_current(state) || state->door_closed.validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN);
    }
    return ui_string(state->door_closed.value ? FURNACE_HMI_UI_STRING_DASHBOARD_CLOSED : FURNACE_HMI_UI_STRING_DASHBOARD_OPEN);
}

/* Geometry is recalculated from the final canvas size, with no flex-managed
 * overlay pieces or transforms. All child objects are owned by this canvas. */
static void furnace_quad(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b, lv_point_precise_t c, lv_point_precise_t d, uint32_t color)
{
    lv_draw_triangle_dsc_t triangle; lv_draw_triangle_dsc_init(&triangle);
    triangle.color = lv_color_hex(color); triangle.opa = LV_OPA_COVER;
    triangle.p[0] = a; triangle.p[1] = b; triangle.p[2] = c; lv_draw_triangle(layer, &triangle);
    triangle.p[1] = c; triangle.p[2] = d; lv_draw_triangle(layer, &triangle);
}

static void furnace_draw(lv_event_t *event)
{
    lv_obj_t *canvas = lv_event_get_target_obj(event);
    furnace_hmi_ui_lab_explorer_t *explorer = lv_event_get_user_data(event);
    lv_area_t bounds; lv_obj_get_coords(canvas, &bounds);
    int32_t width = lv_area_get_width(&bounds), height = lv_area_get_height(&bounds);
    if (height < 32 || width < 48) { return; }
    int32_t h = height - 24; if (h > 192) { h = 192; }
    int32_t w = h * 3 / 4; if (w > width - 32) { w = width - 32; h = w * 4 / 3; }
    int32_t depth = w / 5, rise = depth * 2 / 3;
    int32_t x = bounds.x1 + (width - w - depth) / 2, y = bounds.y1 + (height - h - rise) / 2 + rise;
    lv_layer_t *layer = lv_event_get_layer(event);
    furnace_quad(layer, (lv_point_precise_t){x,y}, (lv_point_precise_t){x+depth,y-rise},
        (lv_point_precise_t){x+w+depth,y-rise}, (lv_point_precise_t){x+w,y}, 0x657179);
    furnace_quad(layer, (lv_point_precise_t){x+w,y}, (lv_point_precise_t){x+w+depth,y-rise},
        (lv_point_precise_t){x+w+depth,y+h-rise}, (lv_point_precise_t){x+w,y+h}, 0x303B42);
    lv_draw_rect_dsc_t rect; lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_hex(0x465259); rect.bg_opa = LV_OPA_COVER;
    rect.border_color = lv_color_hex(0x7A878F); rect.border_width = 1;
    lv_area_t front = {x,y,x+w,y+h}; lv_draw_rect(layer, &rect, &front);
    lv_area_t door = {x+w/8,y+h/10,x+w*7/8,y+h*9/10};
    rect.bg_color = UI_LAB_SURFACE; rect.radius = 5; rect.border_color = UI_LAB_MUTED;
    lv_draw_rect(layer, &rect, &door);
    lv_area_t window = {x+w/4,y+h/5,x+w*3/4,y+h*3/5};
    bool heating = state_is_current(explorer->state) && explorer->state->heater_demand_percent.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT && explorer->state->heater_demand_percent.value > 0;
    rect.bg_color = lv_color_hex(heating ? 0x47372A : 0x171B1E); rect.border_width = 0; rect.radius = 3;
    lv_draw_rect(layer, &rect, &window);
    lv_draw_line_dsc_t line; lv_draw_line_dsc_init(&line); line.width = 2;
    line.color = heating ? UI_LAB_RUNNING : UI_LAB_BORDER;
    for (int32_t i = 1; i <= 3; ++i) {
        line.p1 = (lv_point_precise_t){window.x1+4, window.y1+(window.y2-window.y1)*i/4};
        line.p2 = (lv_point_precise_t){window.x2-4, line.p1.y}; lv_draw_line(layer, &line);
    }
    line.color = UI_LAB_MUTED; line.width = 3;
    line.p1 = (lv_point_precise_t){door.x2-5,y+h*3/5}; line.p2 = (lv_point_precise_t){door.x2-5,y+h*3/4}; lv_draw_line(layer, &line);
}

static bool create_furnace_visual(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *parent, bool expansive)
{
    (void)expansive;
    lv_obj_t *visual = create_panel(parent, UI_LAB_SURFACE);
    if (visual == NULL) { return false; }
    lv_obj_set_width(visual, lv_pct(100)); lv_obj_set_height(visual, 1);
    const bool short_panel = lv_obj_get_height(explorer->root) <= 480 && explorer->page != FURNACE_HMI_UI_LAB_PAGE_DEVICE;
    lv_obj_set_flex_grow(visual, 1); lv_obj_set_flex_flow(visual, short_panel ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    if (!short_panel && create_label(visual, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FURNACE_VISUAL), &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) { return false; }
    lv_obj_t *canvas = create_transparent_container(visual);
    if (canvas == NULL) { return false; }
    lv_obj_set_width(canvas, short_panel ? lv_pct(42) : lv_pct(100));
    lv_obj_set_height(canvas, short_panel ? lv_pct(100) : 1);
    if (!short_panel) { lv_obj_set_flex_grow(canvas, 1); }
    lv_obj_add_event_cb(canvas, furnace_draw, LV_EVENT_DRAW_MAIN, explorer);
    lv_obj_t *readouts = visual;
    if (short_panel) {
        readouts = create_transparent_container(visual); if (!readouts) { return false; }
        lv_obj_set_width(readouts, 0); lv_obj_set_flex_grow(readouts, 1); lv_obj_set_height(readouts, lv_pct(100));
        lv_obj_set_style_pad_row(readouts, 0, LV_PART_MAIN);
        lv_obj_set_flex_flow(readouts, LV_FLEX_FLOW_COLUMN); lv_obj_set_flex_align(readouts, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    }
    char rpm[32];
    format_observation_value(rpm, sizeof(rpm), &explorer->state->fan_rpm, FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT);
    const char *labels[] = { ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_DOOR), ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAN) };
    const char *values[] = { door_text(explorer->state), rpm };
    for (size_t i = 0; i < 2; ++i) {
        lv_obj_t *row = create_transparent_container(readouts);
        if (row == NULL) { return false; }
        lv_obj_set_size(row, lv_pct(100), short_panel ? 48 : 24); lv_obj_set_flex_flow(row, short_panel ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_row(row, 4, LV_PART_MAIN);
        lv_obj_t *label = create_label(row, labels[i], &lv_font_montserrat_14, UI_LAB_MUTED);
        lv_obj_t *value = create_label(row, values[i], &lv_font_montserrat_20, UI_LAB_TEXT);
        if (label == NULL || value == NULL) { return false; }
        if (!short_panel) { lv_obj_set_flex_grow(label, 1); }
        if (i == 0) { explorer->live_door = value; } else { explorer->live_fan = value; }
    }
    return true;
}

static int32_t manual_current_temperature_c(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer != NULL && explorer->state != NULL &&
        explorer->state->current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        return explorer->state->current_temperature_c.value;
    }
    return 25;
}

static uint32_t manual_prediction_minutes(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const int32_t delta = explorer->manual_target_c - manual_current_temperature_c(explorer);
    const int32_t rate = explorer->manual_rate_tenths_c_per_minute;
    if (delta <= 0 || rate <= 0) {
        return 0U;
    }
    return (uint32_t)((delta * 10 + rate - 1) / rate);
}

static void prepare_manual_prediction(furnace_hmi_ui_lab_explorer_t *explorer)
{
    furnace_hmi_dashboard_state_initialize(&explorer->preview_graph_state);
    explorer->preview_graph_state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    explorer->preview_graph_state.graph_sample_count = 2U;
    explorer->preview_graph_state.graph_samples[0].elapsed_seconds = 0U;
    explorer->preview_graph_state.graph_samples[0].planned_temperature_c =
        manual_current_temperature_c(explorer);
    explorer->preview_graph_state.graph_samples[1].elapsed_seconds =
        manual_prediction_minutes(explorer) * 60U;
    explorer->preview_graph_state.graph_samples[1].planned_temperature_c =
        explorer->manual_target_c;
    explorer->trajectory_uses_program_preview = true;
    /* A Manual forecast is an operator control, not a chart that may zoom
     * around each selected target. Keep the controller's complete permitted
     * temperature range visible while the plotted data changes in place. */
    explorer->trajectory_domain_minimum_temperature_c = 0;
    explorer->trajectory_domain_maximum_temperature_c = UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
    explorer->trajectory_domain_maximum_seconds =
        explorer->preview_graph_state.graph_samples[1].elapsed_seconds < 60U
            ? 60U
            : explorer->preview_graph_state.graph_samples[1].elapsed_seconds;
    explorer->trajectory_domain_initialized = true;
}

static bool make_manual_control(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *label_text,
    const char *value_text,
    ui_lab_action_t decrement_action,
    ui_lab_action_t edit_action,
    ui_lab_action_t increment_action,
    lv_obj_t **value_out
)
{
    lv_obj_t *row = create_transparent_container(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_t *label = create_label(row, label_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *values = label == NULL ? NULL : create_transparent_container(row);
    if (label == NULL || values == NULL) {
        return false;
    }
    lv_obj_set_width(row, 0);
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_height(row, 76);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(row, 4, LV_PART_MAIN);
    lv_obj_set_width(values, lv_pct(100));
    lv_obj_set_height(values, 52);
    lv_obj_set_flex_flow(values, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(values, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(values, 4, LV_PART_MAIN);
    lv_obj_t *decrement = create_page_button(explorer, values, "-", UI_LAB_BACKGROUND,
                                             decrement_action, 0);
    lv_obj_t *value = create_page_button(explorer, values, value_text, UI_LAB_BACKGROUND,
                                         edit_action, 0);
    lv_obj_t *increment = create_page_button(explorer, values, "+", UI_LAB_BACKGROUND,
                                             increment_action, 0);
    if (decrement == NULL || value == NULL || increment == NULL) {
        return false;
    }
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_size(decrement, 48, 52);
    lv_obj_set_width(value, 0);
    lv_obj_set_flex_grow(value, 1);
    lv_obj_set_height(value, 52);
    lv_obj_set_size(increment, 48, 52);
    lv_obj_set_style_border_color(decrement, UI_LAB_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_color(value, UI_LAB_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_color(increment, UI_LAB_PRIMARY, LV_PART_MAIN);
    if (value_out != NULL) {
        *value_out = value;
    }
    return true;
}

static void set_button_label(lv_obj_t *button, const char *text)
{
    lv_obj_t *label = button == NULL ? NULL : lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_label_set_text(label, text);
    }
}

static bool update_manual_prediction(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->trajectory == NULL ||
        explorer->manual_prediction_label == NULL) {
        return false;
    }
    char target[32];
    char rate[32];
    char prediction[64];
    const uint32_t minutes = manual_prediction_minutes(explorer);
    (void)snprintf(target, sizeof(target), "%ld C", (long)explorer->manual_target_c);
    (void)snprintf(rate, sizeof(rate), "%ld.%ld C/min",
                   (long)(explorer->manual_rate_tenths_c_per_minute / 10),
                   (long)(explorer->manual_rate_tenths_c_per_minute % 10));
    (void)snprintf(prediction, sizeof(prediction), "Prediction to target: %lu min",
                   (unsigned long)minutes);
    set_button_label(explorer->manual_target_value, target);
    set_button_label(explorer->manual_rate_value, rate);
    lv_label_set_text(explorer->manual_prediction_label, prediction);
    prepare_manual_prediction(explorer);
    return update_trajectory(explorer);
}

static bool update_manual_run_state(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->manual_status_label == NULL ||
        explorer->manual_context_label == NULL || explorer->manual_start_button == NULL ||
        explorer->manual_pause_button == NULL || explorer->manual_stop_button == NULL) {
        return false;
    }
    const bool running = explorer->manual_running;
    lv_label_set_text(explorer->manual_status_label,
                      running ? (explorer->manual_paused ? "Heating paused" : "Heating") : "Ready");
    lv_obj_set_style_text_color(explorer->manual_status_label,
                                running ? UI_LAB_RUNNING : UI_LAB_MUTED, LV_PART_MAIN);
    char context_text[64];
    if (running) {
        char elapsed[32];
        format_remaining(elapsed, sizeof(elapsed), &explorer->state->elapsed_seconds);
        (void)snprintf(context_text, sizeof(context_text), "Curing elapsed: %s", elapsed);
    }
    else {
        (void)snprintf(context_text, sizeof(context_text), "Furnace stays idle until Start.");
    }
    lv_label_set_text(explorer->manual_context_label, context_text);
    if (running) {
        lv_obj_add_flag(explorer->manual_start_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(explorer->manual_pause_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(explorer->manual_stop_button, LV_OBJ_FLAG_HIDDEN);
        set_button_label(explorer->manual_pause_button,
                         explorer->manual_paused ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW)
                                                 : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW));
    }
    else {
        lv_obj_remove_flag(explorer->manual_start_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(explorer->manual_pause_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(explorer->manual_stop_button, LV_OBJ_FLAG_HIDDEN);
    }
    if (explorer->nav != NULL) {
        if (running) {
            lv_obj_add_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(explorer->nav, -1);
        }
    }
    lv_obj_update_layout(explorer->root);
    return update_manual_prediction(explorer);
}

static bool build_manual(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const bool running = explorer->manual_running;
    char target[32];
    char rate[32];
    char prediction[64];
    const uint32_t prediction_minutes = manual_prediction_minutes(explorer);
    (void)snprintf(target, sizeof(target), "%ld C", (long)explorer->manual_target_c);
    (void)snprintf(rate, sizeof(rate), "%ld.%ld C/min",
                   (long)(explorer->manual_rate_tenths_c_per_minute / 10),
                   (long)(explorer->manual_rate_tenths_c_per_minute % 10));
    (void)snprintf(prediction, sizeof(prediction), "Prediction to target: %lu min",
                   (unsigned long)prediction_minutes);
    prepare_manual_prediction(explorer);

    lv_obj_t *columns = create_transparent_container(explorer->content);
    lv_obj_t *graph = create_transparent_container(columns);
    lv_obj_t *controls = create_panel(columns, UI_LAB_SURFACE);
    if (columns == NULL || graph == NULL || controls == NULL) {
        return false;
    }
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(columns, 8, LV_PART_MAIN);
    lv_obj_set_width(graph, lv_pct(100));
    lv_obj_set_height(graph, 0);
    lv_obj_set_flex_grow(graph, 7);
    lv_obj_set_flex_flow(graph, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(graph, 5, LV_PART_MAIN);
    lv_obj_set_width(controls, lv_pct(100));
    lv_obj_set_height(controls, 0);
    lv_obj_set_flex_grow(controls, 3);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(controls, 4, LV_PART_MAIN);

    if (create_label(graph, "Manual mode", &lv_font_montserrat_20,
                     UI_LAB_TEXT) == NULL) {
        return false;
    }
    explorer->manual_prediction_label = create_label(graph, prediction,
                                                      &lv_font_montserrat_14, UI_LAB_MUTED);
    if (explorer->manual_prediction_label == NULL) {
        return false;
    }
    if (!make_trajectory(explorer, graph, 1)) {
        return false;
    }

    lv_obj_t *context = create_transparent_container(controls);
    if (context == NULL) {
        return false;
    }
    lv_obj_add_flag(context, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(context, lv_pct(100), 24);
    lv_obj_align(context, LV_ALIGN_TOP_LEFT, 0, -4);
    lv_obj_set_flex_flow(context, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(context, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    const char *status = running
        ? (explorer->manual_paused ? "Heating paused" : "Heating")
        : "Ready";
    explorer->manual_status_label = create_label(context, status, &lv_font_montserrat_16,
                                                  running ? UI_LAB_RUNNING : UI_LAB_MUTED);
    if (explorer->manual_status_label == NULL) {
        return false;
    }
    lv_obj_set_width(explorer->manual_status_label, 0);
    lv_obj_set_flex_grow(explorer->manual_status_label, 1);
    char elapsed[32];
    char context_text[64];
    if (running) {
        format_remaining(elapsed, sizeof(elapsed), &explorer->state->elapsed_seconds);
        (void)snprintf(context_text, sizeof(context_text), "Curing elapsed: %s", elapsed);
    }
    else {
        (void)snprintf(context_text, sizeof(context_text), "Furnace stays idle until Start.");
    }
    explorer->manual_context_label = create_label(context, context_text,
                                                   &lv_font_montserrat_16, UI_LAB_MUTED);
    if (explorer->manual_context_label == NULL) {
        return false;
    }

    lv_obj_t *control_stack = create_transparent_container(controls);
    if (control_stack == NULL) {
        return false;
    }
    lv_obj_add_flag(control_stack, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(control_stack, lv_pct(65), 76);
    lv_obj_align(control_stack, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_flex_flow(control_stack, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control_stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(control_stack, 10, LV_PART_MAIN);
    if (!make_manual_control(explorer, control_stack, "Target temperature", target,
                             UI_LAB_ACTION_MANUAL_DECREMENT_TARGET,
                             UI_LAB_ACTION_MANUAL_EDIT_TARGET,
                             UI_LAB_ACTION_MANUAL_INCREMENT_TARGET,
                             &explorer->manual_target_value) ||
        !make_manual_control(explorer, control_stack, "Rate", rate,
                             UI_LAB_ACTION_MANUAL_DECREMENT_RATE,
                             UI_LAB_ACTION_MANUAL_EDIT_RATE,
                             UI_LAB_ACTION_MANUAL_INCREMENT_RATE,
                             &explorer->manual_rate_value)) {
        return false;
    }

    lv_obj_t *actions = create_transparent_container(controls);
    if (actions == NULL) {
        return false;
    }
    lv_obj_add_flag(actions, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(actions, lv_pct(32), UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_align(actions, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 7, LV_PART_MAIN);
    explorer->manual_start_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START),
        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_MANUAL_START, 0);
    explorer->manual_pause_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW),
        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_MANUAL_PAUSE, 0);
    explorer->manual_stop_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW),
        UI_LAB_FAULT, UI_LAB_ACTION_MANUAL_STOP, 0);
    if (explorer->manual_start_button == NULL || explorer->manual_pause_button == NULL ||
        explorer->manual_stop_button == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(explorer->manual_start_button, 1);
    lv_obj_set_flex_grow(explorer->manual_pause_button, 1);
    lv_obj_set_flex_grow(explorer->manual_stop_button, 1);
    return update_manual_run_state(explorer);
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
    lv_obj_t *readings = create_panel(visual_column, UI_LAB_SURFACE);
    if (readings == NULL) {
        return false;
    }
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    /* Match the quick-launch rail to one of the four equal-width bottom-nav
     * buttons. */
    lv_obj_set_width(quick_launch, lv_pct(25));
    lv_obj_set_height(quick_launch, lv_pct(100));
    lv_obj_set_flex_flow(quick_launch, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(quick_launch, 4, LV_PART_MAIN);
    lv_obj_set_width(visual_column, 0);
    lv_obj_set_flex_grow(visual_column, 1);
    lv_obj_set_height(visual_column, lv_pct(100));
    lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(readings, lv_pct(100));
    lv_obj_set_height(readings, 96);
    lv_obj_set_flex_flow(readings, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(readings, UI_LAB_SPACE, LV_PART_MAIN);
    char current[32];
    char target[32];
    char remaining[32];
    format_observation_value(current, sizeof(current), &explorer->state->current_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    format_observation_value(target, sizeof(target), &explorer->state->target_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    format_remaining(remaining, sizeof(remaining), &explorer->state->remaining_seconds);
    if (!make_primary_metric(readings,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_TEMPERATURE), current, &explorer->home_temperature) ||
        !make_metric(readings, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET), target, &explorer->home_target) ||
        !make_metric(readings, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REMAINING), remaining, &explorer->home_remaining)) {
        return false;
    }
    lv_obj_t *title = create_label(quick_launch,
                                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_QUICK_LAUNCH),
                                   &lv_font_montserrat_20, UI_LAB_TEXT);
    if (title == NULL) { return false; }
    if (explorer->favourite_count > 0U &&
        create_label(quick_launch, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAVOURITES),
                     &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) { return false; }
    for (size_t index = 0; index < explorer->favourite_count; ++index) {
        const uint8_t program_index = explorer->quick_launch_favourites[index];
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                program_name_text(explorer, program_index),
                                                UI_LAB_SURFACE_RAISED,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 48);
    }
    size_t quick_count = explorer->favourite_count;
    bool has_recent = false;
    for (size_t index = 0; index < explorer->recent_count && quick_count < 3U; ++index) {
        const uint8_t program_index = explorer->quick_launch_recent[index];
        if (program_is_favourite(explorer, program_index)) { continue; }
        if (!has_recent && create_label(quick_launch,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RECENT_PROGRAMS),
                                        &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) { return false; }
        has_recent = true;
        ++quick_count;
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                program_name_text(explorer, program_index),
                                                UI_LAB_SURFACE_RAISED,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 48);
    }
    if (explorer->favourite_count == 0U && !has_recent &&
        create_label(quick_launch, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_FAVOURITE_PROGRAM_PROMPT), &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) { return false; }
    lv_obj_t *spacer = create_transparent_container(quick_launch);
    if (spacer == NULL) {
        return false;
    }
    lv_obj_set_size(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_t *choose = create_page_button(explorer, quick_launch,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_MANUAL),
                                           UI_LAB_DEEP_BLUE, UI_LAB_ACTION_MANUAL, 0);
    if (choose == NULL) {
        return false;
    }
    lv_obj_set_width(choose, lv_pct(100));
    lv_obj_set_height(choose, 48);
    return create_furnace_visual(explorer, visual_column, true);
}

static bool make_program_card(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const ui_lab_program_fixture_t *program,
    int program_index
)
{
    lv_obj_t *card = create_page_button(explorer, parent, "", UI_LAB_SURFACE,
                                        UI_LAB_ACTION_PROGRAM_DETAIL, program_index);
    if (card == NULL) {
        return false;
    }
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, lv_obj_get_height(explorer->root) < 600 ? 80 : 96);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(card, UI_LAB_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, UI_LAB_PROGRAM_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, UI_LAB_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_left(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 8, LV_PART_MAIN);
    lv_obj_t *name = create_label(card, program_name_text(explorer, (size_t)program_index), &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *summary = create_transparent_container(card);
    if (name == NULL || summary == NULL) {
        return false;
    }
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name, lv_pct(100));
    lv_obj_set_width(summary, lv_pct(100));
    lv_obj_set_height(summary, 28);
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(summary, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(summary, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_column(summary, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(summary, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(summary, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(summary, UI_LAB_DIVIDER, LV_PART_MAIN);
    char stage_count[32];
    char duration[32];
    char maximum[32];
    (void)snprintf(stage_count, sizeof(stage_count),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_COUNT_FORMAT),
                   (unsigned int)program_authored_stage_count(program));
    const char *duration_text = program_duration(program);
    size_t duration_length = strlen(duration_text);
    if (duration_length >= sizeof(duration)) { duration_length = sizeof(duration) - 1U; }
    memcpy(duration, duration_text, duration_length);
    duration[duration_length] = '\0';
    const char *maximum_text = program_maximum(program);
    size_t maximum_length = strlen(maximum_text);
    if (maximum_length >= sizeof(maximum)) { maximum_length = sizeof(maximum) - 1U; }
    memcpy(maximum, maximum_text, maximum_length);
    maximum[maximum_length] = '\0';
    const char *summary_values[] = { stage_count, duration, maximum };
    for (size_t index = 0; index < sizeof(summary_values) / sizeof(summary_values[0]); ++index) {
        lv_obj_t *value = create_label(summary, summary_values[index], &lv_font_montserrat_14, UI_LAB_MUTED);
        if (value == NULL) {
            return false;
        }
        lv_obj_set_width(value, 0);
        lv_obj_set_flex_grow(value, 1);
        lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
        if (index > 0U) {
            lv_obj_set_style_border_width(value, 1, LV_PART_MAIN);
            lv_obj_set_style_border_side(value, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
            lv_obj_set_style_border_color(value, UI_LAB_DIVIDER, LV_PART_MAIN);
            lv_obj_set_style_pad_left(value, 12, LV_PART_MAIN);
        }
        if (index == 2U) {
            lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        }
    }
    return true;
}

static bool build_programs(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t program_count = explorer->library_count;
    const size_t per_page = 6U;
    const size_t page_count = program_count == 0 ? 1 : (program_count + per_page - 1U) / per_page;
    const size_t start = (size_t)explorer->program_page * per_page;
    const size_t end = start + per_page < program_count ? start + per_page : program_count;
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    if (toolbar == NULL || list == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 8, LV_PART_MAIN);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(list, 8, LV_PART_MAIN);
    lv_obj_t *new_draft = create_page_button(explorer, toolbar,
                                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT),
                                             UI_LAB_DEEP_BLUE, UI_LAB_ACTION_NEW_DRAFT, 0);
    char page_text[32];
    (void)snprintf(page_text, sizeof(page_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAGE_FORMAT),
                   (unsigned int)explorer->program_page + 1U, (unsigned int)page_count);
    lv_obj_t *page = create_label(toolbar, page_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *previous = create_page_button(explorer, toolbar,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS),
                                            UI_LAB_SURFACE_RAISED,
                                            UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE, 0);
    lv_obj_t *next = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_NEXT_PROGRAM_PAGE, 0);
    if (new_draft == NULL || page == NULL || previous == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_width(new_draft, 150);
    lv_obj_set_width(page, 0);
    lv_obj_set_flex_grow(page, 1);
    lv_obj_set_style_pad_right(page, 8, LV_PART_MAIN);
    lv_obj_set_style_text_align(page, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_width(previous, 82);
    lv_obj_set_width(next, 82);
    lv_obj_set_height(previous, 48);
    lv_obj_set_height(next, 48);
    if (explorer->program_page == 0U) {
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    }
    if ((size_t)explorer->program_page + 1U >= page_count) {
        lv_obj_add_state(next, LV_STATE_DISABLED);
    }
    lv_obj_t *columns[2] = {
        create_transparent_container(list),
        create_transparent_container(list),
    };
    if (columns[0] == NULL || columns[1] == NULL) {
        return false;
    }
    for (size_t column = 0; column < 2U; ++column) {
        lv_obj_set_width(columns[column], 0);
    lv_obj_set_flex_grow(columns[column], 1);
        lv_obj_set_height(columns[column], lv_pct(100));
        lv_obj_set_flex_flow(columns[column], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(columns[column], 8, LV_PART_MAIN);
    }
    if (program_count == 0 && create_label(columns[0], ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_LIBRARY), &lv_font_montserrat_16, UI_LAB_MUTED) == NULL) { return false; }
    for (size_t index = start; index < end; ++index) {
        if (!make_program_card(explorer, columns[(index - start) % 2U],
                               &program_fixtures[index], (int)index)) {
            return false;
        }
    }
    return true;
}

static bool make_stage_card(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list,
                            size_t index, bool editable);

static bool build_program_editor(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t count = editor_stage_count(explorer);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (toolbar == NULL || list == NULL || actions == NULL) { return false; }
    /* Back, the editable name, then compact stage actions share one row. */
    lv_obj_set_size(toolbar, lv_pct(100), 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 8, LV_PART_MAIN);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR, 0);
    lv_obj_t *name = create_page_button(
        explorer, toolbar,
        explorer->program_editor_name[0] == '\0' ? "Enter program name" : explorer->program_editor_name,
        UI_LAB_BACKGROUND, UI_LAB_ACTION_SHOW_RENAME, 0);
    lv_obj_t *add = create_page_button(explorer, toolbar, "+ Add stage", UI_LAB_PRIMARY,
                                       UI_LAB_ACTION_ADD_STAGE, 0);
    lv_obj_t *edit = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGES),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_STAGE_EDITOR, 0);
    if (back == NULL || name == NULL || add == NULL || edit == NULL) { return false; }
    lv_obj_set_width(back, 72);
    lv_obj_set_width(name, 0);
    lv_obj_set_flex_grow(name, 1);
    lv_obj_set_style_border_width(name, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(name, UI_LAB_DEEP_BLUE, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(name, 12, LV_PART_MAIN);
    lv_obj_t *name_label = lv_obj_get_child(name, 0);
    if (name_label != NULL) {
        lv_obj_set_width(name_label, lv_pct(100));
        lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_20, LV_PART_MAIN);
    }
    lv_obj_set_width(add, 124);
    lv_obj_set_width(edit, 124);
    if (count >= FURNACE_HMI_UI_LAB_MAX_STAGES) { lv_obj_add_state(add, LV_STATE_DISABLED); }
    if (count == 0U) { lv_obj_add_state(edit, LV_STATE_DISABLED); }

    /* Stage cards match Program stages; tapping a card edits that stage. */
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_height(list, 1);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(list, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, 8, LV_PART_MAIN);
    if (count == 0U &&
        create_label(list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_PROGRAM_STAGES),
                     &lv_font_montserrat_16, UI_LAB_MUTED) == NULL) {
        return false;
    }
    for (size_t index = 0; index < count; ++index) {
        if (!make_stage_card(explorer, list, index, true)) { return false; }
    }

    /* No bottom navigation here: Cancel and Save are the only ways out, and
     * leaving with unsaved changes asks first. */
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 48);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    lv_obj_t *cancel = create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
                                          UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR, 0);
    lv_obj_t *save = create_page_button(explorer, actions, "Save program", UI_LAB_PRIMARY,
                                        UI_LAB_ACTION_SHOW_SAVE, 0);
    if (cancel == NULL || save == NULL) { return false; }
    lv_obj_set_flex_grow(cancel, 1);
    lv_obj_set_flex_grow(save, 1);
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
    lv_obj_set_height(toolbar, 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_HOME, 0);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *title = create_label(toolbar, program_name_text(explorer, explorer->selected_program), &lv_font_montserrat_24,
                                   UI_LAB_TEXT);
    if (back == NULL || title == NULL) {
        return false;
    }
    lv_obj_set_width(back, 70);
    lv_obj_set_flex_grow(title, 1);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
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
    lv_obj_set_width(summary, 0);
    lv_obj_set_flex_grow(summary, 1);
    lv_obj_set_height(summary, lv_pct(100));
    lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(summary, 7, LV_PART_MAIN);
    if (!make_trajectory(explorer, graph_column, 0)) {
        return false;
    }
    char details[96];
    (void)snprintf(details, sizeof(details),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_SUMMARY_FORMAT),
                   (unsigned int)program_authored_stage_count(program), program_duration(program),
                   program_maximum(program));
    lv_obj_t *facts = create_label(summary, details, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *energy = create_label(summary, ui_string(program->estimated_energy),
                                    &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *start = create_page_button(explorer, summary,
                                         ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW),
                                         UI_LAB_RUNNING, UI_LAB_ACTION_RUNNING, 0);
    lv_obj_t *details_button = create_page_button(explorer, summary,
                                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VIEW_DETAILS),
                                                  UI_LAB_DEEP_BLUE,
                                                  UI_LAB_ACTION_PROGRAM_DETAIL,
                                                  (int)explorer->selected_program);
    if (facts == NULL || energy == NULL || start == NULL || details_button == NULL) {
        return false;
    }
    lv_obj_set_width(start, lv_pct(100));
    lv_obj_set_height(start, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_width(details_button, lv_pct(100));
    lv_obj_set_height(details_button, 48);
    return true;
}

static void format_running_time_value(char *text, size_t capacity,
                                      const furnace_hmi_i32_value_t *value)
{
    if (value->validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT && value->value >= 0) {
        (void)snprintf(text, capacity, "%ld:%02ld", (long)(value->value / 3600),
                       (long)((value->value / 60) % 60));
    }
    else {
        (void)snprintf(text, capacity, "--:--");
    }
}

static void format_power_value(char *text, size_t capacity, const furnace_hmi_i32_value_t *power)
{
    if (power->validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        const int32_t value = power->value;
        (void)snprintf(text, capacity, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_POWER_FORMAT),
                       value / 100, value >= 0 ? value % 100 : -(value % 100));
    }
    else {
        (void)snprintf(text, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
    }
}

/* One line of label/value readings from current controller observations.
 * Curing: Elapsed, Remaining, Current, Target, Est. power. Automatic cooling
 * keeps curing Elapsed and adds only cooling time and its estimate, so no
 * label is repeated. */
static void update_running_readings(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer->running_reading_values[0] == NULL) {
        return;
    }
    const furnace_hmi_dashboard_state_t *state = explorer->state;
    const bool cooling = state->run_phase == FURNACE_HMI_RUN_PHASE_COOLING;
    furnace_hmi_ui_string_id_t names[FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT];
    char values[FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT][32];
    names[0] = FURNACE_HMI_UI_STRING_UI_LAB_ELAPSED;
    format_running_time_value(values[0], sizeof(values[0]), &state->elapsed_seconds);
    if (cooling) {
        names[1] = FURNACE_HMI_UI_STRING_UI_LAB_READING_COOLING;
        format_running_time_value(values[1], sizeof(values[1]), &state->cooling_elapsed_seconds);
        names[2] = FURNACE_HMI_UI_STRING_UI_LAB_READING_EST_REMAINING;
        format_running_time_value(values[2], sizeof(values[2]), &state->cooling_remaining_seconds);
    }
    else {
        names[1] = FURNACE_HMI_UI_STRING_UI_LAB_REMAINING;
        format_running_time_value(values[1], sizeof(values[1]), &state->remaining_seconds);
        names[2] = FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT;
        format_observation_value(values[2], sizeof(values[2]), &state->current_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    }
    names[3] = cooling ? FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT : FURNACE_HMI_UI_STRING_DASHBOARD_TARGET;
    format_observation_value(values[3], sizeof(values[3]),
                             cooling ? &state->current_temperature_c : &state->target_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    if (cooling) {
        names[4] = FURNACE_HMI_UI_STRING_DASHBOARD_TARGET;
        format_observation_value(values[4], sizeof(values[4]), &state->target_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    }
    else {
        names[4] = FURNACE_HMI_UI_STRING_DASHBOARD_ESTIMATED_POWER;
        format_power_value(values[4], sizeof(values[4]), &state->estimated_power_kw_x100);
    }
    for (size_t index = 0; index < FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT; ++index) {
        lv_label_set_text(explorer->running_reading_names[index], ui_string(names[index]));
        lv_label_set_text(explorer->running_reading_values[index], values[index]);
    }
}

static bool make_running_readings(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *parent)
{
    for (size_t index = 0; index < FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT; ++index) {
        lv_obj_t *pair = create_transparent_container(parent);
        if (pair == NULL) {
            return false;
        }
        lv_obj_set_size(pair, LV_SIZE_CONTENT, lv_pct(100));
        lv_obj_set_flex_flow(pair, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(pair, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(pair, 6, LV_PART_MAIN);
        explorer->running_reading_names[index] = create_label(pair, "", &lv_font_montserrat_14, UI_LAB_MUTED);
        explorer->running_reading_values[index] = create_label(pair, "", &lv_font_montserrat_16, UI_LAB_TEXT);
        if (explorer->running_reading_names[index] == NULL || explorer->running_reading_values[index] == NULL) {
            return false;
        }
    }
    update_running_readings(explorer);
    return true;
}

static bool build_running(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *phase = create_transparent_container(explorer->content);
    lv_obj_t *middle = create_transparent_container(explorer->content);
    lv_obj_t *readings = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (phase == NULL || middle == NULL || readings == NULL || actions == NULL) {
        return false;
    }
    const bool compact_view = lv_obj_get_height(explorer->root) <= 400;

    lv_obj_set_width(phase, lv_pct(100));
    lv_obj_set_height(phase, compact_view ? 24 : 28);
    lv_obj_set_flex_flow(phase, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(phase, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL) {
        lv_obj_add_flag(phase, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_move_to_index(phase, 0);
    lv_obj_t *program_name = create_page_button(explorer, phase, explorer->state->program_name,
        UI_LAB_BACKGROUND, UI_LAB_ACTION_SHOW_RUN_DETAILS, 0);
    if (!program_name) { return false; }
    lv_obj_set_style_min_height(program_name, 28, LV_PART_MAIN); lv_obj_set_height(program_name, 28);
    lv_obj_set_style_pad_all(program_name, 0, LV_PART_MAIN); lv_obj_set_style_border_width(program_name, 0, LV_PART_MAIN);
    lv_obj_set_flex_grow(program_name, 1);
    lv_obj_t *name_label = lv_obj_get_child(program_name, 0);
    if (name_label) { lv_obj_set_width(name_label, lv_pct(100)); lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT); lv_obj_set_style_text_font(name_label, &lv_font_montserrat_20, LV_PART_MAIN); }
    char phase_text[96];
    snprintf(phase_text, sizeof(phase_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_IDENTITY_FORMAT),
        (unsigned)explorer->state->stage_index + 1U, (unsigned)explorer->state->stage_count, explorer->state->stage_name);
    const char *phase_label = explorer->state->run_phase == FURNACE_HMI_RUN_PHASE_COOLING
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_AUTOMATIC_COOLING)
        : explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL ? explorer->state->stage_name : phase_text;
    if (create_label(phase, phase_label,
        &lv_font_montserrat_16, state_color(explorer->state)) == NULL) { return false; }

    lv_obj_set_width(middle, lv_pct(100));
    /* Give the flex child a non-zero base so LVGL can distribute the
     * remaining content height at both the 800x480 target and the compact
     * 640x360 structural floor. */
    lv_obj_set_height(middle, 1);
    lv_obj_set_style_min_height(middle, compact_view ? 72 : 0, LV_PART_MAIN);
    lv_obj_set_flex_grow(middle, 1);
    lv_obj_set_flex_flow(middle, explorer->running_view_mode == 1U
                                      ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_column(middle, UI_LAB_SPACE, LV_PART_MAIN);
    if (explorer->running_view_mode == 0U) {
        if (!make_trajectory(explorer, middle, 0)) {
            return false;
        }
    }
    else if (explorer->running_view_mode == 1U) {
        lv_obj_t *graph_column = create_transparent_container(middle);
        lv_obj_t *visual_column = create_transparent_container(middle);
        if (graph_column == NULL || visual_column == NULL) {
            return false;
        }
        lv_obj_set_width(graph_column, lv_pct(62));
        lv_obj_set_height(graph_column, lv_pct(100));
        lv_obj_set_flex_flow(graph_column, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_width(visual_column, 0);
    lv_obj_set_flex_grow(visual_column, 1);
        lv_obj_set_height(visual_column, lv_pct(100));
        lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
        if (!make_trajectory(explorer, graph_column, 0) ||
            !create_furnace_visual(explorer, visual_column, false)) {
            return false;
        }
    }
    else if (!create_furnace_visual(explorer, middle, true)) {
        return false;
    }

    /* One reading line between the run visual and its actions, in every view. */
    lv_obj_set_width(readings, lv_pct(100));
    lv_obj_set_height(readings, compact_view ? 24 : 32);
    lv_obj_set_flex_flow(readings, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(readings, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(readings, 4, LV_PART_MAIN);
    if (!make_running_readings(explorer, readings)) { return false; }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, 4, LV_PART_MAIN);
    const bool paused = explorer->state->run_state == FURNACE_HMI_RUN_STATE_PAUSED;
    const ui_lab_action_t primary_action = paused ? UI_LAB_ACTION_RESUME_PREVIEW
                                                   : UI_LAB_ACTION_PAUSE_PREVIEW;
    const furnace_hmi_ui_string_id_t primary_text = paused
        ? FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW
        : FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW;
    lv_obj_t *primary = create_page_button(explorer, actions, ui_string(primary_text),
                                           state_color(explorer->state), primary_action, 0);
    lv_obj_t *stop = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW),
                                        UI_LAB_FAULT, UI_LAB_ACTION_STOP_PREVIEW, 0);
    if (primary == NULL || stop == NULL) { return false; }
    lv_obj_set_flex_grow(primary, 3); lv_obj_set_flex_grow(stop, 3);
    lv_obj_set_height(primary, UI_LAB_BUTTON_MIN_HEIGHT); lv_obj_set_height(stop, UI_LAB_BUTTON_MIN_HEIGHT);

    const furnace_hmi_ui_string_id_t mode_texts[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_FULL_GRAPH,
        FURNACE_HMI_UI_STRING_UI_LAB_SPLIT_VIEW,
        FURNACE_HMI_UI_STRING_UI_LAB_FULL_VISUAL,
    };
    const ui_lab_action_t mode_actions[] = {
        UI_LAB_ACTION_SHOW_GRAPH_VIEW,
        UI_LAB_ACTION_SHOW_SPLIT_VIEW,
        UI_LAB_ACTION_SHOW_VISUAL_VIEW,
    };
    for (size_t index = 0; index < 3U; ++index) {
        lv_obj_t *button = create_page_button(
            explorer, actions, ui_string(mode_texts[index]),
            explorer->running_view_mode == index ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE,
            mode_actions[index], 0);
        if (button == NULL) { return false; }
        const bool selected = explorer->running_view_mode == index;
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, UI_LAB_BUTTON_MIN_HEIGHT);
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(button, selected ? 2 : 0, LV_PART_MAIN);
        lv_obj_set_style_border_side(button, selected ? LV_BORDER_SIDE_BOTTOM : LV_BORDER_SIDE_NONE, LV_PART_MAIN);
        lv_obj_set_style_border_color(button, UI_LAB_TIME_BORDER, LV_PART_MAIN);
        lv_obj_set_style_radius(button, 0, LV_PART_MAIN);
        lv_obj_t *mode_label = lv_obj_get_child(button, 0);
        if (mode_label != NULL) {
            lv_obj_set_style_text_color(mode_label, selected ? lv_color_hex(0x9CC9E4) : UI_LAB_MUTED, LV_PART_MAIN);
        }
        const lv_style_selector_t pressed_mode =
            (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED;
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, pressed_mode);
        lv_obj_set_style_border_width(button, selected ? 2 : 0, pressed_mode);
        lv_obj_set_style_border_side(button, selected ? LV_BORDER_SIDE_BOTTOM : LV_BORDER_SIDE_NONE, pressed_mode);
        lv_obj_set_style_border_color(button, UI_LAB_TIME_BORDER, pressed_mode);
    }
    return true;
}

/* One stage card. Program stages shows it read-only; the program editor makes
 * the whole card the tap target that opens the stage editor. */
static bool make_stage_card(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list,
                            size_t index, bool editable)
{
    const ui_lab_stage_fixture_t fixture = program_stage(explorer, index);
    const lv_color_t kind_color = stage_kind_color(fixture_stage_kind(&fixture));
    const char *type_name = fixture.kind == FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD
        ? "Holding" : "Heating";
    char number[8]; char duration[24]; char temperature_text[24];
    (void)snprintf(number, sizeof(number), "%u", (unsigned)index + 1U);
    (void)snprintf(duration, sizeof(duration), "%ld min", (long)fixture.duration_minutes);
    (void)snprintf(temperature_text, sizeof(temperature_text), "%ld C", (long)fixture.target_c);
    lv_obj_t *card = editable
        ? create_page_button(explorer, list, "", UI_LAB_SURFACE, UI_LAB_ACTION_STAGE_EDITOR, (int)index)
        : create_panel(list, UI_LAB_SURFACE);
    if (card != NULL && editable && lv_obj_get_child_count(card) > 0U) {
        lv_obj_delete(lv_obj_get_child(card, 0));
    }
    lv_obj_t *top = card == NULL ? NULL : create_transparent_container(card);
    lv_obj_t *values = card == NULL ? NULL : create_transparent_container(card);
    if (card == NULL || top == NULL || values == NULL) { return false; }
    lv_obj_remove_flag(top, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(values, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(card, lv_pct(32)); lv_obj_set_height(card, lv_obj_get_height(explorer->root) < 600 ? 84 : 104); lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(card, UI_LAB_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, UI_LAB_PROGRAM_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(card, UI_LAB_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, 7, LV_PART_MAIN); lv_obj_set_style_pad_bottom(card, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_left(card, 10, LV_PART_MAIN); lv_obj_set_style_pad_right(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 3, LV_PART_MAIN);
    lv_obj_set_size(top, lv_pct(100), 26); lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *name = create_label(top, type_name, &lv_font_montserrat_14, UI_LAB_TEXT);
    lv_obj_t *badge = create_label(top, number, &lv_font_montserrat_14, kind_color);
    lv_obj_set_width(values, lv_pct(100)); lv_obj_set_height(values, 24);
    lv_obj_set_flex_flow(values, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(values, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_t *time = create_label(values, duration, &lv_font_montserrat_14, UI_LAB_TEXT);
    lv_obj_t *temperature = create_label(values, temperature_text, &lv_font_montserrat_20, kind_color);
    if (name == NULL || badge == NULL || time == NULL || temperature == NULL) { return false; }
    lv_obj_set_width(name, 0); lv_obj_set_flex_grow(name, 1); lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_set_size(badge, 24, 24);
    lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(badge, kind_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_text_color(badge, kind_color, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(badge, 0, LV_PART_MAIN);
    lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    return true;
}

static bool build_program_stages(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t stage_count = program_authored_stage_count(&program_fixtures[explorer->selected_program]);
    const size_t per_page = 9U;
    const size_t page_count = (stage_count + per_page - 1U) / per_page;
    if (explorer->program_stage_page >= page_count) { explorer->program_stage_page = (uint8_t)(page_count - 1U); }
    const size_t start = (size_t)explorer->program_stage_page * per_page;
    const size_t end = start + per_page < stage_count ? start + per_page : stage_count;
    lv_obj_t *heading = create_label(explorer->content,
                                     program_name_text(explorer, explorer->selected_program),
                                     &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    if (heading == NULL || toolbar == NULL || list == NULL) { return false; }
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_width(toolbar, lv_pct(100)); lv_obj_set_height(toolbar, 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK), UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_BACK_PROGRAM_DETAIL, 0);
    lv_obj_t *add_stage = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE), UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_PROGRAM_EDITOR, 0);
    lv_obj_t *edit_program = create_page_button(explorer, toolbar, LV_SYMBOL_EDIT, UI_LAB_PRIMARY, UI_LAB_ACTION_PROGRAM_EDITOR, 0);
    char page_text[32];
    (void)snprintf(page_text, sizeof(page_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAGE_FORMAT), (unsigned)explorer->program_stage_page + 1U, (unsigned)page_count);
    lv_obj_t *page = create_label(toolbar, page_text, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *previous = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS), UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_PREVIOUS_STAGE_PAGE, 0);
    lv_obj_t *next = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT), UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_NEXT_STAGE_PAGE, 0);
    if (back == NULL || add_stage == NULL || edit_program == NULL || page == NULL || previous == NULL || next == NULL) { return false; }
    lv_obj_set_width(back, 62); lv_obj_set_width(add_stage, 96); lv_obj_set_size(edit_program, 48, 48);
    lv_obj_set_width(page, 0); lv_obj_set_flex_grow(page, 1);
    lv_obj_set_style_pad_right(page, 8, LV_PART_MAIN);
    lv_obj_set_style_text_align(page, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_width(previous, 72); lv_obj_set_width(next, 72); lv_obj_set_height(add_stage, 48); lv_obj_set_height(previous, 48); lv_obj_set_height(next, 48);
    lv_obj_set_width(list, lv_pct(100)); lv_obj_set_flex_grow(list, 1); lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(list, 8, LV_PART_MAIN); lv_obj_set_style_pad_row(list, 8, LV_PART_MAIN);
    if (explorer->program_stage_page == 0U) {
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    }
    if ((size_t)explorer->program_stage_page + 1U >= page_count) {
        lv_obj_add_state(next, LV_STATE_DISABLED);
    }
    for (size_t index = start; index < end; ++index) {
        if (!make_stage_card(explorer, list, index, false)) { return false; }
    }
    return true;
}
static bool build_program_detail(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const ui_lab_program_fixture_t *program = &program_fixtures[explorer->selected_program % program_fixture_count()];
    prepare_program_preview(explorer, program);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *columns = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (!toolbar || !columns || !actions) { return false; }
    lv_obj_set_size(toolbar, lv_pct(100), 48); lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 8, LV_PART_MAIN);
    const bool favourite = program_is_favourite(explorer, explorer->selected_program);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK), UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_BACK_PROGRAMS, 0);
    lv_obj_t *title = create_label(toolbar, program_name_text(explorer, explorer->selected_program), &lv_font_montserrat_24, UI_LAB_TEXT);
    lv_obj_t *star = create_page_button(explorer, toolbar, "", favourite ? UI_LAB_RUNNING : UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_TOGGLE_FAVOURITE, explorer->selected_program);
    lv_obj_t *delete_program = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT), UI_LAB_PROGRAM_DELETE_BACKGROUND, UI_LAB_ACTION_SHOW_DELETE, explorer->selected_program);
    if (!back || !title || !star || !delete_program) { return false; }
    if (make_favourite_star(star, favourite ? UI_LAB_TEXT : UI_LAB_RUNNING, favourite) == NULL) { return false; }
    lv_obj_set_width(back, 72); lv_obj_set_flex_grow(title, 1); lv_obj_set_size(star, 48, 48); lv_obj_set_width(delete_program, 132);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(delete_program, UI_LAB_PROGRAM_DELETE_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(delete_program, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(delete_program, UI_LAB_PROGRAM_DELETE_BORDER, LV_PART_MAIN);
    lv_obj_set_style_text_color(delete_program, UI_LAB_PROGRAM_DELETE_TEXT, LV_PART_MAIN);
    lv_obj_t *delete_label = lv_obj_get_child(delete_program, 0);
    if (delete_label != NULL) {
        lv_obj_set_style_text_color(delete_label, UI_LAB_PROGRAM_DELETE_TEXT, LV_PART_MAIN);
        lv_obj_set_style_text_font(delete_label, &lv_font_montserrat_14, LV_PART_MAIN);
    }
    /* Columns and actions share one three-column grid and gap, so the graph
     * spans exactly the first two actions and the facts sit above the third. */
    static const int32_t thirds[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t single_row[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    lv_obj_set_width(columns, lv_pct(100)); lv_obj_set_height(columns, 1); lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_grid_dsc_array(columns, thirds, single_row);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN); lv_obj_set_style_pad_row(columns, 0, LV_PART_MAIN);
    lv_obj_t *graph = create_transparent_container(columns), *facts = create_panel(columns, UI_LAB_SURFACE);
    if (!graph || !facts) { return false; }
    lv_obj_set_grid_cell(graph, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(facts, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(graph, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(graph, UI_LAB_GRAPH, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(graph, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(graph, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(graph, UI_LAB_PROGRAM_BORDER, LV_PART_MAIN);
    lv_obj_set_style_radius(graph, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(graph, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(facts, LV_FLEX_FLOW_COLUMN); lv_obj_set_style_pad_column(facts, 0, LV_PART_MAIN); lv_obj_set_style_pad_row(facts, 8, LV_PART_MAIN);
    lv_obj_t *fact_top_row = create_transparent_container(facts);
    lv_obj_t *duration_section = create_transparent_container(facts);
    /* The Duration heading is created first so it sits above its values. */
    lv_obj_t *duration_heading = duration_section == NULL
        ? NULL : create_label(duration_section, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                              &lv_font_montserrat_16, UI_LAB_TEXT);
    lv_obj_t *duration_values = duration_section == NULL
        ? NULL : create_transparent_container(duration_section);
    if (fact_top_row == NULL || duration_section == NULL || duration_heading == NULL ||
        duration_values == NULL) { return false; }
    lv_obj_set_width(fact_top_row, lv_pct(100)); lv_obj_set_height(fact_top_row, 0);
    lv_obj_set_flex_grow(fact_top_row, 1); lv_obj_set_flex_flow(fact_top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(fact_top_row, 8, LV_PART_MAIN);
    lv_obj_set_width(duration_section, lv_pct(100)); lv_obj_set_height(duration_section, 0);
    lv_obj_set_flex_grow(duration_section, 2); lv_obj_set_flex_flow(duration_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(duration_section, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(duration_section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(duration_section, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(duration_section, UI_LAB_PROGRAM_BORDER, LV_PART_MAIN);
    lv_obj_set_width(duration_values, lv_pct(100)); lv_obj_set_height(duration_values, 0);
    lv_obj_set_flex_grow(duration_values, 1); lv_obj_set_flex_flow(duration_values, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(duration_values, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    char count[24]; snprintf(count, sizeof(count), "%u", (unsigned)program_authored_stage_count(program));
    if (!make_detail_fact(fact_top_row, "Number of stages", count, 50) ||
        !make_detail_fact(fact_top_row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MAX_TEMPERATURE), program_maximum(program), 50) ||
        !make_detail_row(duration_values, "Curing", program_duration(program)) ||
        !make_detail_row(duration_values, "Est. cooling", program_cooling(program)) ||
        !make_detail_row(duration_values, "Est. total", program_total(program)) ||
        !make_trajectory(explorer, graph, 0)) { return false; }
    lv_obj_set_style_border_width(explorer->trajectory, 0, LV_PART_MAIN);
    lv_obj_set_size(actions, lv_pct(100), 48);
    lv_obj_set_grid_dsc_array(actions, thirds, single_row);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN); lv_obj_set_style_pad_row(actions, 0, LV_PART_MAIN);
    lv_obj_t *stages = create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VIEW_STAGES), UI_LAB_SURFACE, UI_LAB_ACTION_PROGRAM_STAGES, 0);
    lv_obj_t *edit = create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM), UI_LAB_SURFACE, UI_LAB_ACTION_PROGRAM_EDITOR, explorer->selected_program);
    lv_obj_t *load = create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOAD_PROGRAM), UI_LAB_DEEP_BLUE, UI_LAB_ACTION_PROGRAM_LOADING, explorer->selected_program);
    if (!stages || !edit || !load) { return false; }
    style_outline_action(stages);
    style_outline_action(edit);
    lv_obj_set_grid_cell(stages, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(edit, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(load, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    return true;
}static bool build_stage_detail(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const ui_lab_stage_fixture_t fixture = program_stage(explorer, explorer->selected_stage);
    const ui_lab_stage_fixture_t *stage = &fixture;
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *summary = create_panel(explorer->content, UI_LAB_SURFACE);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    if (toolbar == NULL || summary == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_t *back = create_page_button(explorer, toolbar,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_SURFACE_RAISED,
                                        UI_LAB_ACTION_BACK_PROGRAM_DETAIL, 0);
    char identity[128];
    snprintf(identity, sizeof(identity), "%s\nStage %u / %u - %s",
             program_name_text(explorer, explorer->selected_program),
             (unsigned)explorer->selected_stage + 1U,
             (unsigned)editor_stage_count(explorer),
             ui_string(stage->kind));
    lv_obj_t *title = create_label(toolbar, identity, &lv_font_montserrat_20, UI_LAB_TEXT);
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
    lv_obj_t *kind = create_label(summary, row, &lv_font_montserrat_16, stage_kind_color(fixture_stage_kind(stage)));
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TARGET_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET), (long)stage->target_c);
    lv_obj_t *target = create_label(summary, row, &lv_font_montserrat_20, UI_LAB_TEXT);
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE), "");
    char stage_rate[32];
    if (stage->kind == FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD) { snprintf(stage_rate, sizeof(stage_rate), "%s", ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INHERITED_TARGET)); }
    else { format_rate_tenths(stage_rate, sizeof(stage_rate), stage->delta_c_per_minute); }
    snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE), stage_rate);
    lv_obj_t *rate = create_label(summary, row, &lv_font_montserrat_20, UI_LAB_TEXT);
    (void)snprintf(row, sizeof(row), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_DURATION_FORMAT),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION), (long)stage->duration_minutes);
    lv_obj_t *duration = create_label(summary, row, &lv_font_montserrat_20, UI_LAB_TEXT);
    if (kind == NULL || target == NULL || rate == NULL || duration == NULL) {
        return false;
    }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 56);
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
    ui_lab_action_t increment,
    uint8_t numeric_target
)
{
    lv_obj_t *row = create_panel(parent, UI_LAB_SURFACE_RAISED);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 58);
    lv_obj_set_style_pad_top(row, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(row, 4, LV_PART_MAIN);
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
    lv_obj_t *shown_value = create_page_button(
        explorer, row, formatted, UI_LAB_SURFACE,
        UI_LAB_ACTION_STAGE_EDIT_VALUE, numeric_target);
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
    lv_obj_set_width(shown_value, 160);
    lv_obj_set_style_text_font(lv_obj_get_child(shown_value, 0), &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_size(minus, 48, 48);
    lv_obj_set_size(plus, 48, 48);
    return true;
}

static bool build_stage_editor(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *heading = create_panel(explorer->content, UI_LAB_SURFACE);
    if (heading == NULL) { return false; }
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 56);
    lv_obj_set_style_pad_all(heading, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(heading, 6, LV_PART_MAIN);
    lv_obj_t *previous = create_page_button(explorer, heading, LV_SYMBOL_LEFT,
                                            UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_PREVIOUS_STAGE, 0);
    char identity[128];
    (void)snprintf(identity, sizeof(identity), "%s\nStage %u / %u - %s",
                   explorer->program_editor_active ? explorer->program_editor_name :
                       program_name_text(explorer, explorer->selected_program),
                   (unsigned)explorer->selected_stage + 1U,
                   (unsigned)editor_stage_count(explorer),
                   ui_string(program_stage(explorer, explorer->selected_stage).kind));
    lv_obj_t *title = create_label(heading, identity, &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *next = create_page_button(explorer, heading, LV_SYMBOL_RIGHT,
                                        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_NEXT_STAGE, 0);
    if (previous == NULL || title == NULL || next == NULL) { return false; }
    lv_obj_set_size(previous, 40, 40); lv_obj_set_flex_grow(title, 1); lv_obj_set_size(next, 40, 40);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_t *kind_actions = create_transparent_container(explorer->content);
    if (kind_actions == NULL) {
        return false;
    }
    lv_obj_set_width(kind_actions, lv_pct(100));
    lv_obj_set_height(kind_actions, 48);
    lv_obj_set_flex_flow(kind_actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(kind_actions, 7, LV_PART_MAIN);
    const furnace_hmi_ui_string_id_t kind_ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING,
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD,
    };
    const ui_lab_action_t kind_actions_ids[] = {
        UI_LAB_ACTION_SET_DRAFT_HEATING,
        UI_LAB_ACTION_SET_DRAFT_HOLD,
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
        lv_obj_set_height(button, 48);
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
                             UI_LAB_ACTION_INCREMENT_DURATION,
                             UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION)) {
            return false;
        }
    }
    else {
        if (!make_editor_row(explorer, explorer->content,
                             ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET),
                             explorer->draft_target_c,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                             UI_LAB_ACTION_DECREMENT_TARGET,
                             UI_LAB_ACTION_INCREMENT_TARGET,
                             UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET)) {
            return false;
        }
        if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HEATING) {
            if (!make_editor_row(explorer, explorer->content,
                                 explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_COOL
                                     ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOL_CONTROLLED)
                                     : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE),
                                 explorer->draft_rate_tenths_c_per_minute,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_RATE_TENTHS),
                                 UI_LAB_ACTION_DECREMENT_RATE,
                                 UI_LAB_ACTION_INCREMENT_RATE,
                                 UI_LAB_KEYBOARD_TARGET_DRAFT_RATE)) {
                return false;
            }
        }
        if (explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HEATING) {
            if (!make_editor_row(explorer, explorer->content,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                                 explorer->draft_duration_minutes,
                                 ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                                 UI_LAB_ACTION_DECREMENT_DURATION,
                                 UI_LAB_ACTION_INCREMENT_DURATION,
                                 UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION)) {
                return false;
            }
            lv_obj_t *relationship = create_transparent_container(explorer->content);
            if (relationship == NULL) {
                return false;
            }
            lv_obj_set_width(relationship, lv_pct(100));
            lv_obj_set_height(relationship, 18);
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
            const int32_t inherited = explorer->selected_stage == 0 ? 25 : program_stage(explorer, explorer->selected_stage - 1U).target_c;
            const bool invalid = explorer->draft_target_c <= inherited || explorer->draft_rate_tenths_c_per_minute <= 0;
            if (create_label(relationship, invalid ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DRAFT_VALIDATION) : calculated, &lv_font_montserrat_14, invalid ? UI_LAB_FAULT : UI_LAB_MUTED) == NULL) {
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
    lv_obj_set_height(action_spacer, 0);
    lv_obj_set_flex_grow(action_spacer, 1);
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 56);
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
    lv_obj_set_height(discard, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_height(save, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_flex_grow(discard, 1);
    lv_obj_set_flex_grow(save, 1);
    return true;
}

enum {
    UI_LAB_DEVICE_TAB_OVERVIEW = 0,
    UI_LAB_DEVICE_TAB_COMPONENTS,
    UI_LAB_DEVICE_TAB_NOTIFICATIONS,
    UI_LAB_DEVICE_TAB_HISTORY,
    UI_LAB_DEVICE_TAB_INFO,
    UI_LAB_DEVICE_TAB_COUNT,
};

enum {
    UI_LAB_DEVICE_FILTER_ALL = 0,
    UI_LAB_DEVICE_FILTER_ATTENTION,
    UI_LAB_DEVICE_FILTER_SERVICE,
    UI_LAB_DEVICE_FILTER_ERRORS,
};

#define UI_LAB_DEVICE_PAGE_SIZE 3U
#define UI_LAB_DEVICE_FAULT_NOTICE 0xFFFF

typedef struct {
    bool fault;
    uint8_t component;
    uint8_t task;
} ui_lab_device_notice_ref_t;

static const furnace_hmi_ui_string_id_t device_tab_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_OVERVIEW,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG,
    FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_HISTORY,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INFO,
};

static const furnace_hmi_ui_string_id_t device_history_title_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_INSPECTION_TITLE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_ACK_TITLE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_REPLACEMENT_TITLE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_FAN_TITLE,
};

static const furnace_hmi_ui_string_id_t device_history_detail_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_INSPECTION_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_ACK_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_REPLACEMENT_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_FAN_DETAIL,
};

static const furnace_hmi_ui_string_id_t device_history_text_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_INSPECTION_TEXT,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_ACK_TEXT,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_REPLACEMENT_TEXT,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_FAN_TEXT,
};

static const furnace_hmi_ui_string_id_t device_info_label_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_IDENTITY,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FIRMWARE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROLLER_LINK,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CATALOGUE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT,
};

static const furnace_hmi_ui_string_id_t device_info_detail_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_IDENTITY_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FIRMWARE_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROLLER_LINK_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CATALOGUE_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_DETAIL,
};

static const char *device_task_state_text(
    furnace_hmi_ui_lab_device_task_state_t state
)
{
    switch (state) {
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_OVERDUE);
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DUE_SOON);
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONDITION_BASED);
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE:
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_UP_TO_DATE);
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN:
    default:
        return ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN);
    }
}

static lv_color_t device_task_state_color(
    furnace_hmi_ui_lab_device_task_state_t state
)
{
    switch (state) {
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE:
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON:
        return UI_LAB_RUNNING;
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE:
        return UI_LAB_MANUAL;
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION:
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN:
    default:
        return UI_LAB_MUTED;
    }
}

static bool device_task_needs_attention(
    const furnace_hmi_ui_lab_device_task_t *task
)
{
    return task != NULL &&
        (task->task_state == FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE ||
         task->task_state == FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON ||
         task->task_state == FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN);
}

static bool device_component_visible(
    const furnace_hmi_ui_lab_device_component_t *component,
    uint8_t filter
)
{
    if (component == NULL) {
        return false;
    }
    if (filter != UI_LAB_DEVICE_FILTER_ATTENTION) {
        return true;
    }
    for (uint8_t index = 0U; index < component->task_count; ++index) {
        if (device_task_needs_attention(&component->tasks[index])) {
            return true;
        }
    }
    return false;
}

static const furnace_hmi_ui_lab_device_task_t *device_component_summary(
    const furnace_hmi_ui_lab_device_component_t *component
)
{
    static const uint8_t rank[] = { 4U, 2U, 0U, 3U, 1U };
    const furnace_hmi_ui_lab_device_task_t *summary = NULL;
    uint8_t best = UINT8_MAX;
    if (component == NULL) {
        return NULL;
    }
    for (uint8_t index = 0U; index < component->task_count; ++index) {
        const furnace_hmi_ui_lab_device_task_t *task = &component->tasks[index];
        const uint8_t task_rank = rank[task->task_state <=
            FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN ? task->task_state : 4U];
        if (summary == NULL || task_rank < best) {
            summary = task;
            best = task_rank;
        }
    }
    return summary;
}

static bool device_add_category_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    uint8_t tab
)
{
    lv_obj_t *button = create_page_button(
        explorer, parent, ui_string(device_tab_ids[tab]),
        explorer->device_tab == tab ? UI_LAB_DEEP_BLUE : UI_LAB_BACKGROUND,
        UI_LAB_ACTION_DEVICE_TAB, tab
    );
    if (button == NULL) {
        return false;
    }
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_grow(button, 0);
    lv_obj_set_style_bg_opa(
        button, explorer->device_tab == tab ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, explorer->device_tab == tab ? 1 : 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, UI_LAB_NAV_GLOW, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_width(label, lv_pct(100));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_text_color(label, explorer->device_tab == tab ? UI_LAB_TEXT : UI_LAB_MUTED,
                                    LV_PART_MAIN);
    }
    return true;
}

static bool device_add_toolbar(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *title,
    bool back,
    bool part_info,
    uint8_t filter_count
)
{
    lv_obj_t *toolbar = create_transparent_container(parent);
    if (toolbar == NULL) {
        return false;
    }
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 6, LV_PART_MAIN);

    if (back) {
        lv_obj_t *back_button = create_page_button(
            explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
            UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_DEVICE_BACK, 0);
        if (back_button == NULL) {
            return false;
        }
        lv_obj_set_width(back_button, 64);
    }
    lv_obj_t *heading = create_label(toolbar, title, &lv_font_montserrat_20, UI_LAB_TEXT);
    if (heading == NULL) {
        return false;
    }
    lv_obj_set_width(heading, 0);
    lv_obj_set_flex_grow(heading, 1);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);

    if (part_info) {
        lv_obj_t *info = create_page_button(
            explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PART_INFO),
            UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_DEVICE_PART_INFO, 0);
        if (info == NULL) {
            return false;
        }
        lv_obj_set_width(info, 92);
    }

    static const furnace_hmi_ui_string_id_t filter_ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ALL,
        FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ATTENTION,
        FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE,
        FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ERRORS,
    };
    static const uint8_t filter_values[] = { 0U, 1U, 2U, 3U };
    for (uint8_t index = 0U; index < filter_count; ++index) {
        const uint8_t filter_index = filter_count == 3U && index > 0U
            ? (uint8_t)(index + 1U) : index;
        lv_obj_t *filter = create_page_button(
            explorer, toolbar, ui_string(filter_ids[filter_index]),
            explorer->device_filter == filter_values[filter_index]
                ? UI_LAB_DEEP_BLUE : UI_LAB_SURFACE_RAISED,
            UI_LAB_ACTION_DEVICE_FILTER, filter_values[filter_index]);
        if (filter == NULL) {
            return false;
        }
        lv_obj_set_width(filter, index < 2U ? 82 : 70);
    }
    return true;
}

static bool device_add_row(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *title,
    const char *detail,
    const char *value,
    lv_color_t value_color,
    ui_lab_action_t action,
    int action_value
)
{
    lv_obj_t *row = create_panel(parent, UI_LAB_SURFACE);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 52);
    lv_obj_set_style_pad_hor(row, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = create_transparent_container(row);
    if (left == NULL) {
        return false;
    }
    lv_obj_set_width(left, 0);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, detail == NULL ? 40 : 46);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *title_label = create_label(left, title, &lv_font_montserrat_16, UI_LAB_TEXT);
    lv_obj_t *detail_label = detail == NULL ? NULL :
        create_label(left, detail, &lv_font_montserrat_14, UI_LAB_MUTED);
    if (title_label == NULL || (detail != NULL && detail_label == NULL)) {
        return false;
    }
    lv_obj_set_width(title_label, lv_pct(100));
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
    if (detail_label != NULL) {
        lv_obj_set_width(detail_label, lv_pct(100));
        lv_label_set_long_mode(detail_label, LV_LABEL_LONG_DOT);
    }
    lv_obj_t *tag = create_label(row, value, &lv_font_montserrat_14, value_color);
    if (tag == NULL ||
        explorer->page_action_count >= sizeof(explorer->page_actions) /
            sizeof(explorer->page_actions[0])) {
        return false;
    }
    lv_obj_set_width(tag, 156);
    lv_obj_set_style_text_align(tag, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_label_set_long_mode(tag, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_USER_1);
    furnace_hmi_ui_lab_action_slot_t *slot =
        &explorer->page_actions[explorer->page_action_count++];
    slot->explorer = explorer;
    slot->action = (int)action;
    slot->value = action_value;
    lv_obj_add_event_cb(row, action_callback, LV_EVENT_CLICKED, slot);
    return true;
}

static lv_obj_t *device_create_list(lv_obj_t *parent)
{
    lv_obj_t *list = create_transparent_container(parent);
    if (list == NULL) {
        return NULL;
    }
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_height(list, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 6, LV_PART_MAIN);
    return list;
}

static bool device_add_empty(lv_obj_t *parent, const char *text)
{
    lv_obj_t *empty = create_panel(parent, UI_LAB_SURFACE);
    if (empty == NULL) {
        return false;
    }
    lv_obj_set_width(empty, lv_pct(100));
    lv_obj_set_height(empty, 120);
    lv_obj_set_flex_flow(empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(empty, 18, LV_PART_MAIN);
    lv_obj_t *label = create_label(empty, text, &lv_font_montserrat_14, UI_LAB_MUTED);
    if (label == NULL) {
        return false;
    }
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    return true;
}

static bool device_add_footer(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    uint16_t item_count
)
{
    const uint16_t page_count = item_count == 0U
        ? 1U : (uint16_t)((item_count + UI_LAB_DEVICE_PAGE_SIZE - 1U) /
                          UI_LAB_DEVICE_PAGE_SIZE);
    if (explorer->device_page >= page_count) {
        explorer->device_page = (uint8_t)(page_count - 1U);
    }
    lv_obj_t *spacer = create_transparent_container(parent);
    lv_obj_t *footer = create_transparent_container(parent);
    if (spacer == NULL || footer == NULL) {
        return false;
    }
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_height(spacer, 0);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_height(footer, 48);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 8, LV_PART_MAIN);

    char count[64];
    (void)snprintf(
        count, sizeof(count),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ITEM_COUNT_FORMAT),
        (unsigned)item_count, (unsigned)explorer->device_page + 1U,
        (unsigned)page_count,
        explorer->state != NULL &&
                explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_KNOWN_SUFFIX) : "");
    lv_obj_t *count_label = create_label(footer, count, &lv_font_montserrat_14, UI_LAB_MUTED);
    if (count_label == NULL) {
        return false;
    }
    lv_obj_set_width(count_label, 0);
    lv_obj_set_flex_grow(count_label, 1);
    lv_label_set_long_mode(count_label, LV_LABEL_LONG_DOT);

    lv_obj_t *previous = create_page_button(
        explorer, footer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_DEVICE_PREVIOUS, 0);
    lv_obj_t *next = create_page_button(
        explorer, footer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEXT),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_DEVICE_NEXT, 0);
    if (previous == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_width(previous, 84);
    lv_obj_set_width(next, 64);
    if (explorer->device_page == 0U) {
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    }
    if ((uint16_t)explorer->device_page + 1U >= page_count) {
        lv_obj_add_state(next, LV_STATE_DISABLED);
    }
    return true;
}

static uint16_t device_count_components(
    const furnace_hmi_ui_lab_explorer_t *explorer
)
{
    uint16_t count = 0U;
    if (explorer == NULL) {
        return 0U;
    }
    for (uint8_t index = 0U; index < explorer->device_catalogue.component_count; ++index) {
        if (device_component_visible(&explorer->device_catalogue.components[index],
                                     explorer->device_filter)) {
            ++count;
        }
    }
    return count;
}

static size_t device_collect_notices(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t filter,
    ui_lab_device_notice_ref_t *refs,
    size_t capacity
)
{
    size_t count = 0U;
    if (explorer == NULL || refs == NULL || capacity == 0U ||
        explorer->state == NULL ||
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return 0U;
    }
    if (filter != UI_LAB_DEVICE_FILTER_SERVICE &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT &&
        explorer->state->run_state == FURNACE_HMI_RUN_STATE_FAULT && count < capacity) {
        refs[count++] = (ui_lab_device_notice_ref_t){ true, 0U, 0U };
    }
    static const furnace_hmi_ui_lab_device_task_state_t order[] = {
        FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN,
    };
    for (size_t rank = 0U; rank < sizeof(order) / sizeof(order[0]); ++rank) {
        for (uint8_t component = 0U;
             component < explorer->device_catalogue.component_count && count < capacity;
             ++component) {
            const furnace_hmi_ui_lab_device_component_t *part =
                &explorer->device_catalogue.components[component];
            for (uint8_t task = 0U; task < part->task_count && count < capacity; ++task) {
                if (filter == UI_LAB_DEVICE_FILTER_ERRORS ||
                    part->tasks[task].task_state != order[rank]) {
                    continue;
                }
                refs[count++] = (ui_lab_device_notice_ref_t){ false, component, task };
            }
        }
    }
    return count;
}

static bool device_add_metric(
    lv_obj_t *parent,
    const char *label,
    const char *value,
    const char *basis
)
{
    lv_obj_t *metric = create_panel(parent, UI_LAB_SURFACE);
    if (metric == NULL) {
        return false;
    }
    lv_obj_set_width(metric, 0);
    lv_obj_set_flex_grow(metric, 1);
    lv_obj_set_height(metric, 84);
    lv_obj_set_style_pad_all(metric, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(metric, 0, LV_PART_MAIN);
    lv_obj_t *label_object = create_label(metric, label, &lv_font_montserrat_14, UI_LAB_MUTED);
    lv_obj_t *value_object = create_label(metric, value, &lv_font_montserrat_20, UI_LAB_TEXT);
    lv_obj_t *basis_object = create_label(metric, basis, &lv_font_montserrat_14, UI_LAB_MUTED);
    if (label_object == NULL || value_object == NULL || basis_object == NULL) {
        return false;
    }
    lv_obj_set_width(label_object, lv_pct(100));
    lv_obj_set_width(value_object, lv_pct(100));
    lv_obj_set_width(basis_object, lv_pct(100));
    lv_label_set_long_mode(label_object, LV_LABEL_LONG_WRAP);
    lv_label_set_long_mode(basis_object, LV_LABEL_LONG_WRAP);
    return true;
}

static bool device_build_overview(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_RUNTIME_SERVICE),
                     &lv_font_montserrat_20, UI_LAB_TEXT) == NULL ||
        create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_RUNTIME_SUBTITLE),
                     &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) {
        return false;
    }
    lv_obj_t *metrics = create_transparent_container(body);
    if (metrics == NULL) {
        return false;
    }
    lv_obj_set_width(metrics, lv_pct(100));
    lv_obj_set_height(metrics, 84);
    lv_obj_set_flex_flow(metrics, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(metrics, 8, LV_PART_MAIN);
    if (!device_add_metric(metrics,
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_POWERED_ON_TIME),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_POWERED_ON_VALUE),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FURNACE_LIFETIME)) ||
        !device_add_metric(metrics,
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PROGRAM_RUNTIME),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PROGRAM_RUNTIME_VALUE),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_EXCLUDES_MANUAL_PAUSES)) ||
        !device_add_metric(metrics,
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAN_RUNTIME),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAN_RUNTIME_VALUE),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONFIRMED_ROTATION))) {
        return false;
    }
    ui_lab_device_notice_ref_t notices[
        FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS *
        FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS + 1U];
    const size_t service_count = device_collect_notices(
        explorer, UI_LAB_DEVICE_FILTER_SERVICE, notices, sizeof(notices) / sizeof(notices[0]));
    const size_t error_count = device_collect_notices(
        explorer, UI_LAB_DEVICE_FILTER_ERRORS, notices, sizeof(notices) / sizeof(notices[0]));
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) {
        return false;
    }
    char service_value[32];
    char error_value[32];
    (void)snprintf(service_value, sizeof(service_value),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TASK_COUNT_FORMAT),
                   (unsigned)service_count);
    (void)snprintf(error_value, sizeof(error_value),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ACTIVE_COUNT_FORMAT),
                   (unsigned)error_count);
    const char *service_detail = explorer->device_catalogue.component_count == 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_COMPONENTS)
        : service_count > 0U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_REVIEW_TASKS)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_SCHEDULED_TASK);
    const char *error_title = explorer->state != NULL &&
            explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_REPORTED_ERRORS)
        : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CURRENT_ERRORS);
    const char *error_detail = error_count > 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_READ_CONTROLLER_REPORT)
        : explorer->state != NULL &&
            explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_ERRORS_LAST_SNAPSHOT)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_CURRENT_REPORTED_ERRORS);
    return device_add_row(
               explorer, list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_ATTENTION),
               service_detail, service_value, service_count > 0U ? UI_LAB_RUNNING : UI_LAB_MUTED,
               UI_LAB_ACTION_DEVICE_FILTER,
               (UI_LAB_DEVICE_TAB_NOTIFICATIONS << 8) | UI_LAB_DEVICE_FILTER_SERVICE) &&
           device_add_row(
               explorer, list, error_title, error_detail, error_value,
               error_count > 0U ? UI_LAB_FAULT : UI_LAB_MUTED, UI_LAB_ACTION_DEVICE_FILTER,
               (UI_LAB_DEVICE_TAB_COMPONENTS << 8) | UI_LAB_DEVICE_FILTER_ERRORS);
}

static bool device_build_log(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (!device_add_toolbar(explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG),
                            false, false, 0U)) {
        return false;
    }
    static const char *const titles[] = {
        "Program start", "Manual mode started", "Program paused", "Interior light toggled",
    };
    static const char *const details[] = {
        "Control CPU accepted a program-start request",
        "Control CPU accepted a manual-start request",
        "Operator pause request recorded",
        "Operator light request recorded",
    };
    static const char *const times[] = { "Today 14:32", "Today 13:08", "Yesterday 17:44", "Yesterday 17:42" };
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) { return false; }
    for (size_t index = 0; index < sizeof(titles) / sizeof(titles[0]); ++index) {
        if (!device_add_row(explorer, list, titles[index], details[index], times[index],
                            UI_LAB_MUTED, UI_LAB_ACTION_DEVICE_LOG_EVENT, (int)index)) {
            return false;
        }
    }
    return true;
}

static bool device_build_components(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (explorer->device_component_selected &&
        explorer->device_selected_component < explorer->device_catalogue.component_count) {
        const furnace_hmi_ui_lab_device_component_t *part =
            &explorer->device_catalogue.components[explorer->device_selected_component];
        if (!device_add_toolbar(explorer, body, part->name, true, true, 0U)) {
            return false;
        }
        lv_obj_t *list = device_create_list(body);
        if (list == NULL) {
            return false;
        }
        const uint16_t task_count = part->task_count;
        const uint16_t first = (uint16_t)explorer->device_page * UI_LAB_DEVICE_PAGE_SIZE;
        const uint16_t last = first + UI_LAB_DEVICE_PAGE_SIZE < task_count
            ? first + UI_LAB_DEVICE_PAGE_SIZE : task_count;
        if (task_count == 0U &&
            !device_add_empty(list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_TASKS))) {
            return false;
        }
        for (uint16_t index = first; index < last; ++index) {
            const furnace_hmi_ui_lab_device_task_t *task = &part->tasks[index];
            char detail[256];
            (void)snprintf(
                detail, sizeof(detail),
                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ROW_DETAIL_FORMAT),
                task->basis, task->interval);
            if (!device_add_row(
                    explorer, list, task->name, detail, task->remaining,
                    device_task_state_color(task->task_state), UI_LAB_ACTION_DEVICE_TASK,
                    ((int)explorer->device_selected_component << 8) | (int)index)) {
                return false;
            }
        }
        return device_add_footer(explorer, body, task_count);
    }

    if (!device_add_toolbar(
            explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_COMPONENTS),
            false, false, 2U)) {
        return false;
    }
    const uint16_t count = device_count_components(explorer);
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) {
        return false;
    }
    uint16_t visible = 0U;
    const uint16_t first = (uint16_t)explorer->device_page * UI_LAB_DEVICE_PAGE_SIZE;
    for (uint8_t index = 0U; index < explorer->device_catalogue.component_count; ++index) {
        const furnace_hmi_ui_lab_device_component_t *part =
            &explorer->device_catalogue.components[index];
        if (!device_component_visible(part, explorer->device_filter)) {
            continue;
        }
        if (visible >= first && visible < first + UI_LAB_DEVICE_PAGE_SIZE) {
            const furnace_hmi_ui_lab_device_task_t *summary = device_component_summary(part);
            char detail[256];
            const char *value = summary == NULL
                ? ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN)
                : device_task_state_text(summary->task_state);
            (void)snprintf(
                detail, sizeof(detail),
                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ROW_DETAIL_FORMAT),
                part->usage,
                summary == NULL
                    ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_TASKS)
                    : summary->name);
            if (!device_add_row(
                    explorer, list, part->name, detail, value,
                    summary == NULL ? UI_LAB_MUTED :
                        device_task_state_color(summary->task_state),
                    UI_LAB_ACTION_DEVICE_COMPONENT, index)) {
                return false;
            }
        }
        ++visible;
    }
    if (count == 0U &&
        !device_add_empty(
            list, explorer->device_catalogue.component_count == 0U
                ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_COMPONENTS)
                : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_MATCH))) {
        return false;
    }
    return device_add_footer(explorer, body, count);
}

static bool device_build_notifications(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (!device_add_toolbar(
            explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE),
            false, false, 3U)) {
        return false;
    }
    ui_lab_device_notice_ref_t refs[
        FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS *
        FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS + 1U];
    const size_t count = device_collect_notices(
        explorer, explorer->device_filter, refs, sizeof(refs) / sizeof(refs[0]));
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) {
        return false;
    }
    const size_t first = (size_t)explorer->device_page * UI_LAB_DEVICE_PAGE_SIZE;
    const size_t last = first + UI_LAB_DEVICE_PAGE_SIZE < count
        ? first + UI_LAB_DEVICE_PAGE_SIZE : count;
    for (size_t index = first; index < last; ++index) {
        const ui_lab_device_notice_ref_t *ref = &refs[index];
        if (ref->fault) {
            if (!device_add_row(
                    explorer, list,
                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_ROW_TITLE),
                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_ROW_DETAIL),
                    ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAULT), UI_LAB_FAULT,
                    UI_LAB_ACTION_DEVICE_NOTICE, UI_LAB_DEVICE_FAULT_NOTICE)) {
                return false;
            }
            continue;
        }
        const furnace_hmi_ui_lab_device_component_t *part =
            &explorer->device_catalogue.components[ref->component];
        const furnace_hmi_ui_lab_device_task_t *task = &part->tasks[ref->task];
        char detail[256];
        (void)snprintf(
            detail, sizeof(detail),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ROW_DETAIL_FORMAT),
            task->name, task->remaining);
        if (!device_add_row(
                explorer, list, part->name, detail,
                device_task_state_text(task->task_state),
                device_task_state_color(task->task_state), UI_LAB_ACTION_DEVICE_NOTICE,
                ((int)ref->component << 8) | (int)ref->task)) {
            return false;
        }
    }
    if (count == 0U &&
        !device_add_empty(list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_NOTICES))) {
        return false;
    }
    return device_add_footer(explorer, body, (uint16_t)count);
}

static bool device_build_history(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (!device_add_toolbar(
            explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_HISTORY),
            false, false, 0U)) {
        return false;
    }
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) {
        return false;
    }
    const uint16_t count = (uint16_t)(sizeof(device_history_title_ids) /
                                      sizeof(device_history_title_ids[0]));
    const uint16_t first = (uint16_t)explorer->device_page * UI_LAB_DEVICE_PAGE_SIZE;
    const uint16_t last = first + UI_LAB_DEVICE_PAGE_SIZE < count
        ? first + UI_LAB_DEVICE_PAGE_SIZE : count;
    for (uint16_t index = first; index < last; ++index) {
        const char *kind = index == 1U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ACKNOWLEDGEMENT)
            : index == 2U
                ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_REPLACEMENT)
                : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INSPECTION);
        if (!device_add_row(
                explorer, list, ui_string(device_history_title_ids[index]),
                ui_string(device_history_detail_ids[index]), kind, UI_LAB_MUTED,
                UI_LAB_ACTION_DEVICE_HISTORY, index)) {
            return false;
        }
    }
    if (count == 0U &&
        !device_add_empty(list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_HISTORY))) {
        return false;
    }
    return device_add_footer(explorer, body, count);
}

static bool device_build_info(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (!device_add_toolbar(
            explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INFO),
            false, false, 0U)) {
        return false;
    }
    char catalogue_value[96];
    (void)snprintf(
        catalogue_value, sizeof(catalogue_value),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CATALOGUE_VALUE_FORMAT),
        explorer->device_catalogue.revision,
        (unsigned)explorer->device_catalogue.component_count);
    const char *values[] = {
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_IDENTITY_VALUE),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FIRMWARE_VALUE),
        explorer->state != NULL &&
                explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_STALE_SNAPSHOT)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CURRENT_SNAPSHOT),
        catalogue_value,
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_VALUE),
    };
    lv_obj_t *list = device_create_list(body);
    if (list == NULL) {
        return false;
    }
    const uint8_t count = 5U;
    const uint8_t first = (uint8_t)explorer->device_page * UI_LAB_DEVICE_PAGE_SIZE;
    const uint8_t last = first + UI_LAB_DEVICE_PAGE_SIZE < count
        ? (uint8_t)(first + UI_LAB_DEVICE_PAGE_SIZE) : count;
    for (uint8_t index = first; index < last; ++index) {
        if (!device_add_row(
                explorer, list, ui_string(device_info_label_ids[index]),
                values[index], ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DETAILS),
                UI_LAB_MUTED,
                UI_LAB_ACTION_DEVICE_INFO, index)) {
            return false;
        }
    }
    return device_add_footer(explorer, body, count);
}

static bool device_build_unavailable(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    const uint8_t tab = explorer->device_tab < UI_LAB_DEVICE_TAB_COUNT
        ? explorer->device_tab : UI_LAB_DEVICE_TAB_OVERVIEW;
    return device_add_toolbar(explorer, body, ui_string(device_tab_ids[tab]),
                              false, false, 0U) &&
           device_add_empty(
               body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_UNAVAILABLE));
}

static bool device_build(
    furnace_hmi_ui_lab_explorer_t *explorer
)
{
    lv_obj_set_flex_flow(explorer->content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(explorer->content, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->content, 0, LV_PART_MAIN);
    lv_obj_t *rail = create_transparent_container(explorer->content);
    lv_obj_t *body = create_transparent_container(explorer->content);
    if (rail == NULL || body == NULL) {
        return false;
    }
    lv_obj_set_width(rail, 184);
    lv_obj_set_height(rail, lv_pct(100));
    lv_obj_set_flex_flow(rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rail, 8, LV_PART_MAIN);
    lv_obj_set_width(body, 0);
    lv_obj_set_height(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, LV_PART_MAIN);
    for (uint8_t tab = 0U; tab < UI_LAB_DEVICE_TAB_COUNT; ++tab) {
        if (!device_add_category_button(explorer, rail, tab)) {
            return false;
        }
    }
    if (explorer->state != NULL &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE &&
        create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_STALE_BANNER),
                     &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) {
        return false;
    }
    if (explorer->state != NULL &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return device_build_unavailable(explorer, body);
    }
    switch (explorer->device_tab) {
    case UI_LAB_DEVICE_TAB_OVERVIEW:
        return device_build_overview(explorer, body);
   case UI_LAB_DEVICE_TAB_COMPONENTS:
        return device_build_log(explorer, body);
    case UI_LAB_DEVICE_TAB_NOTIFICATIONS:
        return device_build_notifications(explorer, body);
    case UI_LAB_DEVICE_TAB_HISTORY:
        return device_build_history(explorer, body);
    case UI_LAB_DEVICE_TAB_INFO:
        return device_build_info(explorer, body);
    default:
        explorer->device_tab = UI_LAB_DEVICE_TAB_OVERVIEW;
        return device_build_overview(explorer, body);
    }
}

static bool build_device(furnace_hmi_ui_lab_explorer_t *explorer)
{
    return device_build(explorer);
}

enum {
    UI_LAB_SETTINGS_TAB_DISPLAY = 0,
    UI_LAB_SETTINGS_TAB_TIME,
    UI_LAB_SETTINGS_TAB_CONNECTIONS,
    UI_LAB_SETTINGS_TAB_MAINTENANCE,
};

enum {
    UI_LAB_SETTINGS_NUMBER_RATE = 0,
    UI_LAB_SETTINGS_NUMBER_TEMPERATURE,
    UI_LAB_SETTINGS_NUMBER_DURATION,
    UI_LAB_SETTINGS_NUMBER_BRIGHTNESS,
    UI_LAB_SETTINGS_NUMBER_DIM,
    UI_LAB_SETTINGS_NUMBER_COUNT,
};

static const furnace_hmi_ui_string_id_t settings_tab_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_INTERACTION,
    FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_TIME,
    FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONNECTIONS,
    FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MAINTENANCE,
};

static bool settings_dirty(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL) {
        return false;
    }
    for (size_t index = 0U; index < UI_LAB_SETTINGS_NUMBER_COUNT; ++index) {
        if (explorer->settings_saved_numeric[index] != explorer->settings_candidate_numeric[index]) {
            return true;
        }
    }
    return explorer->settings_saved_reduced_motion != explorer->settings_candidate_reduced_motion ||
           explorer->settings_saved_running_view != explorer->settings_candidate_running_view ||
           explorer->settings_saved_24_hour != explorer->settings_candidate_24_hour;
}

static void settings_copy_candidate_to_saved(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memcpy(explorer->settings_saved_numeric, explorer->settings_candidate_numeric,
           sizeof(explorer->settings_saved_numeric));
    explorer->settings_saved_reduced_motion = explorer->settings_candidate_reduced_motion;
    explorer->settings_saved_running_view = explorer->settings_candidate_running_view;
    explorer->settings_saved_24_hour = explorer->settings_candidate_24_hour;
}

static void settings_restore_saved(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memcpy(explorer->settings_candidate_numeric, explorer->settings_saved_numeric,
           sizeof(explorer->settings_candidate_numeric));
    explorer->settings_candidate_reduced_motion = explorer->settings_saved_reduced_motion;
    explorer->settings_candidate_running_view = explorer->settings_saved_running_view;
    explorer->settings_candidate_24_hour = explorer->settings_saved_24_hour;
}

static void format_settings_rate(char *buffer, size_t capacity, int32_t rate_tenths)
{
    (void)snprintf(buffer, capacity, "%ld.%ld C/min", (long)(rate_tenths / 10),
                   (long)(rate_tenths % 10));
}

static void format_settings_duration(char *buffer, size_t capacity, int32_t minutes)
{
    (void)snprintf(buffer, capacity, "%ld h %02ld min", (long)(minutes / 60),
                   (long)(minutes % 60));
}

static void settings_append_value_arrow(char *buffer, size_t capacity)
{
    const size_t length = strlen(buffer);
    (void)snprintf(buffer + length, capacity > length ? capacity - length : 0U,
                   "%s", ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
}

static const char *settings_running_view_text(uint8_t view)
{
    static const furnace_hmi_ui_string_id_t ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_GRAPH,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SPLIT,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_FURNACE,
    };
    return ui_string(ids[view % 3U]);
}

static const char *settings_tab_subtitle(uint8_t tab)
{
    static const furnace_hmi_ui_string_id_t ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUBTITLE_DISPLAY,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUBTITLE_TIME,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUBTITLE_CONNECTIONS,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUBTITLE_MAINTENANCE,
    };
    return ui_string(ids[tab % 4U]);
}

static void settings_number_descriptor(
    uint8_t key,
    furnace_hmi_ui_string_id_t *title,
    furnace_hmi_ui_string_id_t *help,
    int32_t *minimum,
    int32_t *maximum,
    int32_t *step
)
{
    static const furnace_hmi_ui_string_id_t titles[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_EDIT_RATE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_EDIT_TEMPERATURE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_EDIT_DURATION,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_EDIT_BRIGHTNESS,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_EDIT_DIM,
    };
    static const furnace_hmi_ui_string_id_t helps[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HELP_RATE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HELP_TEMPERATURE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HELP_DURATION,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HELP_BRIGHTNESS,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HELP_DIM,
    };
    static const int32_t minimums[] = { 1, 1, 1, 10, 1 };
    static const int32_t maximums[] = { UI_LAB_MAX_PROGRAM_RATE_TENTHS, UI_LAB_MAX_PROGRAM_TEMPERATURE_C, 59999, 100, 60 };
    static const int32_t steps[] = { 1, 1, 1, 1, 1 };
    const uint8_t index = key < UI_LAB_SETTINGS_NUMBER_COUNT ? key : 0U;
    *title = titles[index];
    *help = helps[index];
    *minimum = minimums[index];
    *maximum = maximums[index];
    *step = steps[index];
}

static lv_obj_t *create_settings_value_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    ui_lab_action_t action,
    int value
)
{
    lv_obj_t *button = create_page_button(explorer, parent, text, UI_LAB_SURFACE,
                                           action, value);
    if (button == NULL) {
        return NULL;
    }
    lv_obj_set_width(button, 170);
    lv_obj_set_flex_grow(button, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_width(label, lv_pct(100));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    return button;
}

static bool settings_add_row(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *label_text,
    const char *detail_text,
    const char *value_text,
    bool interactive,
    ui_lab_action_t action,
    int value
)
{
    lv_obj_t *row = lv_obj_create(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, explorer->settings_tab == UI_LAB_SETTINGS_TAB_MAINTENANCE ? 48 : 52);
    lv_obj_set_style_bg_color(row, UI_LAB_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, UI_LAB_RADIUS, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(row, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    remove_scroll(row);

    lv_obj_t *left = create_transparent_container(row);
    if (left == NULL) {
        return false;
    }
    lv_obj_set_width(left, 0);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, detail_text == NULL ? 40 : 48);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    if (create_label(left, label_text, &lv_font_montserrat_16, UI_LAB_TEXT) == NULL) {
        return false;
    }
    if (detail_text != NULL) {
        lv_obj_t *detail = create_label(left, detail_text, &lv_font_montserrat_14, UI_LAB_MUTED);
        if (detail == NULL) {
            return false;
        }
        lv_label_set_long_mode(detail, LV_LABEL_LONG_DOT);
        lv_obj_set_width(detail, lv_pct(100));
    }
    if (interactive) {
        return create_settings_value_button(explorer, row, value_text, action, value) != NULL;
    }
    lv_obj_t *value_label = create_label(row, value_text, &lv_font_montserrat_14, UI_LAB_TEXT);
    if (value_label == NULL) {
        return false;
    }
    lv_obj_set_width(value_label, 170);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    return true;
}

static lv_obj_t *create_settings_category_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    uint8_t tab
)
{
    lv_obj_t *button = create_page_button(explorer, parent, ui_string(settings_tab_ids[tab]),
                                           UI_LAB_SURFACE, UI_LAB_ACTION_SETTINGS_TAB, tab);
    if (button == NULL) {
        return NULL;
    }
    const bool active = explorer->settings_tab == tab;
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_grow(button, 0);
    lv_obj_set_style_bg_color(button, active ? UI_LAB_DEEP_BLUE : UI_LAB_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(button, active ? 1 : 0, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, UI_LAB_NAV_GLOW, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_width(label, lv_pct(100));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_text_color(label, UI_LAB_TEXT, LV_PART_MAIN);
    }
    return button;
}

static bool settings_modal_shell(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title_text,
    const char *detail_text,
    lv_obj_t **dialog_out,
    lv_obj_t **actions_out
)
{
    clear_modal(explorer);
    lv_obj_t *overlay = lv_obj_create(explorer->root);
    if (overlay == NULL) {
        return false;
    }
    explorer->modal = overlay;
    lv_obj_set_width(overlay, lv_pct(100));
    const int32_t root_height = lv_obj_get_height(explorer->root);
    lv_obj_set_height(overlay, root_height);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *dialog = create_panel(overlay, UI_LAB_SURFACE);
    if (dialog == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_width(dialog, 430);
    lv_obj_set_height(dialog, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dialog, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_row(dialog, UI_LAB_SPACE, LV_PART_MAIN);
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
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(actions, UI_LAB_SPACE, LV_PART_MAIN);
    *dialog_out = dialog;
    *actions_out = actions;
    return true;
}

static lv_obj_t *create_settings_modal_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    lv_color_t color,
    ui_lab_action_t action,
    int value
)
{
    lv_obj_t *button = create_modal_button(explorer, parent, text, color, action, value);
    if (button == NULL) {
        return NULL;
    }
    lv_obj_set_width(button, 0);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_height(button, 48);
    return button;
}

static bool settings_show_message(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title_text,
    const char *detail_text
)
{
    return open_dialog(explorer, title_text, detail_text, UI_LAB_ACTION_CLOSE_MODAL,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), UI_LAB_DEEP_BLUE);
}

static bool device_show_task(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t component_index,
    uint8_t task_index
)
{
    if (explorer == NULL || component_index >= explorer->device_catalogue.component_count) {
        return false;
    }
    const furnace_hmi_ui_lab_device_component_t *part =
        &explorer->device_catalogue.components[component_index];
    if (task_index >= part->task_count) {
        return false;
    }
    const furnace_hmi_ui_lab_device_task_t *task = &part->tasks[task_index];
    char title[256];
    char detail[1024];
    char formatted[900];
    (void)snprintf(
        title, sizeof(title),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TASK_TITLE_FORMAT),
        part->name, task->name);
    (void)snprintf(
        formatted, sizeof(formatted),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TASK_DETAIL_FORMAT),
        device_task_state_text(task->task_state), task->basis, task->interval,
        task->remaining, task->last, task->procedure);
    (void)snprintf(
        detail, sizeof(detail), "%s%s",
        explorer->state != NULL &&
                explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_KNOWN_PREFIX) : "",
        formatted);
    return settings_show_message(explorer, title, detail);
}

static bool device_show_part_info(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t component_index
)
{
    if (explorer == NULL || component_index >= explorer->device_catalogue.component_count) {
        return false;
    }
    const furnace_hmi_ui_lab_device_component_t *part =
        &explorer->device_catalogue.components[component_index];
    char title[256];
    char detail[1024];
    (void)snprintf(
        title, sizeof(title),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PART_INFO_TITLE_FORMAT),
        part->name);
    (void)snprintf(
        detail, sizeof(detail),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PART_INFO_FORMAT),
        part->part, part->location, part->instance, part->usage, part->source,
        explorer->device_catalogue.revision);
    return settings_show_message(explorer, title, detail);
}

static bool device_show_history(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t index
)
{
    if (index >= sizeof(device_history_text_ids) / sizeof(device_history_text_ids[0])) {
        return false;
    }
    return settings_show_message(
        explorer, ui_string(device_history_title_ids[index]),
        ui_string(device_history_text_ids[index]));
}

static bool device_show_info(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t index
)
{
    if (index >= sizeof(device_info_detail_ids) / sizeof(device_info_detail_ids[0])) {
        return false;
    }
    return settings_show_message(
        explorer, ui_string(device_info_label_ids[index]),
        ui_string(device_info_detail_ids[index]));
}

static bool settings_open_numeric_editor(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t key
)
{
    furnace_hmi_ui_string_id_t title_id;
    furnace_hmi_ui_string_id_t help_id;
    int32_t minimum;
    int32_t maximum;
    int32_t step;
    settings_number_descriptor(key, &title_id, &help_id, &minimum, &maximum, &step);
    explorer->settings_editor_key = key;
    return keyboard_open_number(
        explorer, ui_string(title_id), explorer->settings_candidate_numeric[key],
        UI_LAB_KEYBOARD_TARGET_SETTINGS_NUMBER, 0U, minimum, maximum, step,
        key == UI_LAB_SETTINGS_NUMBER_RATE);
}

static bool settings_open_unsaved_dialog(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *dialog;
    lv_obj_t *actions;
    if (!settings_modal_shell(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNSAVED_TITLE),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNSAVED_DETAIL),
            &dialog, &actions)) {
        return false;
    }
    (void)dialog;
    lv_obj_t *stay = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STAY),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_UNSAVED_STAY, 0);
    lv_obj_t *discard = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB,
        explorer->settings_pending_tab);
    lv_obj_t *save = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE),
        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_SETTINGS_SAVE_TO_TAB,
        explorer->settings_pending_tab);
    return stay != NULL && discard != NULL && save != NULL;
}

static bool settings_open_reset_dialog(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *dialog;
    lv_obj_t *actions;
    if (!settings_modal_shell(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_TITLE),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DETAIL),
            &dialog, &actions)) {
        return false;
    }
    (void)dialog;
    lv_obj_t *cancel = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_CLOSE_MODAL, 0);
    lv_obj_t *display = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_PREFERENCES),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 0);
    lv_obj_t *program = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAM_PREFERENCES_SCOPE),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 1);
    lv_obj_t *both = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BOTH_PREFERENCES),
        UI_LAB_DEEP_BLUE, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 2);
    return cancel != NULL && display != NULL && program != NULL && both != NULL;
}

static bool settings_open_delete_dialog(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *dialog;
    lv_obj_t *actions;
    if (!settings_modal_shell(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_TITLE),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_DETAIL),
            &dialog, &actions)) {
        return false;
    }
    (void)dialog;
    lv_obj_t *cancel = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_CLOSE_MODAL, 0);
    lv_obj_t *selected = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_SELECTED),
        UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_DELETE_SCOPE, 0);
    lv_obj_t *all = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ALL),
        UI_LAB_FAULT, UI_LAB_ACTION_SETTINGS_DELETE_SCOPE, 1);
    return cancel != NULL && selected != NULL && all != NULL;
}

static bool settings_open_restart_assessment(furnace_hmi_ui_lab_explorer_t *explorer)
{
    static const furnace_hmi_ui_string_id_t titles[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_OK_TITLE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_BUSY_TITLE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_TIMEOUT_TITLE,
    };
    static const furnace_hmi_ui_string_id_t details[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_OK_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_BUSY_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_TIMEOUT_DETAIL,
    };
    const uint8_t assessment = explorer->settings_restart_assessment < 3U
        ? explorer->settings_restart_assessment : 0U;
    const furnace_hmi_ui_string_id_t action = assessment == 0U
        ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REQUEST_RESTART
        : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ATTEMPT_RESTART;
    return open_dialog(explorer, ui_string(titles[assessment]), ui_string(details[assessment]),
                       UI_LAB_ACTION_SETTINGS_RESTART_CONFIRM, ui_string(action), UI_LAB_FAULT);
}

static bool settings_show_restart_outcome(furnace_hmi_ui_lab_explorer_t *explorer)
{
    static const furnace_hmi_ui_string_id_t titles[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_ACCEPTED,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_REFUSED,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_UNKNOWN,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_UNSENT,
    };
    static const furnace_hmi_ui_string_id_t details[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_ACCEPTED_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_REFUSED_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_UNKNOWN_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_UNSENT_DETAIL,
    };
    const uint8_t result = explorer->settings_restart_result < 4U
        ? explorer->settings_restart_result : 2U;
    return settings_show_message(explorer, ui_string(titles[result]), ui_string(details[result]));
}

static bool settings_reset_scope(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t scope)
{
    char title[64];
    char detail[320];
    const char *scope_text = scope == 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_PREFERENCES)
        : scope == 1U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAM_PREFERENCES_SCOPE)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BOTH_PREFERENCES);
    (void)snprintf(title, sizeof(title), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DIALOG_TITLE), scope_text);
    const furnace_hmi_ui_string_id_t defaults_id = scope == 0U
        ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_DISPLAY
        : scope == 1U
            ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_PROGRAM
            : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_BOTH;
    (void)snprintf(detail, sizeof(detail), "%s %s", ui_string(defaults_id),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_CONFIRM));
    explorer->settings_pending_scope = scope;
    return open_dialog(explorer, title, detail, UI_LAB_ACTION_SETTINGS_RESET_CONFIRM,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_PREFERENCES), UI_LAB_FAULT);
}

static bool settings_delete_scope(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t scope)
{
    const char *name = scope == 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_EXAMPLE_SELECTED)
        : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_EXAMPLE_ALL);
    char title[96];
    char detail[420];
    const char *scope_name = scope == 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_SCOPE_SELECTED)
        : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_SCOPE_ALL);
    (void)snprintf(title, sizeof(title), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_DIALOG_TITLE), scope_name);
    (void)snprintf(detail, sizeof(detail), "%s %s", name,
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_CONFIRM));
    explorer->settings_pending_scope = scope;
    return open_dialog(explorer, title, detail, UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REQUEST_DELETION), UI_LAB_FAULT);
}

static bool settings_build_footer(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    bool display_paging,
    bool save_buttons
)
{
    lv_obj_t *spacer = create_transparent_container(parent);
    lv_obj_t *actions = create_transparent_container(parent);
    if (spacer == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_height(spacer, 0);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, save_buttons ? 56 : 48);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);

    const furnace_hmi_ui_string_id_t status_id = explorer->settings_tab == UI_LAB_SETTINGS_TAB_MAINTENANCE
        ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_MAINTENANCE
        : explorer->settings_tab == UI_LAB_SETTINGS_TAB_CONNECTIONS
            ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_CONNECTIONS
            : settings_dirty(explorer)
                ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_UNSAVED
                : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_DISPLAY;
    lv_obj_t *status = create_label(actions, ui_string(status_id), &lv_font_montserrat_14, UI_LAB_MUTED);
    if (status == NULL) {
        return false;
    }
    lv_obj_set_width(status, 0);
    lv_obj_set_flex_grow(status, 1);
    lv_label_set_long_mode(status, LV_LABEL_LONG_DOT);
    if (display_paging) {
        const char *page_text = explorer->settings_display_page == 0U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MORE)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BACK);
        lv_obj_t *more = create_page_button(explorer, actions, page_text, UI_LAB_SURFACE_RAISED,
                                            UI_LAB_ACTION_SETTINGS_DISPLAY_MORE, 0);
        if (more == NULL) {
            return false;
        }
        lv_obj_set_width(more, 92);
    }
    if (save_buttons) {
        lv_obj_t *discard = create_page_button(explorer, actions,
                                                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD),
                                                UI_LAB_SURFACE_RAISED, UI_LAB_ACTION_SETTINGS_DISCARD, 0);
        lv_obj_t *save = create_page_button(explorer, actions,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE),
                                            UI_LAB_DEEP_BLUE, UI_LAB_ACTION_SETTINGS_SAVE, 0);
        if (discard == NULL || save == NULL) {
            return false;
        }
        lv_obj_set_width(discard, 82);
        lv_obj_set_width(save, 72);
        if (!settings_dirty(explorer)) {
            lv_obj_add_state(discard, LV_STATE_DISABLED);
            lv_obj_add_state(save, LV_STATE_DISABLED);
        }
    }
    return true;
}

static bool build_settings(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_set_flex_flow(explorer->content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(explorer->content, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->content, 0, LV_PART_MAIN);

    lv_obj_t *rail = create_transparent_container(explorer->content);
    lv_obj_t *body = create_transparent_container(explorer->content);
    if (rail == NULL || body == NULL) {
        return false;
    }
    lv_obj_set_width(rail, 184);
    lv_obj_set_height(rail, lv_pct(100));
    lv_obj_set_flex_flow(rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rail, 8, LV_PART_MAIN);
    lv_obj_set_width(body, 0);
    lv_obj_set_height(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, LV_PART_MAIN);

    for (uint8_t tab = 0U; tab < 4U; ++tab) {
        if (create_settings_category_button(explorer, rail, tab) == NULL) {
            return false;
        }
    }
    if (create_label(body, ui_string(settings_tab_ids[explorer->settings_tab]),
                     &lv_font_montserrat_20, UI_LAB_TEXT) == NULL ||
        create_label(body, settings_tab_subtitle(explorer->settings_tab),
                     &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) {
        return false;
    }
    lv_obj_t *rows = create_transparent_container(body);
    if (rows == NULL) {
        return false;
    }
    lv_obj_set_width(rows, lv_pct(100));
    lv_obj_set_height(rows, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(rows, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rows, 6, LV_PART_MAIN);

    char value[80];
    switch (explorer->settings_tab) {
    case UI_LAB_SETTINGS_TAB_DISPLAY:
        if (explorer->settings_display_page == 0U) {
            (void)snprintf(value, sizeof(value), "%ld%%%s",
                           (long)explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS],
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
            if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BRIGHTNESS),
                                  NULL, value, true, UI_LAB_ACTION_SETTINGS_EDIT_VALUE, UI_LAB_SETTINGS_NUMBER_BRIGHTNESS)) {
                return false;
            }
            (void)snprintf(value, sizeof(value), "%ld min%s",
                           (long)explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM],
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
            if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_WHEN_IDLE),
                                  NULL, value, true, UI_LAB_ACTION_SETTINGS_EDIT_VALUE, UI_LAB_SETTINGS_NUMBER_DIM)) {
                return false;
            }
            if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REDUCED_MOTION),
                                  NULL, explorer->settings_candidate_reduced_motion
                                      ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ON)
                                      : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_OFF),
                                  true, UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION, 0)) {
                return false;
            }
        }
        else {
            (void)snprintf(value, sizeof(value), "%s%s",
                           settings_running_view_text(explorer->settings_candidate_running_view),
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
            if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DEFAULT_RUNNING_VIEW),
                                  NULL, value, true, UI_LAB_ACTION_SETTINGS_CYCLE_VIEW, 0) ||
                !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_LANGUAGE_UNITS),
                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUPPORTED_OPTIONS),
                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ENGLISH_CELSIUS), false,
                                  UI_LAB_ACTION_CLOSE_MODAL, 0) ||
                !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_THEME),
                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CURRENT_THEME),
                                  ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_GRAPHITE), false,
                                  UI_LAB_ACTION_CLOSE_MODAL, 0)) {
                return false;
            }
        }
        return settings_build_footer(explorer, body, true, true);
    case UI_LAB_SETTINGS_TAB_TIME:
        (void)snprintf(value, sizeof(value), "14 Sep 2026%s",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
        if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONTROLLER_DATE),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ILLUSTRATIVE_VALUE),
                              value, true, UI_LAB_ACTION_SETTINGS_SHOW_DATE_TIME, 0)) {
            return false;
        }
        (void)snprintf(value, sizeof(value), "%s%s",
                       explorer->settings_candidate_24_hour ? "14:32" : "2:32 PM",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VALUE_ARROW));
        if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONTROLLER_TIME),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ILLUSTRATIVE_VALUE),
                              value,
                              true, UI_LAB_ACTION_SETTINGS_SHOW_DATE_TIME, 1) ||
            !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_FORMAT),
                              NULL, explorer->settings_candidate_24_hour
                                  ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_24_HOUR)
                                  : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_12_HOUR),
                              true, UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT, 0)) {
            return false;
        }
        return settings_build_footer(explorer, body, false, true);
    case UI_LAB_SETTINGS_TAB_CONNECTIONS: {
        static const furnace_hmi_ui_string_id_t names[] = {
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SERVER,
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BLUETOOTH,
        };
        static const furnace_hmi_ui_string_id_t statuses[] = {
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISABLED_CONFIGURATION,
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NOT_SUPPORTED,
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CHECKING_AVAILABILITY,
            FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NOT_CONNECTED,
        };
        const uint8_t fixture = explorer->settings_connection_fixture < 4U
            ? explorer->settings_connection_fixture : 0U;
        for (uint8_t index = 0U; index < 2U; ++index) {
            const bool enabled = fixture == 3U;
            const char *status = enabled ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP)
                                         : ui_string(statuses[fixture]);
            if (!settings_add_row(explorer, rows, ui_string(names[index]),
                                  enabled ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NOT_CONNECTED) : NULL,
                                  status, enabled, UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP, index)) {
                return false;
            }
        }
        if (create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SEPARATE_CONNECTION),
                         &lv_font_montserrat_14, UI_LAB_MUTED) == NULL) {
            return false;
        }
        return settings_build_footer(explorer, body, false, false);
    }
    case UI_LAB_SETTINGS_TAB_MAINTENANCE:
        if (!settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SETTINGS),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_ROW_DETAIL), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REVIEW),
                              true, UI_LAB_ACTION_SETTINGS_SHOW_RESET, 0) ||
            !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ROW_DETAIL), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REVIEW),
                              true, UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS, 0) ||
            !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_ROW_DETAIL), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REVIEW),
                              true, UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY, 0) ||
            !settings_add_row(explorer, rows, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ATTEMPT_FURNACE_RESTART),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ATTEMPT_RESTART_ROW_DETAIL), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CHECK),
                              true, UI_LAB_ACTION_SETTINGS_ASSESS_RESTART, 0)) {
            return false;
        }
        return settings_build_footer(explorer, body, false, false);
    default:
        return false;
    }
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
    explorer->cooling_overlay = NULL;
    explorer->trajectory_minimum_label = NULL;
    explorer->trajectory_maximum_label = NULL;
    explorer->trajectory_time_axis_label = NULL;
    explorer->trajectory_end_time_label = NULL;
    explorer->trajectory_middle_temperature_label = NULL;
    explorer->trajectory_middle_time_label = NULL;
    explorer->manual_prediction_label = NULL;
    explorer->manual_target_value = NULL;
    explorer->manual_rate_value = NULL;
    explorer->manual_status_label = NULL;
    explorer->manual_context_label = NULL;
    explorer->manual_start_button = NULL;
    explorer->manual_pause_button = NULL;
    explorer->manual_stop_button = NULL;
    memset(explorer->grid_lines, 0, sizeof(explorer->grid_lines));
    memset(explorer->trajectory_stage_nodes, 0, sizeof(explorer->trajectory_stage_nodes));
    memset(explorer->measured_segments, 0, sizeof(explorer->measured_segments));
    explorer->trajectory_uses_program_preview = false;
    memset(explorer->device_values, 0, sizeof(explorer->device_values));
    explorer->cooling_point_count = 0U;
    memset(explorer->running_reading_names, 0, sizeof(explorer->running_reading_names));
    memset(explorer->running_reading_values, 0, sizeof(explorer->running_reading_values));
    explorer->live_door = NULL;
    explorer->live_fan = NULL;
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
    lv_obj_set_style_pad_row(explorer->content, 6, LV_PART_MAIN);
    if (explorer->nav != NULL) {
        update_nav_selection(explorer, explorer->nav_selection_animation_pending);
        explorer->nav_selection_animation_pending = false;
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
            (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL && explorer->manual_running) ||
            explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
            explorer->page == FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR) {
            lv_obj_add_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(explorer->nav, -1);
        }
    }

    lv_obj_update_layout(explorer->root);
   switch (explorer->page) {
   case FURNACE_HMI_UI_LAB_PAGE_HOME:
       return build_home(explorer);
    case FURNACE_HMI_UI_LAB_PAGE_MANUAL:
        return build_manual(explorer);
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
     case FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR:
         return build_program_editor(explorer);
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
    if (explorer == NULL || explorer->state_strip_line == NULL) {
        return false;
    }
    lv_obj_set_style_bg_color(explorer->state_strip_line,
                              explorer->manual_light_on ? UI_LAB_MEASURED : UI_LAB_PRIMARY,
                              LV_PART_MAIN);
    lv_obj_set_style_shadow_color(explorer->state_strip_line, UI_LAB_MEASURED, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(explorer->state_strip_line,
                                  explorer->manual_light_on ? 16 : 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(explorer->state_strip_line,
                                explorer->manual_light_on ? LV_OPA_80 : LV_OPA_TRANSP,
                                LV_PART_MAIN);
    return true;
}

static void begin_program_editor(
    furnace_hmi_ui_lab_explorer_t *explorer,
    bool new_program
)
{
    clear_staged_session(explorer);
    explorer->program_editor_active = true;
    explorer->program_editor_new = new_program;
    explorer->program_editor_return_page = new_program
        ? FURNACE_HMI_UI_LAB_PAGE_PROGRAMS
        : FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL;
    explorer->staged_session_active = true;
    explorer->staged_program = explorer->selected_program;
    explorer->staged_stage_count = new_program
        ? 0U
        : (uint8_t)program_authored_stage_count(
            &program_fixtures[explorer->selected_program % program_fixture_count()]);
    copy_text_bounded(
        explorer->program_editor_name,
        sizeof(explorer->program_editor_name),
        new_program ? "New program" : program_name_text(explorer, explorer->selected_program));
    explorer->program_stage_page = 0U;
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
    case UI_LAB_ACTION_MANUAL:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_MANUAL);
        break;
    case UI_LAB_ACTION_PROGRAM_EDITOR:
        begin_program_editor(explorer, false);
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR);
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
        record_recent_program(explorer, explorer->selected_program);
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW),
            "", UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
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
    case UI_LAB_ACTION_STAGE_EDITOR:
        if (value >= 0 && value < (int)editor_stage_count(explorer)) {
            if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
                store_selected_stage_draft(explorer);
            } else if (!explorer->staged_session_active) {
                clear_staged_session(explorer);
                explorer->staged_session_active = true;
            }
            load_stage_draft(explorer, (uint8_t)value);
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR);
        break;
    case UI_LAB_ACTION_MANUAL_START:
        explorer->manual_running = true;
        explorer->manual_paused = false;
        (void)update_manual_run_state(explorer);
        break;
    case UI_LAB_ACTION_MANUAL_STOP:
        explorer->manual_running = false;
        explorer->manual_paused = false;
        (void)update_manual_run_state(explorer);
        break;
    case UI_LAB_ACTION_MANUAL_PAUSE:
        if (explorer->manual_running) {
            explorer->manual_paused = !explorer->manual_paused;
            (void)update_manual_run_state(explorer);
        }
        break;
    case UI_LAB_ACTION_MANUAL_EDIT_TARGET:
        (void)keyboard_open_number(
            explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_TARGET),
            explorer->manual_target_c, UI_LAB_KEYBOARD_TARGET_MANUAL_TARGET, 0U,
            0, UI_LAB_MAX_PROGRAM_TEMPERATURE_C, 1, false);
        break;
    case UI_LAB_ACTION_MANUAL_EDIT_RATE:
        (void)keyboard_open_number(
            explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_RATE),
            explorer->manual_rate_tenths_c_per_minute,
            UI_LAB_KEYBOARD_TARGET_MANUAL_RATE, 0U,
            1, UI_LAB_MAX_PROGRAM_RATE_TENTHS, 1, true);
        break;
    case UI_LAB_ACTION_MANUAL_DECREMENT_TARGET:
        if (explorer->manual_target_c > 0) {
            --explorer->manual_target_c;
            (void)update_manual_prediction(explorer);
        }
        break;
    case UI_LAB_ACTION_MANUAL_INCREMENT_TARGET:
        if (explorer->manual_target_c < UI_LAB_MAX_PROGRAM_TEMPERATURE_C) {
            ++explorer->manual_target_c;
            (void)update_manual_prediction(explorer);
        }
        break;
    case UI_LAB_ACTION_MANUAL_DECREMENT_RATE:
        if (explorer->manual_rate_tenths_c_per_minute > 1) {
            --explorer->manual_rate_tenths_c_per_minute;
            (void)update_manual_prediction(explorer);
        }
        break;
    case UI_LAB_ACTION_MANUAL_INCREMENT_RATE:
        if (explorer->manual_rate_tenths_c_per_minute < UI_LAB_MAX_PROGRAM_RATE_TENTHS) {
            ++explorer->manual_rate_tenths_c_per_minute;
            (void)update_manual_prediction(explorer);
        }
        break;
    case UI_LAB_ACTION_TOGGLE_LIGHT:
        explorer->manual_light_on = !explorer->manual_light_on;
        (void)update_header(explorer);
        break;
    case UI_LAB_ACTION_DEVICE:
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_SETTINGS &&
            settings_dirty(explorer)) {
            explorer->settings_pending_tab = UINT8_MAX;
            (void)settings_open_unsaved_dialog(explorer);
        }
        else {
            (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        }
        break;
    case UI_LAB_ACTION_DEVICE_TAB:
        if (value >= 0 && value < UI_LAB_DEVICE_TAB_COUNT) {
            explorer->device_tab = (uint8_t)value;
            explorer->device_filter = UI_LAB_DEVICE_FILTER_ALL;
            explorer->device_page = 0U;
            explorer->device_component_selected = false;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_DEVICE_FILTER:
        if (value >= 0x100) {
            const uint8_t tab = (uint8_t)((unsigned)value >> 8);
            if (tab < UI_LAB_DEVICE_TAB_COUNT) {
                explorer->device_tab = tab;
                explorer->device_component_selected = false;
            }
            explorer->device_filter = (uint8_t)((unsigned)value & 0xFFU);
        }
        else if (value >= 0 && value <= UI_LAB_DEVICE_FILTER_ERRORS) {
            explorer->device_filter = (uint8_t)value;
        }
        explorer->device_page = 0U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DEVICE_PREVIOUS:
        if (explorer->device_page > 0U) {
            --explorer->device_page;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DEVICE_NEXT:
        if (explorer->device_page < UINT8_MAX) {
            ++explorer->device_page;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DEVICE_COMPONENT:
        if (value >= 0 && value < explorer->device_catalogue.component_count) {
            explorer->device_selected_component = (uint8_t)value;
            explorer->device_selected_task = 0U;
            explorer->device_component_selected = true;
            explorer->device_page = 0U;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_DEVICE_BACK:
        explorer->device_component_selected = false;
        explorer->device_page = 0U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DEVICE_PART_INFO:
        (void)device_show_part_info(explorer, explorer->device_selected_component);
        break;
    case UI_LAB_ACTION_DEVICE_TASK:
        if (value >= 0) {
            const uint8_t component = (uint8_t)((unsigned)value >> 8);
            const uint8_t task = (uint8_t)((unsigned)value & 0xFFU);
            explorer->device_selected_component = component;
            explorer->device_selected_task = task;
            (void)device_show_task(explorer, component, task);
        }
        break;
    case UI_LAB_ACTION_DEVICE_NOTICE:
        if (value == UI_LAB_DEVICE_FAULT_NOTICE) {
            (void)settings_show_message(
                explorer,
                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_TITLE),
                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_DETAIL));
        }
        else if (value >= 0) {
            const uint8_t component = (uint8_t)((unsigned)value >> 8);
            const uint8_t task = (uint8_t)((unsigned)value & 0xFFU);
            (void)device_show_task(explorer, component, task);
        }
        break;
    case UI_LAB_ACTION_DEVICE_INFO:
        (void)device_show_info(explorer, (uint8_t)value);
        break;
    case UI_LAB_ACTION_DEVICE_HISTORY:
        (void)device_show_history(explorer, (uint8_t)value);
        break;
    case UI_LAB_ACTION_DEVICE_LOG_EVENT: {
        static const char *const details[] = {
            "The Control CPU accepted the program-start request.",
            "The Control CPU accepted the manual-start request.",
            "The operator pause request was recorded.",
            "The interior-light request was recorded.",
        };
        if (value >= 0 && value < (int)(sizeof(details) / sizeof(details[0]))) {
            (void)settings_show_message(explorer,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG),
                                        details[value]);
        }
        break;
    }
    case UI_LAB_ACTION_SETTINGS:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS);
        break;
    case UI_LAB_ACTION_SETTINGS_TAB:
        if (value < 0 || value >= 4 || (uint8_t)value == explorer->settings_tab) {
            break;
        }
        explorer->settings_pending_tab = (uint8_t)value;
        if (settings_dirty(explorer)) {
            (void)settings_open_unsaved_dialog(explorer);
        }
        else {
            explorer->settings_tab = (uint8_t)value;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_DISPLAY_MORE:
        explorer->settings_display_page = explorer->settings_display_page == 0U ? 1U : 0U;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_EDIT_VALUE:
        if (value >= 0 && value < UI_LAB_SETTINGS_NUMBER_COUNT) {
            (void)settings_open_numeric_editor(explorer, (uint8_t)value);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION:
        explorer->settings_candidate_reduced_motion = !explorer->settings_candidate_reduced_motion;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_CYCLE_VIEW:
        explorer->settings_candidate_running_view = (uint8_t)((explorer->settings_candidate_running_view + 1U) % 3U);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT:
        explorer->settings_candidate_24_hour = !explorer->settings_candidate_24_hour;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_DATE_TIME:
        (void)settings_show_message(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET_DATE_TIME),
            value == 0 ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET_DATE_TIME_DETAIL)
                       : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_TIME_PENDING_DETAIL));
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP:
        (void)settings_show_message(
            explorer,
            ui_string(value == 0 ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SERVER
                                 : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BLUETOOTH),
            value == 0 ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP_SERVER)
                       : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP_BLUETOOTH));
        break;
    case UI_LAB_ACTION_SETTINGS_SAVE:
        settings_copy_candidate_to_saved(explorer);
        (void)rebuild_page(explorer);
        (void)settings_show_message(explorer,
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVED_TITLE),
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVED_DETAIL));
        break;
    case UI_LAB_ACTION_SETTINGS_DISCARD:
        settings_restore_saved(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_UNSAVED_STAY:
        clear_modal(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB:
        settings_restore_saved(explorer);
        clear_modal(explorer);
        if ((uint8_t)value == UINT8_MAX) {
            (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        }
        else {
            explorer->settings_tab = (uint8_t)value;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_SAVE_TO_TAB:
        settings_copy_candidate_to_saved(explorer);
        clear_modal(explorer);
        if ((uint8_t)value == UINT8_MAX) {
            (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        }
        else {
            explorer->settings_tab = (uint8_t)value;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_EDITOR_CANCEL:
        clear_modal(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_EDITOR_APPLY:
        if (explorer->settings_editor_spinbox != NULL &&
            explorer->settings_editor_key < UI_LAB_SETTINGS_NUMBER_COUNT) {
            explorer->settings_candidate_numeric[explorer->settings_editor_key] =
                lv_spinbox_get_value(explorer->settings_editor_spinbox);
        }
        clear_modal(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_EDITOR_DECREMENT:
    case UI_LAB_ACTION_SETTINGS_EDITOR_INCREMENT:
        if (explorer->settings_editor_spinbox != NULL &&
            explorer->settings_editor_key < UI_LAB_SETTINGS_NUMBER_COUNT) {
            furnace_hmi_ui_string_id_t title_id;
            furnace_hmi_ui_string_id_t help_id;
            int32_t minimum;
            int32_t maximum;
            int32_t step;
            settings_number_descriptor(explorer->settings_editor_key, &title_id, &help_id,
                                       &minimum, &maximum, &step);
            const int32_t current = lv_spinbox_get_value(explorer->settings_editor_spinbox);
            const int64_t candidate = action == UI_LAB_ACTION_SETTINGS_EDITOR_DECREMENT
                ? (int64_t)current - step : (int64_t)current + step;
            const int32_t next = candidate < minimum ? minimum :
                candidate > maximum ? maximum : (int32_t)candidate;
            lv_spinbox_set_value(explorer->settings_editor_spinbox, next);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_RESET:
        (void)settings_open_reset_dialog(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_RESET_SCOPE:
        if (value >= 0 && value <= 2) {
            (void)settings_reset_scope(explorer, (uint8_t)value);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_RESET_CONFIRM: {
        const uint8_t scope = explorer->settings_pending_scope;
        if (scope == 0U || scope == 2U) {
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS] = 80;
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM] = 5;
            explorer->settings_candidate_reduced_motion = false;
            explorer->settings_candidate_running_view = 1U;
            explorer->settings_candidate_24_hour = true;
        }
        if (scope == 1U || scope == 2U) {
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_RATE] = UI_LAB_MAX_PROGRAM_RATE_TENTHS;
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_TEMPERATURE] = UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DURATION] = 720;
        }
        settings_copy_candidate_to_saved(explorer);
        clear_modal(explorer);
        (void)rebuild_page(explorer);
        (void)settings_show_message(explorer,
                                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_TITLE),
                                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_COMPLETE));
        break;
    }
    case UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS:
        (void)settings_open_delete_dialog(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_DELETE_SCOPE:
        if (value >= 0 && value <= 1) {
            (void)settings_delete_scope(explorer, (uint8_t)value);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM:
        clear_modal(explorer);
        (void)settings_show_message(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETION_REQUEST_PREVIEW),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_COMPLETE));
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY:
        (void)open_dialog(explorer,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_TITLE),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_DETAIL),
                          UI_LAB_ACTION_SETTINGS_RESTART_DISPLAY_CONFIRM,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_CONFIRM),
                          UI_LAB_FAULT);
        break;
    case UI_LAB_ACTION_SETTINGS_RESTART_DISPLAY_CONFIRM:
        clear_modal(explorer);
        (void)settings_show_message(
            explorer,
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_RESTART_PREVIEW),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_COMPLETE));
        break;
    case UI_LAB_ACTION_SETTINGS_ASSESS_RESTART:
        (void)settings_open_restart_assessment(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_RESTART_CONFIRM:
        clear_modal(explorer);
        (void)settings_show_restart_outcome(explorer);
        break;
    case UI_LAB_ACTION_BACK_PROGRAM_DETAIL:
        if (explorer->program_editor_active) {
            (void)furnace_hmi_ui_lab_explorer_navigate(
                explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR);
        }
        else {
            (void)furnace_hmi_ui_lab_explorer_navigate(
                explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL);
        }
        break;
    case UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE:
        if (explorer->program_page > 0U) {
            explorer->program_page--;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_NEXT_PROGRAM_PAGE:
        if ((size_t)explorer->program_page + 1U <
            (explorer->library_count + 5U) / 6U) {
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
            (program_authored_stage_count(
                &program_fixtures[explorer->selected_program]) + 8U) / 9U) {
            explorer->program_stage_page++;
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_PREVIOUS_STAGE:
        if (explorer->selected_stage > 0U) {
            store_selected_stage_draft(explorer);
            load_stage_draft(explorer, explorer->selected_stage - 1U);
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_NEXT_STAGE:
        if (explorer->selected_stage + 1U < editor_stage_count(explorer)) {
            store_selected_stage_draft(explorer);
            load_stage_draft(explorer, explorer->selected_stage + 1U);
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SHOW_NOTICES:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NOTIFICATIONS),
                          "",
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        break;
    case UI_LAB_ACTION_SHOW_SERVICE:
        (void)furnace_hmi_ui_lab_explorer_show_service_warning(explorer);
        break;
    case UI_LAB_ACTION_SHOW_DELETE:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_PROGRAM_DETAIL),
                          UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE), UI_LAB_FAULT);
        break;
    case UI_LAB_ACTION_SHOW_RENAME:
        (void)keyboard_open_text(
            explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_PROGRAM_NAME),
            explorer->program_editor_active ? explorer->program_editor_name :
                program_name_text(explorer, explorer->selected_program),
            UI_LAB_KEYBOARD_TARGET_PROGRAM_NAME, 0U);
        break;
    case UI_LAB_ACTION_SHOW_STAGE_RENAME:
        if (value >= 0 && value < FURNACE_HMI_UI_LAB_MAX_STAGES) {
            (void)keyboard_open_text(
                explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_STAGE_NAME),
                stage_name_text(explorer, (size_t)value),
                UI_LAB_KEYBOARD_TARGET_STAGE_NAME, (uint8_t)value);
        }
        break;
    case UI_LAB_ACTION_STAGE_EDIT_VALUE:
        if (value == UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET) {
            (void)keyboard_open_number(
                explorer, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_TARGET),
                explorer->draft_target_c, UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET,
                explorer->selected_stage, 0, UI_LAB_MAX_PROGRAM_TEMPERATURE_C, 1, false);
        }
        else if (value == UI_LAB_KEYBOARD_TARGET_DRAFT_RATE) {
            const int32_t rate = explorer->draft_rate_tenths_c_per_minute < 0
                ? -explorer->draft_rate_tenths_c_per_minute
                : explorer->draft_rate_tenths_c_per_minute;
            (void)keyboard_open_number(
                explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE), rate,
                UI_LAB_KEYBOARD_TARGET_DRAFT_RATE, explorer->selected_stage,
                1, UI_LAB_MAX_PROGRAM_RATE_TENTHS, 1, true);
        }
        else if (value == UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION) {
            (void)keyboard_open_number(
                explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                explorer->draft_duration_minutes, UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION,
                explorer->selected_stage, 1, 59999, 1, false);
        }
        break;
    case UI_LAB_ACTION_SHOW_SAVE:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT),
                          "",
                          UI_LAB_ACTION_CONFIRM_SAVE_ALL,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT), UI_LAB_DEEP_BLUE);
        break;
    case UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR:
        if (explorer->program_editor_dirty) {
            (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_UNSAVED_PROGRAM_DETAIL),
                              UI_LAB_ACTION_CONFIRM_DISCARD,
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_FAULT);
        }
        else if (explorer->program_editor_active) {
            const furnace_hmi_ui_lab_page_t return_page = explorer->program_editor_return_page;
            clear_staged_session(explorer);
            explorer->page = return_page;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SHOW_DISCARD:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_DETAIL),
                          UI_LAB_ACTION_CONFIRM_DISCARD,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_FAULT);
        break;
    case UI_LAB_ACTION_SHOW_RUN_DETAILS: {
        char detail[256];
        snprintf(detail, sizeof(detail), "%s\n%s", explorer->state->program_name, explorer->state->stage_name);
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_DETAILS),
                          detail,
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_DEEP_BLUE);
        break;
    }
    case UI_LAB_ACTION_ADD_STAGE:
        if (explorer->program_editor_active) {
            if (explorer->staged_stage_count < FURNACE_HMI_UI_LAB_MAX_STAGES) {
                explorer->program_editor_dirty = true;
                explorer->selected_stage = explorer->staged_stage_count;
                explorer->staged_stage_count++;
                explorer->draft_target_c = explorer->selected_stage == 0U
                    ? 120 : program_stage(explorer, explorer->selected_stage - 1U).target_c;
                explorer->draft_rate_tenths_c_per_minute = 10;
                explorer->draft_duration_minutes = 10;
                explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
                explorer->draft_cooling_rate_enabled = false;
                explorer->staged_valid[explorer->selected_stage] = false;
                (void)furnace_hmi_ui_lab_explorer_navigate(
                    explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR);
            }
            break;
        }
        /* Legacy stage-editor entry remains bounded for existing smoke paths. */
    case UI_LAB_ACTION_NEW_DRAFT:
        if (action == UI_LAB_ACTION_NEW_DRAFT) {
            begin_program_editor(explorer, true);
            (void)furnace_hmi_ui_lab_explorer_navigate(
                explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR);
            break;
        }
        explorer->selected_stage = 0U;
        explorer->draft_target_c = stage_fixtures[0].target_c;
        explorer->draft_rate_tenths_c_per_minute = stage_fixtures[0].delta_c_per_minute;
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
                          "",
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, state_color(explorer->state));
        break;
    case UI_LAB_ACTION_DECREMENT_TARGET:
        if (explorer->draft_target_c > 0) { explorer->draft_target_c -= 1; }
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_INCREMENT_TARGET:
        if (explorer->draft_target_c < UI_LAB_MAX_PROGRAM_TEMPERATURE_C) { explorer->draft_target_c += 1; }
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_DECREMENT_RATE:
        if (explorer->draft_rate_tenths_c_per_minute > -UI_LAB_MAX_PROGRAM_RATE_TENTHS && explorer->draft_rate_tenths_c_per_minute != 1) { explorer->draft_rate_tenths_c_per_minute -= 1; }
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_INCREMENT_RATE:
        if (explorer->draft_rate_tenths_c_per_minute < UI_LAB_MAX_PROGRAM_RATE_TENTHS && explorer->draft_rate_tenths_c_per_minute != -1) { explorer->draft_rate_tenths_c_per_minute += 1; }
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
        if (explorer->draft_duration_minutes < INT32_MAX) { explorer->draft_duration_minutes += 1; }
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
        explorer->draft_target_c = explorer->selected_stage == 0 ? 25 : program_stage(explorer, explorer->selected_stage - 1U).target_c;
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
    case UI_LAB_ACTION_CONFIRM_DISCARD:
        if (explorer->program_editor_active &&
            explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            clear_modal(explorer);
            load_stage_draft(explorer, explorer->selected_stage);
            explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR;
            (void)rebuild_page(explorer);
            break;
        }
        if (explorer->program_editor_active) {
            const furnace_hmi_ui_lab_page_t return_page =
                explorer->program_editor_return_page;
            clear_staged_session(explorer);
            clear_modal(explorer);
            explorer->page = return_page;
            (void)rebuild_page(explorer);
            break;
        }
        clear_staged_session(explorer);
        clear_modal(explorer);
        explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_CONFIRM_SAVE_ALL: {
        const ui_lab_stage_fixture_t before = program_stage(explorer, explorer->selected_stage);
        store_selected_stage_draft(explorer);
        const ui_lab_stage_fixture_t after = program_stage(explorer, explorer->selected_stage);
        if (explorer->program_editor_active &&
            (before.kind != after.kind || before.target_c != after.target_c ||
             before.delta_c_per_minute != after.delta_c_per_minute ||
             before.duration_minutes != after.duration_minutes)) {
            explorer->program_editor_dirty = true;
        }
        if (explorer->program_editor_active &&
            explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            clear_modal(explorer);
            explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR;
            (void)rebuild_page(explorer);
            break;
        }
        if (explorer->program_editor_active) {
            const furnace_hmi_ui_lab_page_t return_page =
                explorer->program_editor_return_page;
            clear_staged_session(explorer);
            clear_modal(explorer);
            explorer->page = return_page;
            (void)rebuild_page(explorer);
            break;
        }
        explorer->staged_program = UINT8_MAX;
        explorer->staged_session_active = false;
        clear_modal(explorer);
        explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES;
        (void)rebuild_page(explorer);
        break;
    }
    case UI_LAB_ACTION_CLOSE_MODAL:
        clear_modal(explorer);
        break;
    case UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONTROLLER_OWNED),
                          "",
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
    bool is_modal_action = false;
    if (explorer->modal != NULL) {
        for (size_t index = 0U; index < sizeof(explorer->modal_actions) / sizeof(explorer->modal_actions[0]); ++index) {
            if (slot == &explorer->modal_actions[index]) {
                is_modal_action = true;
                break;
            }
        }
    }
    if (explorer->modal != NULL && lv_event_get_target_obj(event) != NULL &&
        !is_modal_action && slot->action != UI_LAB_ACTION_STOP_PREVIEW) { return; }
    if (explorer->action_pending) {
        if (slot->action == UI_LAB_ACTION_STOP_PREVIEW) { explorer->pending_action = slot->action; explorer->pending_action_value = slot->value; }
        return;
    }
    explorer->pending_action = slot->action;
    explorer->pending_action_value = slot->value;
    if (!explorer->action_pending) {
        explorer->action_pending = true;
        lv_async_call(deferred_action, explorer);
    }
}

static void normalize_observation_validity(furnace_hmi_dashboard_state_t *state)
{
    if (state->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT) { return; }
    const furnace_hmi_value_validity_t validity = state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
        ? FURNACE_HMI_VALUE_VALIDITY_STALE : FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    furnace_hmi_i32_value_t *values[] = { &state->current_temperature_c, &state->target_temperature_c,
        &state->target_delta_c_per_minute, &state->elapsed_seconds, &state->remaining_seconds,
        &state->heater_demand_percent, &state->estimated_power_kw_x100, &state->fan_rpm,
        &state->service_time_hours, &state->service_due_at_hours, &state->service_forecast_completion_hours };
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) { values[i]->validity = validity; }
    state->door_closed.validity = validity; state->fan_running.validity = validity;
    state->service_run_gate = FURNACE_HMI_SERVICE_RUN_GATE_UNKNOWN;
    for (size_t i = 0; i < state->graph_sample_count; ++i) { state->graph_samples[i].measured_validity = validity; }
}

bool furnace_hmi_ui_lab_explorer_create(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state,
    const furnace_hmi_ui_lab_device_catalogue_t *device_catalogue
)
{
    if (explorer == NULL || parent == NULL || explorer->root != NULL || state == NULL ||
        device_catalogue == NULL ||
        !furnace_hmi_dashboard_state_is_well_formed(state)) {
        return false;
    }
    memset(explorer, 0, sizeof(*explorer));
    explorer->device_catalogue = *device_catalogue;
    explorer->observation = *state;
    normalize_observation_validity(&explorer->observation);
    explorer->state = &explorer->observation;
    explorer->page = (state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM ||
                      state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL) &&
        (state->run_state == FURNACE_HMI_RUN_STATE_RUNNING || state->run_state == FURNACE_HMI_RUN_STATE_PAUSED)
        ? FURNACE_HMI_UI_LAB_PAGE_RUNNING : FURNACE_HMI_UI_LAB_PAGE_HOME;
    explorer->library_count = FURNACE_HMI_UI_LAB_MAX_PROGRAMS;
    explorer->running_view_mode = 1U;
    explorer->settings_tab = UI_LAB_SETTINGS_TAB_DISPLAY;
    explorer->settings_pending_tab = UI_LAB_SETTINGS_TAB_DISPLAY;
    explorer->settings_display_page = 0U;
    explorer->settings_connection_fixture = 0U;
    explorer->settings_restart_assessment = 0U;
    explorer->settings_restart_result = 0U;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_RATE] = UI_LAB_MAX_PROGRAM_RATE_TENTHS;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_TEMPERATURE] = UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_DURATION] = 720;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS] = 80;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_DIM] = 5;
    memcpy(explorer->settings_candidate_numeric, explorer->settings_saved_numeric,
           sizeof(explorer->settings_candidate_numeric));
    explorer->settings_saved_running_view = 1U;
    explorer->settings_candidate_running_view = 1U;
    explorer->settings_saved_24_hour = true;
    explorer->settings_candidate_24_hour = true;
    explorer->device_tab = 0U;
    explorer->device_filter = 0U;
    explorer->device_page = 0U;
    explorer->device_selected_component = 0U;
    explorer->device_selected_task = 0U;
    explorer->device_component_selected = false;
   explorer->selected_program = 0U;
   explorer->selected_stage = 2U;
    explorer->manual_target_c = 120;
    explorer->manual_rate_tenths_c_per_minute = 20;
    explorer->manual_running = false;
    explorer->manual_paused = false;
    explorer->manual_light_on = false;
   explorer->draft_target_c = stage_fixtures[2].target_c;
    explorer->draft_rate_tenths_c_per_minute = stage_fixtures[2].delta_c_per_minute;
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
    lv_obj_add_flag(explorer->root, LV_OBJ_FLAG_FLOATING);
    /* The smoke path changes the display resolution before creating a route.
     * Use the current display dimensions when the screen object has not yet
     * resolved its percentage geometry after that change. */
    const int32_t parent_width = lv_obj_get_width(parent);
    const int32_t parent_height = lv_obj_get_height(parent);
    lv_obj_set_size(
        explorer->root,
        parent_width > 0 ? parent_width : lv_display_get_horizontal_resolution(NULL),
        parent_height > 0 ? parent_height : lv_display_get_vertical_resolution(NULL)
    );
    lv_obj_set_style_bg_color(explorer->root, UI_LAB_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->root, UI_LAB_PANEL_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->root, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_top(explorer->root, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(explorer->root, LV_FLEX_FLOW_COLUMN);
    remove_scroll(explorer->root);
    explorer->state_strip = lv_obj_create(explorer->root);
    if (explorer->state_strip == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_width(explorer->state_strip, parent_width);
    lv_obj_set_height(explorer->state_strip, FURNACE_HMI_UI_LAB_LIGHT_TARGET_HEIGHT);
    lv_obj_set_style_min_height(explorer->state_strip, FURNACE_HMI_UI_LAB_LIGHT_TARGET_HEIGHT,
                                LV_PART_MAIN);
    lv_obj_set_style_margin_left(explorer->state_strip, -UI_LAB_PANEL_PADDING, LV_PART_MAIN);
    lv_obj_set_style_margin_right(explorer->state_strip, -UI_LAB_PANEL_PADDING, LV_PART_MAIN);
    /* Cancel the root row gap: the strip itself is the top border. */
    lv_obj_set_style_margin_bottom(explorer->state_strip, -UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->state_strip, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->state_strip, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->state_strip, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->state_strip, 0, LV_PART_MAIN);
    remove_scroll(explorer->state_strip);
    lv_obj_add_flag(explorer->state_strip, LV_OBJ_FLAG_CLICKABLE);
    explorer->state_strip_line = lv_obj_create(explorer->state_strip);
    if (explorer->state_strip_line == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_size(explorer->state_strip_line, parent_width, 4);
    lv_obj_set_pos(explorer->state_strip_line, 0, 0);
    lv_obj_set_style_bg_color(explorer->state_strip_line, state_color(state), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(explorer->state_strip_line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(explorer->state_strip_line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->state_strip_line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->state_strip_line, 0, LV_PART_MAIN);
    remove_scroll(explorer->state_strip_line);
    explorer->header_actions[0].explorer = explorer;
    explorer->header_actions[0].action = UI_LAB_ACTION_TOGGLE_LIGHT;
    explorer->header_actions[0].value = 0;
    lv_obj_add_event_cb(explorer->state_strip, action_callback, LV_EVENT_CLICKED,
                        &explorer->header_actions[0]);

    explorer->nav = create_panel(explorer->root, UI_LAB_SURFACE);
    if (explorer->nav == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_width(explorer->nav, lv_pct(100));
    lv_obj_set_height(explorer->nav, 52);
    lv_obj_set_flex_flow(explorer->nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_width(explorer->nav, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->nav, UI_LAB_RADIUS, LV_PART_MAIN);
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
        lv_obj_set_height(button, 48);
    }
    update_nav_selection(explorer, false);
    if (!update_header(explorer) || !rebuild_page(explorer)) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_update_layout(explorer->root);
    return true;
}

static void update_value_label(lv_obj_t *label, const furnace_hmi_i32_value_t *value, furnace_hmi_ui_string_id_t format)
{
    if (label == NULL) { return; }
    char text[48]; format_observation_value(text, sizeof(text), value, format); lv_label_set_text(label, text);
}

bool furnace_hmi_ui_lab_explorer_update(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_dashboard_state_t *state)
{
    if (explorer == NULL || explorer->root == NULL || state == NULL || !furnace_hmi_dashboard_state_is_well_formed(state)) { return false; }
    const bool new_run = state->controller_session_epoch != explorer->observation.controller_session_epoch ||
        state->program_id != explorer->observation.program_id || state->program_revision != explorer->observation.program_revision ||
        state->elapsed_seconds.value < explorer->observation.elapsed_seconds.value;
    const bool structure_changed = state->availability != explorer->observation.availability ||
        state->mode != explorer->observation.mode || state->run_state != explorer->observation.run_state ||
        state->stage_index != explorer->observation.stage_index || new_run;
    explorer->observation = *state; normalize_observation_validity(&explorer->observation);
    explorer->state = &explorer->observation; state = explorer->state;
    if (new_run) { explorer->trajectory_domain_initialized = false; }
    if (!update_header(explorer)) { return false; }
    /* Manual is a locally composed host presentation. A new observation can
     * refresh its live labels and forecast but must not destroy/recreate its
     * trajectory while the operator starts, pauses, resumes or stops. */
    if (structure_changed && explorer->page != FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
        if (state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && (state->run_state == FURNACE_HMI_RUN_STATE_RUNNING || state->run_state == FURNACE_HMI_RUN_STATE_PAUSED)) {
            explorer->page = FURNACE_HMI_UI_LAB_PAGE_RUNNING;
        } else if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING && state->mode == FURNACE_HMI_MACHINE_MODE_IDLE) {
            explorer->page = FURNACE_HMI_UI_LAB_PAGE_HOME;
        }
        /* Modal lifetime is independent of telemetry/page subtree refreshes. */
        lv_obj_t *modal = explorer->modal; size_t modal_count = explorer->modal_action_count;
        explorer->modal = NULL;
        bool result = rebuild_page(explorer);
        explorer->modal = modal; explorer->modal_action_count = modal_count;
        if (modal != NULL) { lv_obj_move_to_index(modal, -1); }
        if (!result) { return false; }
    }
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
        return update_manual_run_state(explorer);
    }
    update_value_label(explorer->home_temperature, &state->current_temperature_c, FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    update_value_label(explorer->home_target, &state->target_temperature_c, FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C);
    update_value_label(explorer->live_fan, &state->fan_rpm, FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT);
    if (explorer->live_door) { lv_label_set_text(explorer->live_door, door_text(state)); }
    char text[80], value[40];
    if (explorer->device_values[0] != NULL) {
        const furnace_hmi_i32_value_t *values[] = { &state->current_temperature_c, &state->heater_demand_percent, &state->fan_rpm };
        const furnace_hmi_ui_string_id_t formats[] = { FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C, FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_PERCENT, FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT };
        const furnace_hmi_ui_string_id_t labels[] = { FURNACE_HMI_UI_STRING_UI_LAB_CURRENT_TEMPERATURE, FURNACE_HMI_UI_STRING_UI_LAB_HEATER_DEMAND, FURNACE_HMI_UI_STRING_UI_LAB_FAN_STATUS, FURNACE_HMI_UI_STRING_UI_LAB_DOOR_STATUS };
        for (size_t i = 0; i < 4; ++i) {
            if (i < 3) { format_observation_value(value, sizeof(value), values[i], formats[i]); }
            else { snprintf(value, sizeof(value), "%s", door_text(state)); }
            snprintf(text, sizeof(text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FIELD_TEXT_FORMAT), ui_string(labels[i]), value);
            lv_label_set_text(explorer->device_values[i], text);
        }
    }
    if (explorer->home_remaining) { format_remaining(text, sizeof(text), &state->remaining_seconds); lv_label_set_text(explorer->home_remaining, text); }
    update_running_readings(explorer);
    lv_obj_update_layout(explorer->root);
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
    if ((explorer->modal != NULL && page != explorer->page) ||
        (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR && page != explorer->page) ||
        (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL && explorer->manual_running && page != explorer->page) ||
        (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING &&
        page != FURNACE_HMI_UI_LAB_PAGE_RUNNING)) {
        return false;
    }
    if (explorer->page != page) {
        explorer->trajectory_domain_initialized = false;
        explorer->nav_selection_animation_pending =
            nav_destination_for_page(explorer->page) != nav_destination_for_page(page);
    }
    explorer->page = page;
    if (!rebuild_page(explorer)) { return false; }
    lv_obj_update_layout(explorer->root);
    return explorer->trajectory == NULL || update_trajectory(explorer);
}

bool furnace_hmi_ui_lab_explorer_show_service_warning(
    furnace_hmi_ui_lab_explorer_t *explorer
)
{
    if (explorer == NULL || explorer->root == NULL) {
        return false;
    }
    char detail[512];
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
                       ui_string(explorer->state->service_run_gate == FURNACE_HMI_SERVICE_RUN_GATE_ACKNOWLEDGEMENT_REQUIRED
                           ? FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGEMENT_REQUIRED : FURNACE_HMI_UI_STRING_DASHBOARD_UNKNOWN));
    }
    else {
        (void)snprintf(detail, sizeof(detail), "%s",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NO_CURRENT_SNAPSHOT));
    }
    return open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SERVICE_WARNING), detail,
                       state_is_current(explorer->state) && explorer->state->service_run_gate == FURNACE_HMI_SERVICE_RUN_GATE_ACKNOWLEDGEMENT_REQUIRED ? UI_LAB_ACTION_ACKNOWLEDGE_PREVIEW : UI_LAB_ACTION_CLOSE_MODAL,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGE_PREVIEW), UI_LAB_STALE);
}

void furnace_hmi_ui_lab_explorer_destroy(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL) {
        return;
    }
    lv_async_call_cancel(deferred_action, explorer);
    if (explorer->root != NULL && lv_obj_is_valid(explorer->root)) {
        lv_obj_delete(explorer->root);
    }
    memset(explorer, 0, sizeof(*explorer));
}
