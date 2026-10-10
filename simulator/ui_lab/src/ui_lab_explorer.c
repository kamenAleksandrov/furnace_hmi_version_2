/* SPDX-License-Identifier: Apache-2.0 */

#include <inttypes.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_lab_explorer.h"
#include "ui_lab_theme.h"

#include <furnace_hmi/hmi_fonts.h>
#include <furnace_hmi/hmi_icons.h>
#include <furnace_hmi/support_qr.h>

#include <furnace_hmi/ui_strings.h>

/* Colours are theme roles (ui_lab_theme.h, app/hmi/presentation/theme);
 * never write a colour value here. */
#define UI_LAB_BACKGROUND UI_LAB_THEME_PAGE
#define UI_LAB_DIVIDER ui_lab_color(FURNACE_HMI_COLOR_DIVIDER)
#define UI_LAB_TEXT UI_LAB_THEME_TEXT
#define UI_LAB_MUTED UI_LAB_THEME_MUTED
#define UI_LAB_PLANNED ui_lab_color(FURNACE_HMI_COLOR_GRAPH_PLAN)
#define UI_LAB_COOLING_ESTIMATE ui_lab_color(FURNACE_HMI_COLOR_GRAPH_COOLING_PLAN)
#define UI_LAB_COOLING_DASH 6.0
#define UI_LAB_COOLING_GAP 4.0
#define UI_LAB_RUNNING ui_lab_color(FURNACE_HMI_COLOR_STATE_RUNNING)
#define UI_LAB_PAUSED ui_lab_color(FURNACE_HMI_COLOR_STATE_PAUSED)
#define UI_LAB_MANUAL ui_lab_color(FURNACE_HMI_COLOR_STATE_MANUAL)
#define UI_LAB_IDLE ui_lab_color(FURNACE_HMI_COLOR_STATE_IDLE)
#define UI_LAB_FAULT UI_LAB_THEME_RED
#define UI_LAB_STALE ui_lab_color(FURNACE_HMI_COLOR_STALE_TEXT)
#define UI_LAB_UNAVAILABLE ui_lab_color(FURNACE_HMI_COLOR_STATE_UNAVAILABLE)
#define UI_LAB_BUTTON_MIN_HEIGHT 56
#define UI_LAB_RADIUS 0
#define UI_LAB_SPACE 8
#define UI_LAB_PANEL_PADDING 12
#define UI_LAB_PROGRAM_DELETE_TEXT ui_lab_color(FURNACE_HMI_COLOR_DELETE_TEXT)
/* Graph panel geometry (web programGraph): a caption row inside a 9 px inset,
 * then the plot with a 46 px degree gutter and a 20 px minute gutter. */
#define UI_LAB_GRAPH_INSET 9
#define UI_LAB_GRAPH_LEFT (UI_LAB_GRAPH_INSET + 46)
#define UI_LAB_GRAPH_RIGHT (UI_LAB_GRAPH_INSET + 6)
#define UI_LAB_GRAPH_TOP (UI_LAB_GRAPH_INSET + 36)
#define UI_LAB_GRAPH_BOTTOM (UI_LAB_GRAPH_INSET + 20)
#define UI_LAB_GRAPH_GRID ui_lab_color(FURNACE_HMI_COLOR_GRAPH_GRID)
#define UI_LAB_MAX_PROGRAM_TEMPERATURE_C 180
#define UI_LAB_MAX_PROGRAM_RATE_TENTHS 30
/* Touch keyboards: keys fill their row height but never drop below the touch minimum. */
#define UI_LAB_KEY_MIN_HEIGHT 48
/* Numeric keypad panel width: narrower, taller keys instead of wide slabs. */
#define UI_LAB_NUMERIC_KEYPAD_WIDTH 460
#define UI_LAB_KEYBOARD_TEXT_LIMIT 32U
#define UI_LAB_DIALOG_WIDTH 430
#define UI_LAB_DIALOG_PADDING 25
#define UI_LAB_DIALOG_GAP 14
#define UI_LAB_DIALOG_BUTTON_HEIGHT 48

/* Text keyboard rows come from the catalogue (one key per UTF-8 character) so
 * a language can bring its own layout; Shift uses the catalogue's upper-case
 * rows rather than an ASCII-only case conversion. */
static const furnace_hmi_ui_string_id_t keyboard_rows[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_NUMBERS,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_TOP,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_MIDDLE,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_BOTTOM,
};
static const furnace_hmi_ui_string_id_t keyboard_shift_rows[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_NUMBERS,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_TOP_SHIFT,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_MIDDLE_SHIFT,
    FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_ROW_BOTTOM_SHIFT,
};

typedef enum {
    UI_LAB_ACTION_HOME = 0,
    UI_LAB_ACTION_MANUAL,
    UI_LAB_ACTION_PROGRAM_EDITOR,
    UI_LAB_ACTION_PROGRAMS,
    UI_LAB_ACTION_PROGRAM_LOADING,
    UI_LAB_ACTION_CONFIRM_START,
    UI_LAB_ACTION_RUNNING,
    UI_LAB_ACTION_PROGRAM_DETAIL,
    UI_LAB_ACTION_PROGRAM_STAGES,
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
    UI_LAB_ACTION_DELETE_REQUESTED,
    UI_LAB_ACTION_CONFIRM_DISCARD,
    UI_LAB_ACTION_SHOW_RENAME,
    UI_LAB_ACTION_SHOW_STAGE_RENAME,
    UI_LAB_ACTION_CONFIRM_SAVE_ALL,
    UI_LAB_ACTION_SETTINGS_TAB,
    UI_LAB_ACTION_SETTINGS_BRIGHTNESS_SET,
    UI_LAB_ACTION_SETTINGS_BRIGHTNESS_STEP,
    UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION,
    UI_LAB_ACTION_SETTINGS_SHOW_CHOICE,
    UI_LAB_ACTION_SETTINGS_CHOOSE,
    UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT,
    UI_LAB_ACTION_SETTINGS_SHOW_CLOCK,
    UI_LAB_ACTION_SETTINGS_CLOCK_STEP,
    UI_LAB_ACTION_SETTINGS_CLOCK_SET,
    UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP,
    UI_LAB_ACTION_SETTINGS_SAVE,
    UI_LAB_ACTION_SETTINGS_DISCARD,
    UI_LAB_ACTION_SETTINGS_UNSAVED_STAY,
    UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB,
    UI_LAB_ACTION_SETTINGS_SAVE_TO_TAB,
    UI_LAB_ACTION_SETTINGS_SHOW_RESET,
    UI_LAB_ACTION_SETTINGS_RESET_SCOPE,
    UI_LAB_ACTION_SETTINGS_RESET_CONFIRM,
    UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS,
    UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM,
    UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY,
    UI_LAB_ACTION_SETTINGS_RESTART_DISPLAY_CONFIRM,
    UI_LAB_ACTION_SETTINGS_ASSESS_RESTART,
    UI_LAB_ACTION_SETTINGS_RESTART_CONFIRM,
    UI_LAB_ACTION_STAGE_EDIT_VALUE,
    UI_LAB_ACTION_SHOW_DELETE_STAGE,
    UI_LAB_ACTION_DELETE_STAGE,
    UI_LAB_ACTION_ADD_STAGE_AFTER,
    UI_LAB_ACTION_EDIT_STAGE_POSITION,
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
    UI_LAB_ACTION_DEVICE_MAINTENANCE_DONE,
    UI_LAB_ACTION_LOG_FILTER,
    UI_LAB_ACTION_DEVICE_LOG_ALARMS,
    UI_LAB_ACTION_MANUAL_START,
    UI_LAB_ACTION_MANUAL_STOP,
    UI_LAB_ACTION_MANUAL_EXIT,
    UI_LAB_ACTION_MANUAL_NEW,
    UI_LAB_ACTION_MANUAL_PAUSE,
    UI_LAB_ACTION_MANUAL_EDIT_TARGET,
    UI_LAB_ACTION_MANUAL_EDIT_RATE,
    UI_LAB_ACTION_MANUAL_DECREMENT_TARGET,
    UI_LAB_ACTION_MANUAL_INCREMENT_TARGET,
    UI_LAB_ACTION_MANUAL_DECREMENT_RATE,
    UI_LAB_ACTION_MANUAL_INCREMENT_RATE,
    UI_LAB_ACTION_TOGGLE_LIGHT,
    UI_LAB_ACTION_TOGGLE_COOLING,
    UI_LAB_ACTION_RETURN_HOME,
    UI_LAB_ACTION_START_AGAIN,
} ui_lab_action_t;

typedef struct {
    furnace_hmi_ui_string_id_t name;
    furnace_hmi_ui_string_id_t kind;
    int32_t target_c;
    int32_t delta_c_per_minute;
    int32_t duration_minutes;
} ui_lab_stage_fixture_t;


/* The delivered program library (web programCatalogue): three epoxy cure
 * schedules within the furnace limits (180 C, 3.0 C/min, 8 stages). Heating
 * ramps from the previous target at the given rate (tenths of a degree per
 * minute); its minutes follow from that. Illustrations, not datasheet values. */
typedef struct {
    furnace_hmi_ui_string_id_t name;
    uint8_t stage_count;
    furnace_hmi_ui_lab_program_stage_t stages[FURNACE_HMI_UI_LAB_MAX_STAGES];
} ui_lab_library_program_t;

#define UI_LAB_HEAT(target, rate, minutes) { FURNACE_HMI_UI_LAB_STAGE_HEATING, (target), (rate), (minutes) }
#define UI_LAB_HOLD(target, minutes) { FURNACE_HMI_UI_LAB_STAGE_HOLD, (target), 0, (minutes) }
static const ui_lab_library_program_t library_programs[] = {
    /* Room-temperature laminating resin with an elevated post-cure. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_1, 4U,
      { UI_LAB_HEAT(40, 3, 50), UI_LAB_HOLD(40, 120), UI_LAB_HEAT(60, 3, 67), UI_LAB_HOLD(60, 240) } },
    /* 120 C prepreg cure with a resin-flow dwell. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_2, 4U,
      { UI_LAB_HEAT(85, 15, 40), UI_LAB_HOLD(85, 30), UI_LAB_HEAT(120, 15, 24), UI_LAB_HOLD(120, 90) } },
    /* High-Tg structural resin: 180 C cure in steps. */
    { FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_EPOXY_3, 6U,
      { UI_LAB_HEAT(90, 10, 65), UI_LAB_HOLD(90, 60), UI_LAB_HEAT(135, 10, 45), UI_LAB_HOLD(135, 90),
        UI_LAB_HEAT(180, 5, 90), UI_LAB_HOLD(180, 120) } },
};
#undef UI_LAB_HEAT
#undef UI_LAB_HOLD
#define UI_LAB_LIBRARY_PROGRAM_COUNT (sizeof(library_programs) / sizeof(library_programs[0]))

_Static_assert(
    UI_LAB_LIBRARY_PROGRAM_COUNT < FURNACE_HMI_UI_LAB_MAX_PROGRAMS,
    "UI-lab program storage must leave room for operator programs"
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
        return UI_LAB_THEME_HEAT;
    case UI_LAB_DRAFT_STAGE_HOLD:
        return UI_LAB_THEME_HOLD;
    case UI_LAB_DRAFT_STAGE_COOL:
        return UI_LAB_THEME_COOL;
    }
    return UI_LAB_MUTED;
}

/* Faint background of a stage number badge in the kind's colour. */
static furnace_hmi_color_role_t stage_kind_tint(ui_lab_draft_stage_kind_t kind)
{
    return kind == UI_LAB_DRAFT_STAGE_HOLD ? FURNACE_HMI_COLOR_HOLD_TINT
        : kind == UI_LAB_DRAFT_STAGE_COOL ? FURNACE_HMI_COLOR_COOL_GLOW : FURNACE_HMI_COLOR_HEAT_GLOW;
}

static ui_lab_tone_t stage_kind_tone(ui_lab_draft_stage_kind_t kind)
{
    switch (kind) {
    case UI_LAB_DRAFT_STAGE_HEATING:
        return UI_LAB_TONE_HEAT;
    case UI_LAB_DRAFT_STAGE_HOLD:
        return UI_LAB_TONE_HOLD;
    case UI_LAB_DRAFT_STAGE_COOL:
        return UI_LAB_TONE_COOL;
    }
    return UI_LAB_TONE_NEUTRAL;
}

static const char *ui_string(furnace_hmi_ui_string_id_t id);

/* A stored program the operator can open: present, and a delivered library
 * program only while the library is listed. */
static bool program_listed(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    return explorer != NULL && index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS &&
        explorer->programs[index].present &&
        (!explorer->programs[index].library || explorer->library_count > 0U);
}

static size_t listed_program_count(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    size_t count = 0U;
    for (size_t index = 0U; index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++index) {
        count += program_listed(explorer, index) ? 1U : 0U;
    }
    return count;
}

/* Storage index of the n-th listed program, or FURNACE_HMI_UI_LAB_MAX_PROGRAMS. */
static size_t listed_program_at(const furnace_hmi_ui_lab_explorer_t *explorer, size_t position)
{
    for (size_t index = 0U; index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++index) {
        if (program_listed(explorer, index) && position-- == 0U) {
            return index;
        }
    }
    return FURNACE_HMI_UI_LAB_MAX_PROGRAMS;
}

static const furnace_hmi_ui_lab_program_t *selected_program_entry(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    return &explorer->programs[explorer->selected_program % FURNACE_HMI_UI_LAB_MAX_PROGRAMS];
}

static bool run_was_stopped(const furnace_hmi_ui_lab_explorer_t *explorer);

static void load_library_programs(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memset(explorer->programs, 0, sizeof(explorer->programs));
    for (size_t index = 0U; index < UI_LAB_LIBRARY_PROGRAM_COUNT; ++index) {
        furnace_hmi_ui_lab_program_t *program = &explorer->programs[index];
        program->present = true;
        program->library = true;
        program->name_string = (uint16_t)library_programs[index].name;
        program->stage_count = library_programs[index].stage_count;
        memcpy(program->stages, library_programs[index].stages, sizeof(program->stages));
    }
    explorer->library_count = (uint8_t)UI_LAB_LIBRARY_PROGRAM_COUNT;
}

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
    if (explorer == NULL || index >= FURNACE_HMI_UI_LAB_MAX_PROGRAMS) {
        return "";
    }
    if (explorer->program_name_overrides[index][0] != 0) {
        return explorer->program_name_overrides[index];
    }
    return explorer->programs[index].library
        ? ui_string((furnace_hmi_ui_string_id_t)explorer->programs[index].name_string) : "";
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
    if (explorer == NULL || !program_listed(explorer, program_index)) {
        return;
    }
    uint8_t values[FURNACE_HMI_UI_LAB_MAX_RECENT_PROGRAMS];
    size_t count = 0U;
    values[count++] = program_index;
    for (size_t index = 0; index < explorer->recent_count &&
                           count < FURNACE_HMI_UI_LAB_MAX_RECENT_PROGRAMS; ++index) {
        if (explorer->quick_launch_recent[index] != program_index) {
            values[count++] = explorer->quick_launch_recent[index];
        }
    }
    memcpy(explorer->quick_launch_recent, values, count);
    explorer->recent_count = (uint8_t)count;
}

static bool toggle_program_favourite(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t program_index
)
{
    if (explorer == NULL || !program_listed(explorer, program_index)) {
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
static size_t program_authored_stage_count(const furnace_hmi_ui_lab_program_t *program);

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

/* One source for summaries, nodes and stage cards: the stored stage, or the
 * open edit session's copy of it. Nothing here reaches the controller until
 * the operator saves the program. */
static ui_lab_stage_fixture_t program_stage(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    const furnace_hmi_ui_lab_program_t *program = selected_program_entry(explorer);
    size_t count = program_authored_stage_count(program);
    if (explorer->staged_session_active &&
        explorer->staged_program == explorer->selected_program &&
        explorer->staged_stage_count > 0U) {
        count = explorer->staged_stage_count;
    }
    ui_lab_stage_fixture_t stage = {
        FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, 120, 10, 1
    };
    if (count == 0U) {
        return stage;
    }
    if (index >= count) { index = count - 1U; }
    if (index < program->stage_count) {
        const furnace_hmi_ui_lab_program_stage_t *stored = &program->stages[index];
        stage.kind = stored->kind == FURNACE_HMI_UI_LAB_STAGE_HOLD
            ? FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD : FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING;
        stage.name = stage.kind;
        stage.target_c = stored->target_c;
        stage.delta_c_per_minute = stored->rate_tenths_c_per_minute;
        stage.duration_minutes = stored->minutes;
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
static int32_t program_curing_minutes(const furnace_hmi_ui_lab_program_t *program)
{
    int64_t minutes = 0;
    for (size_t index = 0U; program != NULL && index < program->stage_count; ++index) {
        minutes += program->stages[index].minutes > 0 ? program->stages[index].minutes : 0;
    }
    return minutes > INT32_MAX ? INT32_MAX : (int32_t)minutes;
}

/* Highest target within the controller limit (30 C for a program without stages). */
static int32_t program_peak_c(const furnace_hmi_ui_lab_program_t *program)
{
    int32_t peak = 30;
    for (size_t index = 0U; program != NULL && index < program->stage_count; ++index) {
        if (program->stages[index].target_c > peak) { peak = program->stages[index].target_c; }
    }
    return peak > UI_LAB_MAX_PROGRAM_TEMPERATURE_C ? UI_LAB_MAX_PROGRAM_TEMPERATURE_C : peak;
}

static const char *program_duration(const furnace_hmi_ui_lab_program_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
             (long)program_curing_minutes(program));
    return text;
}

/* The maximum without its unit, for the Max. temp fact whose label carries the unit. */
static const char *program_maximum_number(const furnace_hmi_ui_lab_program_t *program)
{
    static char text[16];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER),
             (long)program_peak_c(program));
    return text;
}

static const char *program_maximum(const furnace_hmi_ui_lab_program_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
             (long)program_peak_c(program));
    return text;
}

static size_t program_authored_stage_count(const furnace_hmi_ui_lab_program_t *program)
{
    return program != NULL ? program->stage_count : 0U;
}

static size_t editor_stage_count(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer != NULL && explorer->program_editor_active &&
        explorer->staged_program == explorer->selected_program) {
        return explorer->staged_stage_count;
    }
    return program_authored_stage_count(selected_program_entry(explorer));
}

/* ---- Stage editor session (web stageSession) -------------------------------
 * Opening the stage editor copies every stage of the program into the staged
 * arrays and snapshots them. Edits, moves, inserts and deletes work on that
 * copy; Discard restores the snapshot and Save keeps it. Nothing here reaches
 * the controller: a saved program remains a local draft until submitted. */

typedef struct {
    int32_t target;
    int32_t rate;
    int32_t duration;
    int32_t start;
    uint8_t kind;
    bool cooling_rate_enabled;
    bool valid;
    bool name_valid;
    char name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
} ui_lab_stage_slot_t;

static ui_lab_stage_slot_t stage_slot_get(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    ui_lab_stage_slot_t slot;
    slot.target = explorer->staged_targets[index];
    slot.rate = explorer->staged_rates[index];
    slot.duration = explorer->staged_durations[index];
    slot.start = explorer->stage_starts[index];
    slot.kind = explorer->staged_kinds[index];
    slot.cooling_rate_enabled = explorer->staged_cooling_rate_enabled[index];
    slot.valid = explorer->staged_valid[index];
    slot.name_valid = explorer->staged_name_valid[index];
    memcpy(slot.name, explorer->staged_names[index], sizeof(slot.name));
    return slot;
}

static void stage_slot_set(furnace_hmi_ui_lab_explorer_t *explorer, size_t index,
                           const ui_lab_stage_slot_t *slot)
{
    explorer->staged_targets[index] = slot->target;
    explorer->staged_rates[index] = slot->rate;
    explorer->staged_durations[index] = slot->duration;
    explorer->stage_starts[index] = slot->start;
    explorer->staged_kinds[index] = slot->kind;
    explorer->staged_cooling_rate_enabled[index] = slot->cooling_rate_enabled;
    explorer->staged_valid[index] = slot->valid;
    explorer->staged_name_valid[index] = slot->name_valid;
    memcpy(explorer->staged_names[index], slot->name, sizeof(slot->name));
}

static void stage_set_take(const furnace_hmi_ui_lab_explorer_t *explorer, furnace_hmi_ui_lab_stage_set_t *set)
{
    memcpy(set->targets, explorer->staged_targets, sizeof(set->targets));
    memcpy(set->rates, explorer->staged_rates, sizeof(set->rates));
    memcpy(set->durations, explorer->staged_durations, sizeof(set->durations));
    memcpy(set->starts, explorer->stage_starts, sizeof(set->starts));
    memcpy(set->kinds, explorer->staged_kinds, sizeof(set->kinds));
    memcpy(set->cooling_rate_enabled, explorer->staged_cooling_rate_enabled, sizeof(set->cooling_rate_enabled));
    memcpy(set->valid, explorer->staged_valid, sizeof(set->valid));
    memcpy(set->name_valid, explorer->staged_name_valid, sizeof(set->name_valid));
    memcpy(set->names, explorer->staged_names, sizeof(set->names));
    set->program = explorer->staged_program;
    set->count = explorer->staged_stage_count;
    set->active = explorer->staged_session_active;
}

static void stage_set_restore(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_stage_set_t *set)
{
    memcpy(explorer->staged_targets, set->targets, sizeof(set->targets));
    memcpy(explorer->staged_rates, set->rates, sizeof(set->rates));
    memcpy(explorer->staged_durations, set->durations, sizeof(set->durations));
    memcpy(explorer->stage_starts, set->starts, sizeof(set->starts));
    memcpy(explorer->staged_kinds, set->kinds, sizeof(set->kinds));
    memcpy(explorer->staged_cooling_rate_enabled, set->cooling_rate_enabled, sizeof(set->cooling_rate_enabled));
    memcpy(explorer->staged_valid, set->valid, sizeof(set->valid));
    memcpy(explorer->staged_name_valid, set->name_valid, sizeof(set->name_valid));
    memcpy(explorer->staged_names, set->names, sizeof(set->names));
    explorer->staged_program = set->program;
    explorer->staged_stage_count = set->count;
    explorer->staged_session_active = set->active;
}

/* Whether the stages differ from the session's starting point. */
static bool stage_set_changed(const furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_stage_set_t *set)
{
    if (explorer->staged_stage_count != set->count) {
        return true;
    }
    for (size_t index = 0U; index < set->count; ++index) {
        if (explorer->staged_targets[index] != set->targets[index] ||
            explorer->staged_rates[index] != set->rates[index] ||
            explorer->staged_durations[index] != set->durations[index] ||
            explorer->staged_kinds[index] != set->kinds[index]) {
            return true;
        }
    }
    return false;
}

/* Number of stages the editor works on for the selected program. */
static size_t stage_session_count(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    size_t count = program_authored_stage_count(selected_program_entry(explorer));
    if (explorer->staged_session_active && explorer->staged_program == explorer->selected_program &&
        explorer->staged_stage_count > 0U) {
        count = explorer->staged_stage_count;
    }
    return count > FURNACE_HMI_UI_LAB_MAX_STAGES ? FURNACE_HMI_UI_LAB_MAX_STAGES : count;
}

static int32_t stage_start_temperature(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    return index == 0U ? 25 : explorer->staged_targets[index - 1U];
}

static void stage_session_record_starts(furnace_hmi_ui_lab_explorer_t *explorer)
{
    for (size_t index = 0U; index < explorer->staged_stage_count; ++index) {
        explorer->stage_starts[index] = stage_start_temperature(explorer, index);
    }
}

/* Web inheritTargets and reflowStages: a Hold keeps the previous target; a
 * stage with a rate whose starting temperature changed keeps its rate and gets
 * a new duration. Starts travel with their stage when stages move. */
static void stage_session_reflow(furnace_hmi_ui_lab_explorer_t *explorer)
{
    for (size_t index = 0U; index < explorer->staged_stage_count; ++index) {
        const int32_t start = stage_start_temperature(explorer, index);
        if (explorer->staged_kinds[index] == UI_LAB_DRAFT_STAGE_HOLD) {
            explorer->staged_targets[index] = start;
        }
        else if (start != explorer->stage_starts[index] && explorer->staged_rates[index] > 0) {
            const int64_t change = llabs((int64_t)explorer->staged_targets[index] - start);
            const int64_t rate = explorer->staged_rates[index];
            const int64_t minutes = (change * 10 + rate - 1) / rate;
            explorer->staged_durations[index] = minutes < 1 ? 1 : (minutes > INT32_MAX ? INT32_MAX : (int32_t)minutes);
        }
    }
    stage_session_record_starts(explorer);
}

/* Fold the open draft into the session, reflow, and reload the draft. */
static void stage_session_sync(furnace_hmi_ui_lab_explorer_t *explorer)
{
    store_selected_stage_draft(explorer);
    stage_session_reflow(explorer);
    load_stage_draft(explorer, explorer->selected_stage);
}

static void begin_stage_session(furnace_hmi_ui_lab_explorer_t *explorer, furnace_hmi_ui_lab_page_t origin)
{
    explorer->stage_editor_origin =
        origin == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ? FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES : origin;
    stage_set_take(explorer, &explorer->stage_backup);
    const size_t count = stage_session_count(explorer);
    store_selected_stage_draft(explorer);
    for (size_t index = 0U; index < count; ++index) {
        if (!explorer->staged_valid[index]) {
            const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
            explorer->staged_targets[index] = stage.target_c;
            explorer->staged_rates[index] = stage.delta_c_per_minute;
            explorer->staged_durations[index] = stage.duration_minutes;
            explorer->staged_kinds[index] = fixture_stage_kind(&stage) == UI_LAB_DRAFT_STAGE_HOLD
                ? (uint8_t)UI_LAB_DRAFT_STAGE_HOLD : (uint8_t)UI_LAB_DRAFT_STAGE_HEATING;
            explorer->staged_cooling_rate_enabled[index] = false;
            explorer->staged_valid[index] = true;
        }
    }
    explorer->staged_program = explorer->selected_program;
    explorer->staged_stage_count = (uint8_t)count;
    explorer->staged_session_active = true;
    stage_session_record_starts(explorer);
    stage_session_reflow(explorer);
    stage_set_take(explorer, &explorer->stage_baseline);
    explorer->stage_session_open = true;
    load_stage_draft(explorer, explorer->selected_stage);
}

static void stage_session_move(furnace_hmi_ui_lab_explorer_t *explorer, size_t to)
{
    store_selected_stage_draft(explorer);
    const size_t count = explorer->staged_stage_count;
    const size_t from = explorer->selected_stage;
    if (count == 0U || from >= count) {
        return;
    }
    if (to >= count) {
        to = count - 1U;
    }
    if (to != from) {
        const ui_lab_stage_slot_t moving = stage_slot_get(explorer, from);
        if (from < to) {
            for (size_t index = from; index < to; ++index) {
                const ui_lab_stage_slot_t slot = stage_slot_get(explorer, index + 1U);
                stage_slot_set(explorer, index, &slot);
            }
        }
        else {
            for (size_t index = from; index > to; --index) {
                const ui_lab_stage_slot_t slot = stage_slot_get(explorer, index - 1U);
                stage_slot_set(explorer, index, &slot);
            }
        }
        stage_slot_set(explorer, to, &moving);
        explorer->selected_stage = (uint8_t)to;
    }
    stage_session_reflow(explorer);
    load_stage_draft(explorer, explorer->selected_stage);
}

/* Web addStageAfter: a heating stage 20 C above the current target at 1 C/min. */
static void stage_session_add_after(furnace_hmi_ui_lab_explorer_t *explorer)
{
    store_selected_stage_draft(explorer);
    const size_t count = explorer->staged_stage_count;
    if (count >= FURNACE_HMI_UI_LAB_MAX_STAGES || explorer->selected_stage >= count) {
        return;
    }
    const size_t at = (size_t)explorer->selected_stage + 1U;
    for (size_t index = count; index > at; --index) {
        const ui_lab_stage_slot_t slot = stage_slot_get(explorer, index - 1U);
        stage_slot_set(explorer, index, &slot);
    }
    const int32_t start = explorer->staged_targets[explorer->selected_stage];
    const int32_t target = start + 20 > UI_LAB_MAX_PROGRAM_TEMPERATURE_C
        ? UI_LAB_MAX_PROGRAM_TEMPERATURE_C : start + 20;
    ui_lab_stage_slot_t slot;
    memset(&slot, 0, sizeof(slot));
    slot.target = target;
    slot.rate = 10;
    slot.duration = target - start > 1 ? target - start : 1;
    slot.start = start;
    slot.kind = (uint8_t)UI_LAB_DRAFT_STAGE_HEATING;
    slot.valid = true;
    stage_slot_set(explorer, at, &slot);
    explorer->staged_stage_count = (uint8_t)(count + 1U);
    explorer->selected_stage = (uint8_t)at;
    stage_session_reflow(explorer);
    load_stage_draft(explorer, explorer->selected_stage);
}

static void stage_session_delete(furnace_hmi_ui_lab_explorer_t *explorer)
{
    store_selected_stage_draft(explorer);
    const size_t count = explorer->staged_stage_count;
    if (count < 2U || explorer->selected_stage >= count) {
        return;
    }
    for (size_t index = explorer->selected_stage; index + 1U < count; ++index) {
        const ui_lab_stage_slot_t slot = stage_slot_get(explorer, index + 1U);
        stage_slot_set(explorer, index, &slot);
    }
    explorer->staged_valid[count - 1U] = false;
    explorer->staged_name_valid[count - 1U] = false;
    explorer->staged_stage_count = (uint8_t)(count - 1U);
    if (explorer->selected_stage >= explorer->staged_stage_count) {
        explorer->selected_stage = (uint8_t)(explorer->staged_stage_count - 1U);
    }
    stage_session_reflow(explorer);
    load_stage_draft(explorer, explorer->selected_stage);
}

/* Web stageErrors: the first problem, if any, in stage order. */
static bool stage_session_error(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity)
{
    buffer[0] = '\0';
    int32_t start = 25;
    int64_t total = 0;
    for (size_t index = 0U; index < explorer->staged_stage_count; ++index) {
        const int32_t target = explorer->staged_targets[index];
        const int32_t rate = explorer->staged_rates[index];
        const int32_t duration = explorer->staged_durations[index];
        const bool heating = explorer->staged_kinds[index] == UI_LAB_DRAFT_STAGE_HEATING;
        const bool hold = explorer->staged_kinds[index] == UI_LAB_DRAFT_STAGE_HOLD;
        furnace_hmi_ui_string_id_t problem = FURNACE_HMI_UI_STRING_COUNT;
        if (target < 0 || target > UI_LAB_MAX_PROGRAM_TEMPERATURE_C) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_TARGET_FORMAT;
        }
        else if (heating && target <= start) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_START_FORMAT;
        }
        else if (!hold && (rate < 1 || rate > UI_LAB_MAX_PROGRAM_RATE_TENTHS)) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_RATE_FORMAT;
        }
        else if (heating && duration < 1) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_DURATION_FORMAT;
        }
        if (problem != FURNACE_HMI_UI_STRING_COUNT) {
            (void)snprintf(buffer, capacity, ui_string(problem), (unsigned int)index + 1U);
            return true;
        }
        total += duration > 0 ? duration : 0;
        start = target;
    }
    if (total > 48 * 60) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_TOTAL));
        return true;
    }
    return false;
}

/* Leave the dialog for the page it was opened from; `keep` saves the session. */
static bool rebuild_page(furnace_hmi_ui_lab_explorer_t *explorer);
static void clear_modal(furnace_hmi_ui_lab_explorer_t *explorer);

static void close_stage_session(furnace_hmi_ui_lab_explorer_t *explorer, bool keep)
{
    clear_modal(explorer);
    if (keep) {
        stage_session_sync(explorer);
        if (explorer->program_editor_active && stage_set_changed(explorer, &explorer->stage_baseline)) {
            explorer->program_editor_dirty = true;
        }
    }
    else {
        stage_set_restore(explorer, &explorer->stage_backup);
    }
    explorer->stage_session_open = false;
    explorer->page = explorer->stage_editor_origin;
    (void)rebuild_page(explorer);
}

static int32_t program_cooling_minutes(const furnace_hmi_ui_lab_program_t *program)
{
    const int32_t maximum = program_peak_c(program);
    const int32_t delta = maximum > 30 ? maximum - 30 : 0;
    const int32_t estimate = (delta * 5 + 5) / 6;
    return estimate < 20 ? 20 : estimate;
}

static const char *program_cooling(const furnace_hmi_ui_lab_program_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
             (long)program_cooling_minutes(program));
    return text;
}

static const char *program_total(const furnace_hmi_ui_lab_program_t *program)
{
    static char text[32];
    snprintf(text, sizeof(text), furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
             (long)(program_curing_minutes(program) + program_cooling_minutes(program)));
    return text;
}

static void prepare_program_preview(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_program_t *program)
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

/* Decimal separator of the selected language (one character). */
static char decimal_separator(void)
{
    const char *separator = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_SEPARATOR);
    return separator[0] != '\0' ? separator[0] : '.';
}

/* Fill a catalogue pattern such as "{weekday} {day} {month} {year}": each
 * {name} takes the value of that name, so a language can reorder the parts.
 * Unknown names are dropped; the output is always terminated. */
static void format_pattern(char *buffer, size_t capacity, const char *pattern,
                           const char *const *names, const char *const *values, size_t count)
{
    if (buffer == NULL || capacity == 0U) {
        return;
    }
    size_t used = 0U;
    const char *at = pattern != NULL ? pattern : "";
    while (*at != '\0' && used + 1U < capacity) {
        const char *close = *at == '{' ? strchr(at, '}') : NULL;
        if (close == NULL) {
            buffer[used++] = *at++;
            continue;
        }
        const size_t length = (size_t)(close - at - 1);
        for (size_t index = 0U; index < count; ++index) {
            if (strlen(names[index]) == length && strncmp(at + 1, names[index], length) == 0) {
                const size_t value_length = strlen(values[index]);
                if (used + value_length < capacity) {
                    memcpy(buffer + used, values[index], value_length);
                    used += value_length;
                }
                break;
            }
        }
        at = close + 1;
    }
    buffer[used] = '\0';
}

static void format_clock_time(char *buffer, size_t capacity, const furnace_hmi_ui_lab_clock_t *clock,
                              bool twenty_four_hour);

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

static lv_obj_t *create_panel(lv_obj_t *parent, bool raised)
{
    lv_obj_t *panel = lv_obj_create(parent);
    if (panel == NULL) {
        return NULL;
    }
    /* Raised panels are information tiles; everything else is a flat panel. */
    if (raised) {
        ui_lab_theme_info_tile(panel);
    }
    else {
        ui_lab_theme_panel(panel);
    }
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

/* Quick launch entry (web .quick-launch button): 14 px, 6/10 px padding, a
 * long program name wraps to a second line inside the 48 px tile. */
static void style_quick_item(lv_obj_t *button)
{
    lv_obj_set_style_pad_ver(button, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(button, 10, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL && lv_obj_check_type(label, &lv_label_class)) {
        lv_obj_set_style_text_font(label, &furnace_hmi_font_regular_14, LV_PART_MAIN);
        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_center(label);
    }
}

/* Width of one bottom-navigation tab. Rails and the Home Quick launch column
 * use it so their edges line up with the tabs below (web: (100% - 24px) / 4). */
static int32_t nav_column_width(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const int32_t width = lv_obj_get_content_width(explorer->root);
    return width > 3 * UI_LAB_SPACE ? (width - 3 * UI_LAB_SPACE) / 4 : width;
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
    if (state->run_state == FURNACE_HMI_RUN_STATE_COMPLETE) {
        return ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOLING_COMPLETE);
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
    else if (format_id == FURNACE_HMI_UI_STRING_DASHBOARD_FAN_FORMAT && value->value >= 1000) {
        (void)snprintf(buffer, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAN_THOUSANDS_FORMAT),
                       (long)(value->value / 1000),
                       (long)(value->value % 1000), ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_UNIT_RPM));
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

/* Bottom navigation: label and icon of each destination. */
static const furnace_hmi_ui_string_id_t nav_texts[] = {
    FURNACE_HMI_UI_STRING_DASHBOARD_HOME,
    FURNACE_HMI_UI_STRING_DASHBOARD_PROGRAMS,
    FURNACE_HMI_UI_STRING_DASHBOARD_DEVICE,
    FURNACE_HMI_UI_STRING_DASHBOARD_SETTINGS,
};

static void configure_nav_button(lv_obj_t *button, bool active)
{
    if (button == NULL) {
        return;
    }
    ui_lab_theme_button(button, active ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET, UI_LAB_EDGE_TOP);
}

static void update_nav_selection(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->nav == NULL) {
        return;
    }
    const uint8_t selected = nav_destination_for_page(explorer->page);
    for (uint8_t index = 0U; index < 4U; ++index) {
        configure_nav_button(lv_obj_get_child(explorer->nav, index), index == selected);
    }
}

/* A row declares which side its tiles carry their edge on: the side facing
 * away from where the row sits (bottom action rows up, left rails right). The
 * mark is an LVGL user flag, so it costs no storage and dies with the row. */
static void mark_edge_row(lv_obj_t *row, ui_lab_edge_t edge)
{
    if (row == NULL) {
        return;
    }
    lv_obj_add_flag(row, edge == UI_LAB_EDGE_TOP ? LV_OBJ_FLAG_USER_1
                       : edge == UI_LAB_EDGE_RIGHT ? LV_OBJ_FLAG_USER_2
                       : edge == UI_LAB_EDGE_LEFT ? LV_OBJ_FLAG_USER_3 : 0);
}

static ui_lab_edge_t edge_for_parent(const lv_obj_t *button)
{
    const lv_obj_t *row = lv_obj_get_parent(button);
    if (row != NULL && lv_obj_has_flag(row, LV_OBJ_FLAG_USER_1)) {
        return UI_LAB_EDGE_TOP;
    }
    if (row != NULL && lv_obj_has_flag(row, LV_OBJ_FLAG_USER_2)) {
        return UI_LAB_EDGE_RIGHT;
    }
    if (row != NULL && lv_obj_has_flag(row, LV_OBJ_FLAG_USER_3)) {
        return UI_LAB_EDGE_LEFT;
    }
    return UI_LAB_EDGE_BOTTOM;
}

/* Geometry shared by every tile; call after the label exists so the tone
 * also colours it. */
static void configure_button(lv_obj_t *button, ui_lab_tone_t tone)
{
    lv_obj_set_style_pad_all(button, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_min_height(button, 48, LV_PART_MAIN);
    lv_obj_set_height(button, 48);
    lv_obj_set_width(button, 0);
    remove_scroll(button);
    ui_lab_theme_button(button, tone, edge_for_parent(button));
}

static furnace_hmi_ui_lab_explorer_t *action_owner(lv_event_t *event)
{
    const furnace_hmi_ui_lab_action_slot_t *slot = lv_event_get_user_data(event);
    return slot == NULL ? NULL : slot->explorer;
}

typedef enum {
    UI_LAB_ICON_START = 0,  /**< Icon before the label. */
    UI_LAB_ICON_END,        /**< Icon after the label. */
    UI_LAB_ICON_ONLY,       /**< Icon replaces the label (label kept, hidden). */
} ui_lab_icon_place_t;

#define UI_LAB_ICON_SIZE 20
#define UI_LAB_ICON_ONLY_SIZE 22
#define UI_LAB_RAIL_ICON_SIZE 18

/* The label stays child 0 so existing label lookups hold; the icon is appended
 * and a reversed row puts it before the label when it leads. */
static bool add_button_icon(
    lv_obj_t *button,
    furnace_hmi_icon_t icon,
    ui_lab_icon_place_t place,
    int32_t size
)
{
    const lv_image_dsc_t *source = furnace_hmi_icon_image(icon, size);
    if (button == NULL || source == NULL) {
        return false;
    }
    lv_obj_t *image = lv_image_create(button);
    if (image == NULL) {
        return false;
    }
    lv_image_set_src(image, source);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    /* A8 icons draw in the recolour colour: follow the tile's text colour. */
    lv_obj_set_style_image_recolor(image, lv_obj_get_style_text_color(button, LV_PART_MAIN),
                                   LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_image_recolor(image, UI_LAB_THEME_DIM,
                                   (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL && !lv_obj_check_type(label, &lv_label_class)) {
        label = NULL;
    }
    if (place == UI_LAB_ICON_ONLY && label != NULL) {
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_flex_flow(button, place == UI_LAB_ICON_START ? LV_FLEX_FLOW_ROW_REVERSE : LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(button, UI_LAB_SPACE, LV_PART_MAIN);
    return true;
}

typedef struct {
    furnace_hmi_ui_string_id_t text;
    furnace_hmi_icon_t icon;
    ui_lab_icon_place_t place;
} ui_lab_button_icon_t;

/* Mirrors BUTTON_ICONS in the web demo: the same labels carry the same icons. */
static const ui_lab_button_icon_t button_icons[] = {
    { FURNACE_HMI_UI_STRING_UI_LAB_BACK, FURNACE_HMI_ICON_ARROW_LEFT, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BACK, FURNACE_HMI_ICON_ARROW_LEFT, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_PREVIOUS, FURNACE_HMI_ICON_CHEVRON_LEFT, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_NEXT, FURNACE_HMI_ICON_CHEVRON_RIGHT, UI_LAB_ICON_END },
    { FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MORE, FURNACE_HMI_ICON_CHEVRON_RIGHT, UI_LAB_ICON_END },
    { FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM, FURNACE_HMI_ICON_PENCIL, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGES, FURNACE_HMI_ICON_PENCIL, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT, FURNACE_HMI_ICON_TRASH, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT, FURNACE_HMI_ICON_PLUS, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_VIEW_STAGES, FURNACE_HMI_ICON_LIST, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PART_INFO, FURNACE_HMI_ICON_INFO, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM, FURNACE_HMI_ICON_PLAY, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_START_PREVIEW, FURNACE_HMI_ICON_PLAY, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_START, FURNACE_HMI_ICON_PLAY, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_START_MANUAL, FURNACE_HMI_ICON_PLAY, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW, FURNACE_HMI_ICON_PLAY, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW, FURNACE_HMI_ICON_PAUSE, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW, FURNACE_HMI_ICON_STOP, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME, FURNACE_HMI_ICON_HOME, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_FULL_GRAPH, FURNACE_HMI_ICON_CHART, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_SHOW_COOLING, FURNACE_HMI_ICON_CHART, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_HIDE_COOLING, FURNACE_HMI_ICON_CHART, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_SPLIT_VIEW, FURNACE_HMI_ICON_SPLIT, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_FULL_VISUAL, FURNACE_HMI_ICON_FURNACE, UI_LAB_ICON_START },
    { FURNACE_HMI_UI_STRING_UI_LAB_INCREASE, FURNACE_HMI_ICON_PLUS, UI_LAB_ICON_ONLY },
    { FURNACE_HMI_UI_STRING_UI_LAB_DECREASE, FURNACE_HMI_ICON_MINUS, UI_LAB_ICON_ONLY },
};

static void apply_button_icon(lv_obj_t *button, const char *text, ui_lab_tone_t tone)
{
    /* Value fields show data (a program may be named "New program"). */
    if (button == NULL || text == NULL || text[0] == '\0' || tone == UI_LAB_TONE_WELL) {
        return;
    }
    /* Symbol-only buttons: pencil and pager arrows. */
    if (strcmp(text, LV_SYMBOL_EDIT) == 0) {
        (void)add_button_icon(button, FURNACE_HMI_ICON_PENCIL, UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE);
        return;
    }
    if (strcmp(text, LV_SYMBOL_LEFT) == 0 || strcmp(text, LV_SYMBOL_RIGHT) == 0) {
        (void)add_button_icon(button,
                              strcmp(text, LV_SYMBOL_LEFT) == 0 ? FURNACE_HMI_ICON_CHEVRON_LEFT
                                                                : FURNACE_HMI_ICON_CHEVRON_RIGHT,
                              UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE);
        return;
    }
    for (size_t index = 0U; index < sizeof(button_icons) / sizeof(button_icons[0]); ++index) {
        if (strcmp(text, ui_string(button_icons[index].text)) == 0) {
            (void)add_button_icon(button, button_icons[index].icon, button_icons[index].place,
                                  button_icons[index].place == UI_LAB_ICON_ONLY
                                      ? UI_LAB_ICON_ONLY_SIZE : UI_LAB_ICON_SIZE);
            return;
        }
    }
}

static lv_obj_t *create_action_button(
    lv_obj_t *parent,
    const char *text,
    ui_lab_tone_t tone,
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
    if (text != NULL && text[0] != '\0') {
        lv_obj_t *label = create_label(button, text, &furnace_hmi_font_regular_16, UI_LAB_TEXT);
        if (label == NULL) { lv_obj_delete(button); return NULL; }
        lv_obj_center(label);
    }
    configure_button(button, tone);
    apply_button_icon(button, text, tone);
    lv_obj_add_event_cb(button, action_callback, LV_EVENT_CLICKED, slot);
    return button;
}

static lv_obj_t *create_page_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    ui_lab_tone_t tone,
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
        tone,
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
    ui_lab_tone_t tone,
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
        tone,
        &explorer->modal_actions[explorer->modal_action_count++],
        explorer,
        action,
        value
    );
}

/* The one pager component (web .pager): previous arrow, "1 / 2", next arrow
 * on a single tile whose steel edge faces away from where it sits. The arrows
 * are disabled at the first and last page. */
static lv_obj_t *create_pager(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    size_t page,
    size_t page_count,
    ui_lab_action_t previous_action,
    ui_lab_action_t next_action,
    ui_lab_edge_t edge
)
{
    lv_obj_t *pager = lv_obj_create(parent);
    if (pager == NULL) {
        return NULL;
    }
    ui_lab_theme_info_tile(pager);
    ui_lab_theme_edge(pager, UI_LAB_TONE_NAV, edge);
    lv_obj_set_size(pager, LV_SIZE_CONTENT, 48);
    lv_obj_set_style_pad_all(pager, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(pager, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(pager, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pager, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    remove_scroll(pager);
    char text[24];
    (void)snprintf(text, sizeof(text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAGE_FORMAT),
                   (unsigned int)page + 1U, (unsigned int)(page_count == 0U ? 1U : page_count));
    lv_obj_t *previous = create_page_button(explorer, pager, LV_SYMBOL_LEFT, UI_LAB_TONE_FLAT,
                                            previous_action, 0);
    lv_obj_t *label = create_label(pager, text, &furnace_hmi_font_semibold_15, UI_LAB_TEXT);
    lv_obj_t *next = create_page_button(explorer, pager, LV_SYMBOL_RIGHT, UI_LAB_TONE_FLAT,
                                        next_action, 0);
    if (previous == NULL || label == NULL || next == NULL) {
        return NULL;
    }
    lv_obj_set_size(previous, 48, 46);
    lv_obj_set_size(next, 48, 46);
    lv_obj_set_style_min_height(previous, 46, LV_PART_MAIN);
    lv_obj_set_style_min_height(next, 46, LV_PART_MAIN);
    lv_obj_set_style_min_width(label, 64, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(label, 12, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (page == 0U) {
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    }
    if (page + 1U >= page_count) {
        lv_obj_add_state(next, LV_STATE_DISABLED);
    }
    return pager;
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
    lv_obj_set_height(metric, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metric, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(metric, 0, LV_PART_MAIN);
    lv_obj_t *label = create_label(metric, label_text, &furnace_hmi_font_regular_14, UI_LAB_MUTED);
    lv_obj_t *value = create_label(metric, value_text, &furnace_hmi_font_regular_23, UI_LAB_TEXT);
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
    lv_obj_set_height(metric, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(metric, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(metric, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(metric, 0, LV_PART_MAIN);
    lv_obj_t *label = create_label(metric, label_text, &furnace_hmi_font_regular_14, UI_LAB_MUTED);
    lv_obj_t *value = create_label(metric, value_text, &furnace_hmi_font_regular_36, UI_LAB_TEXT);
    if (value_out != NULL) { *value_out = value; }
    return label != NULL && value != NULL;
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

/* Running program: the visible window ends a short way into automatic cooling
 * (web coolingWindowMinutes: at most a quarter of the curing time), or at the
 * cooling measured so far while the operator shows all of it. False when the
 * observation carries no planned cooling. `overflows` reports whether the
 * measured cooling has passed the short window (the toggle is then offered). */
static bool running_cooling_window(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    int64_t *window_end_seconds,
    bool *overflows
)
{
    const furnace_hmi_dashboard_state_t *state = explorer->state;
    if (state == NULL || explorer->trajectory_uses_program_preview ||
        state->mode != FURNACE_HMI_MACHINE_MODE_PROGRAM || state->graph_sample_count < 2U) {
        return false;
    }
    const size_t count = state->graph_sample_count;
    size_t curing_count = count;
    for (size_t index = 1U; index < count; ++index) {
        if (state->graph_samples[index].planned_cooling) {
            curing_count = index;
            break;
        }
    }
    if (curing_count >= count) {
        return false;
    }
    const int64_t curing_end = state->graph_samples[curing_count - 1U].elapsed_seconds;
    const int64_t estimate = (int64_t)state->graph_samples[count - 1U].elapsed_seconds - curing_end;
    int64_t cap = (curing_end / 60 / 4) * 60;
    if (cap < 60) {
        cap = 60;
    }
    if (cap > estimate) {
        cap = estimate > 0 ? estimate : 0;
    }
    const int64_t cooled = state->cooling_elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
                           state->cooling_elapsed_seconds.value > 0
        ? state->cooling_elapsed_seconds.value : 0;
    *overflows = cooled > cap;
    *window_end_seconds = curing_end + (explorer->running_cooling_full && cooled > cap ? cooled : cap);
    return true;
}

/* Time and temperature extent of the plotted data. With a time limit only
 * samples inside the window count, and the window end is the time extent. */
static void trajectory_domain(
    const furnace_hmi_dashboard_state_t *state,
    int64_t time_limit,
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
            if (time_limit > 0 && (int64_t)sample->elapsed_seconds > time_limit) {
                continue;
            }
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
            state->elapsed_seconds.value > latest_seconds &&
            (time_limit <= 0 || state->elapsed_seconds.value <= time_limit)) {
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
    if (time_limit > latest_seconds) {
        latest_seconds = time_limit;
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
    /* The time axis ends where the data ends (web: no trailing margin). */
    *maximum_seconds = latest_seconds < 60 ? 60 : latest_seconds;
}

static bool trajectory_measurement_outside_domain(
    const furnace_hmi_dashboard_state_t *state,
    const furnace_hmi_ui_lab_explorer_t *explorer
)
{
    if (state == NULL || explorer == NULL || !explorer->trajectory_domain_initialized) {
        return true;
    }
    const int64_t limit = explorer->trajectory_time_limit_seconds;
    if (limit > explorer->trajectory_domain_maximum_seconds) {
        return true;
    }
    if (state->elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
        (limit <= 0 || state->elapsed_seconds.value <= limit) &&
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
        if (limit > 0 && (int64_t)sample->elapsed_seconds > limit) {
            continue;
        }
        if (sample->measured_validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            (sample->elapsed_seconds > (uint32_t)explorer->trajectory_domain_maximum_seconds ||
             sample->measured_temperature_c < explorer->trajectory_domain_minimum_temperature_c ||
             sample->measured_temperature_c > explorer->trajectory_domain_maximum_temperature_c)) {
            return true;
        }
    }
    return false;
}

typedef struct {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    int64_t maximum_seconds;
    int32_t minimum_temperature_c;
    int32_t maximum_temperature_c;
} ui_lab_plot_t;

static lv_point_precise_t plot_point(const ui_lab_plot_t *plot, int64_t seconds, int64_t temperature_c)
{
    const int64_t x = plot->left + seconds * plot->width / plot->maximum_seconds;
    const int64_t y = plot->top + plot->height -
        (temperature_c - plot->minimum_temperature_c) * plot->height /
            ((int64_t)plot->maximum_temperature_c - plot->minimum_temperature_c);
    return (lv_point_precise_t) { (lv_value_precise_t)x, (lv_value_precise_t)y };
}

/* Value of a straight segment at `seconds` (used to clip lines at the window). */
static int64_t interpolate_at(int64_t seconds, int64_t start_seconds, int64_t start_value,
                              int64_t end_seconds, int64_t end_value)
{
    if (end_seconds <= start_seconds) {
        return end_value;
    }
    return start_value + (end_value - start_value) * (seconds - start_seconds) / (end_seconds - start_seconds);
}

static bool update_trajectory(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->trajectory == NULL || explorer->planned_line == NULL ||
        explorer->state == NULL) {
        return false;
    }
    const furnace_hmi_dashboard_state_t *graph_state = explorer->trajectory_uses_program_preview
        ? &explorer->preview_graph_state : explorer->state;
    int64_t window_end = 0;
    bool overflows = false;
    explorer->trajectory_time_limit_seconds =
        running_cooling_window(explorer, &window_end, &overflows) ? window_end : 0;
    const int64_t limit = explorer->trajectory_time_limit_seconds;
    if (!explorer->trajectory_domain_initialized ||
        (!explorer->trajectory_uses_program_preview &&
         trajectory_measurement_outside_domain(graph_state, explorer))) {
        const int64_t old_time = explorer->trajectory_domain_maximum_seconds;
        const int32_t old_min = explorer->trajectory_domain_minimum_temperature_c;
        const int32_t old_max = explorer->trajectory_domain_maximum_temperature_c;
        trajectory_domain(graph_state, limit, &explorer->trajectory_domain_maximum_seconds,
                          &explorer->trajectory_domain_minimum_temperature_c,
                          &explorer->trajectory_domain_maximum_temperature_c);
        if (explorer->trajectory_domain_initialized) {
            if (old_time > explorer->trajectory_domain_maximum_seconds) { explorer->trajectory_domain_maximum_seconds = old_time; }
            if (old_min < explorer->trajectory_domain_minimum_temperature_c) { explorer->trajectory_domain_minimum_temperature_c = old_min; }
            if (old_max > explorer->trajectory_domain_maximum_temperature_c) { explorer->trajectory_domain_maximum_temperature_c = old_max; }
        }
        explorer->trajectory_domain_initialized = true;
    }
    const int32_t graph_width = (int32_t)lv_obj_get_width(explorer->trajectory);
    const int32_t graph_height = (int32_t)lv_obj_get_height(explorer->trajectory);
    if (graph_width < 160 || graph_height <= UI_LAB_GRAPH_TOP + UI_LAB_GRAPH_BOTTOM) {
        /* Compact structural floor: no drawable plot until layout supplies room. */
        return true;
    }
    const ui_lab_plot_t plot = {
        UI_LAB_GRAPH_LEFT,
        UI_LAB_GRAPH_TOP,
        graph_width - UI_LAB_GRAPH_LEFT - UI_LAB_GRAPH_RIGHT,
        graph_height - UI_LAB_GRAPH_TOP - UI_LAB_GRAPH_BOTTOM,
        explorer->trajectory_domain_maximum_seconds,
        explorer->trajectory_domain_minimum_temperature_c,
        explorer->trajectory_domain_maximum_temperature_c,
    };
    if (plot.width <= 0 || plot.height <= 0 || plot.maximum_seconds <= 0 ||
        plot.maximum_temperature_c <= plot.minimum_temperature_c) {
        return false;
    }
    char minimum_text[24];
    char middle_text[24];
    char maximum_text[24];
    char middle_time_text[24];
    char end_time_text[24];
    (void)snprintf(minimum_text, sizeof(minimum_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)plot.minimum_temperature_c);
    (void)snprintf(maximum_text, sizeof(maximum_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)plot.maximum_temperature_c);
    (void)snprintf(middle_text, sizeof(middle_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C),
                   (long)(plot.minimum_temperature_c +
                          ((int64_t)plot.maximum_temperature_c - plot.minimum_temperature_c) / 2));
    (void)snprintf(middle_time_text, sizeof(middle_time_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                   (long)(plot.maximum_seconds / 120));
    (void)snprintf(end_time_text, sizeof(end_time_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                   (long)(plot.maximum_seconds / 60));
    lv_obj_t *const temperature_labels[] = {
        explorer->trajectory_maximum_label, explorer->trajectory_middle_temperature_label,
        explorer->trajectory_minimum_label,
    };
    const char *const temperature_texts[] = { maximum_text, middle_text, minimum_text };
    for (size_t index = 0U; index < 3U; ++index) {
        if (temperature_labels[index] != NULL) {
            lv_label_set_text(temperature_labels[index], temperature_texts[index]);
            /* Degree labels sit in the left gutter, centred on their value. */
            lv_obj_set_pos(temperature_labels[index], UI_LAB_GRAPH_INSET + 2,
                           plot.top + plot.height * (int32_t)index / 2 - 9);
        }
    }
    const int32_t time_y = graph_height - UI_LAB_GRAPH_INSET - 17;
    if (explorer->trajectory_time_axis_label != NULL) {
        lv_obj_set_pos(explorer->trajectory_time_axis_label, plot.left, time_y);
    }
    if (explorer->trajectory_middle_time_label != NULL) {
        lv_label_set_text(explorer->trajectory_middle_time_label, middle_time_text);
        lv_obj_align(explorer->trajectory_middle_time_label, LV_ALIGN_TOP_MID,
                     plot.left + plot.width / 2 - graph_width / 2, time_y);
    }
    if (explorer->trajectory_end_time_label != NULL) {
        lv_label_set_text(explorer->trajectory_end_time_label, end_time_text);
        lv_obj_align(explorer->trajectory_end_time_label, LV_ALIGN_TOP_RIGHT, -UI_LAB_GRAPH_RIGHT, time_y);
    }
    for (size_t index = 0;
         index < FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT; ++index) {
        const int32_t y = plot.top + plot.height * (int32_t)index /
            (int32_t)(FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT - 1U);
        explorer->grid_line_points[index][0] = (lv_point_precise_t) { plot.left, y };
        explorer->grid_line_points[index][1] = (lv_point_precise_t) { plot.left + plot.width, y };
        if (explorer->grid_lines[index] != NULL) {
            lv_line_set_points(explorer->grid_lines[index], explorer->grid_line_points[index], 2U);
        }
    }
    for (size_t index = 0;
         index < FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT; ++index) {
        const size_t line_index = FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT + index;
        const int32_t x = plot.left + plot.width * (int32_t)index /
            (int32_t)(FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT - 1U);
        explorer->grid_line_points[line_index][0] = (lv_point_precise_t) { x, plot.top };
        explorer->grid_line_points[line_index][1] = (lv_point_precise_t) { x, plot.top + plot.height };
        if (explorer->grid_lines[line_index] != NULL) {
            lv_line_set_points(explorer->grid_lines[line_index], explorer->grid_line_points[line_index], 2U);
        }
    }
    const size_t count = graph_state->graph_sample_count;
    for (size_t index = 0; index < count; ++index) {
        const furnace_hmi_graph_sample_t *sample = &graph_state->graph_samples[index];
        explorer->planned_points[index] = plot_point(&plot, sample->elapsed_seconds, sample->planned_temperature_c);
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
    explorer->cooling_point_count = 0U;
    if (show_cooling) {
        for (size_t index = curing_count - 1U; index < count; ++index) {
            const furnace_hmi_graph_sample_t *sample = &graph_state->graph_samples[index];
            if (limit > 0 && (int64_t)sample->elapsed_seconds > limit) {
                const furnace_hmi_graph_sample_t *previous = &graph_state->graph_samples[index - 1U];
                explorer->cooling_points[explorer->cooling_point_count++] = plot_point(
                    &plot, limit,
                    interpolate_at(limit, previous->elapsed_seconds, previous->planned_temperature_c,
                                   sample->elapsed_seconds, sample->planned_temperature_c));
                break;
            }
            explorer->cooling_points[explorer->cooling_point_count++] = explorer->planned_points[index];
        }
    }
    /* Planned samples beyond the window are not drawn; keep their stored
     * points on the window's end so every point stays inside the graph. */
    if (limit > 0 && explorer->cooling_point_count > 0U) {
        for (size_t index = 0; index < count; ++index) {
            if ((int64_t)graph_state->graph_samples[index].elapsed_seconds > limit) {
                explorer->planned_points[index] = explorer->cooling_points[explorer->cooling_point_count - 1U];
            }
        }
    }
    const int64_t curing_end = curing_count < count
        ? (int64_t)graph_state->graph_samples[curing_count - 1U].elapsed_seconds : INT64_MAX;
    explorer->measured_cooling_point_count = 0U;
    for (size_t index = 0; index + 1U < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
        lv_obj_t *segment = explorer->measured_segments[index];
        if (segment == NULL) {
            continue;
        }
        lv_obj_add_flag(segment, LV_OBJ_FLAG_HIDDEN);
        if (index + 1U >= count ||
            graph_state->graph_samples[index].measured_validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT ||
            graph_state->graph_samples[index + 1U].measured_validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
            continue;
        }
        const furnace_hmi_graph_sample_t *first = &graph_state->graph_samples[index];
        const furnace_hmi_graph_sample_t *second = &graph_state->graph_samples[index + 1U];
        if (limit > 0 && (int64_t)first->elapsed_seconds >= limit) {
            continue;
        }
        const lv_point_precise_t start = plot_point(&plot, first->elapsed_seconds, first->measured_temperature_c);
        lv_point_precise_t end = plot_point(&plot, second->elapsed_seconds, second->measured_temperature_c);
        if (limit > 0 && (int64_t)second->elapsed_seconds > limit) {
            end = plot_point(&plot, limit,
                             interpolate_at(limit, first->elapsed_seconds, first->measured_temperature_c,
                                            second->elapsed_seconds, second->measured_temperature_c));
        }
        if ((int64_t)first->elapsed_seconds >= curing_end) {
            /* Measured cooling: part of the dashed blue overlay. */
            if (explorer->measured_cooling_point_count == 0U) {
                explorer->measured_cooling_points[explorer->measured_cooling_point_count++] = start;
            }
            explorer->measured_cooling_points[explorer->measured_cooling_point_count++] = end;
            continue;
        }
        explorer->measured_segment_points[index][0] = start;
        explorer->measured_segment_points[index][1] = end;
        lv_line_set_points(segment, explorer->measured_segment_points[index], 2U);
        lv_obj_remove_flag(segment, LV_OBJ_FLAG_HIDDEN);
    }
    if (explorer->cooling_overlay != NULL) {
        if (explorer->cooling_point_count >= 2U || explorer->measured_cooling_point_count >= 2U) {
            lv_obj_remove_flag(explorer->cooling_overlay, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(explorer->cooling_overlay);
        }
        else {
            lv_obj_add_flag(explorer->cooling_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (count < 2U) {
        lv_obj_add_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN);
        return true;
    }
    lv_obj_remove_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN);
    /* With a cooling segment the solid planned line ends where curing ends. */
    lv_line_set_points(explorer->planned_line, explorer->planned_points,
                       (uint32_t)(show_cooling ? curing_count : count));
    if (!planned_visible) {
        lv_obj_add_flag(explorer->planned_line, LV_OBJ_FLAG_HIDDEN);
    }
    for (size_t index = 0; index < FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT; ++index) {
        lv_obj_t *node = explorer->trajectory_stage_nodes[index];
        if (node == NULL) {
            continue;
        }
        const size_t point_index = index < count ? index : count - 1U;
        lv_obj_set_pos(node, explorer->planned_points[point_index].x - 5,
                       explorer->planned_points[point_index].y - 5);
    }
    if (explorer->manual_marker != NULL && explorer->manual_marker_index >= 0 &&
        (size_t)explorer->manual_marker_index < count) {
        const lv_point_precise_t now = explorer->planned_points[explorer->manual_marker_index];
        lv_obj_set_pos(explorer->manual_marker, now.x - 5, now.y - 5);
    }
    lv_obj_invalidate(explorer->trajectory);
    return true;
}

/* LVGL's software renderer dashes only horizontal and vertical lines, so the
 * diagonal cooling estimate is drawn as a bounded run of short segments. */
static void draw_dashed_polyline(lv_layer_t *layer, const lv_area_t *coords,
                                 const lv_point_precise_t *points, uint32_t count, lv_color_t color)
{
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = color;
    line.width = 3;
    const double cycle = UI_LAB_COOLING_DASH + UI_LAB_COOLING_GAP;
    double travelled = 0.0;
    for (uint32_t index = 0U; index + 1U < count; ++index) {
        const double x0 = (double)coords->x1 + (double)points[index].x;
        const double y0 = (double)coords->y1 + (double)points[index].y;
        const double dx = (double)points[index + 1U].x - (double)points[index].x;
        const double dy = (double)points[index + 1U].y - (double)points[index].y;
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

/* Planned cooling (green) and measured cooling (blue) are both dashed. */
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
    draw_dashed_polyline(layer, &coords, explorer->cooling_points, explorer->cooling_point_count,
                         UI_LAB_COOLING_ESTIMATE);
    draw_dashed_polyline(layer, &coords, explorer->measured_cooling_points,
                         explorer->measured_cooling_point_count,
                         ui_lab_color(FURNACE_HMI_COLOR_GRAPH_COOLING));
}

static void trajectory_size_changed(lv_event_t *event)
{
    furnace_hmi_ui_lab_explorer_t *explorer = lv_event_get_user_data(event);
    if (explorer->planned_line != NULL) { (void)update_trajectory(explorer); }
}

/* The graph's legend (web programGraph caption, right-hand side). */
static const char *trajectory_legend_text(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
        return ui_string(explorer->manual_running || explorer->manual_cooling ? FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_MEASURED_PREDICTED
                                                  : FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_PREDICTED);
    }
    return ui_string(explorer->trajectory_uses_program_preview ? FURNACE_HMI_UI_STRING_DASHBOARD_PLANNED
                                                               : FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_PLANNED_MEASURED);
}

/* A graph panel (web .program-graph): caption and legend on one row, the plot
 * below with degree labels on the left and 0 / middle / end minutes under it. */
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
    ui_lab_theme_panel(explorer->trajectory);
    lv_obj_set_style_pad_all(explorer->trajectory, 0, LV_PART_MAIN);
    remove_scroll(explorer->trajectory);
    for (size_t index = 0; index < sizeof(explorer->grid_lines) /
                           sizeof(explorer->grid_lines[0]); ++index) {
        explorer->grid_lines[index] = make_graph_line(
            explorer->trajectory, explorer->grid_line_points[index], 2U, UI_LAB_GRAPH_GRID, 1, false
        );
        if (explorer->grid_lines[index] == NULL) {
            return false;
        }
    }
    explorer->planned_line = make_graph_line(explorer->trajectory, explorer->planned_points, 0U,
                                              UI_LAB_PLANNED, 2, false);
    /* Created after the planned line and before the markers: above the grid,
     * below the node markers. Hidden unless there is cooling to draw. */
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
        /* The measured trace is amber in every run state (web). */
        explorer->measured_segments[index] = make_graph_line(
            explorer->trajectory, explorer->measured_segment_points[index], 0U,
            UI_LAB_THEME_AMBER, 3, false
        );
        if (explorer->measured_segments[index] == NULL) {
            return false;
        }
    }
    const lv_font_t *font = &furnace_hmi_font_regular_13;
    lv_obj_t *caption = create_label(explorer->trajectory,
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_GRAPH_TEMPERATURE_AXIS),
                                     font, UI_LAB_MUTED);
    explorer->trajectory_minimum_label = create_label(explorer->trajectory, "", font, UI_LAB_MUTED);
    explorer->trajectory_maximum_label = create_label(explorer->trajectory, "", font, UI_LAB_MUTED);
    explorer->trajectory_middle_temperature_label = create_label(explorer->trajectory, "", font, UI_LAB_MUTED);
    explorer->trajectory_time_axis_label = create_label(explorer->trajectory, "0", font, UI_LAB_MUTED);
    explorer->trajectory_end_time_label = create_label(explorer->trajectory, "", font, UI_LAB_MUTED);
    explorer->trajectory_middle_time_label = create_label(explorer->trajectory, "", font, UI_LAB_MUTED);
    explorer->trajectory_legend_label = create_label(explorer->trajectory, trajectory_legend_text(explorer),
                                                     font, UI_LAB_MUTED);
    if (explorer->planned_line == NULL || caption == NULL ||
        explorer->trajectory_minimum_label == NULL || explorer->trajectory_maximum_label == NULL ||
        explorer->trajectory_middle_temperature_label == NULL || explorer->trajectory_time_axis_label == NULL ||
        explorer->trajectory_end_time_label == NULL || explorer->trajectory_middle_time_label == NULL ||
        explorer->trajectory_legend_label == NULL) {
        return false;
    }
    lv_obj_align(caption, LV_ALIGN_TOP_LEFT, UI_LAB_GRAPH_INSET, UI_LAB_GRAPH_INSET);
    lv_obj_align(explorer->trajectory_legend_label, LV_ALIGN_TOP_RIGHT, -UI_LAB_GRAPH_INSET, UI_LAB_GRAPH_INSET);
    /* Program previews mark each stage start; tapping one opens the stage
     * editor. Not while running, in Manual, or under the stage dialog. */
    if (explorer->trajectory_uses_program_preview && explorer->page != FURNACE_HMI_UI_LAB_PAGE_MANUAL &&
        explorer->page != FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
        size_t node_count = program_authored_stage_count(selected_program_entry(explorer));
        if (node_count > FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT) {
            node_count = FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT;
        }
        for (size_t index = 0; index < node_count; ++index) {
            const ui_lab_stage_fixture_t node_stage = program_stage(explorer, index);
            explorer->trajectory_stage_nodes[index] = create_page_button(
                explorer, explorer->trajectory, "", UI_LAB_TONE_FLAT,
                UI_LAB_ACTION_STAGE_EDITOR, (int)index
            );
            if (explorer->trajectory_stage_nodes[index] == NULL) {
                return false;
            }
            /* Stage nodes are round 10 px markers in the stage kind's colour. */
            lv_obj_set_style_bg_color(explorer->trajectory_stage_nodes[index],
                                      stage_kind_color(fixture_stage_kind(&node_stage)), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(explorer->trajectory_stage_nodes[index], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_radius(explorer->trajectory_stage_nodes[index], LV_RADIUS_CIRCLE, LV_PART_MAIN);
            lv_obj_set_style_min_height(explorer->trajectory_stage_nodes[index], 10, LV_PART_MAIN);
            lv_obj_set_style_pad_all(explorer->trajectory_stage_nodes[index], 0, LV_PART_MAIN);
            lv_obj_set_size(explorer->trajectory_stage_nodes[index], 10, 10);
            lv_obj_set_ext_click_area(explorer->trajectory_stage_nodes[index], 8);
            lv_obj_add_flag(explorer->trajectory_stage_nodes[index], LV_OBJ_FLAG_FLOATING);
        }
    }
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
        /* Manual "now" marker at the current temperature (web amber dot). */
        explorer->manual_marker = lv_obj_create(explorer->trajectory);
        if (explorer->manual_marker == NULL) {
            return false;
        }
        lv_obj_remove_style_all(explorer->manual_marker);
        lv_obj_set_size(explorer->manual_marker, 10, 10);
        lv_obj_set_style_radius(explorer->manual_marker, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(explorer->manual_marker, UI_LAB_THEME_AMBER, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(explorer->manual_marker, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_remove_flag(explorer->manual_marker, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(explorer->manual_marker, LV_OBJ_FLAG_FLOATING);
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
        explorer->alert_open = 0U;
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
    UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET,
    UI_LAB_KEYBOARD_TARGET_DRAFT_RATE,
    UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION,
    UI_LAB_KEYBOARD_TARGET_MANUAL_TARGET,
    UI_LAB_KEYBOARD_TARGET_MANUAL_RATE,
    UI_LAB_KEYBOARD_TARGET_STAGE_POSITION,
};

enum {
    UI_LAB_KEYBOARD_BUTTON_INSERT = 1,
    UI_LAB_KEYBOARD_BUTTON_BACKSPACE,
    UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT,
    UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT,
    UI_LAB_KEYBOARD_BUTTON_CLEAR,
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
    lv_obj_t *label = lv_textarea_get_label(explorer->keyboard_input);
    if (explorer->keyboard_selected != 0U && explorer->keyboard_anchor != explorer->keyboard_cursor) {
        const size_t start = explorer->keyboard_cursor < explorer->keyboard_anchor
            ? explorer->keyboard_cursor : explorer->keyboard_anchor;
        const size_t end = explorer->keyboard_cursor > explorer->keyboard_anchor
            ? explorer->keyboard_cursor : explorer->keyboard_anchor;
        lv_label_set_text_selection_start(
            label, (uint32_t)keyboard_utf8_character_count(explorer->keyboard_value, start));
        lv_label_set_text_selection_end(
            label, (uint32_t)keyboard_utf8_character_count(explorer->keyboard_value, end));
    }
    else {
        lv_textarea_clear_selection(explorer->keyboard_input);
    }
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
            if (!isdigit((unsigned char)text[index]) && text[index] != decimal_separator()) {
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
        else if (character == decimal_separator() && !decimal) {
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
        explorer->keyboard_value, explorer->keyboard_length) <= UI_LAB_KEYBOARD_TEXT_LIMIT;
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
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT &&
        explorer->keyboard_count != NULL && lv_obj_is_valid(explorer->keyboard_count)) {
        char count[24];
        (void)snprintf(count, sizeof(count),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_COUNT_FORMAT),
                       (unsigned int)keyboard_utf8_character_count(explorer->keyboard_value,
                                                                  explorer->keyboard_length),
                       (unsigned int)UI_LAB_KEYBOARD_TEXT_LIMIT);
        lv_label_set_text(explorer->keyboard_count, count);
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
    ui_lab_tone_t tone,
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
    const char *caption = action == UI_LAB_KEYBOARD_BUTTON_INSERT && strcmp(slot->text, " ") == 0
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_SPACE) : slot->text;
    lv_obj_t *label = create_label(button, caption,
                                   function_key ? &furnace_hmi_font_regular_14 : &furnace_hmi_font_regular_20,
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
    configure_button(button, tone);
    ui_lab_theme_set_edge_side(button, UI_LAB_EDGE_TOP);
    if (strcmp(slot->text, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CAPS)) == 0) {
        (void)add_button_icon(button, FURNACE_HMI_ICON_SHIFT, UI_LAB_ICON_START, UI_LAB_ICON_SIZE);
    }
    else if (strcmp(slot->text, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE)) == 0) {
        (void)add_button_icon(button, FURNACE_HMI_ICON_BACKSPACE, UI_LAB_ICON_START, UI_LAB_ICON_SIZE);
    }
    else if (strcmp(slot->text, LV_SYMBOL_LEFT) == 0 || strcmp(slot->text, LV_SYMBOL_RIGHT) == 0) {
        (void)add_button_icon(button,
                              strcmp(slot->text, LV_SYMBOL_LEFT) == 0 ? FURNACE_HMI_ICON_ARROW_LEFT
                                                                      : FURNACE_HMI_ICON_ARROW_RIGHT,
                              UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE);
    }
    lv_obj_set_width(button, 0);
    lv_obj_set_height(button, lv_pct(100));
    if (disabled) {
        /* Unavailable keys recede rather than looking highlighted: override the
         * default theme's disabled recolor, which lightens the key. */
        const lv_style_selector_t disabled_main =
            (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED;
        lv_obj_add_state(button, LV_STATE_DISABLED);
        lv_obj_set_style_bg_color(button, ui_lab_color(FURNACE_HMI_COLOR_KEY_DISABLED), disabled_main);
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
        const ui_lab_tone_t tone = explorer->keyboard_shift != 0U
            ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_NEUTRAL;
        lv_obj_t *function_button = keyboard_add_button(
            explorer, row, function, strlen(function),
            UI_LAB_KEYBOARD_BUTTON_SHIFT,
            tone, true, false);
        if (function_button == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(function_button, 3);
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
        lv_obj_t *key = keyboard_add_button(explorer, row, label, length,
                                            UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY,
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
            UI_LAB_KEYBOARD_BUTTON_BACKSPACE, UI_LAB_TONE_DANGER, true, false);
        if (delete_key == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(delete_key, 3);
    }
    return true;
}

/* Bottom row of the text keyboard on a 20-column grid, as on the web:
 * Cancel | Clear | gap | Space | left | right | gap | Apply. */
static const int32_t keyboard_bottom_columns[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
    LV_GRID_TEMPLATE_LAST
};
static const int32_t keyboard_single_row[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static const int32_t keyboard_number_columns[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};
static const int32_t keyboard_number_rows[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};

typedef struct {
    const char *text;
    uint8_t action;
    ui_lab_tone_t tone;
    bool function_key;
    uint8_t column;
    uint8_t column_span;
    uint8_t row;
    uint8_t row_span;
} ui_lab_grid_key_t;

static lv_obj_t *keyboard_add_grid(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const int32_t *columns,
    const int32_t *rows,
    uint8_t grow
)
{
    lv_obj_t *grid = create_transparent_container(explorer->keyboard_layout);
    if (grid == NULL) {
        return NULL;
    }
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_height(grid, 1);
    lv_obj_set_flex_grow(grid, grow);
    lv_obj_set_style_min_height(grid, UI_LAB_KEY_MIN_HEIGHT * grow, LV_PART_MAIN);
    lv_obj_set_grid_dsc_array(grid, columns, rows);
    lv_obj_set_style_pad_column(grid, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_row(grid, 6, LV_PART_MAIN);
    return grid;
}

static lv_obj_t *keyboard_add_grid_keys(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *grid,
    const ui_lab_grid_key_t *keys,
    size_t count
)
{
    lv_obj_t *apply = NULL;
    for (size_t index = 0U; index < count; ++index) {
        const ui_lab_grid_key_t *key = &keys[index];
        const bool disabled = key->text[0] == decimal_separator() && key->text[1] == '\0' &&
                              explorer->keyboard_rate_tenths == 0U;
        lv_obj_t *button = keyboard_add_button(explorer, grid, key->text, strlen(key->text),
                                               key->action, key->tone, key->function_key, disabled);
        if (button == NULL) {
            return NULL;
        }
        lv_obj_set_grid_cell(button, LV_GRID_ALIGN_STRETCH, key->column, key->column_span,
                             LV_GRID_ALIGN_STRETCH, key->row, key->row_span);
        if (key->action == UI_LAB_KEYBOARD_BUTTON_APPLY) {
            apply = button;
        }
    }
    return apply;
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
    const char *cancel = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL);
    const char *clear = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR);
    const char *apply = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY);
    const char *backspace = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_BACKSPACE);
    if (explorer->keyboard_mode == UI_LAB_KEYBOARD_MODE_TEXT) {
        const furnace_hmi_ui_string_id_t *rows =
            explorer->keyboard_shift != 0U ? keyboard_shift_rows : keyboard_rows;
        for (uint8_t row = 0U; row < sizeof(keyboard_rows) / sizeof(keyboard_rows[0]); ++row) {
            if (!keyboard_add_text_row(explorer, ui_string(rows[row]), row)) {
                return false;
            }
        }
        lv_obj_t *bottom = keyboard_add_grid(explorer, keyboard_bottom_columns, keyboard_single_row, 1);
        if (bottom == NULL) {
            return false;
        }
        const ui_lab_grid_key_t keys[] = {
            { cancel, UI_LAB_KEYBOARD_BUTTON_CANCEL, UI_LAB_TONE_PRIMARY, true, 0U, 3U, 0U, 1U },
            { clear, UI_LAB_KEYBOARD_BUTTON_CLEAR, UI_LAB_TONE_DANGER, true, 3U, 3U, 0U, 1U },
            { " ", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, true, 7U, 5U, 0U, 1U },
            { LV_SYMBOL_LEFT, UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT, UI_LAB_TONE_KEY, true, 12U, 2U, 0U, 1U },
            { LV_SYMBOL_RIGHT, UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT, UI_LAB_TONE_KEY, true, 14U, 2U, 0U, 1U },
            { apply, UI_LAB_KEYBOARD_BUTTON_APPLY, UI_LAB_TONE_PRIMARY, true, 17U, 3U, 0U, 1U },
        };
        explorer->keyboard_apply = keyboard_add_grid_keys(explorer, bottom, keys, sizeof(keys) / sizeof(keys[0]));
        return explorer->keyboard_apply != NULL;
    }
    /* Numeric keypad (web): Delete spans two rows, 0 spans two columns; no
     * sign key, and the decimal key only where the value takes tenths. */
    lv_obj_t *grid = keyboard_add_grid(explorer, keyboard_number_columns, keyboard_number_rows, 4);
    lv_obj_t *actions = keyboard_add_grid(explorer, keyboard_number_columns, keyboard_single_row, 1);
    if (grid == NULL || actions == NULL) {
        return false;
    }
    const ui_lab_grid_key_t keys[] = {
        { "7", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 0U, 1U, 0U, 1U },
        { "8", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 1U, 1U, 0U, 1U },
        { "9", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 2U, 1U, 0U, 1U },
        { backspace, UI_LAB_KEYBOARD_BUTTON_BACKSPACE, UI_LAB_TONE_DANGER, true, 3U, 1U, 0U, 2U },
        { "4", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 0U, 1U, 1U, 1U },
        { "5", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 1U, 1U, 1U, 1U },
        { "6", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 2U, 1U, 1U, 1U },
        { "1", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 0U, 1U, 2U, 1U },
        { "2", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 1U, 1U, 2U, 1U },
        { "3", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 2U, 1U, 2U, 1U },
        { ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_SEPARATOR), UI_LAB_KEYBOARD_BUTTON_INSERT,
          UI_LAB_TONE_KEY, false, 3U, 1U, 2U, 1U },
        { "0", UI_LAB_KEYBOARD_BUTTON_INSERT, UI_LAB_TONE_KEY, false, 0U, 2U, 3U, 1U },
        { LV_SYMBOL_LEFT, UI_LAB_KEYBOARD_BUTTON_MOVE_LEFT, UI_LAB_TONE_KEY, true, 2U, 1U, 3U, 1U },
        { LV_SYMBOL_RIGHT, UI_LAB_KEYBOARD_BUTTON_MOVE_RIGHT, UI_LAB_TONE_KEY, true, 3U, 1U, 3U, 1U },
    };
    const ui_lab_grid_key_t action_keys[] = {
        { cancel, UI_LAB_KEYBOARD_BUTTON_CANCEL, UI_LAB_TONE_PRIMARY, true, 0U, 1U, 0U, 1U },
        { clear, UI_LAB_KEYBOARD_BUTTON_CLEAR, UI_LAB_TONE_DANGER, true, 1U, 1U, 0U, 1U },
        { apply, UI_LAB_KEYBOARD_BUTTON_APPLY, UI_LAB_TONE_PRIMARY, true, 2U, 2U, 0U, 1U },
    };
    if (keyboard_add_grid_keys(explorer, grid, keys, sizeof(keys) / sizeof(keys[0])) != NULL ||
        explorer->keyboard_button_count != sizeof(keys) / sizeof(keys[0])) {
        return false;
    }
    explorer->keyboard_apply = keyboard_add_grid_keys(
        explorer, actions, action_keys, sizeof(action_keys) / sizeof(action_keys[0]));
    return explorer->keyboard_apply != NULL;
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
    ui_lab_theme_background(overlay, FURNACE_HMI_COLOR_SCRIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 12, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *panel = create_panel(overlay, false);
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
    lv_obj_set_style_pad_row(panel, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(panel);
    lv_obj_t *heading = create_transparent_container(panel);
    lv_obj_t *input = lv_textarea_create(panel);
    explorer->keyboard_input = input;
    lv_obj_t *hint = create_label(panel, "", &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    explorer->keyboard_hint = hint;
    if (heading == NULL || input == NULL || hint == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, 24);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(heading, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *heading_title = create_label(
        heading, title, &furnace_hmi_font_semibold_18, UI_LAB_TEXT);
    /* Text entry shows a live "length / limit" count; numbers name the mode. */
    lv_obj_t *heading_count = create_label(
        heading,
        mode == UI_LAB_KEYBOARD_MODE_TEXT
            ? "" : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_NUMERIC_ENTRY),
        &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    explorer->keyboard_count = heading_count;
    if (heading_title == NULL || heading_count == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_flex_grow(heading_title, 1);
    lv_obj_set_style_text_align(heading_count, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    /* Recessed well with an amber bottom edge, centred text, an amber caret
     * and an amber-tinted selection, as on the web. */
    lv_obj_set_width(input, lv_pct(100));
    lv_obj_set_height(input, 46);
    lv_obj_set_style_bg_color(input, UI_LAB_THEME_WELL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(input, LV_OPA_COVER, LV_PART_MAIN);
    ui_lab_theme_border(input, FURNACE_HMI_COLOR_HAIRLINE, LV_PART_MAIN);
    lv_obj_set_style_border_width(input, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(input, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(input, 0, LV_PART_MAIN);
    const lv_style_selector_t focused = (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_FOCUSED;
    lv_obj_set_style_outline_width(input, 0, focused);
    ui_lab_theme_border(input, FURNACE_HMI_COLOR_HAIRLINE, focused);
    lv_obj_set_style_text_color(input, UI_LAB_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(input, &furnace_hmi_font_regular_22, LV_PART_MAIN);
    lv_obj_set_style_text_align(input, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(input, 9, LV_PART_MAIN);
    lv_obj_set_style_border_color(input, UI_LAB_THEME_AMBER, LV_PART_CURSOR);
    lv_obj_set_style_border_width(input, 2, LV_PART_CURSOR);
    lv_obj_set_style_border_side(input, LV_BORDER_SIDE_LEFT, LV_PART_CURSOR);
    lv_obj_set_style_anim_duration(input, 530, LV_PART_CURSOR);
    lv_obj_t *input_label = lv_textarea_get_label(input);
    lv_obj_set_style_bg_color(input_label, ui_lab_color(FURNACE_HMI_COLOR_SELECTION_SOLID), LV_PART_SELECTED);
    lv_obj_set_style_text_color(input_label, UI_LAB_TEXT, LV_PART_SELECTED);
    ui_lab_theme_edge(input, UI_LAB_TONE_PRIMARY, UI_LAB_EDGE_BOTTOM);
    lv_textarea_set_one_line(input, true);
    lv_textarea_set_cursor_click_pos(input, false);
    lv_textarea_set_text_selection(input, false);
    lv_textarea_set_max_length(input, mode == UI_LAB_KEYBOARD_MODE_TEXT ? UI_LAB_KEYBOARD_TEXT_LIMIT : 24U);
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
    lv_obj_set_style_pad_top(explorer->keyboard_layout, 0, LV_PART_MAIN);
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
        (void)snprintf(buffer, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_FORMAT), (long)(value / 10),
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
    else if (target == UI_LAB_KEYBOARD_TARGET_STAGE_POSITION && numeric_value >= 1) {
        stage_session_move(explorer, (size_t)numeric_value - 1U);
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

static int32_t dialog_width(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const int32_t available = lv_obj_get_width(explorer->root) - 24;
    return available < UI_LAB_DIALOG_WIDTH ? available : UI_LAB_DIALOG_WIDTH;
}

/* Dialog footer buttons share the row equally, 112..220 px (web footer). */
static void size_dialog_button(lv_obj_t *button)
{
    lv_obj_set_width(button, 0);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_height(button, UI_LAB_DIALOG_BUTTON_HEIGHT);
    lv_obj_set_style_min_width(button, 112, LV_PART_MAIN);
    lv_obj_set_style_max_width(button, 220, LV_PART_MAIN);
}

/* A lone footer button is 220 px and centred, as on the web. LVGL leaves the
 * space a capped flex-grow item gives up at the end, so it gets a fixed width. */
static void centre_lone_dialog_button(lv_obj_t *actions)
{
    if (actions != NULL && lv_obj_get_child_count(actions) == 1U) {
        lv_obj_t *button = lv_obj_get_child(actions, 0);
        lv_obj_set_flex_grow(button, 0);
        lv_obj_set_width(button, 220);
    }
}

static bool open_dialog(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const char *title_text,
    const char *detail_text,
    ui_lab_action_t primary_action,
    const char *primary_text,
    ui_lab_tone_t primary_tone
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
    ui_lab_theme_background(overlay, FURNACE_HMI_COLOR_SCRIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *dialog = create_panel(overlay, true);
    if (dialog == NULL) {
        clear_modal(explorer);
        return false;
    }
    ui_lab_theme_dialog(dialog, primary_tone == UI_LAB_TONE_DANGER);
    lv_obj_set_size(dialog, dialog_width(explorer), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dialog, UI_LAB_DIALOG_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_row(dialog, UI_LAB_DIALOG_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(dialog);
    lv_obj_t *title = create_label(dialog, title_text, &furnace_hmi_font_semibold_20, UI_LAB_TEXT);
    lv_obj_t *detail = create_label(dialog, detail_text, &furnace_hmi_font_regular_14, UI_LAB_TEXT);
    lv_obj_t *actions = create_transparent_container(dialog);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
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
    lv_obj_set_height(actions, UI_LAB_DIALOG_BUTTON_HEIGHT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    const char *dismiss_text = primary_action == UI_LAB_ACTION_CLOSE_MODAL && primary_text != NULL
        ? primary_text : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL);
    lv_obj_t *cancel = create_modal_button(
        explorer,
        actions,
        dismiss_text,
        UI_LAB_TONE_PRIMARY,
        UI_LAB_ACTION_CLOSE_MODAL,
        0
    );
    if (cancel == NULL) {
        clear_modal(explorer);
        return false;
    }
    size_dialog_button(cancel);
    if (primary_action != UI_LAB_ACTION_CLOSE_MODAL && primary_text != NULL) {
        lv_obj_t *primary = create_modal_button(
            explorer,
            actions,
            primary_text,
            primary_tone,
            primary_action,
            0
        );
        if (primary == NULL) {
            clear_modal(explorer);
            return false;
        }
        size_dialog_button(primary);
    }
    centre_lone_dialog_button(actions);
    return true;
}

/* Controller alerts (web CONTROLLER_ALERTS). Close is a local dismissal only:
 * it is not an acknowledgement, and the fault stays on Running and in Device. */
enum {
    UI_LAB_ALERT_NONE = 0,
    UI_LAB_ALERT_FAULT,
    UI_LAB_ALERT_LINK_LOST,
    UI_LAB_ALERT_LINK_RESTORED,
};

static const furnace_hmi_ui_string_id_t fault_alert_titles[FURNACE_HMI_FAULT_KIND_COUNT] = {
    [FURNACE_HMI_FAULT_KIND_NONE] = FURNACE_HMI_UI_STRING_DASHBOARD_FAULT,
    [FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_OVER_TEMPERATURE_TITLE,
    [FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_TARGET_NOT_REACHED_TITLE,
    [FURNACE_HMI_FAULT_KIND_DOOR_OPEN] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_DOOR_OPEN_TITLE,
    [FURNACE_HMI_FAULT_KIND_SENSOR] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_SENSOR_TITLE,
    [FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_POWER_INTERRUPTED_TITLE,
};

static const furnace_hmi_ui_string_id_t fault_alert_details[FURNACE_HMI_FAULT_KIND_COUNT] = {
    [FURNACE_HMI_FAULT_KIND_NONE] = FURNACE_HMI_UI_STRING_DASHBOARD_FAULT,
    [FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_OVER_TEMPERATURE_DETAIL,
    [FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_TARGET_NOT_REACHED_DETAIL,
    [FURNACE_HMI_FAULT_KIND_DOOR_OPEN] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_DOOR_OPEN_DETAIL,
    [FURNACE_HMI_FAULT_KIND_SENSOR] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_SENSOR_DETAIL,
    [FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED] = FURNACE_HMI_UI_STRING_UI_LAB_ALERT_POWER_INTERRUPTED_DETAIL,
};

/* A controller-reported fault with a known kind, observed over a current link. */
static bool state_has_fault_alert(const furnace_hmi_dashboard_state_t *state)
{
    return state_is_current(state) && state->run_state == FURNACE_HMI_RUN_STATE_FAULT &&
        state->fault_kind != FURNACE_HMI_FAULT_KIND_NONE && state->fault_kind < FURNACE_HMI_FAULT_KIND_COUNT;
}

static bool open_alert(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t alert)
{
    const furnace_hmi_fault_kind_t kind = explorer->state->fault_kind < FURNACE_HMI_FAULT_KIND_COUNT
        ? explorer->state->fault_kind : FURNACE_HMI_FAULT_KIND_NONE;
    const furnace_hmi_ui_string_id_t title = alert == UI_LAB_ALERT_FAULT ? fault_alert_titles[kind]
        : alert == UI_LAB_ALERT_LINK_LOST ? FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE
        : FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_RESTORED_TITLE;
    const furnace_hmi_ui_string_id_t detail = alert == UI_LAB_ALERT_FAULT ? fault_alert_details[kind]
        : alert == UI_LAB_ALERT_LINK_LOST ? FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_DETAIL
        : FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_RESTORED_DETAIL;
    if (!open_dialog(explorer, ui_string(title), ui_string(detail), UI_LAB_ACTION_CLOSE_MODAL,
                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE),
                     alert == UI_LAB_ALERT_FAULT ? UI_LAB_TONE_DANGER : UI_LAB_TONE_PRIMARY)) {
        return false;
    }
    /* Put a tinted warning (or check) icon in front of the title. */
    lv_obj_t *dialog = lv_obj_get_child(explorer->modal, 0);
    lv_obj_t *title_label = dialog != NULL ? lv_obj_get_child(dialog, 0) : NULL;
    lv_obj_t *row = dialog != NULL ? create_transparent_container(dialog) : NULL;
    lv_obj_t *image = row != NULL ? lv_image_create(row) : NULL;
    const lv_image_dsc_t *source = furnace_hmi_icon_image(
        alert == UI_LAB_ALERT_LINK_RESTORED ? FURNACE_HMI_ICON_CHECK_CIRCLE : FURNACE_HMI_ICON_WARNING,
        UI_LAB_ICON_ONLY_SIZE);
    if (title_label == NULL || image == NULL || source == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, LV_PART_MAIN);
    lv_obj_move_to_index(row, 0);
    lv_image_set_src(image, source);
    lv_obj_set_style_image_recolor(image, alert == UI_LAB_ALERT_FAULT ? UI_LAB_THEME_RED
        : alert == UI_LAB_ALERT_LINK_LOST ? UI_LAB_THEME_AMBER : UI_LAB_THEME_OK, LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_parent(title_label, row);
    explorer->alert_open = alert;
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
static void furnace_quad(lv_layer_t *layer, lv_point_precise_t a, lv_point_precise_t b, lv_point_precise_t c, lv_point_precise_t d, furnace_hmi_color_role_t role)
{
    lv_draw_triangle_dsc_t triangle; lv_draw_triangle_dsc_init(&triangle);
    triangle.color = ui_lab_color(role); triangle.opa = ui_lab_opa(role);
    triangle.p[0] = a; triangle.p[1] = b; triangle.p[2] = c; lv_draw_triangle(layer, &triangle);
    triangle.p[1] = c; triangle.p[2] = d; lv_draw_triangle(layer, &triangle);
}

/* The web furnace (furnaceIllustration): a 230 x 220 drawing scaled to fit
 * the canvas and centred, like an SVG viewBox. Coordinates below are in that
 * drawing's units. */
#define UI_LAB_FURNACE_VIEW_WIDTH 230
#define UI_LAB_FURNACE_VIEW_HEIGHT 220

typedef struct {
    int32_t x;
    int32_t y;
    int32_t scale_x1000;
} ui_lab_furnace_view_t;

static lv_point_precise_t furnace_point(const ui_lab_furnace_view_t *view, int32_t x, int32_t y)
{
    return (lv_point_precise_t){ view->x + x * view->scale_x1000 / 1000,
                                 view->y + y * view->scale_x1000 / 1000 };
}

static lv_area_t furnace_area(const ui_lab_furnace_view_t *view, int32_t x, int32_t y,
                              int32_t width, int32_t height)
{
    const lv_point_precise_t start = furnace_point(view, x, y);
    const lv_point_precise_t end = furnace_point(view, x + width, y + height);
    return (lv_area_t){ (int32_t)start.x, (int32_t)start.y, (int32_t)end.x - 1, (int32_t)end.y - 1 };
}

static void furnace_draw(lv_event_t *event)
{
    lv_obj_t *canvas = lv_event_get_target_obj(event);
    furnace_hmi_ui_lab_explorer_t *explorer = lv_event_get_user_data(event);
    lv_area_t bounds; lv_obj_get_coords(canvas, &bounds);
    const int32_t width = lv_area_get_width(&bounds), height = lv_area_get_height(&bounds);
    if (height < 32 || width < 48) { return; }
    const int32_t scale_x = width * 1000 / UI_LAB_FURNACE_VIEW_WIDTH;
    const int32_t scale_y = height * 1000 / UI_LAB_FURNACE_VIEW_HEIGHT;
    ui_lab_furnace_view_t view = { 0, 0, scale_x < scale_y ? scale_x : scale_y };
    view.x = bounds.x1 + (width - UI_LAB_FURNACE_VIEW_WIDTH * view.scale_x1000 / 1000) / 2;
    view.y = bounds.y1 + (height - UI_LAB_FURNACE_VIEW_HEIGHT * view.scale_x1000 / 1000) / 2;
    lv_layer_t *layer = lv_event_get_layer(event);
    /* Top and side faces. */
    furnace_quad(layer, furnace_point(&view, 35, 35), furnace_point(&view, 62, 15),
                 furnace_point(&view, 200, 15), furnace_point(&view, 175, 35), FURNACE_HMI_COLOR_FURNACE_TOP);
    furnace_quad(layer, furnace_point(&view, 175, 35), furnace_point(&view, 200, 15),
                 furnace_point(&view, 200, 190), furnace_point(&view, 175, 211), FURNACE_HMI_COLOR_FURNACE_SIDE);
    lv_draw_rect_dsc_t rect; lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_COVER;
    /* Front, door, window and handle. */
    rect.bg_color = ui_lab_color(FURNACE_HMI_COLOR_FURNACE_FRONT);
    rect.border_color = ui_lab_color(FURNACE_HMI_COLOR_FURNACE_OUTLINE); rect.border_width = 1;
    lv_area_t area = furnace_area(&view, 35, 35, 140, 176); lv_draw_rect(layer, &rect, &area);
    rect.bg_color = ui_lab_color(FURNACE_HMI_COLOR_FURNACE_DOOR);
    rect.border_color = ui_lab_color(FURNACE_HMI_COLOR_GRAPH_TEXT); rect.radius = 4;
    area = furnace_area(&view, 51, 52, 108, 144); lv_draw_rect(layer, &rect, &area);
    rect.bg_color = ui_lab_color(FURNACE_HMI_COLOR_FURNACE_WINDOW); rect.border_width = 0; rect.radius = 3;
    area = furnace_area(&view, 65, 69, 80, 92); lv_draw_rect(layer, &rect, &area);
    rect.bg_color = ui_lab_color(FURNACE_HMI_COLOR_GRAPH_TEXT); rect.radius = 0;
    area = furnace_area(&view, 153, 141, 4, 28); lv_draw_rect(layer, &rect, &area);
    /* Heating elements: amber while heating, grey otherwise. */
    const bool heating = state_is_current(explorer->state) &&
        explorer->state->heater_demand_percent.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
        explorer->state->heater_demand_percent.value > 0;
    lv_draw_line_dsc_t line; lv_draw_line_dsc_init(&line);
    line.width = 2;
    line.color = ui_lab_color(heating ? FURNACE_HMI_COLOR_HEATER_ON : FURNACE_HMI_COLOR_HEATER_OFF);
    for (int32_t y = 89; y <= 137; y += 24) {
        line.p1 = furnace_point(&view, 73, y);
        line.p2 = furnace_point(&view, 137, y);
        lv_draw_line(layer, &line);
    }
}

static bool create_furnace_visual(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *parent, bool expansive)
{
    (void)expansive;
    lv_obj_t *visual = create_panel(parent, false);
    if (visual == NULL) { return false; }
    lv_obj_set_width(visual, lv_pct(100)); lv_obj_set_height(visual, 1);
    const bool short_panel = lv_obj_get_height(explorer->root) <= 480 && explorer->page != FURNACE_HMI_UI_LAB_PAGE_DEVICE;
    lv_obj_set_flex_grow(visual, 1); lv_obj_set_flex_flow(visual, short_panel ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
    if (!short_panel && create_label(visual, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FURNACE_VISUAL), &furnace_hmi_font_regular_14, UI_LAB_MUTED) == NULL) { return false; }
    lv_obj_t *canvas = create_transparent_container(visual);
    if (canvas == NULL) { return false; }
    lv_obj_set_width(canvas, short_panel ? lv_pct(58) : lv_pct(100));
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
        lv_obj_t *label = create_label(row, labels[i], &furnace_hmi_font_regular_13, UI_LAB_MUTED);
        lv_obj_t *value = create_label(row, values[i], &furnace_hmi_font_regular_19, UI_LAB_TEXT);
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

static uint32_t manual_cooling_minutes(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const int32_t current = manual_current_temperature_c(explorer);
    if (current <= 30) {
        return 0U;
    }
    const int32_t rate_tenths = current * 10 / 180 > 4 ? current * 10 / 180 : 4;
    return (uint32_t)(((current - 30) * 10 + rate_tenths - 1) / rate_tenths);
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

static void format_running_time_value(char *text, size_t capacity,
                                      const furnace_hmi_i32_value_t *value);
static void format_power_value(char *text, size_t capacity, const furnace_hmi_i32_value_t *power);

/* Manual forecast (web manualPredictionGraph): before Start, from the current
 * temperature to the target at the chosen rate. While running, the measured
 * readings so far, then the prediction from "now", holding at the target for a
 * short while. The full permitted temperature range stays visible. */
static void prepare_manual_prediction(furnace_hmi_ui_lab_explorer_t *explorer)
{
    furnace_hmi_dashboard_state_t *preview = &explorer->preview_graph_state;
    furnace_hmi_dashboard_state_initialize(preview);
    preview->availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    const int32_t current = manual_current_temperature_c(explorer);
    const bool cooling = explorer->manual_cooling;
    const bool active = explorer->manual_running || cooling;
    const uint32_t minutes = cooling ? manual_cooling_minutes(explorer) : manual_prediction_minutes(explorer);
    const int32_t goal = cooling ? 30 : explorer->manual_target_c;
    const furnace_hmi_dashboard_state_t *state = explorer->state;
    uint32_t now = 0U;
    size_t count = 0U;
    if (active && state != NULL) {
        if (state->elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            state->elapsed_seconds.value > 0) {
            now = (uint32_t)state->elapsed_seconds.value;
        }
        for (size_t index = 0U; index < state->graph_sample_count &&
                                count + 3U < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++index) {
            const furnace_hmi_graph_sample_t *sample = &state->graph_samples[index];
            if (sample->measured_validity != FURNACE_HMI_VALUE_VALIDITY_CURRENT ||
                sample->elapsed_seconds >= now) {
                continue;
            }
            preview->graph_samples[count] = *sample;
            preview->graph_samples[count].planned_temperature_c = sample->measured_temperature_c;
            preview->graph_samples[count].planned_cooling = false;
            ++count;
        }
    }
    now += cooling ? explorer->manual_cooling_seconds : 0U;
    const uint32_t reach = now + minutes * 60U;
    uint32_t end = reach < 60U ? 60U : reach;
    if (active) {
        uint32_t margin = (reach * 12U + 50U) / 100U;
        if (margin < 300U) {
            margin = 300U;
        }
        end = reach + margin < 600U ? 600U : reach + margin;
    }
    explorer->manual_marker_index = (int32_t)count;
    furnace_hmi_graph_sample_t *sample = &preview->graph_samples[count++];
    sample->elapsed_seconds = now;
    sample->planned_temperature_c = current;
    sample->measured_temperature_c = current;
    sample->measured_validity = active
        ? FURNACE_HMI_VALUE_VALIDITY_CURRENT : FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    if (reach > now) {
        sample = &preview->graph_samples[count++];
        sample->elapsed_seconds = reach;
        sample->planned_temperature_c = goal;
        sample->planned_cooling = cooling;
    }
    if (end > reach) {
        sample = &preview->graph_samples[count++];
        sample->elapsed_seconds = end;
        sample->planned_temperature_c = goal;
        sample->planned_cooling = cooling;
    }
    preview->graph_sample_count = count;
    explorer->trajectory_uses_program_preview = true;
    explorer->trajectory_domain_minimum_temperature_c = 0;
    explorer->trajectory_domain_maximum_temperature_c = UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
    explorer->trajectory_domain_maximum_seconds = end;
    explorer->trajectory_domain_initialized = true;
}

static void add_manual_reading(lv_obj_t *strip, furnace_hmi_ui_string_id_t name, const char *value)
{
    lv_obj_t *pair = create_transparent_container(strip);
    if (pair == NULL) {
        return;
    }
    /* Label over value in equal columns (web .manual-readings .run-reading), so six
     * readings and the state fit 800 px. */
    lv_obj_set_size(pair, 0, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(pair, 1);
    lv_obj_set_flex_flow(pair, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pair, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(pair, 1, LV_PART_MAIN);
    lv_obj_t *labels[] = {
        create_label(pair, ui_string(name), &furnace_hmi_font_regular_13, UI_LAB_MUTED),
        create_label(pair, value, &furnace_hmi_font_semibold_15, UI_LAB_TEXT),
    };
    for (size_t index = 0U; index < 2U; ++index) {
        if (labels[index] != NULL) {
            lv_obj_set_width(labels[index], lv_pct(100));
            lv_label_set_long_mode(labels[index], LV_LABEL_LONG_DOT);
        }
    }
}

/* Manual readings strip (web manualReadings): the state, then readings. Before
 * Start: Current and Est. to target, and a note. Running: Elapsed, To target,
 * Current, Setpoint, Heater demand and Est. power. The controller setpoint is
 * not in the observation model yet, so it shows as not provided rather than a
 * made-up value. */
static void refresh_manual_readings(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *strip = explorer->manual_readings;
    if (strip == NULL) {
        return;
    }
    lv_obj_clean(strip);
    const bool running = explorer->manual_running;
    const bool paused = explorer->manual_paused;
    const uint32_t minutes = manual_prediction_minutes(explorer);
    char to_target[24];
    if (minutes == 0U) {
        copy_text_bounded(to_target, sizeof(to_target), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_AT_TARGET));
    }
    else {
        (void)snprintf(to_target, sizeof(to_target), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HOURS_MINUTES_FORMAT),
                       (long)(minutes / 60U), (long)(minutes % 60U));
    }
    const bool cooling = explorer->manual_cooling;
    const uint32_t cooling_minutes = cooling ? manual_cooling_minutes(explorer) : 0U;
    const furnace_hmi_ui_string_id_t state_id = cooling
        ? (cooling_minutes > 0U ? FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_COOLING : FURNACE_HMI_UI_STRING_UI_LAB_COOLING_COMPLETE)
        : running
        ? (paused ? FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_PAUSED
                  : minutes > 0U ? FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_HEATING
                                 : FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_HOLDING)
        : FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_READY;
    lv_obj_t *state_label = create_label(strip, ui_string(state_id), &furnace_hmi_font_semibold_15,
                                         cooling ? ui_lab_color(FURNACE_HMI_COLOR_BLUE_TEXT)
                                         : running && !paused ? UI_LAB_THEME_AMBER_TEXT : UI_LAB_MUTED);
    if (state_label != NULL) {
        lv_obj_set_style_min_width(state_label, 76, LV_PART_MAIN);
    }
    const furnace_hmi_dashboard_state_t *state = explorer->state;
    char value[32];
    if (cooling) {
        /* Heating time, cooling time and its estimate, the reading and the cooling target. */
        format_running_time_value(value, sizeof(value), &state->elapsed_seconds);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_ELAPSED, value);
        const furnace_hmi_i32_value_t cooled = {
            (int32_t)explorer->manual_cooling_seconds, FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT };
        format_running_time_value(value, sizeof(value), &cooled);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_COOLING, value);
        const furnace_hmi_i32_value_t remaining = {
            (int32_t)cooling_minutes * 60, FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED, FURNACE_HMI_VALUE_VALIDITY_CURRENT };
        format_running_time_value(value, sizeof(value), &remaining);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_EST_REMAINING, value);
        format_observation_value(value, sizeof(value), &state->current_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT, value);
        (void)snprintf(value, sizeof(value), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER), 30L);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_TARGET, value);
    }
    else if (running) {
        format_running_time_value(value, sizeof(value), &state->elapsed_seconds);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_ELAPSED, value);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_TO_TARGET, to_target);
        format_observation_value(value, sizeof(value), &state->current_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT, value);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_SETPOINT,
                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_NOT_PROVIDED));
        format_observation_value(value, sizeof(value), &state->heater_demand_percent,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_PERCENT);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_HEATER_DEMAND, value);
        format_power_value(value, sizeof(value), &state->estimated_power_kw_x100);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_ESTIMATED_POWER, value);
    }
    else {
        format_observation_value(value, sizeof(value), &state->current_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT, value);
        add_manual_reading(strip, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_EST_TO_TARGET, to_target);
        lv_obj_t *note = create_label(strip, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_IDLE_NOTE),
                                      &furnace_hmi_font_regular_13, UI_LAB_MUTED);
        if (note != NULL) {
            lv_obj_set_width(note, 0);
            lv_obj_set_flex_grow(note, 1);
            lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
            lv_label_set_long_mode(note, LV_LABEL_LONG_DOT);
        }
    }
}

/* One-piece Manual stepper (web .manual-control): minus, a value well with a
 * small caption above the value, plus. */
static bool make_manual_control(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    furnace_hmi_ui_string_id_t caption,
    const char *value_text,
    ui_lab_action_t decrement_action,
    ui_lab_action_t edit_action,
    ui_lab_action_t increment_action,
    lv_obj_t **value_out
)
{
    lv_obj_t *tile = lv_obj_create(parent);
    if (tile == NULL) {
        return false;
    }
    ui_lab_theme_info_tile(tile);
    ui_lab_theme_edge(tile, UI_LAB_TONE_TILE, UI_LAB_EDGE_TOP);
    remove_scroll(tile);
    lv_obj_set_width(tile, 0);
    lv_obj_set_flex_grow(tile, 1);
    lv_obj_set_height(tile, 58);
    lv_obj_set_style_pad_all(tile, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(tile, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_t *decrement = create_page_button(explorer, tile, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE),
                                             UI_LAB_TONE_FLAT, decrement_action, 0);
    lv_obj_t *value = create_page_button(explorer, tile, value_text, UI_LAB_TONE_WELL, edit_action, 0);
    lv_obj_t *increment = create_page_button(explorer, tile, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE),
                                             UI_LAB_TONE_FLAT, increment_action, 0);
    if (decrement == NULL || value == NULL || increment == NULL) {
        return false;
    }
    lv_obj_set_size(decrement, 48, 56);
    lv_obj_set_size(increment, 48, 56);
    lv_obj_set_width(value, 0);
    lv_obj_set_flex_grow(value, 1);
    lv_obj_set_height(value, 54);
    lv_obj_set_style_min_height(value, 54, LV_PART_MAIN);
    lv_obj_set_style_pad_all(value, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(value, (lv_border_side_t)(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT),
                                 LV_PART_MAIN);
    ui_lab_theme_border(value, FURNACE_HMI_COLOR_HAIRLINE_STRONG, LV_PART_MAIN);
    lv_obj_t *value_label = lv_obj_get_child(value, 0);
    if (value_label != NULL) {
        lv_obj_set_style_text_font(value_label, &furnace_hmi_font_regular_18, LV_PART_MAIN);
        lv_obj_align(value_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    }
    /* The caption sits in its own box so the tile's recolouring of its direct
     * labels (pressed, disabled) leaves it muted. */
    lv_obj_t *caption_box = create_transparent_container(value);
    lv_obj_t *caption_label = caption_box == NULL ? NULL :
        create_label(caption_box, ui_string(caption), &furnace_hmi_font_regular_12, UI_LAB_MUTED);
    if (caption_label == NULL) {
        return false;
    }
    lv_obj_remove_flag(caption_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(caption_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(caption_box, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_text_letter_space(caption_label, 0, LV_PART_MAIN);
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
    if (explorer == NULL || explorer->trajectory == NULL || explorer->manual_readings == NULL) {
        return false;
    }
    char target[32];
    char rate[32];
    (void)snprintf(target, sizeof(target), "%ld", (long)explorer->manual_target_c);
    (void)snprintf(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_FORMAT),
                   (long)(explorer->manual_rate_tenths_c_per_minute / 10),
                   (long)(explorer->manual_rate_tenths_c_per_minute % 10));
    set_button_label(explorer->manual_target_value, target);
    set_button_label(explorer->manual_rate_value, rate);
    refresh_manual_readings(explorer);
    prepare_manual_prediction(explorer);
    return update_trajectory(explorer);
}

/* Disable (or enable) every button below `parent`. */
static void set_subtree_disabled(lv_obj_t *parent, bool disabled)
{
    const uint32_t count = lv_obj_get_child_count(parent);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *child = lv_obj_get_child(parent, (int32_t)index);
        if (lv_obj_check_type(child, &lv_button_class)) {
            if (disabled) { lv_obj_add_state(child, LV_STATE_DISABLED); }
            else { lv_obj_remove_state(child, LV_STATE_DISABLED); }
        }
        set_subtree_disabled(child, disabled);
    }
}

static bool update_manual_run_state(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->manual_readings == NULL || explorer->manual_start_button == NULL ||
        explorer->manual_pause_button == NULL || explorer->manual_stop_button == NULL) {
        return false;
    }
    const bool running = explorer->manual_running;
    const bool cooling = explorer->manual_cooling;
    lv_obj_t *const cooling_buttons[] = { explorer->manual_exit_button, explorer->manual_new_button };
    for (size_t index = 0U; index < 2U; ++index) {
        if (cooling_buttons[index] != NULL) {
            if (cooling) { lv_obj_remove_flag(cooling_buttons[index], LV_OBJ_FLAG_HIDDEN); }
            else { lv_obj_add_flag(cooling_buttons[index], LV_OBJ_FLAG_HIDDEN); }
        }
    }
    if (explorer->manual_steppers != NULL) {
        set_subtree_disabled(explorer->manual_steppers, cooling);
    }
    if (explorer->header_title != NULL) {
        lv_label_set_text(explorer->header_title,
                          ui_string(cooling ? FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_COOLING_TITLE
                                    : running ? FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_RUNNING
                                              : FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_MODE));
    }
    if (cooling) {
        lv_obj_add_flag(explorer->manual_start_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(explorer->manual_pause_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(explorer->manual_stop_button, LV_OBJ_FLAG_HIDDEN);
    }
    else if (running) {
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
    if (explorer->trajectory_legend_label != NULL) {
        lv_label_set_text(explorer->trajectory_legend_label, trajectory_legend_text(explorer));
    }
    if (explorer->nav != NULL) {
        if (running || cooling) {
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

/* Manual (web renderManual): readings strip, the forecast filling the page,
 * then a controls tile with Start (or Pause/Stop) beside the two steppers. */
static bool build_manual(furnace_hmi_ui_lab_explorer_t *explorer)
{
    char target[32];
    char rate[32];
    (void)snprintf(target, sizeof(target), "%ld", (long)explorer->manual_target_c);
    (void)snprintf(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_FORMAT),
                   (long)(explorer->manual_rate_tenths_c_per_minute / 10),
                   (long)(explorer->manual_rate_tenths_c_per_minute % 10));
    prepare_manual_prediction(explorer);

    lv_obj_t *readings = lv_obj_create(explorer->content);
    lv_obj_t *layout = create_transparent_container(explorer->content);
    if (readings == NULL || layout == NULL) {
        return false;
    }
    explorer->manual_readings = readings;
    ui_lab_theme_info_tile(readings);
    remove_scroll(readings);
    lv_obj_set_size(readings, lv_pct(100), 50);
    /* Tighter than the Running line: the unit labels (Current °C, Setpoint °C)
     * must still fit six readings at 800 px (web .manual-readings). */
    lv_obj_set_style_pad_hor(readings, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(readings, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(readings, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(readings, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(readings, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_width(layout, lv_pct(100));
    lv_obj_set_height(layout, 1);
    lv_obj_set_flex_grow(layout, 1);
    lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(layout, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *graph = create_transparent_container(layout);
    lv_obj_t *controls = create_panel(layout, false);
    if (graph == NULL || controls == NULL) {
        return false;
    }
    lv_obj_set_width(graph, lv_pct(100));
    lv_obj_set_height(graph, 1);
    lv_obj_set_flex_grow(graph, 1);
    lv_obj_set_flex_flow(graph, LV_FLEX_FLOW_COLUMN);
    if (!make_trajectory(explorer, graph, 0)) {
        return false;
    }
    lv_obj_set_size(controls, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(controls, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_column(controls, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);

    lv_obj_t *actions = create_transparent_container(controls);
    lv_obj_t *steppers = create_transparent_container(controls);
    if (actions == NULL || steppers == NULL) {
        return false;
    }
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    lv_obj_set_width(actions, 0);
    lv_obj_set_flex_grow(actions, 1);
    lv_obj_set_height(actions, 58);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_width(steppers, 0);
    lv_obj_set_flex_grow(steppers, 1);
    lv_obj_set_height(steppers, 58);
    lv_obj_set_flex_flow(steppers, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(steppers, UI_LAB_SPACE, LV_PART_MAIN);
    explorer->manual_start_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START),
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_MANUAL_START, 0);
    explorer->manual_pause_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW),
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_MANUAL_PAUSE, 0);
    explorer->manual_stop_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW),
        UI_LAB_TONE_DANGER, UI_LAB_ACTION_MANUAL_STOP, 0);
    explorer->manual_exit_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_EXIT),
        UI_LAB_TONE_TILE, UI_LAB_ACTION_MANUAL_EXIT, 0);
    explorer->manual_new_button = create_page_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_START_NEW),
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_MANUAL_NEW, 0);
    explorer->manual_steppers = steppers;
    if (explorer->manual_start_button == NULL || explorer->manual_pause_button == NULL ||
        explorer->manual_stop_button == NULL || explorer->manual_exit_button == NULL ||
        explorer->manual_new_button == NULL) {
        return false;
    }
    lv_obj_t *const buttons[] = {
        explorer->manual_start_button, explorer->manual_pause_button, explorer->manual_stop_button,
        explorer->manual_exit_button, explorer->manual_new_button,
    };
    for (size_t index = 0U; index < 5U; ++index) {
        lv_obj_set_flex_grow(buttons[index], 1);
        lv_obj_set_height(buttons[index], 56);
    }
    if (!make_manual_control(explorer, steppers, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_CAPTION_TARGET, target,
                             UI_LAB_ACTION_MANUAL_DECREMENT_TARGET,
                             UI_LAB_ACTION_MANUAL_EDIT_TARGET,
                             UI_LAB_ACTION_MANUAL_INCREMENT_TARGET,
                             &explorer->manual_target_value) ||
        !make_manual_control(explorer, steppers, FURNACE_HMI_UI_STRING_UI_LAB_MANUAL_CAPTION_RATE, rate,
                             UI_LAB_ACTION_MANUAL_DECREMENT_RATE,
                             UI_LAB_ACTION_MANUAL_EDIT_RATE,
                             UI_LAB_ACTION_MANUAL_INCREMENT_RATE,
                             &explorer->manual_rate_value)) {
        return false;
    }
    return update_manual_run_state(explorer);
}

/* Show as many programs as Quick launch can hold: favourites first, then
 * recents, dropping from the end together with an emptied heading (web
 * fitQuickLaunch). */
static void fit_quick_launch(lv_obj_t *quick_launch, lv_obj_t *spacer, lv_obj_t *choose)
{
    lv_area_t panel_area;
    lv_area_t choose_area;
    for (;;) {
        lv_obj_update_layout(quick_launch);
        lv_obj_get_content_coords(quick_launch, &panel_area);
        lv_obj_get_coords(choose, &choose_area);
        if (choose_area.y2 <= panel_area.y2) {
            break;
        }
        const int32_t spacer_index = lv_obj_get_index(spacer);
        lv_obj_t *last = spacer_index > 0 ? lv_obj_get_child(quick_launch, spacer_index - 1) : NULL;
        if (last == NULL || !lv_obj_check_type(last, &lv_button_class)) {
            break;
        }
        lv_obj_delete(last);
        lv_obj_t *heading = lv_obj_get_child(quick_launch, lv_obj_get_index(spacer) - 1);
        if (heading != NULL && lv_obj_check_type(heading, &lv_label_class) &&
            lv_obj_get_index(heading) > 0) {
            lv_obj_delete(heading);
        }
    }
}

static bool build_home(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (!state_is_current(explorer->state)) {
        lv_obj_t *panel = create_panel(explorer->content, false);
        if (panel == NULL) {
            return false;
        }
        lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_t *heading = create_label(panel, state_text(explorer->state), &furnace_hmi_font_regular_20,
                                         state_color(explorer->state));
        lv_obj_t *detail = create_label(panel,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NO_CURRENT_SNAPSHOT),
                                        &furnace_hmi_font_regular_16, UI_LAB_MUTED);
        if (heading == NULL || detail == NULL) {
            return false;
        }
        lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(detail, lv_pct(75));
        return true;
    }

    lv_obj_t *columns = create_transparent_container(explorer->content);
    lv_obj_t *quick_launch = create_panel(columns, false);
    mark_edge_row(quick_launch, UI_LAB_EDGE_RIGHT);
    lv_obj_t *visual_column = create_transparent_container(columns);
    if (columns == NULL || visual_column == NULL || quick_launch == NULL) {
        return false;
    }
    lv_obj_t *readings = create_panel(visual_column, true);
    if (readings == NULL) {
        return false;
    }
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_flex_flow(columns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(columns, 8, LV_PART_MAIN);
    /* Match the quick-launch rail to one of the four equal-width bottom-nav
     * buttons. */
    lv_obj_set_width(quick_launch, nav_column_width(explorer));
    lv_obj_set_height(quick_launch, lv_pct(100));
    lv_obj_set_flex_flow(quick_launch, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(quick_launch, 6, LV_PART_MAIN);
    lv_obj_set_width(visual_column, 0);
    lv_obj_set_flex_grow(visual_column, 1);
    lv_obj_set_height(visual_column, lv_pct(100));
    lv_obj_set_flex_flow(visual_column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_width(readings, lv_pct(100));
    lv_obj_set_height(readings, 90);
    lv_obj_set_flex_flow(readings, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_ver(readings, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_column(readings, UI_LAB_SPACE, LV_PART_MAIN);
    char current[32];
    char target[32];
    char remaining[32];
    format_observation_value(current, sizeof(current), &explorer->state->current_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    format_observation_value(target, sizeof(target), &explorer->state->target_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    format_remaining(remaining, sizeof(remaining), &explorer->state->remaining_seconds);
    if (!make_primary_metric(readings,
                             ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HOME_CURRENT_TEMPERATURE), current, &explorer->home_temperature) ||
        !make_metric(readings, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_READING_TARGET), target, &explorer->home_target) ||
        !make_metric(readings, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_REMAINING), remaining, &explorer->home_remaining)) {
        return false;
    }
    lv_obj_t *title = create_label(quick_launch,
                                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_QUICK_LAUNCH),
                                   &furnace_hmi_font_semibold_20, UI_LAB_TEXT);
    if (title == NULL) { return false; }
    if (explorer->favourite_count > 0U &&
        create_label(quick_launch, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FAVOURITES),
                     &furnace_hmi_font_regular_13, UI_LAB_MUTED) == NULL) { return false; }
    for (size_t index = 0; index < explorer->favourite_count; ++index) {
        const uint8_t program_index = explorer->quick_launch_favourites[index];
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                program_name_text(explorer, program_index),
                                                UI_LAB_TONE_TILE,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        style_quick_item(program);
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 48);
    }
    bool has_recent = false;
    for (size_t index = 0; index < explorer->recent_count; ++index) {
        const uint8_t program_index = explorer->quick_launch_recent[index];
        if (program_is_favourite(explorer, program_index)) { continue; }
        if (!has_recent && create_label(quick_launch,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RECENT_PROGRAMS),
                                        &furnace_hmi_font_regular_13, UI_LAB_MUTED) == NULL) { return false; }
        has_recent = true;
        lv_obj_t *program = create_page_button(explorer, quick_launch,
                                                program_name_text(explorer, program_index),
                                                UI_LAB_TONE_TILE,
                                                UI_LAB_ACTION_PROGRAM_LOADING, (int)program_index);
        if (program == NULL) {
            return false;
        }
        style_quick_item(program);
        lv_obj_set_width(program, lv_pct(100));
        lv_obj_set_height(program, 48);
    }
    if (explorer->favourite_count == 0U && !has_recent &&
        create_label(quick_launch, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_FAVOURITE_PROGRAM_PROMPT), &furnace_hmi_font_regular_14, UI_LAB_MUTED) == NULL) { return false; }
    lv_obj_t *spacer = create_transparent_container(quick_launch);
    if (spacer == NULL) {
        return false;
    }
    lv_obj_set_size(spacer, 0, 0);
    lv_obj_set_style_min_height(spacer, 8, LV_PART_MAIN);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_t *choose = create_page_button(explorer, quick_launch,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_MANUAL),
                                           UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_MANUAL, 0);
    if (choose == NULL) {
        return false;
    }
    lv_obj_set_width(choose, lv_pct(100));
    lv_obj_set_height(choose, 48);
    if (!create_furnace_visual(explorer, visual_column, true)) {
        return false;
    }
    fit_quick_launch(quick_launch, spacer, choose);
    return true;
}

/* Three cards per row and three rows fill a card list, as in the web grid.
 * Call once the page is complete so the list has its final height. */
static void size_grid_cards(lv_obj_t *list, uint32_t columns)
{
    lv_obj_update_layout(list);
    const int32_t width = columns > 1U
        ? (lv_obj_get_content_width(list) - (int32_t)(columns - 1U) * UI_LAB_SPACE) / (int32_t)columns
        : lv_obj_get_content_width(list);
    const int32_t height = (lv_obj_get_content_height(list) - 2 * UI_LAB_SPACE) / 3;
    const uint32_t count = lv_obj_get_child_count(list);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *card = lv_obj_get_child(list, (int32_t)index);
        if (!lv_obj_check_type(card, &lv_label_class)) {
            lv_obj_set_size(card, width, height);
        }
    }
}

static bool make_program_card(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const furnace_hmi_ui_lab_program_t *program,
    int program_index
)
{
    lv_obj_t *card = create_page_button(explorer, parent, "", UI_LAB_TONE_TILE,
                                        UI_LAB_ACTION_PROGRAM_DETAIL, program_index);
    if (card == NULL) {
        return false;
    }
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_ver(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 8, LV_PART_MAIN);
    lv_obj_t *name = create_label(card, program_name_text(explorer, (size_t)program_index), &furnace_hmi_font_regular_20, UI_LAB_TEXT);
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
        lv_obj_t *value = create_label(summary, summary_values[index], &furnace_hmi_font_regular_14, UI_LAB_MUTED);
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
    const size_t program_count = listed_program_count(explorer);
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
                                             UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_NEW_DRAFT, 0);
    lv_obj_t *spacer = create_transparent_container(toolbar);
    lv_obj_t *pager = create_pager(explorer, toolbar, explorer->program_page, page_count,
                                   UI_LAB_ACTION_PREVIOUS_PROGRAM_PAGE,
                                   UI_LAB_ACTION_NEXT_PROGRAM_PAGE, UI_LAB_EDGE_BOTTOM);
    if (new_draft == NULL || spacer == NULL || pager == NULL) {
        return false;
    }
    lv_obj_set_width(new_draft, 162);
    lv_obj_set_size(spacer, 0, 1);
    lv_obj_set_flex_grow(spacer, 1);
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
    if (program_count == 0 && create_label(columns[0], ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EMPTY_LIBRARY), &furnace_hmi_font_regular_16, UI_LAB_MUTED) == NULL) { return false; }
    for (size_t position = start; position < end; ++position) {
        const size_t index = listed_program_at(explorer, position);
        if (index >= FURNACE_HMI_UI_LAB_MAX_PROGRAMS ||
            !make_program_card(explorer, columns[(position - start) % 2U],
                               &explorer->programs[index], (int)index)) {
            return false;
        }
    }
    /* Two columns of three cards fill the page (web 2 x 3 grid). */
    if (program_count > 0) {
        size_grid_cards(columns[0], 1U);
        size_grid_cards(columns[1], 1U);
    }
    return true;
}

static bool make_stage_card(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list,
                            size_t index, bool editable);

/* One headline fact: 13 px caption over a 24 px value. */
static bool make_program_fact(lv_obj_t *parent, const char *caption, const char *value)
{
    lv_obj_t *fact = create_transparent_container(parent);
    if (fact == NULL) {
        return false;
    }
    lv_obj_set_width(fact, 0);
    lv_obj_set_flex_grow(fact, 1);
    lv_obj_set_height(fact, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(fact, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fact, 0, LV_PART_MAIN);
    return create_label(fact, caption, &furnace_hmi_font_regular_13, UI_LAB_MUTED) != NULL &&
           create_label(fact, value, &furnace_hmi_font_regular_24, UI_LAB_TEXT) != NULL;
}

/* One Duration line: 13 px caption on the left, 18 px value on the right. */
static bool make_program_duration_row(lv_obj_t *parent, const char *caption, const char *value)
{
    lv_obj_t *row = create_transparent_container(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return create_label(row, caption, &furnace_hmi_font_regular_13, UI_LAB_MUTED) != NULL &&
           create_label(row, value, &furnace_hmi_font_regular_18, UI_LAB_TEXT) != NULL;
}

/* Program facts tile (web appendProgramFacts): Stages and Max. temp, then
 * Duration with Curing, Est. cooling and Est. total. */
static lv_obj_t *make_program_facts(lv_obj_t *parent, const furnace_hmi_ui_lab_program_t *program)
{
    lv_obj_t *facts = lv_obj_create(parent);
    if (facts == NULL) {
        return NULL;
    }
    ui_lab_theme_info_tile(facts);
    remove_scroll(facts);
    lv_obj_set_style_pad_hor(facts, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(facts, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(facts, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(facts, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *top_row = create_transparent_container(facts);
    lv_obj_t *duration = create_transparent_container(facts);
    if (top_row == NULL || duration == NULL) {
        return NULL;
    }
    /* Web .program-fact-row: 79 px, values centred in it. */
    lv_obj_set_size(top_row, lv_pct(100), 79);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(top_row, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_width(duration, lv_pct(100));
    lv_obj_set_height(duration, 0);
    lv_obj_set_flex_grow(duration, 1);
    lv_obj_set_flex_flow(duration, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(duration, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_top(duration, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_border_width(duration, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(duration, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    ui_lab_theme_border(duration, FURNACE_HMI_COLOR_HAIRLINE_STRONG, LV_PART_MAIN);
    char count[16];
    (void)snprintf(count, sizeof(count), "%u", (unsigned int)program_authored_stage_count(program));
    if (!make_program_fact(top_row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGES), count) ||
        !make_program_fact(top_row, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MAX_TEMPERATURE),
                           program_maximum_number(program)) ||
        create_label(duration, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION),
                     &furnace_hmi_font_regular_16, UI_LAB_TEXT) == NULL ||
        !make_program_duration_row(duration, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION_CURING),
                                   program_duration(program)) ||
        !make_program_duration_row(duration, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION_EST_COOLING),
                                   program_cooling(program)) ||
        !make_program_duration_row(duration, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DURATION_EST_TOTAL),
                                   program_total(program))) {
        return NULL;
    }
    return facts;
}

/* Flat panel that holds a program trajectory. */
static lv_obj_t *make_program_graph_panel(lv_obj_t *parent)
{
    lv_obj_t *graph = create_transparent_container(parent);
    if (graph == NULL) {
        return NULL;
    }
    lv_obj_set_flex_flow(graph, LV_FLEX_FLOW_COLUMN);
    return graph;
}

static bool fill_program_graph(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *graph)
{
    return make_trajectory(explorer, graph, 0);
}

static const int32_t program_quarter_columns[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};
static const int32_t program_single_row[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static const int32_t program_loading_rows[] = { LV_GRID_FR(1), 48, LV_GRID_TEMPLATE_LAST };

/* Title row shared by Program details and Load program: Back and the name. */
static lv_obj_t *make_program_title_row(
    furnace_hmi_ui_lab_explorer_t *explorer,
    ui_lab_action_t back_action
)
{
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    if (toolbar == NULL) {
        return NULL;
    }
    lv_obj_set_size(toolbar, lv_pct(100), 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_TONE_NAV, back_action, 0);
    lv_obj_t *title = create_label(toolbar, program_name_text(explorer, explorer->selected_program),
                                   &furnace_hmi_font_regular_24, UI_LAB_TEXT);
    if (back == NULL || title == NULL) {
        return NULL;
    }
    lv_obj_set_width(back, 95);
    lv_obj_set_width(title, 0);
    lv_obj_set_flex_grow(title, 1);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    return toolbar;
}

static bool program_editor_error(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity);

static bool build_program_editor(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t count = editor_stage_count(explorer);
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    if (toolbar == NULL || list == NULL || actions == NULL) { return false; }
    /* Back, the editable name, then compact stage actions share one row. */
    lv_obj_set_size(toolbar, lv_pct(100), 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 8, LV_PART_MAIN);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_TONE_NAV, UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR, 0);
    lv_obj_t *name = create_page_button(
        explorer, toolbar,
        explorer->program_editor_name[0] == '\0' ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_NAME_PLACEHOLDER)
                                                  : explorer->program_editor_name,
        UI_LAB_TONE_WELL, UI_LAB_ACTION_SHOW_RENAME, 0);
    lv_obj_t *add = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE_ACTION),
                                       UI_LAB_TONE_PRIMARY,
                                       UI_LAB_ACTION_ADD_STAGE, 0);
    lv_obj_t *edit = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_STAGES),
                                        UI_LAB_TONE_EDIT, UI_LAB_ACTION_STAGE_EDITOR, 0);
    if (back == NULL || name == NULL || add == NULL || edit == NULL) { return false; }
    lv_obj_set_width(back, 95);
    lv_obj_set_width(name, 0);
    lv_obj_set_flex_grow(name, 1);
    lv_obj_set_style_pad_hor(name, 12, LV_PART_MAIN);
    lv_obj_t *name_label = lv_obj_get_child(name, 0);
    if (name_label != NULL) {
        lv_obj_set_width(name_label, lv_pct(100));
        lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_text_font(name_label, &furnace_hmi_font_regular_18, LV_PART_MAIN);
    }
    lv_obj_set_width(add, 140);
    lv_obj_set_width(edit, 140);
    lv_obj_t *compact_labels[] = { lv_obj_get_child(add, 0), lv_obj_get_child(edit, 0) };
    for (size_t index = 0U; index < 2U; ++index) {
        if (compact_labels[index] != NULL && lv_obj_check_type(compact_labels[index], &lv_label_class)) {
            lv_obj_set_style_text_font(compact_labels[index], &furnace_hmi_font_semibold_14, LV_PART_MAIN);
        }
    }
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
                     &furnace_hmi_font_regular_16, UI_LAB_MUTED) == NULL) {
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
                                          UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR, 0);
    lv_obj_t *save = create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM),
                                        UI_LAB_TONE_PRIMARY,
                                        UI_LAB_ACTION_SHOW_SAVE, 0);
    if (cancel == NULL || save == NULL) { return false; }
    lv_obj_set_flex_grow(cancel, 1);
    lv_obj_set_flex_grow(save, 1);
    char error[112];
    if (program_editor_error(explorer, error, sizeof(error))) {
        lv_obj_add_state(save, LV_STATE_DISABLED);
        if (count > 0U) {
            lv_obj_t *note = create_label(explorer->content, error, &furnace_hmi_font_regular_14, UI_LAB_THEME_RED_TEXT);
            if (note == NULL) { return false; }
            lv_obj_set_width(note, lv_pct(100));
            lv_obj_move_to_index(note, (int32_t)lv_obj_get_index(actions));
        }
    }
    if (count > 0U) {
        size_grid_cards(list, 3U);
    }
    return true;
}

static bool build_program_loading(furnace_hmi_ui_lab_explorer_t *explorer)
{
    /* Reached from Home Quick launch only: the graph at full height over three
     * quarters, the facts in the fourth with Start program beneath them. */
    const furnace_hmi_ui_lab_program_t *program = selected_program_entry(explorer);
    prepare_program_preview(explorer, program);
    lv_obj_t *toolbar = make_program_title_row(explorer, UI_LAB_ACTION_HOME);
    lv_obj_t *columns = create_transparent_container(explorer->content);
    if (toolbar == NULL || columns == NULL) {
        return false;
    }
    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_height(columns, 1);
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_grid_dsc_array(columns, program_quarter_columns, program_loading_rows);
    lv_obj_set_style_pad_column(columns, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(columns, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *graph = make_program_graph_panel(columns);
    lv_obj_t *facts = make_program_facts(columns, program);
    lv_obj_t *start = create_page_button(explorer, columns,
                                         ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM),
                                         UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_RUNNING, 0);
    if (graph == NULL || facts == NULL || start == NULL) {
        return false;
    }
    lv_obj_set_grid_cell(graph, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 2);
    lv_obj_set_grid_cell(facts, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(start, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    ui_lab_theme_set_edge_side(start, UI_LAB_EDGE_TOP);
    return fill_program_graph(explorer, graph);
}

static void format_running_time_value(char *text, size_t capacity,
                                      const furnace_hmi_i32_value_t *value)
{
    if (value->validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT && value->value >= 0) {
        (void)snprintf(text, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_HOURS_MINUTES_FORMAT),
                       (long)(value->value / 3600), (long)((value->value / 60) % 60));
    }
    else {
        copy_text_bounded(text, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_TIME_UNKNOWN));
    }
}

/* The number alone: the reading label carries the unit ("Est. power kW"). */
static void format_power_value(char *text, size_t capacity, const furnace_hmi_i32_value_t *power)
{
    if (power->validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT) {
        const int32_t value = power->value;
        (void)snprintf(text, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_POWER_NUMBER_FORMAT),
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
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    }
    names[3] = cooling ? FURNACE_HMI_UI_STRING_UI_LAB_READING_CURRENT : FURNACE_HMI_UI_STRING_UI_LAB_READING_TARGET;
    format_observation_value(values[3], sizeof(values[3]),
                             cooling ? &state->current_temperature_c : &state->target_temperature_c,
                             FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    if (cooling) {
        names[4] = FURNACE_HMI_UI_STRING_UI_LAB_READING_TARGET;
        format_observation_value(values[4], sizeof(values[4]), &state->target_temperature_c,
                                 FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    }
    else {
        names[4] = FURNACE_HMI_UI_STRING_UI_LAB_READING_ESTIMATED_POWER;
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
        explorer->running_reading_names[index] = create_label(pair, "", &furnace_hmi_font_regular_14, UI_LAB_MUTED);
        explorer->running_reading_values[index] = create_label(pair, "", &furnace_hmi_font_semibold_16, UI_LAB_TEXT);
        if (explorer->running_reading_names[index] == NULL || explorer->running_reading_values[index] == NULL) {
            return false;
        }
    }
    update_running_readings(explorer);
    return true;
}

static void show_cooling_toggle(furnace_hmi_ui_lab_explorer_t *explorer, bool visible)
{
    lv_obj_t *toggle = explorer->cooling_toggle;
    if (toggle == NULL || visible == !lv_obj_has_flag(toggle, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(toggle, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(toggle, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_pad_right(lv_obj_get_parent(toggle), visible ? 0 : 14, LV_PART_MAIN);
}

static bool build_running(furnace_hmi_ui_lab_explorer_t *explorer)
{
    explorer->cooling_toggle = NULL;
    lv_obj_t *phase = create_transparent_container(explorer->content);
    lv_obj_t *readings = lv_obj_create(explorer->content);
    lv_obj_t *middle = create_transparent_container(explorer->content);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    if (phase == NULL || readings == NULL || middle == NULL || actions == NULL) {
        return false;
    }
    const bool compact_view = lv_obj_get_height(explorer->root) <= 400;

    /* Title (web run-title): the program name, tap for run details, and the
     * stage or phase in amber on the right. */
    lv_obj_set_width(phase, lv_pct(100));
    lv_obj_set_height(phase, 28);
    lv_obj_set_flex_flow(phase, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(phase, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(phase, UI_LAB_SPACE, LV_PART_MAIN);
    if (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL) {
        lv_obj_add_flag(phase, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *program_name = create_page_button(explorer, phase, explorer->state->program_name,
        UI_LAB_TONE_FLAT, UI_LAB_ACTION_SHOW_RUN_DETAILS, 0);
    if (!program_name) { return false; }
    lv_obj_set_style_min_height(program_name, 28, LV_PART_MAIN); lv_obj_set_height(program_name, 28);
    lv_obj_set_style_pad_all(program_name, 0, LV_PART_MAIN); lv_obj_set_style_border_width(program_name, 0, LV_PART_MAIN);
    lv_obj_set_width(program_name, 0);
    lv_obj_set_flex_grow(program_name, 1);
    lv_obj_t *name_label = lv_obj_get_child(program_name, 0);
    if (name_label) {
        lv_obj_set_width(name_label, lv_pct(100)); lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(name_label, &furnace_hmi_font_regular_20, LV_PART_MAIN);
        lv_obj_align(name_label, LV_ALIGN_LEFT_MID, 0, 0);
    }
    /* "3 / 4 - Heating" (web stageKindLabel); the controller's stage name when
     * the kind is not reported. */
    const furnace_hmi_stage_kind_t stage_kind = explorer->state->stage_kind;
    const char *stage_label = stage_kind == FURNACE_HMI_STAGE_KIND_HEATING
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING)
        : stage_kind == FURNACE_HMI_STAGE_KIND_HOLD ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD)
                                                    : explorer->state->stage_name;
    char phase_text[96];
    snprintf(phase_text, sizeof(phase_text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_IDENTITY_FORMAT),
        (unsigned)explorer->state->stage_index + 1U, (unsigned)explorer->state->stage_count, stage_label);
    const char *phase_label = explorer->state->run_phase == FURNACE_HMI_RUN_PHASE_COOLING
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_AUTOMATIC_COOLING)
        : explorer->state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL ? explorer->state->stage_name : phase_text;
    /* A lost link or a controller fault replaces the stage label; neither is animated. */
    const bool link_lost = !state_is_current(explorer->state);
    const bool run_fault = !link_lost && explorer->state->run_state == FURNACE_HMI_RUN_STATE_FAULT;
    const bool run_complete = !link_lost && explorer->state->run_state == FURNACE_HMI_RUN_STATE_COMPLETE;
    const bool run_stopped = !link_lost && run_was_stopped(explorer);
    if (run_complete) {
        phase_label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_COOLING_COMPLETE);
    }
    else if (run_stopped && explorer->state->run_phase == FURNACE_HMI_RUN_PHASE_COOLING) {
        phase_label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_STOPPED_COOLING);
    }
    lv_color_t phase_color = UI_LAB_THEME_AMBER_TEXT;
    if (link_lost) {
        phase_label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE);
        phase_color = UI_LAB_STALE;
    }
    else if (run_fault) {
        phase_label = ui_string(fault_alert_titles[explorer->state->fault_kind < FURNACE_HMI_FAULT_KIND_COUNT
            ? explorer->state->fault_kind : FURNACE_HMI_FAULT_KIND_NONE]);
        phase_color = UI_LAB_THEME_RED_TEXT;
    }
    if (create_label(phase, phase_label, &furnace_hmi_font_regular_14, phase_color) == NULL) {
        return false;
    }

    /* One reading line above the visuals, in every view (web run-readings). */
    ui_lab_theme_info_tile(readings);
    remove_scroll(readings);
    lv_obj_set_size(readings, lv_pct(100), 50);
    lv_obj_set_style_pad_hor(readings, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(readings, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(readings, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(readings, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(readings, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (!make_running_readings(explorer, readings)) { return false; }
    int64_t window_end = 0;
    bool overflows = false;
    if (explorer->running_view_mode != 2U && running_cooling_window(explorer, &window_end, &overflows)) {
        /* Offered once measured cooling passes the start-of-cooling window;
         * telemetry shows it without rebuilding the page. */
        lv_obj_t *toggle = create_page_button(
            explorer, readings,
            ui_string(explorer->running_cooling_full ? FURNACE_HMI_UI_STRING_UI_LAB_HIDE_COOLING
                                                     : FURNACE_HMI_UI_STRING_UI_LAB_SHOW_COOLING),
            UI_LAB_TONE_COOLING, UI_LAB_ACTION_TOGGLE_COOLING, 0);
        if (toggle == NULL) { return false; }
        lv_obj_set_size(toggle, 140, lv_pct(100));
        lv_obj_set_style_min_height(toggle, 0, LV_PART_MAIN);
        lv_obj_set_style_border_side(toggle, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(toggle, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_t *toggle_label = lv_obj_get_child(toggle, 0);
        if (toggle_label != NULL) {
            lv_obj_set_style_text_font(toggle_label, &furnace_hmi_font_semibold_14, LV_PART_MAIN);
        }
        explorer->cooling_toggle = toggle;
        lv_obj_add_flag(toggle, LV_OBJ_FLAG_HIDDEN);
        show_cooling_toggle(explorer, overflows);
    }

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
        /* Split: graph and furnace side by side in equal halves (web). */
        lv_obj_t *graph_column = create_transparent_container(middle);
        lv_obj_t *visual_column = create_transparent_container(middle);
        if (graph_column == NULL || visual_column == NULL) {
            return false;
        }
        lv_obj_set_width(graph_column, 0);
        lv_obj_set_flex_grow(graph_column, 1);
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

    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    const bool paused = explorer->state->run_state == FURNACE_HMI_RUN_STATE_PAUSED;
    const ui_lab_action_t primary_action = paused ? UI_LAB_ACTION_RESUME_PREVIEW
                                                   : UI_LAB_ACTION_PAUSE_PREVIEW;
    const furnace_hmi_ui_string_id_t primary_text = paused
        ? FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW
        : FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW;
    /* After a fault the controller has ended the run: no Pause, and Stop becomes
     * Return Home. Without the link no command can be sent (web parity). */
    const bool stopped = run_was_stopped(explorer) && !run_fault;
    const bool run_ended = run_fault || run_complete || stopped;
    /* After Stop: Start again (the same program) beside Return Home. */
    lv_obj_t *primary = stopped
        ? create_page_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_AGAIN),
                             UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_START_AGAIN, 0)
        : run_ended ? NULL : create_page_button(explorer, actions, ui_string(primary_text),
                                                UI_LAB_TONE_PRIMARY, primary_action, 0);
    lv_obj_t *stop = create_page_button(explorer, actions,
                                        ui_string(run_ended ? FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME
                                                            : FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW),
                                        UI_LAB_TONE_DANGER,
                                        run_ended ? UI_LAB_ACTION_RETURN_HOME : UI_LAB_ACTION_STOP_PREVIEW, 0);
    if ((primary == NULL && (!run_ended || stopped)) || stop == NULL) { return false; }
    if (stopped && !program_listed(explorer, explorer->running_program)) {
        lv_obj_add_state(primary, LV_STATE_DISABLED);
    }
    /* Web proportions: Pause 1, Stop 1, view switch 1.8 (Return Home alone takes 2.2). */
    if (primary != NULL) {
        lv_obj_set_flex_grow(primary, 10);
        lv_obj_set_height(primary, UI_LAB_BUTTON_MIN_HEIGHT);
        if (link_lost) { lv_obj_add_state(primary, LV_STATE_DISABLED); }
    }
    lv_obj_set_flex_grow(stop, run_ended && primary == NULL ? 22 : 10);
    lv_obj_set_height(stop, UI_LAB_BUTTON_MIN_HEIGHT);
    if (link_lost) { lv_obj_add_state(stop, LV_STATE_DISABLED); }

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
    /* One segmented tile: Graph | Split | Furnace. */
    lv_obj_t *views = lv_obj_create(actions);
    if (views == NULL) { return false; }
    ui_lab_theme_info_tile(views);
    lv_obj_set_width(views, 0);
    lv_obj_set_flex_grow(views, 18);
    lv_obj_set_height(views, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_style_pad_all(views, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(views, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(views, LV_FLEX_FLOW_ROW);
    remove_scroll(views);
    for (size_t index = 0; index < 3U; ++index) {
        lv_obj_t *button = create_page_button(
            explorer, views, ui_string(mode_texts[index]), UI_LAB_TONE_SEGMENT,
            mode_actions[index], 0);
        if (button == NULL) { return false; }
        ui_lab_theme_segment(button, explorer->running_view_mode == index, index == 0U);
        lv_obj_set_width(button, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, lv_pct(100));
        lv_obj_set_style_pad_hor(button, 4, LV_PART_MAIN);
    }
    return true;
}

/* One stage card. Program stages shows it read-only; the program editor makes
 * the whole card the tap target that opens the stage editor. */
static const lv_point_precise_t stage_glyph_rise[] = { { 3, 15 }, { 29, 3 } };
static const lv_point_precise_t stage_glyph_hold[] = { { 3, 9 }, { 29, 9 } };
static const lv_point_precise_t stage_glyph_fall[] = { { 3, 3 }, { 29, 15 } };

/* Small segment sketch in the stage colour (web stageGlyph): a rising ramp, a
 * flat hold or a falling ramp, ending in a dot. */
static bool make_stage_glyph(lv_obj_t *parent, ui_lab_draft_stage_kind_t kind, lv_color_t color)
{
    const lv_point_precise_t *points = kind == UI_LAB_DRAFT_STAGE_HOLD ? stage_glyph_hold
        : kind == UI_LAB_DRAFT_STAGE_COOL ? stage_glyph_fall : stage_glyph_rise;
    lv_obj_t *glyph = create_transparent_container(parent);
    lv_obj_t *line = glyph == NULL ? NULL : lv_line_create(glyph);
    lv_obj_t *dot = glyph == NULL ? NULL : lv_obj_create(glyph);
    if (glyph == NULL || line == NULL || dot == NULL) {
        return false;
    }
    lv_obj_set_size(glyph, 36, 18);
    lv_obj_remove_flag(glyph, LV_OBJ_FLAG_CLICKABLE);
    lv_line_set_points(line, points, 2U);
    lv_obj_set_pos(line, 0, 0);
    lv_obj_set_style_line_color(line, color, LV_PART_MAIN);
    lv_obj_set_style_line_width(line, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_pos(dot, 28, (int32_t)points[1].y - 3);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    remove_scroll(dot);
    return true;
}

/* One stage card (web stageCard): tinted ordinal, upper-case kind and glyph,
 * the target temperature large, then duration and rate. The edge takes the
 * stage kind's colour. Program stages shows it read-only; the program editor
 * makes the whole card the tap target that opens the stage editor. */
static bool make_stage_card(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list,
                            size_t index, bool editable)
{
    const ui_lab_stage_fixture_t fixture = program_stage(explorer, index);
    const ui_lab_draft_stage_kind_t kind = fixture_stage_kind(&fixture);
    const lv_color_t kind_color = stage_kind_color(kind);
    const ui_lab_tone_t tone = kind == UI_LAB_DRAFT_STAGE_HOLD ? UI_LAB_TONE_CARD_HOLD
        : kind == UI_LAB_DRAFT_STAGE_COOL ? UI_LAB_TONE_CARD_COOL : UI_LAB_TONE_CARD_HEAT;
    char number[8];
    char kind_text[16];
    char temperature_text[24];
    char duration[24];
    char rate[24];
    char detail[64];
    (void)snprintf(number, sizeof(number), "%u", (unsigned int)index + 1U);
    copy_text_bounded(kind_text, sizeof(kind_text),
                      ui_string(kind == UI_LAB_DRAFT_STAGE_HOLD ? FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CARD_HOLDING
                                                                : FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CARD_HEATING));
    (void)snprintf(temperature_text, sizeof(temperature_text),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_C), (long)fixture.target_c);
    (void)snprintf(duration, sizeof(duration), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_MINUTES),
                   (long)fixture.duration_minutes);
    const long rate_tenths = labs((long)fixture.delta_c_per_minute);
    if (kind != UI_LAB_DRAFT_STAGE_HOLD && rate_tenths != 0L) {
        if (rate_tenths % 10L == 0L) {
            (void)snprintf(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE_FORMAT_WHOLE),
                           rate_tenths / 10L);
        } else {
            (void)snprintf(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RATE_FORMAT_TENTHS),
                           rate_tenths / 10L, rate_tenths % 10L);
        }
        (void)snprintf(detail, sizeof(detail),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_CARD_DETAIL_FORMAT), duration, rate);
    } else {
        copy_text_bounded(detail, sizeof(detail), duration);
    }

    lv_obj_t *card = NULL;
    if (editable) {
        card = create_page_button(explorer, list, "", tone, UI_LAB_ACTION_STAGE_EDITOR, (int)index);
    } else {
        card = lv_obj_create(list);
        if (card != NULL) {
            ui_lab_theme_info_tile(card);
            ui_lab_theme_edge(card, tone, UI_LAB_EDGE_BOTTOM);
            remove_scroll(card);
        }
    }
    if (card == NULL) {
        return false;
    }
    lv_obj_set_style_min_height(card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *top = create_transparent_container(card);
    /* Values sit in their own container so a pressed card's recolouring of its
     * direct labels leaves the muted detail line alone. */
    lv_obj_t *values = create_transparent_container(card);
    lv_obj_t *badge = top == NULL ? NULL : create_transparent_container(top);
    if (top == NULL || values == NULL || badge == NULL) {
        return false;
    }
    lv_obj_remove_flag(top, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(values, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(top, lv_pct(100), 24);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(top, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_size(badge, 24, 24);
    lv_obj_set_style_radius(badge, 0, LV_PART_MAIN);
    ui_lab_theme_background(badge, stage_kind_tint(kind), LV_PART_MAIN);
    lv_obj_t *ordinal = create_label(badge, number, &furnace_hmi_font_semibold_13, kind_color);
    lv_obj_t *kind_label = create_label(top, kind_text, &furnace_hmi_font_semibold_12, kind_color);
    /* Header, temperature and detail line spread over the card (web
     * space-between), with the temperature just under the header. */
    lv_obj_set_width(values, lv_pct(100));
    lv_obj_set_height(values, 0);
    lv_obj_set_flex_grow(values, 1);
    lv_obj_set_flex_flow(values, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(values, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(values, 2, LV_PART_MAIN);
    lv_obj_t *temperature = create_label(values, temperature_text, &furnace_hmi_font_semibold_26,
                                         UI_LAB_TEXT);
    lv_obj_t *detail_label = create_label(values, detail, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (ordinal == NULL || kind_label == NULL || temperature == NULL || detail_label == NULL ||
        !make_stage_glyph(top, kind, kind_color)) {
        return false;
    }
    lv_obj_center(ordinal);
    lv_obj_set_width(kind_label, 0);
    lv_obj_set_flex_grow(kind_label, 1);
    lv_obj_set_style_text_letter_space(kind_label, 1, LV_PART_MAIN);
    lv_label_set_long_mode(kind_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(detail_label, lv_pct(100));
    lv_label_set_long_mode(detail_label, LV_LABEL_LONG_DOT);
    return true;
}

static bool build_program_stages(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t stage_count = stage_session_count(explorer);
    const size_t per_page = 9U;
    const size_t page_count = (stage_count + per_page - 1U) / per_page;
    if (page_count == 0U) {
        explorer->program_stage_page = 0U;
    } else if (explorer->program_stage_page >= page_count) {
        explorer->program_stage_page = (uint8_t)(page_count - 1U);
    }
    const size_t start = (size_t)explorer->program_stage_page * per_page;
    const size_t end = start + per_page < stage_count ? start + per_page : stage_count;
    /* Toolbar: Back, Edit program and the pager (web program-toolbar). */
    lv_obj_t *toolbar = create_transparent_container(explorer->content);
    lv_obj_t *list = create_transparent_container(explorer->content);
    if (toolbar == NULL || list == NULL) { return false; }
    lv_obj_set_size(toolbar, lv_pct(100), 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *back = create_page_button(explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_BACK),
                                        UI_LAB_TONE_NAV, UI_LAB_ACTION_BACK_PROGRAM_DETAIL, 0);
    lv_obj_t *edit_program = create_page_button(explorer, toolbar,
                                                ui_string(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM),
                                                UI_LAB_TONE_EDIT, UI_LAB_ACTION_PROGRAM_EDITOR,
                                                (int)explorer->selected_program);
    lv_obj_t *spacer = create_transparent_container(toolbar);
    lv_obj_t *pager = create_pager(explorer, toolbar, explorer->program_stage_page, page_count,
                                   UI_LAB_ACTION_PREVIOUS_STAGE_PAGE, UI_LAB_ACTION_NEXT_STAGE_PAGE,
                                   UI_LAB_EDGE_BOTTOM);
    if (back == NULL || edit_program == NULL || spacer == NULL || pager == NULL) { return false; }
    lv_obj_set_width(back, 95);
    lv_obj_set_width(edit_program, 157);
    lv_obj_set_size(spacer, 0, 1);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_height(list, 1);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(list, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(list, UI_LAB_SPACE, LV_PART_MAIN);
    for (size_t index = start; index < end; ++index) {
        if (!make_stage_card(explorer, list, index, false)) { return false; }
    }
    size_grid_cards(list, 3U);
    return true;
}
static bool build_program_detail(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const furnace_hmi_ui_lab_program_t *program = selected_program_entry(explorer);
    prepare_program_preview(explorer, program);
    /* Title row (Back, name, favourite), a four-slot action row, then the
     * graph over three quarters and the facts in the fourth. */
    lv_obj_t *toolbar = make_program_title_row(explorer, UI_LAB_ACTION_BACK_PROGRAMS);
    lv_obj_t *actions = create_transparent_container(explorer->content);
    lv_obj_t *columns = create_transparent_container(explorer->content);
    if (toolbar == NULL || actions == NULL || columns == NULL) {
        return false;
    }
    const bool favourite = program_is_favourite(explorer, explorer->selected_program);
    lv_obj_t *star = create_page_button(explorer, toolbar, "", UI_LAB_TONE_TILE,
                                        UI_LAB_ACTION_TOGGLE_FAVOURITE, explorer->selected_program);
    if (star == NULL ||
        !add_button_icon(star, favourite ? FURNACE_HMI_ICON_STAR_FILLED : FURNACE_HMI_ICON_STAR,
                         UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE)) {
        return false;
    }
    lv_obj_set_size(star, 48, 48);

    lv_obj_set_size(actions, lv_pct(100), 48);
    lv_obj_set_grid_dsc_array(actions, program_quarter_columns, program_single_row);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(actions, 0, LV_PART_MAIN);
    const struct {
        furnace_hmi_ui_string_id_t text;
        ui_lab_tone_t tone;
        ui_lab_action_t action;
        int value;
    } entries[] = {
        { FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM, UI_LAB_TONE_PRIMARY,
          UI_LAB_ACTION_CONFIRM_START, (int)explorer->selected_program },
        { FURNACE_HMI_UI_STRING_UI_LAB_VIEW_STAGES, UI_LAB_TONE_NAV, UI_LAB_ACTION_PROGRAM_STAGES, 0 },
        { FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM, UI_LAB_TONE_EDIT,
          UI_LAB_ACTION_PROGRAM_EDITOR, (int)explorer->selected_program },
        { FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT, UI_LAB_TONE_DANGER,
          UI_LAB_ACTION_SHOW_DELETE, (int)explorer->selected_program },
    };
    for (size_t index = 0U; index < sizeof(entries) / sizeof(entries[0]); ++index) {
        lv_obj_t *button = create_page_button(explorer, actions, ui_string(entries[index].text),
                                              entries[index].tone, entries[index].action,
                                              entries[index].value);
        if (button == NULL) {
            return false;
        }
        lv_obj_set_grid_cell(button, LV_GRID_ALIGN_STRETCH, (int32_t)index, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);
    }

    lv_obj_set_width(columns, lv_pct(100));
    lv_obj_set_height(columns, 1);
    lv_obj_set_flex_grow(columns, 1);
    lv_obj_set_grid_dsc_array(columns, program_quarter_columns, program_single_row);
    lv_obj_set_style_pad_column(columns, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(columns, 0, LV_PART_MAIN);
    lv_obj_t *graph = make_program_graph_panel(columns);
    lv_obj_t *facts = make_program_facts(columns, program);
    if (graph == NULL || facts == NULL) {
        return false;
    }
    lv_obj_set_grid_cell(graph, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_grid_cell(facts, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    return fill_program_graph(explorer, graph);
}

/* One stage field (web .edit-field): label on the left, a stepper tile on the
 * right (minus, value well, plus). A locked field is greyed but still shown. */
static bool make_stage_field(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    furnace_hmi_ui_string_id_t label_text,
    const char *value_text,
    bool locked,
    ui_lab_tone_t edge_tone,
    ui_lab_action_t decrement,
    ui_lab_action_t increment,
    uint8_t keyboard_target
)
{
    lv_obj_t *row = create_transparent_container(parent);
    if (row == NULL) {
        return false;
    }
    lv_obj_set_size(row, lv_pct(100), 52);
    lv_obj_set_style_pad_left(row, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *label = create_label(row, ui_string(label_text), &furnace_hmi_font_regular_15,
                                   locked ? UI_LAB_THEME_DIM : UI_LAB_MUTED);
    lv_obj_t *stepper = lv_obj_create(row);
    if (label == NULL || stepper == NULL) {
        return false;
    }
    const ui_lab_tone_t tone = locked ? UI_LAB_TONE_NEUTRAL : edge_tone;
    ui_lab_theme_info_tile(stepper);
    ui_lab_theme_edge(stepper, tone, UI_LAB_EDGE_TOP);
    remove_scroll(stepper);
    lv_obj_set_size(stepper, 194, 52);
    lv_obj_set_style_pad_all(stepper, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(stepper, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(stepper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stepper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    if (locked) {
        lv_obj_set_style_opa(stepper, (lv_opa_t)115, LV_PART_MAIN);
    }
    lv_obj_t *minus = create_page_button(explorer, stepper, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE),
                                         UI_LAB_TONE_FLAT, decrement, 0);
    lv_obj_t *value = create_page_button(explorer, stepper, value_text, UI_LAB_TONE_WELL,
                                         UI_LAB_ACTION_STAGE_EDIT_VALUE, keyboard_target);
    lv_obj_t *plus = create_page_button(explorer, stepper, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE),
                                        UI_LAB_TONE_FLAT, increment, 0);
    if (minus == NULL || value == NULL || plus == NULL) {
        return false;
    }
    lv_obj_set_size(minus, 48, 50);
    lv_obj_set_size(plus, 48, 50);
    lv_obj_set_size(value, 96, 48);
    lv_obj_set_style_min_height(value, 48, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(value, 4, LV_PART_MAIN);
    lv_obj_set_style_border_side(value, (lv_border_side_t)(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT),
                                 LV_PART_MAIN);
    ui_lab_theme_border(value, FURNACE_HMI_COLOR_HAIRLINE_STRONG, LV_PART_MAIN);
    ui_lab_theme_edge(value, tone == UI_LAB_TONE_TILE ? UI_LAB_TONE_WELL : tone, UI_LAB_EDGE_BOTTOM);
    lv_obj_t *value_label = lv_obj_get_child(value, 0);
    if (value_label != NULL && lv_obj_check_type(value_label, &lv_label_class)) {
        lv_obj_set_style_text_font(value_label, &furnace_hmi_font_regular_19, LV_PART_MAIN);
    }
    if (locked) {
        lv_obj_add_state(minus, LV_STATE_DISABLED);
        lv_obj_add_state(value, LV_STATE_DISABLED);
        lv_obj_add_state(plus, LV_STATE_DISABLED);
    }
    return true;
}

/* A 48 px icon-only tile for the dialog's top row. */
static lv_obj_t *make_stage_icon_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    ui_lab_tone_t tone,
    furnace_hmi_icon_t icon,
    ui_lab_action_t action,
    bool enabled
)
{
    lv_obj_t *button = create_page_button(explorer, parent, "", tone, action, 0);
    if (button == NULL || !add_button_icon(button, icon, UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE)) {
        return NULL;
    }
    lv_obj_set_size(button, 48, 48);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    if (!enabled) {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }
    return button;
}

/* The stage editor (web renderStageEditor): a fixed-height dialog over the page
 * it was opened from. Delete / pager with the stage position / add, the
 * Heating-Hold pair, three stepper fields (a Hold locks Target and shows Rate
 * "None"), the first validation problem, then Discard changes / Save all
 * changes. */
static bool build_stage_dialog(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const size_t count = explorer->staged_stage_count;
    const size_t index = explorer->selected_stage;
    const bool hold = explorer->draft_stage_kind == UI_LAB_DRAFT_STAGE_HOLD;
    lv_obj_t *overlay = lv_obj_create(explorer->root);
    if (overlay == NULL) {
        return false;
    }
    explorer->stage_dialog = overlay;
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_pos(overlay, -lv_obj_get_style_pad_left(explorer->root, LV_PART_MAIN),
                   -lv_obj_get_style_pad_top(explorer->root, LV_PART_MAIN));
    lv_obj_set_size(overlay, lv_obj_get_width(explorer->root), lv_obj_get_height(explorer->root));
    ui_lab_theme_background(overlay, FURNACE_HMI_COLOR_OVERLAY, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    remove_scroll(overlay);

    lv_obj_t *card = lv_obj_create(overlay);
    if (card == NULL) {
        return false;
    }
    ui_lab_theme_dialog(card, false);
    remove_scroll(card);
    const int32_t root_height = lv_obj_get_height(explorer->root);
    lv_obj_set_size(card, 440, root_height - 24 < 404 ? root_height - 24 : 404);
    lv_obj_center(card);
    lv_obj_set_style_pad_top(card, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    /* Delete, pager with the tappable position, add. */
    lv_obj_t *top = create_transparent_container(card);
    if (top == NULL) {
        return false;
    }
    lv_obj_set_size(top, lv_pct(100), 48);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *remove = make_stage_icon_button(explorer, top, UI_LAB_TONE_DANGER, FURNACE_HMI_ICON_TRASH,
                                              UI_LAB_ACTION_SHOW_DELETE_STAGE, count >= 2U);
    lv_obj_t *pager = lv_obj_create(top);
    if (remove == NULL || pager == NULL) {
        return false;
    }
    ui_lab_theme_info_tile(pager);
    ui_lab_theme_edge(pager, UI_LAB_TONE_NAV, UI_LAB_EDGE_BOTTOM);
    remove_scroll(pager);
    lv_obj_set_size(pager, LV_SIZE_CONTENT, 48);
    lv_obj_set_style_pad_all(pager, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(pager, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(pager, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pager, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *previous = create_page_button(explorer, pager, LV_SYMBOL_LEFT, UI_LAB_TONE_FLAT,
                                            UI_LAB_ACTION_PREVIOUS_STAGE, 0);
    lv_obj_t *heading = create_transparent_container(pager);
    lv_obj_t *next = create_page_button(explorer, pager, LV_SYMBOL_RIGHT, UI_LAB_TONE_FLAT,
                                        UI_LAB_ACTION_NEXT_STAGE, 0);
    if (previous == NULL || heading == NULL || next == NULL) {
        return false;
    }
    lv_obj_set_size(previous, 48, 46);
    lv_obj_set_size(next, 48, 46);
    if (index == 0U) {
        lv_obj_add_state(previous, LV_STATE_DISABLED);
    }
    if (index + 1U >= count) {
        lv_obj_add_state(next, LV_STATE_DISABLED);
    }
    lv_obj_remove_flag(heading, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(heading, LV_SIZE_CONTENT, 46);
    lv_obj_set_style_pad_hor(heading, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_column(heading, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(heading, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    char position[8];
    char of[16];
    (void)snprintf(position, sizeof(position), "%u", (unsigned int)index + 1U);
    (void)snprintf(of, sizeof(of), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_OF_FORMAT),
                   (unsigned int)count);
    if (count >= 2U) {
        /* Tapping the position moves the stage (web stage-position). */
        lv_obj_t *move = create_page_button(explorer, heading, position, UI_LAB_TONE_WELL,
                                            UI_LAB_ACTION_EDIT_STAGE_POSITION, 0);
        if (move == NULL) {
            return false;
        }
        lv_obj_set_size(move, 52, 38);
        lv_obj_set_style_min_height(move, 38, LV_PART_MAIN);
        lv_obj_set_style_pad_all(move, 0, LV_PART_MAIN);
        lv_obj_t *move_label = lv_obj_get_child(move, 0);
        if (move_label != NULL) {
            lv_obj_set_style_text_font(move_label, &furnace_hmi_font_semibold_19, LV_PART_MAIN);
        }
    }
    else if (create_label(heading, position, &furnace_hmi_font_semibold_19, UI_LAB_TEXT) == NULL) {
        return false;
    }
    if (create_label(heading, of, &furnace_hmi_font_semibold_15, UI_LAB_TEXT) == NULL ||
        make_stage_icon_button(explorer, top, UI_LAB_TONE_PRIMARY, FURNACE_HMI_ICON_PLUS,
                               UI_LAB_ACTION_ADD_STAGE_AFTER,
                               count < FURNACE_HMI_UI_LAB_MAX_STAGES) == NULL) {
        return false;
    }

    /* Heating | Hold. */
    lv_obj_t *types = create_transparent_container(card);
    if (types == NULL) {
        return false;
    }
    lv_obj_set_size(types, lv_pct(100), 48);
    lv_obj_set_flex_flow(types, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(types, UI_LAB_SPACE, LV_PART_MAIN);
    const struct {
        furnace_hmi_ui_string_id_t text;
        ui_lab_draft_stage_kind_t kind;
        ui_lab_action_t action;
    } kinds[] = {
        { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HEATING, UI_LAB_DRAFT_STAGE_HEATING, UI_LAB_ACTION_SET_DRAFT_HEATING },
        { FURNACE_HMI_UI_STRING_UI_LAB_STAGE_HOLD, UI_LAB_DRAFT_STAGE_HOLD, UI_LAB_ACTION_SET_DRAFT_HOLD },
    };
    for (size_t kind = 0U; kind < sizeof(kinds) / sizeof(kinds[0]); ++kind) {
        const bool selected = explorer->draft_stage_kind == (uint8_t)kinds[kind].kind;
        lv_obj_t *button = create_page_button(
            explorer, types, ui_string(kinds[kind].text),
            selected ? stage_kind_tone(kinds[kind].kind) : UI_LAB_TONE_NEUTRAL, kinds[kind].action, 0);
        if (button == NULL) {
            return false;
        }
        lv_obj_set_flex_grow(button, 1);
    }

    /* Target, Rate and Duration keep the same rows for both kinds. */
    lv_obj_t *fields = create_transparent_container(card);
    if (fields == NULL) {
        return false;
    }
    lv_obj_set_width(fields, lv_pct(100));
    lv_obj_set_height(fields, 0);
    lv_obj_set_flex_grow(fields, 1);
    lv_obj_set_flex_flow(fields, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(fields, 6, LV_PART_MAIN);
    const ui_lab_tone_t field_tone = hold ? UI_LAB_TONE_FIELD_HOLD : UI_LAB_TONE_TILE;
    char target[16];
    char rate[16];
    char duration[16];
    (void)snprintf(target, sizeof(target), "%ld", (long)explorer->draft_target_c);
    const long rate_tenths = labs((long)explorer->draft_rate_tenths_c_per_minute);
    if (hold) {
        copy_text_bounded(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_STAGE_RATE_NONE));
    }
    else if (rate_tenths % 10L == 0L) {
        (void)snprintf(rate, sizeof(rate), "%ld", rate_tenths / 10L);
    }
    else {
        (void)snprintf(rate, sizeof(rate), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECIMAL_FORMAT),
                       rate_tenths / 10L, rate_tenths % 10L);
    }
    (void)snprintf(duration, sizeof(duration), "%ld", (long)explorer->draft_duration_minutes);
    if (!make_stage_field(explorer, fields, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_FIELD_TARGET, target, hold,
                          field_tone, UI_LAB_ACTION_DECREMENT_TARGET, UI_LAB_ACTION_INCREMENT_TARGET,
                          UI_LAB_KEYBOARD_TARGET_DRAFT_TARGET) ||
        !make_stage_field(explorer, fields, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_FIELD_RATE, rate, hold,
                          field_tone, UI_LAB_ACTION_DECREMENT_RATE, UI_LAB_ACTION_INCREMENT_RATE,
                          UI_LAB_KEYBOARD_TARGET_DRAFT_RATE) ||
        !make_stage_field(explorer, fields, FURNACE_HMI_UI_STRING_UI_LAB_STAGE_FIELD_DURATION, duration, false,
                          field_tone, UI_LAB_ACTION_DECREMENT_DURATION, UI_LAB_ACTION_INCREMENT_DURATION,
                          UI_LAB_KEYBOARD_TARGET_DRAFT_DURATION)) {
        return false;
    }

    char error[112];
    const bool invalid = stage_session_error(explorer, error, sizeof(error));
    lv_obj_t *note = create_label(card, error, &furnace_hmi_font_regular_13,
                                  invalid ? UI_LAB_PROGRAM_DELETE_TEXT : UI_LAB_MUTED);
    lv_obj_t *actions = create_transparent_container(card);
    if (note == NULL || actions == NULL) {
        return false;
    }
    lv_obj_set_size(note, lv_pct(100), 16);
    lv_label_set_long_mode(note, LV_LABEL_LONG_DOT);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    lv_obj_set_size(actions, lv_pct(100), 48);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *discard = create_page_button(explorer, actions,
                                           ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                                           UI_LAB_TONE_DANGER, UI_LAB_ACTION_SHOW_DISCARD, 0);
    lv_obj_t *save = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_DRAFT),
                                        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_CONFIRM_SAVE_ALL, 0);
    if (discard == NULL || save == NULL) {
        return false;
    }
    lv_obj_set_flex_grow(discard, 1);
    lv_obj_set_flex_grow(save, 1);
    if (invalid) {
        lv_obj_add_state(save, LV_STATE_DISABLED);
    }
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

#define UI_LAB_DEVICE_PAGE_SIZE 5U
#define UI_LAB_DEVICE_CARD_PAGE_SIZE 6U
#define UI_LAB_DEVICE_FAULT_NOTICE 0xFFFF

typedef struct {
    bool fault;
    uint8_t component;
    uint8_t task;
} ui_lab_device_notice_ref_t;

/* ---- Card grids (web `.card-grid` / `.card`) ------------------------------
 * Rectangular tiles in a grid that fills the height left above the footer.
 * Action cards are tappable on their whole surface and carry a left edge in
 * their tone (status colour on Device); information cards have no edge. */
#define UI_LAB_CARD_GAP 8

static const int32_t ui_lab_grid_one[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static const int32_t ui_lab_grid_two[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
static const int32_t ui_lab_grid_three[] = {
    LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
};

static const int32_t *ui_lab_grid_tracks(uint8_t count)
{
    return count <= 1U ? ui_lab_grid_one : count == 2U ? ui_lab_grid_two : ui_lab_grid_three;
}

typedef struct {
    const char *label;
    const char *value;
    const char *caption;            /**< NULL: no caption line. */
    const char *tag;                /**< NULL: no tag right of the label. */
    lv_color_t tag_color;
    furnace_hmi_icon_t icon;        /**< Label icon when has_icon. */
    furnace_hmi_icon_t tag_icon;    /**< Tag icon when has_tag_icon. */
    bool has_icon;
    bool has_tag_icon;
    ui_lab_tone_t edge;             /**< UI_LAB_TONE_FLAT: information card. */
    ui_lab_action_t action;
    int action_value;
    uint8_t column;
    uint8_t row;
    uint8_t column_span;            /**< 0 means 1. */
    uint8_t row_span;               /**< 0 means 1. */
    const lv_font_t *value_font;    /**< NULL: Regular 24. */
    const lv_font_t *caption_font;  /**< NULL: Regular 11, muted. */
    bool centred;                   /**< Value and caption centred under the label. */
} ui_lab_card_t;

static lv_obj_t *create_card_grid(lv_obj_t *parent, uint8_t columns, uint8_t rows)
{
    lv_obj_t *grid = create_transparent_container(parent);
    if (grid == NULL) {
        return NULL;
    }
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_height(grid, 0);
    lv_obj_set_flex_grow(grid, 1);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, ui_lab_grid_tracks(columns), ui_lab_grid_tracks(rows));
    lv_obj_set_style_pad_column(grid, UI_LAB_CARD_GAP, LV_PART_MAIN);
    lv_obj_set_style_pad_row(grid, UI_LAB_CARD_GAP, LV_PART_MAIN);
    remove_scroll(grid);
    return grid;
}

static lv_obj_t *create_card_icon(lv_obj_t *parent, furnace_hmi_icon_t icon, lv_color_t color)
{
    lv_obj_t *image = lv_image_create(parent);
    if (image == NULL) {
        return NULL;
    }
    lv_image_set_src(image, furnace_hmi_icon_image(icon, 16));
    lv_obj_set_style_image_recolor(image, color, LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, LV_PART_MAIN);
    return image;
}

/* An empty tile in a grid cell; the caller fills it. */
static lv_obj_t *create_card_tile(
    lv_obj_t *grid,
    uint8_t column,
    uint8_t row,
    uint8_t column_span,
    uint8_t row_span
)
{
    lv_obj_t *card = lv_obj_create(grid);
    if (card == NULL) {
        return NULL;
    }
    ui_lab_theme_info_tile(card);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, column, column_span == 0U ? 1 : column_span,
                         LV_GRID_ALIGN_STRETCH, row, row_span == 0U ? 1 : row_span);
    lv_obj_set_style_pad_hor(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(card, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_row(card, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    remove_scroll(card);
    return card;
}

/* Make a whole tile one touch target with the given edge tone. */
static bool make_card_interactive(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *card,
    ui_lab_tone_t edge,
    ui_lab_action_t action,
    int value
)
{
    if (explorer->page_action_count >=
        sizeof(explorer->page_actions) / sizeof(explorer->page_actions[0])) {
        return false;
    }
    const lv_style_selector_t pressed =
        (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED;
    if (edge != UI_LAB_TONE_FLAT) {
        ui_lab_theme_edge(card, edge, UI_LAB_EDGE_LEFT);
    }
    lv_obj_set_style_bg_color(card, ui_lab_color(FURNACE_HMI_COLOR_TILE_HI_HOVER), pressed);
    lv_obj_set_style_bg_grad_color(card, ui_lab_color(FURNACE_HMI_COLOR_TILE_LO_HOVER), pressed);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_USER_1);
    furnace_hmi_ui_lab_action_slot_t *slot = &explorer->page_actions[explorer->page_action_count++];
    slot->explorer = explorer;
    slot->action = (int)action;
    slot->value = value;
    lv_obj_add_event_cb(card, action_callback, LV_EVENT_CLICKED, slot);
    return true;
}

/* Label row of a card: optional icon, the label, and an optional tag on the right. */
static lv_obj_t *create_card_head(lv_obj_t *card, const ui_lab_card_t *spec)
{
    lv_obj_t *head = create_transparent_container(card);
    if (head == NULL) {
        return NULL;
    }
    lv_obj_set_width(head, lv_pct(100));
    lv_obj_set_height(head, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(head, 6, LV_PART_MAIN);
    if (spec->has_icon && create_card_icon(head, spec->icon, UI_LAB_MUTED) == NULL) {
        return NULL;
    }
    lv_obj_t *label = create_label(head, spec->label, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (label == NULL) {
        return NULL;
    }
    lv_obj_set_width(label, 0);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    if (spec->tag != NULL) {
        if (spec->has_tag_icon && create_card_icon(head, spec->tag_icon, spec->tag_color) == NULL) {
            return NULL;
        }
        if (create_label(head, spec->tag, &furnace_hmi_font_regular_13, spec->tag_color) == NULL) {
            return NULL;
        }
    }
    return head;
}

static lv_obj_t *create_card_spacer(lv_obj_t *card)
{
    lv_obj_t *spacer = create_transparent_container(card);
    if (spacer != NULL) {
        lv_obj_set_width(spacer, lv_pct(100));
        lv_obj_set_height(spacer, 0);
        lv_obj_set_flex_grow(spacer, 1);
    }
    return spacer;
}

static lv_obj_t *create_card(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *grid,
    const ui_lab_card_t *spec
)
{
    lv_obj_t *card = create_card_tile(grid, spec->column, spec->row,
                                      spec->column_span, spec->row_span);
    if (card == NULL || create_card_head(card, spec) == NULL ||
        (spec->centred && create_card_spacer(card) == NULL)) {
        return NULL;
    }
    lv_obj_t *value = create_label(
        card, spec->value,
        spec->value_font != NULL ? spec->value_font : &furnace_hmi_font_regular_24, UI_LAB_TEXT);
    if (value == NULL) {
        return NULL;
    }
    if (spec->value_font == NULL) {
        /* Large values stay on one line and end in an ellipsis. */
        lv_obj_set_size(value, lv_pct(100),
                        lv_font_get_line_height(&furnace_hmi_font_regular_24));
        lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
    }
    else {
        /* Names (tasks, records, actions) may take a second line. */
        lv_obj_set_width(value, lv_pct(100));
        lv_label_set_long_mode(value, LV_LABEL_LONG_WRAP);
    }
    if (spec->caption != NULL) {
        lv_obj_t *caption = create_label(
            card, spec->caption,
            spec->caption_font != NULL ? spec->caption_font : &furnace_hmi_font_regular_13,
            spec->caption_font != NULL ? UI_LAB_TEXT : UI_LAB_MUTED);
        if (caption == NULL) {
            return NULL;
        }
        lv_obj_set_width(caption, lv_pct(100));
        lv_label_set_long_mode(caption, LV_LABEL_LONG_WRAP);
    }
    if (spec->centred) {
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        if (create_card_spacer(card) == NULL) {
            return NULL;
        }
    }
    if (spec->edge != UI_LAB_TONE_FLAT &&
        !make_card_interactive(explorer, card, spec->edge, spec->action, spec->action_value)) {
        return NULL;
    }
    return card;
}

/* Left rail entry: 14 px label after an 18 px icon, left-aligned, 50 px high,
 * edge on the right (web `.rail button`). */
static void style_rail_button(lv_obj_t *button, furnace_hmi_icon_t icon)
{
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_grow(button, 0);
    lv_obj_set_height(button, 50);
    lv_obj_set_style_pad_left(button, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(button, 4, LV_PART_MAIN);
    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL) {
        lv_obj_set_style_text_font(label, &furnace_hmi_font_regular_14, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_width(label, 0);
        lv_obj_set_flex_grow(label, 1);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }
    (void)add_button_icon(button, icon, UI_LAB_ICON_START, UI_LAB_RAIL_ICON_SIZE);
    /* Reversed row: "end" is the left-hand side. */
    lv_obj_set_flex_align(button, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(button, 8, LV_PART_MAIN);
}

static const furnace_hmi_icon_t device_tab_icons[] = {
    FURNACE_HMI_ICON_GAUGE, FURNACE_HMI_ICON_LIST, FURNACE_HMI_ICON_WRENCH,
    FURNACE_HMI_ICON_HISTORY, FURNACE_HMI_ICON_INFO,
};

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

#define UI_LAB_FIXED_HISTORY_COUNT (sizeof(device_history_title_ids) / sizeof(device_history_title_ids[0]))

static size_t device_history_count(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    return explorer->service_record_count + UI_LAB_FIXED_HISTORY_COUNT;
}

static const char *device_history_title(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    return index < explorer->service_record_count ? explorer->service_records[index].title
        : ui_string(device_history_title_ids[(index - explorer->service_record_count) % UI_LAB_FIXED_HISTORY_COUNT]);
}

static const char *device_history_detail(const furnace_hmi_ui_lab_explorer_t *explorer, size_t index)
{
    return index < explorer->service_record_count ? explorer->service_records[index].detail
        : ui_string(device_history_detail_ids[(index - explorer->service_record_count) % UI_LAB_FIXED_HISTORY_COUNT]);
}

#define UI_LAB_DEVICE_INFO_COUNT 5U
#define UI_LAB_DEVICE_INFO_SUPPORT 4U

static const furnace_hmi_ui_string_id_t device_info_label_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MODEL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HMI_FIRMWARE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROLLER_FIRMWARE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CATALOGUE,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT,
};

static const furnace_hmi_ui_string_id_t device_info_detail_ids[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MODEL_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HMI_FIRMWARE_DETAIL,
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROLLER_FIRMWARE_DETAIL,
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

/* Status of a Device row or card: decides its edge, tag colour and tag icon. */
typedef enum {
    UI_LAB_STATUS_NEUTRAL = 0,
    UI_LAB_STATUS_DUE,
    UI_LAB_STATUS_FAULT,
    UI_LAB_STATUS_OK,
} ui_lab_status_t;

static ui_lab_status_t device_task_status(furnace_hmi_ui_lab_device_task_state_t state)
{
    switch (state) {
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE:
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON:
        return UI_LAB_STATUS_DUE;
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE:
        return UI_LAB_STATUS_OK;
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION:
    case FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN:
    default:
        return UI_LAB_STATUS_NEUTRAL;
    }
}

static lv_color_t status_color(ui_lab_status_t status)
{
    switch (status) {
    case UI_LAB_STATUS_DUE:
        return ui_lab_color(FURNACE_HMI_COLOR_STATUS_DUE);
    case UI_LAB_STATUS_FAULT:
        return UI_LAB_FAULT;
    case UI_LAB_STATUS_OK:
        return ui_lab_color(FURNACE_HMI_COLOR_STATUS_OK);
    case UI_LAB_STATUS_NEUTRAL:
    default:
        return UI_LAB_MUTED;
    }
}

/* Edge tone: amber due, red fault, green up to date, neutral otherwise (web `.card-*`). */
static ui_lab_tone_t status_tone(ui_lab_status_t status)
{
    return status == UI_LAB_STATUS_DUE ? UI_LAB_TONE_PRIMARY
        : status == UI_LAB_STATUS_FAULT ? UI_LAB_TONE_DANGER
        : status == UI_LAB_STATUS_OK ? UI_LAB_TONE_OK : UI_LAB_TONE_NEUTRAL;
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
        explorer->device_tab == tab ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET,
        UI_LAB_ACTION_DEVICE_TAB, tab
    );
    if (button == NULL) {
        return false;
    }
    style_rail_button(button, device_tab_icons[tab % (sizeof(device_tab_icons) / sizeof(device_tab_icons[0]))]);
    ui_lab_theme_button(button, explorer->device_tab == tab ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET,
                        UI_LAB_EDGE_RIGHT);
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
            UI_LAB_TONE_NAV, UI_LAB_ACTION_DEVICE_BACK, 0);
        if (back_button == NULL) {
            return false;
        }
        lv_obj_set_width(back_button, 64);
    }
    lv_obj_t *heading = create_label(toolbar, title, &furnace_hmi_font_semibold_20, UI_LAB_TEXT);
    if (heading == NULL) {
        return false;
    }
    lv_obj_set_width(heading, 0);
    lv_obj_set_flex_grow(heading, 1);
    lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);

    if (part_info) {
        lv_obj_t *info = create_page_button(
            explorer, toolbar, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PART_INFO),
            UI_LAB_TONE_NAV, UI_LAB_ACTION_DEVICE_PART_INFO, 0);
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
            explorer->device_filter == filter_values[filter_index] ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET,
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
    ui_lab_status_t status,
    ui_lab_action_t action,
    int action_value
)
{
    lv_obj_t *row = create_panel(parent, true);
    if (row == NULL) {
        return false;
    }
    const lv_color_t value_color = status_color(status);
    const bool has_icon = status != UI_LAB_STATUS_NEUTRAL;
    ui_lab_theme_edge(row, status_tone(status), UI_LAB_EDGE_LEFT);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 48);
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
    lv_obj_set_height(left, 40);
    lv_obj_set_style_pad_row(left, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *title_label = create_label(left, title, &furnace_hmi_font_regular_14, UI_LAB_TEXT);
    lv_obj_t *detail_label = detail == NULL ? NULL :
        create_label(left, detail, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (title_label == NULL || (detail != NULL && detail_label == NULL)) {
        return false;
    }
    lv_obj_set_width(title_label, lv_pct(100));
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
    if (detail_label != NULL) {
        lv_obj_set_width(detail_label, lv_pct(100));
        lv_label_set_long_mode(detail_label, LV_LABEL_LONG_DOT);
    }
    lv_obj_t *tag_box = create_transparent_container(row);
    lv_obj_t *tag_icon = NULL;
    if (tag_box != NULL && has_icon) {
        tag_icon = lv_image_create(tag_box);
        if (tag_icon != NULL) {
            lv_image_set_src(tag_icon, furnace_hmi_icon_image(
                status == UI_LAB_STATUS_OK ? FURNACE_HMI_ICON_CHECK_CIRCLE : FURNACE_HMI_ICON_WARNING, 16));
            lv_obj_set_style_image_recolor(tag_icon, value_color, LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(tag_icon, LV_OPA_COVER, LV_PART_MAIN);
        }
    }
    lv_obj_t *tag = tag_box == NULL ? NULL
        : create_label(tag_box, value, &furnace_hmi_font_regular_13, value_color);
    if (tag == NULL || (has_icon && tag_icon == NULL) ||
        explorer->page_action_count >= sizeof(explorer->page_actions) /
            sizeof(explorer->page_actions[0])) {
        return false;
    }
    lv_obj_set_size(tag_box, 156, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tag_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tag_box, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tag_box, 6, LV_PART_MAIN);
    lv_obj_set_style_max_width(tag, tag_icon == NULL ? 156 : 134, LV_PART_MAIN);
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
    lv_obj_t *empty = create_panel(parent, false);
    if (empty == NULL) {
        return false;
    }
    lv_obj_set_width(empty, lv_pct(100));
    lv_obj_set_height(empty, 120);
    lv_obj_set_flex_flow(empty, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(empty, 18, LV_PART_MAIN);
    lv_obj_t *label = create_label(empty, text, &furnace_hmi_font_regular_14, UI_LAB_MUTED);
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
    uint16_t item_count,
    uint16_t page_size
)
{
    const uint16_t page_count = item_count == 0U
        ? 1U : (uint16_t)((item_count + page_size - 1U) / page_size);
    if (explorer->device_page >= page_count) {
        explorer->device_page = (uint8_t)(page_count - 1U);
    }
    /* A list above leaves free space; a card grid has already taken it. */
    lv_obj_t *spacer = create_transparent_container(parent);
    lv_obj_t *footer = create_transparent_container(parent);
    mark_edge_row(footer, UI_LAB_EDGE_TOP);
    if (spacer == NULL || footer == NULL) {
        return false;
    }
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_height(spacer, 0);
    lv_obj_set_flex_grow(spacer, page_size == UI_LAB_DEVICE_CARD_PAGE_SIZE ? 0 : 1);
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_height(footer, 48);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 8, LV_PART_MAIN);

    char count[64];
    (void)snprintf(
        count, sizeof(count),
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ITEM_COUNT_FORMAT),
        (unsigned)item_count,
        explorer->state != NULL &&
                explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_KNOWN_SUFFIX) : "");
    lv_obj_t *count_label = create_label(footer, count, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (count_label == NULL) {
        return false;
    }
    lv_obj_set_width(count_label, 0);
    lv_obj_set_flex_grow(count_label, 1);
    lv_label_set_long_mode(count_label, LV_LABEL_LONG_DOT);

    return create_pager(explorer, footer, explorer->device_page, page_count,
                        UI_LAB_ACTION_DEVICE_PREVIOUS, UI_LAB_ACTION_DEVICE_NEXT,
                        UI_LAB_EDGE_TOP) != NULL;
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

/* ---- Device log (web LOG_TYPES, buildDeviceLog, renderDeviceLog) -------------
 * The controller's event log: one month of history, simulated from the
 * delivered program library with a fixed seed (the web demo draws the same
 * history from its own clock), plus this session's events from controller
 * reports. Filter tabs, date headings with their age, and pages that hold as
 * many events as the screen fits. Times are wall-clock minutes since 1970. */
enum {
    UI_LAB_LOG_PROGRAM_STARTED = 0,
    UI_LAB_LOG_PROGRAM_COMPLETED,
    UI_LAB_LOG_PROGRAM_STOPPED,
    UI_LAB_LOG_PROGRAM_PAUSED,
    UI_LAB_LOG_PROGRAM_RESUMED,
    UI_LAB_LOG_PROGRAM_SAVED,
    UI_LAB_LOG_PROGRAM_DELETED,
    UI_LAB_LOG_MANUAL_STARTED,
    UI_LAB_LOG_MANUAL_STOPPED,
    UI_LAB_LOG_ALARM,
    UI_LAB_LOG_MAINTENANCE_DONE,
    UI_LAB_LOG_SERVICE_DUE,
    UI_LAB_LOG_DISPLAY_STARTED,
    UI_LAB_LOG_SETTINGS_SAVED,
    UI_LAB_LOG_LINK_LOST,
    UI_LAB_LOG_LINK_RESTORED,
    UI_LAB_LOG_TYPE_COUNT,
};

enum {
    UI_LAB_LOG_ALL = 0,
    UI_LAB_LOG_PROGRAMS,
    UI_LAB_LOG_ALARMS,
    UI_LAB_LOG_MAINTENANCE,
    UI_LAB_LOG_SYSTEM,
    UI_LAB_LOG_FILTER_COUNT,
};

#define UI_LAB_LOG_DAYS 30
#define UI_LAB_LOG_ROW 44
#define UI_LAB_LOG_HEADING 22
#define UI_LAB_LOG_GAP 4

static const struct {
    uint8_t kind;
    furnace_hmi_icon_t icon;
    ui_lab_status_t tone;
    furnace_hmi_ui_string_id_t text;
    bool display;
} log_types[UI_LAB_LOG_TYPE_COUNT] = {
    [UI_LAB_LOG_PROGRAM_STARTED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_PLAY, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_STARTED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_COMPLETED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_CHECK_CIRCLE, UI_LAB_STATUS_OK, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_COMPLETED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_STOPPED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_STOP, UI_LAB_STATUS_DUE, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_STOPPED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_PAUSED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_PAUSE, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_PAUSED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_RESUMED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_PLAY, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_RESUMED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_SAVED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_PENCIL, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_SAVED_FORMAT, false },
    [UI_LAB_LOG_PROGRAM_DELETED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_TRASH, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_PROGRAM_DELETED_FORMAT, false },
    [UI_LAB_LOG_MANUAL_STARTED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_PLAY, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_MANUAL_STARTED, false },
    [UI_LAB_LOG_MANUAL_STOPPED] = { UI_LAB_LOG_PROGRAMS, FURNACE_HMI_ICON_STOP, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_MANUAL_STOPPED, false },
    [UI_LAB_LOG_ALARM] = { UI_LAB_LOG_ALARMS, FURNACE_HMI_ICON_WARNING, UI_LAB_STATUS_FAULT, FURNACE_HMI_UI_STRING_UI_LAB_LOG_ALARM_FORMAT, false },
    [UI_LAB_LOG_MAINTENANCE_DONE] = { UI_LAB_LOG_MAINTENANCE, FURNACE_HMI_ICON_WRENCH, UI_LAB_STATUS_OK, FURNACE_HMI_UI_STRING_UI_LAB_LOG_MAINTENANCE_DONE_FORMAT, false },
    [UI_LAB_LOG_SERVICE_DUE] = { UI_LAB_LOG_MAINTENANCE, FURNACE_HMI_ICON_WRENCH, UI_LAB_STATUS_DUE, FURNACE_HMI_UI_STRING_UI_LAB_LOG_SERVICE_DUE_FORMAT, false },
    [UI_LAB_LOG_DISPLAY_STARTED] = { UI_LAB_LOG_SYSTEM, FURNACE_HMI_ICON_POWER, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_DISPLAY_STARTED, true },
    [UI_LAB_LOG_SETTINGS_SAVED] = { UI_LAB_LOG_SYSTEM, FURNACE_HMI_ICON_SLIDERS, UI_LAB_STATUS_NEUTRAL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_SETTINGS_SAVED, true },
    [UI_LAB_LOG_LINK_LOST] = { UI_LAB_LOG_SYSTEM, FURNACE_HMI_ICON_SIGNAL, UI_LAB_STATUS_DUE, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE, true },
    [UI_LAB_LOG_LINK_RESTORED] = { UI_LAB_LOG_SYSTEM, FURNACE_HMI_ICON_SIGNAL, UI_LAB_STATUS_OK, FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_RESTORED_TITLE, true },
};

static const furnace_hmi_ui_string_id_t log_filter_ids[UI_LAB_LOG_FILTER_COUNT] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ALL, FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_PROGRAMS,
    FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_ALARMS, FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_MAINTENANCE,
    FURNACE_HMI_UI_STRING_UI_LAB_LOG_FILTER_SYSTEM,
};

/* Web LOG_FAULTS order. */
static const uint8_t log_faults[] = {
    FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE, FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED,
    FURNACE_HMI_FAULT_KIND_DOOR_OPEN, FURNACE_HMI_FAULT_KIND_SENSOR, FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED,
};

/* Days since 1970-01-01 of a civil date, and back (H. Hinnant's algorithms). */
static int32_t days_from_civil(int32_t year, int32_t month, int32_t day)
{
    year -= month <= 2 ? 1 : 0;
    const int32_t era = (year >= 0 ? year : year - 399) / 400;
    const int32_t year_of_era = year - era * 400;
    const int32_t day_of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const int32_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

static furnace_hmi_ui_lab_clock_t civil_from_minute(int32_t minute)
{
    const int32_t days = minute >= 0 ? minute / 1440 : (minute - 1439) / 1440;
    const int32_t z = days + 719468;
    const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int32_t day_of_era = z - era * 146097;
    const int32_t year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    const int32_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    const int32_t mp = (5 * day_of_year + 2) / 153;
    const int32_t day = day_of_year - (153 * mp + 2) / 5 + 1;
    const int32_t month = mp < 10 ? mp + 3 : mp - 9;
    const int32_t in_day = minute - days * 1440;
    return (furnace_hmi_ui_lab_clock_t) {
        (uint16_t)(year_of_era + era * 400 + (month <= 2 ? 1 : 0)), (uint8_t)month, (uint8_t)day,
        (uint8_t)(in_day / 60), (uint8_t)(in_day % 60),
    };
}

static int32_t log_now(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const furnace_hmi_ui_lab_clock_t *clock = &explorer->clock;
    return days_from_civil(clock->year, clock->month, clock->day) * 1440 + clock->hour * 60 + clock->minute;
}

/* Web rand(): Park-Miller minimal standard generator. */
static int32_t log_rand(uint64_t *seed, uint32_t range)
{
    *seed = (*seed * 48271U) % 2147483647U;
    return (int32_t)(*seed % range);
}

static furnace_hmi_ui_lab_log_event_t log_event(int32_t minute, uint8_t type)
{
    furnace_hmi_ui_lab_log_event_t event;
    memset(&event, 0, sizeof(event));
    event.minute = minute;
    event.type = type;
    event.program = UINT8_MAX;
    return event;
}

/* A session event on top of the log (web logEvent); the oldest drops off when full. */
static void log_add(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_log_event_t *event)
{
    const size_t kept = explorer->log_count < FURNACE_HMI_UI_LAB_LOG_CAPACITY
        ? explorer->log_count : FURNACE_HMI_UI_LAB_LOG_CAPACITY - 1U;
    memmove(&explorer->log_events[1], &explorer->log_events[0], kept * sizeof(explorer->log_events[0]));
    explorer->log_events[0] = *event;
    explorer->log_count = (uint16_t)(kept + 1U);
}

static void log_add_named(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t type, const char *name)
{
    furnace_hmi_ui_lab_log_event_t event = log_event(log_now(explorer), type);
    copy_text_bounded(event.name, sizeof(event.name), name);
    log_add(explorer, &event);
}

static bool log_find_task(const furnace_hmi_ui_lab_explorer_t *explorer, const char *component_id, const char *task_id,
                          uint8_t *component, uint8_t *task)
{
    for (uint8_t part = 0U; part < explorer->device_catalogue.component_count; ++part) {
        const furnace_hmi_ui_lab_device_component_t *item = &explorer->device_catalogue.components[part];
        for (uint8_t index = 0U; strcmp(item->id, component_id) == 0 && index < item->task_count; ++index) {
            if (strcmp(item->tasks[index].id, task_id) == 0) {
                *component = part;
                *task = index;
                return true;
            }
        }
    }
    return false;
}

/* Web buildDeviceLog: the same seed and draws in the same order. */
static void log_build(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const int32_t now = log_now(explorer);
    const int32_t today = now / 1440;
    uint64_t seed = 20261006U;
    furnace_hmi_ui_lab_log_event_t *events = explorer->log_events;
    size_t count = 0U;
#define UI_LAB_LOG_PUSH(event_value) do { const furnace_hmi_ui_lab_log_event_t pushed_ = (event_value); \
        if (pushed_.minute <= now && count < FURNACE_HMI_UI_LAB_LOG_CAPACITY) { events[count++] = pushed_; } } while (0)
    for (int32_t ago = UI_LAB_LOG_DAYS - 1; ago >= 0; --ago) {
        const int32_t day = (today - ago) * 1440;
        if (log_rand(&seed, 12U) == 0) {
            UI_LAB_LOG_PUSH(log_event(day + 6 * 60 + log_rand(&seed, 60U), UI_LAB_LOG_DISPLAY_STARTED));
        }
        const int32_t runs = log_rand(&seed, 10U) < 3 ? 0 : (log_rand(&seed, 10U) < 7 ? 1 : 2);
        int32_t at = day + 7 * 60 + log_rand(&seed, 120U);
        for (int32_t run = 0; run < runs; ++run) {
            const uint8_t program = (uint8_t)log_rand(&seed, 3U);
            const int32_t curing = program_curing_minutes(&explorer->programs[program]);
            const int32_t length = curing + program_cooling_minutes(&explorer->programs[program]);
            const int32_t roll = log_rand(&seed, 20U);
            if (at + length > now) {
                break; /* no run is still going on the controller's clock */
            }
            furnace_hmi_ui_lab_log_event_t started = log_event(at, UI_LAB_LOG_PROGRAM_STARTED);
            started.program = program;
            UI_LAB_LOG_PUSH(started);
            if (roll == 0) {
                const int32_t end = at + curing * (30 + log_rand(&seed, 50U)) / 100;
                furnace_hmi_ui_lab_log_event_t alarm = log_event(end, UI_LAB_LOG_ALARM);
                alarm.program = program;
                alarm.fault = log_faults[log_rand(&seed, 5U)];
                UI_LAB_LOG_PUSH(alarm);
                at = end + 30;
            }
            else if (roll < 3) {
                const int32_t end = at + curing * (20 + log_rand(&seed, 60U)) / 100;
                furnace_hmi_ui_lab_log_event_t stopped = log_event(end, UI_LAB_LOG_PROGRAM_STOPPED);
                stopped.program = program;
                UI_LAB_LOG_PUSH(stopped);
                at = end + 20;
            }
            else {
                furnace_hmi_ui_lab_log_event_t completed = log_event(at + length, UI_LAB_LOG_PROGRAM_COMPLETED);
                completed.program = program;
                UI_LAB_LOG_PUSH(completed);
                at += length + 30;
            }
            at += 15 + log_rand(&seed, 60U);
        }
    }
    /* The controller's service reminders, a short connection loss and a settings change. */
    static const struct { int32_t ago; int32_t minute; const char *component; const char *task; } reminders[] = {
        { 4, 8 * 60 + 2, "air_filter", "filter_inspect" },
        { 11, 8 * 60 + 1, "cabinet_fan", "fan_calendar" },
    };
    for (size_t index = 0U; index < sizeof(reminders) / sizeof(reminders[0]); ++index) {
        furnace_hmi_ui_lab_log_event_t due = log_event((today - reminders[index].ago) * 1440 + reminders[index].minute,
                                                       UI_LAB_LOG_SERVICE_DUE);
        if (log_find_task(explorer, reminders[index].component, reminders[index].task, &due.component, &due.task)) {
            UI_LAB_LOG_PUSH(due);
        }
    }
    UI_LAB_LOG_PUSH(log_event((today - 9) * 1440 + 13 * 60 + 5, UI_LAB_LOG_LINK_LOST));
    UI_LAB_LOG_PUSH(log_event((today - 9) * 1440 + 13 * 60 + 7, UI_LAB_LOG_LINK_RESTORED));
    UI_LAB_LOG_PUSH(log_event((today - 17) * 1440 + 10 * 60 + 15, UI_LAB_LOG_SETTINGS_SAVED));
#undef UI_LAB_LOG_PUSH
    /* Newest first; equal times keep their order (web stable sort). */
    for (size_t index = 1U; index < count; ++index) {
        const furnace_hmi_ui_lab_log_event_t moving = events[index];
        size_t at = index;
        while (at > 0U && events[at - 1U].minute < moving.minute) {
            events[at] = events[at - 1U];
            --at;
        }
        events[at] = moving;
    }
    explorer->log_count = (uint16_t)count;
}

static void log_text(const furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_log_event_t *event,
                     char *buffer, size_t capacity)
{
    const furnace_hmi_ui_string_id_t format = log_types[event->type % UI_LAB_LOG_TYPE_COUNT].text;
    const char *name = event->name[0] != '\0' ? event->name
        : event->program < UI_LAB_LIBRARY_PROGRAM_COUNT ? ui_string(library_programs[event->program].name) : "";
    if (event->type == UI_LAB_LOG_ALARM) {
        (void)snprintf(buffer, capacity, ui_string(format),
                       ui_string(fault_alert_titles[event->fault < FURNACE_HMI_FAULT_KIND_COUNT ? event->fault : 0U]), name);
    }
    else if (event->type == UI_LAB_LOG_MAINTENANCE_DONE || event->type == UI_LAB_LOG_SERVICE_DUE) {
        const bool known = event->component < explorer->device_catalogue.component_count &&
            event->task < explorer->device_catalogue.components[event->component].task_count;
        const furnace_hmi_ui_lab_device_component_t *part = &explorer->device_catalogue.components[event->component];
        (void)snprintf(buffer, capacity, ui_string(format), known ? part->name : "",
                       known ? part->tasks[event->task].name : "");
    }
    else {
        (void)snprintf(buffer, capacity, ui_string(format), name);
    }
}

static void format_short_date(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity,
                              const furnace_hmi_ui_lab_clock_t *clock);

static void log_time_text(const furnace_hmi_ui_lab_explorer_t *explorer, int32_t minute, char *buffer, size_t capacity)
{
    const furnace_hmi_ui_lab_clock_t at = civil_from_minute(minute);
    format_clock_time(buffer, capacity, &at, explorer->settings_candidate_24_hour);
}

static bool log_listed(const furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_ui_lab_log_event_t *event)
{
    return explorer->log_filter == UI_LAB_LOG_ALL ||
        log_types[event->type % UI_LAB_LOG_TYPE_COUNT].kind == explorer->log_filter;
}

/* Web logPages: the page of every listed event; a date heading starts every
 * page and every new day. Returns the page count. */
static uint16_t log_paginate(const furnace_hmi_ui_lab_explorer_t *explorer, int32_t height, uint8_t *pages)
{
    uint16_t page = 0U;
    int32_t used = 0;
    int32_t day = INT32_MIN;
    bool started = false;
    for (uint16_t index = 0U; index < explorer->log_count; ++index) {
        const furnace_hmi_ui_lab_log_event_t *event = &explorer->log_events[index];
        if (!log_listed(explorer, event)) {
            continue;
        }
        const int32_t event_day = event->minute / 1440;
        const int32_t need = UI_LAB_LOG_ROW + UI_LAB_LOG_GAP +
            (started && event_day == day ? 0 : UI_LAB_LOG_HEADING + UI_LAB_LOG_GAP);
        if (!started || used + need > height + UI_LAB_LOG_GAP) {
            page = started ? (uint16_t)(page + 1U) : 0U;
            started = true;
            used = 0;
            day = INT32_MIN;
        }
        if (event_day != day) {
            used += UI_LAB_LOG_HEADING + UI_LAB_LOG_GAP;
            day = event_day;
        }
        used += UI_LAB_LOG_ROW + UI_LAB_LOG_GAP;
        pages[index] = (uint8_t)(page > UINT8_MAX ? UINT8_MAX : page);
    }
    return (uint16_t)(page + 1U);
}

static bool log_add_heading(const furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list, int32_t day)
{
    const furnace_hmi_ui_lab_clock_t date = civil_from_minute(day * 1440);
    char date_text[48];
    char text[96];
    format_short_date(explorer, date_text, sizeof(date_text), &date);
    const int32_t ago = log_now(explorer) / 1440 - day;
    if (ago == 0) {
        (void)snprintf(text, sizeof(text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_DAY_TODAY_FORMAT), date_text);
    }
    else if (ago == 1) {
        (void)snprintf(text, sizeof(text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_DAY_YESTERDAY_FORMAT), date_text);
    }
    else {
        (void)snprintf(text, sizeof(text), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_DAY_AGO_FORMAT), date_text,
                       (unsigned)ago);
    }
    lv_obj_t *heading = create_label(list, text, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (heading == NULL) {
        return false;
    }
    lv_obj_set_size(heading, lv_pct(100), UI_LAB_LOG_HEADING);
    lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_top(heading, 3, LV_PART_MAIN);
    return true;
}

/* One event row (web .log-row): the type icon and time, then the event text;
 * the left edge takes the event's tone. */
static bool log_add_row(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *list, uint16_t index)
{
    const furnace_hmi_ui_lab_log_event_t *event = &explorer->log_events[index];
    const ui_lab_status_t tone = log_types[event->type % UI_LAB_LOG_TYPE_COUNT].tone;
    char time_text[24];
    char text[2 * FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    log_time_text(explorer, event->minute, time_text, sizeof(time_text));
    log_text(explorer, event, text, sizeof(text));
    lv_obj_t *row = create_panel(list, true);
    lv_obj_t *icon = row != NULL ? lv_image_create(row) : NULL;
    lv_obj_t *time_label = row != NULL ? create_label(row, time_text, &furnace_hmi_font_regular_13, UI_LAB_MUTED) : NULL;
    lv_obj_t *text_label = row != NULL ? create_label(row, text, &furnace_hmi_font_regular_16, UI_LAB_TEXT) : NULL;
    if (text_label == NULL || icon == NULL || time_label == NULL ||
        explorer->page_action_count >= sizeof(explorer->page_actions) / sizeof(explorer->page_actions[0])) {
        return false;
    }
    ui_lab_theme_edge(row, status_tone(tone), UI_LAB_EDGE_LEFT);
    lv_obj_set_size(row, lv_pct(100), UI_LAB_LOG_ROW);
    lv_obj_set_style_pad_hor(row, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_image_set_src(icon, furnace_hmi_icon_image(log_types[event->type % UI_LAB_LOG_TYPE_COUNT].icon, 18));
    lv_obj_set_style_image_recolor(icon, tone == UI_LAB_STATUS_NEUTRAL ? UI_LAB_THEME_AMBER_TEXT : status_color(tone),
                                   LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_width(time_label, 66);
    lv_obj_set_width(text_label, 0);
    lv_obj_set_flex_grow(text_label, 1);
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_USER_1);
    furnace_hmi_ui_lab_action_slot_t *slot = &explorer->page_actions[explorer->page_action_count++];
    slot->explorer = explorer;
    slot->action = (int)UI_LAB_ACTION_DEVICE_LOG_EVENT;
    slot->value = (int)index;
    lv_obj_add_event_cb(row, action_callback, LV_EVENT_CLICKED, slot);
    return true;
}

/* Session events from controller reports (a real log is the controller's):
 * runs started, paused, resumed, completed, faulted or stopped, and the link. */
static void log_observation(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_dashboard_state_t *before,
                            const furnace_hmi_dashboard_state_t *after)
{
    const bool was_current = before->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    const bool is_current = after->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    if (was_current && !is_current) {
        log_add_named(explorer, UI_LAB_LOG_LINK_LOST, "");
        return;
    }
    if (!was_current && is_current) {
        log_add_named(explorer, UI_LAB_LOG_LINK_RESTORED, "");
    }
    if (!is_current) {
        return;
    }
    const bool before_active = before->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM &&
        (before->run_state == FURNACE_HMI_RUN_STATE_RUNNING || before->run_state == FURNACE_HMI_RUN_STATE_PAUSED);
    const bool after_active = after->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM &&
        (after->run_state == FURNACE_HMI_RUN_STATE_RUNNING || after->run_state == FURNACE_HMI_RUN_STATE_PAUSED);
    if (after_active && (!before_active || before->program_id != after->program_id)) {
        log_add_named(explorer, UI_LAB_LOG_PROGRAM_STARTED, after->program_name);
    }
    else if (before_active && after_active && before->run_state != after->run_state) {
        log_add_named(explorer, after->run_state == FURNACE_HMI_RUN_STATE_PAUSED ? UI_LAB_LOG_PROGRAM_PAUSED
                                                                                 : UI_LAB_LOG_PROGRAM_RESUMED,
                      after->program_name);
    }
    if (after->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && after->run_state == FURNACE_HMI_RUN_STATE_COMPLETE &&
        before->run_state != FURNACE_HMI_RUN_STATE_COMPLETE &&
        !(explorer->run_stopped && after->program_id == explorer->stopped_program_id)) {
        log_add_named(explorer, UI_LAB_LOG_PROGRAM_COMPLETED, after->program_name);
    }
    if (after->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && after->run_state == FURNACE_HMI_RUN_STATE_FAULT &&
        before->run_state != FURNACE_HMI_RUN_STATE_FAULT) {
        furnace_hmi_ui_lab_log_event_t alarm = log_event(log_now(explorer), UI_LAB_LOG_ALARM);
        alarm.fault = (uint8_t)after->fault_kind;
        copy_text_bounded(alarm.name, sizeof(alarm.name), after->program_name);
        log_add(explorer, &alarm);
    }
    if (before_active && before->run_phase == FURNACE_HMI_RUN_PHASE_CURING &&
        after->mode == FURNACE_HMI_MACHINE_MODE_IDLE) {
        log_add_named(explorer, UI_LAB_LOG_PROGRAM_STOPPED, before->program_name);
    }
}

static bool device_build_log(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    /* The filter tabs take the whole toolbar; the rail already names the page. */
    lv_obj_t *toolbar = create_transparent_container(body);
    lv_obj_t *list = create_transparent_container(body);
    lv_obj_t *footer = create_transparent_container(body);
    mark_edge_row(footer, UI_LAB_EDGE_TOP);
    if (toolbar == NULL || list == NULL || footer == NULL) {
        return false;
    }
    lv_obj_set_size(toolbar, lv_pct(100), 48);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(toolbar, 6, LV_PART_MAIN);
    for (uint8_t filter = 0U; filter < UI_LAB_LOG_FILTER_COUNT; ++filter) {
        lv_obj_t *button = create_page_button(
            explorer, toolbar, ui_string(log_filter_ids[filter]),
            explorer->log_filter == filter ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET,
            UI_LAB_ACTION_LOG_FILTER, filter);
        if (button == NULL) {
            return false;
        }
        lv_obj_set_width(button, 0);
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_style_pad_hor(button, 6, LV_PART_MAIN);
        lv_obj_t *label = lv_obj_get_child(button, 0);
        if (label != NULL && lv_obj_check_type(label, &lv_label_class)) {
            /* Web .log-toolbar: 14 px semibold on one line. */
            lv_obj_set_style_text_font(label, &furnace_hmi_font_semibold_14, LV_PART_MAIN);
            lv_obj_set_size(label, lv_pct(100), lv_font_get_line_height(&furnace_hmi_font_semibold_14));
            lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        }
    }
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_height(list, 0);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, UI_LAB_LOG_GAP, LV_PART_MAIN);
    lv_obj_set_size(footer, lv_pct(100), 48);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 8, LV_PART_MAIN);
    lv_obj_update_layout(explorer->root);

    static uint8_t pages[FURNACE_HMI_UI_LAB_LOG_CAPACITY];
    const uint16_t page_count = log_paginate(explorer, lv_obj_get_content_height(list), pages);
    if (explorer->device_page >= page_count) {
        explorer->device_page = (uint8_t)(page_count - 1U);
    }
    uint16_t listed = 0U;
    int32_t day = INT32_MIN;
    for (uint16_t index = 0U; index < explorer->log_count; ++index) {
        const furnace_hmi_ui_lab_log_event_t *event = &explorer->log_events[index];
        if (!log_listed(explorer, event)) {
            continue;
        }
        ++listed;
        if (pages[index] != explorer->device_page) {
            continue;
        }
        if (event->minute / 1440 != day) {
            day = event->minute / 1440;
            if (!log_add_heading(explorer, list, day)) {
                return false;
            }
        }
        if (!log_add_row(explorer, list, index)) {
            return false;
        }
    }
    if (listed == 0U && !device_add_empty(list, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG_EMPTY))) {
        return false;
    }
    char count[64];
    (void)snprintf(count, sizeof(count), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_EVENT_COUNT_FORMAT),
                   (unsigned)listed,
                   explorer->state != NULL && explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
                       ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_KNOWN_SUFFIX) : "");
    lv_obj_t *count_label = create_label(footer, count, &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    if (count_label == NULL) {
        return false;
    }
    lv_obj_set_width(count_label, 0);
    lv_obj_set_flex_grow(count_label, 1);
    return create_pager(explorer, footer, explorer->device_page, page_count,
                        UI_LAB_ACTION_DEVICE_PREVIOUS, UI_LAB_ACTION_DEVICE_NEXT, UI_LAB_EDGE_TOP) != NULL;
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
                    device_task_status(task->task_state), UI_LAB_ACTION_DEVICE_TASK,
                    ((int)explorer->device_selected_component << 8) | (int)index)) {
                return false;
            }
        }
        return device_add_footer(explorer, body, task_count, UI_LAB_DEVICE_PAGE_SIZE);
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
                    summary == NULL ? UI_LAB_STATUS_NEUTRAL :
                        device_task_status(summary->task_state),
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
    return device_add_footer(explorer, body, count, UI_LAB_DEVICE_PAGE_SIZE);
}

/* Status tag and edge of a Device card: warning icon for due and fault,
 * check for up to date. */
static void device_card_status(ui_lab_card_t *card, const char *text, ui_lab_status_t status)
{
    card->tag = text;
    card->tag_color = status_color(status);
    card->has_tag_icon = status != UI_LAB_STATUS_NEUTRAL;
    card->tag_icon = status == UI_LAB_STATUS_OK ? FURNACE_HMI_ICON_CHECK_CIRCLE : FURNACE_HMI_ICON_WARNING;
    card->edge = status_tone(status);
}

/* Paged card list: two columns and two or three rows (fewer items get taller
 * cards), the count and pager below. */
static lv_obj_t *device_create_card_page(lv_obj_t *body, size_t item_count)
{
    return create_card_grid(body, 2U, item_count <= 4U ? 2U : 3U);
}

/* The empty text of a card page fills the whole grid. */
static bool device_add_card_empty(lv_obj_t *grid, const char *text)
{
    if (!device_add_empty(grid, text)) {
        return false;
    }
    lv_obj_t *empty = lv_obj_get_child(grid, -1);
    lv_obj_set_grid_cell(empty, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 2);
    return true;
}

static bool device_build_overview(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_RUNTIME_SERVICE),
                     &furnace_hmi_font_semibold_20, UI_LAB_TEXT) == NULL ||
        create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_RUNTIME_SUBTITLE),
                     &furnace_hmi_font_regular_13, UI_LAB_MUTED) == NULL) {
        return false;
    }
    lv_obj_t *grid = create_card_grid(body, 3U, 2U);
    if (grid == NULL) {
        return false;
    }
    static const furnace_hmi_ui_string_id_t metrics[][3] = {
        { FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_POWERED_ON_TIME,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_POWERED_ON_VALUE,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FURNACE_LIFETIME },
        { FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PROGRAM_RUNTIME,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_PROGRAM_RUNTIME_VALUE,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_EXCLUDES_MANUAL_PAUSES },
        { FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAN_RUNTIME,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAN_RUNTIME_VALUE,
          FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONFIRMED_ROTATION },
    };
    for (uint8_t index = 0U; index < 3U; ++index) {
        const ui_lab_card_t metric = {
            .label = ui_string(metrics[index][0]),
            .value = ui_string(metrics[index][1]),
            .caption = ui_string(metrics[index][2]),
            .edge = UI_LAB_TONE_FLAT,
            .column = index,
        };
        if (create_card(explorer, grid, &metric) == NULL) {
            return false;
        }
    }

    ui_lab_device_notice_ref_t notices[
        FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS *
        FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS + 1U];
    const size_t service_count = device_collect_notices(
        explorer, UI_LAB_DEVICE_FILTER_SERVICE, notices, sizeof(notices) / sizeof(notices[0]));
    char service_caption[160];
    if (explorer->device_catalogue.component_count == 0U) {
        (void)snprintf(service_caption, sizeof(service_caption), "%s",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_COMPONENTS));
    }
    else if (service_count > 0U) {
        const furnace_hmi_ui_lab_device_component_t *part =
            &explorer->device_catalogue.components[notices[0].component];
        (void)snprintf(service_caption, sizeof(service_caption),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CARD_DETAIL_FORMAT),
                       part->name, part->tasks[notices[0].task].remaining);
    }
    else {
        (void)snprintf(service_caption, sizeof(service_caption), "%s",
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_SERVICE_ATTENTION));
    }
    const size_t error_count = device_collect_notices(
        explorer, UI_LAB_DEVICE_FILTER_ERRORS, notices, sizeof(notices) / sizeof(notices[0]));
    const bool stale = explorer->state != NULL &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE;
    char service_value[32];
    char error_value[32];
    (void)snprintf(service_value, sizeof(service_value),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TASK_COUNT_FORMAT),
                   (unsigned)service_count);
    (void)snprintf(error_value, sizeof(error_value),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ACTIVE_COUNT_FORMAT),
                   (unsigned)error_count);
    /* Last service: the date before " / " in the newest history record. */
    char last_date[32];
    const char *newest = device_history_detail(explorer, 0U);
    const char *separator = strstr(newest, " / ");
    const size_t date_length = separator != NULL ? (size_t)(separator - newest) : strlen(newest);
    (void)snprintf(last_date, sizeof(last_date), "%.*s", (int)date_length, newest);

    const ui_lab_card_t cards[] = {
        {
            .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERVICE_ATTENTION),
            .value = service_value,
            .caption = service_caption,
            .icon = FURNACE_HMI_ICON_WARNING,
            .has_icon = service_count > 0U,
            .edge = service_count > 0U ? UI_LAB_TONE_PRIMARY : UI_LAB_TONE_NEUTRAL,
            .action = UI_LAB_ACTION_DEVICE_TAB,
            .action_value = UI_LAB_DEVICE_TAB_NOTIFICATIONS,
            .column = 0U,
            .row = 1U,
        },
        {
            .label = stale ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_REPORTED_ERRORS)
                           : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CURRENT_ERRORS),
            .value = error_value,
            .caption = error_count > 0U
                ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_ROW_TITLE)
                : stale ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_ERRORS_LAST_SNAPSHOT)
                        : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_CURRENT_REPORTED_ERRORS),
            .icon = FURNACE_HMI_ICON_WARNING,
            .has_icon = error_count > 0U,
            .edge = error_count > 0U ? UI_LAB_TONE_DANGER : UI_LAB_TONE_NEUTRAL,
            .action = UI_LAB_ACTION_DEVICE_LOG_ALARMS,
            .action_value = 0,
            .column = 1U,
            .row = 1U,
        },
        {
            .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LAST_SERVICE),
            .value = last_date,
            .caption = device_history_title(explorer, 0U),
            .icon = FURNACE_HMI_ICON_HISTORY,
            .has_icon = true,
            .edge = UI_LAB_TONE_NEUTRAL,
            .action = UI_LAB_ACTION_DEVICE_TAB,
            .action_value = UI_LAB_DEVICE_TAB_HISTORY,
            .column = 2U,
            .row = 1U,
        },
    };
    for (size_t index = 0U; index < sizeof(cards) / sizeof(cards[0]); ++index) {
        if (create_card(explorer, grid, &cards[index]) == NULL) {
            return false;
        }
    }
    return true;
}

/* Maintenance: every catalogue task, worst first; an active controller fault leads. */
static size_t device_collect_tasks(
    const furnace_hmi_ui_lab_explorer_t *explorer,
    ui_lab_device_notice_ref_t *refs,
    size_t capacity
)
{
    size_t count = 0U;
    if (explorer->state != NULL &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT &&
        explorer->state->run_state == FURNACE_HMI_RUN_STATE_FAULT && count < capacity) {
        refs[count++] = (ui_lab_device_notice_ref_t){ true, 0U, 0U };
    }
    static const furnace_hmi_ui_lab_device_task_state_t order[] = {
        FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION,
        FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE,
    };
    for (size_t rank = 0U; rank < sizeof(order) / sizeof(order[0]); ++rank) {
        for (uint8_t component = 0U;
             component < explorer->device_catalogue.component_count && count < capacity;
             ++component) {
            const furnace_hmi_ui_lab_device_component_t *part =
                &explorer->device_catalogue.components[component];
            for (uint8_t task = 0U; task < part->task_count && count < capacity; ++task) {
                if (part->tasks[task].task_state == order[rank]) {
                    refs[count++] = (ui_lab_device_notice_ref_t){ false, component, task };
                }
            }
        }
    }
    return count;
}

static bool device_build_maintenance(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *body
)
{
    if (!device_add_toolbar(
            explorer, body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MAINTENANCE),
            false, false, 0U)) {
        return false;
    }
    ui_lab_device_notice_ref_t refs[
        FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS *
        FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS + 1U];
    const size_t count = device_collect_tasks(explorer, refs, sizeof(refs) / sizeof(refs[0]));
    const size_t pages = count == 0U ? 1U : (count + UI_LAB_DEVICE_CARD_PAGE_SIZE - 1U) /
                                            UI_LAB_DEVICE_CARD_PAGE_SIZE;
    if (explorer->device_page >= pages) {
        explorer->device_page = (uint8_t)(pages - 1U);
    }
    lv_obj_t *grid = device_create_card_page(body, count);
    if (grid == NULL) {
        return false;
    }
    const size_t first = (size_t)explorer->device_page * UI_LAB_DEVICE_CARD_PAGE_SIZE;
    for (size_t index = first; index < count && index < first + UI_LAB_DEVICE_CARD_PAGE_SIZE; ++index) {
        const ui_lab_device_notice_ref_t *ref = &refs[index];
        const size_t slot = index - first;
        ui_lab_card_t card = {
            .value_font = &furnace_hmi_font_semibold_18,
            .column = (uint8_t)(slot % 2U),
            .row = (uint8_t)(slot / 2U),
        };
        if (ref->fault) {
            card.label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROLLER);
            card.value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_ROW_TITLE);
            card.caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FAULT_ROW_DETAIL);
            device_card_status(&card, ui_string(FURNACE_HMI_UI_STRING_DASHBOARD_FAULT), UI_LAB_STATUS_FAULT);
            card.action = UI_LAB_ACTION_DEVICE_NOTICE;
            card.action_value = UI_LAB_DEVICE_FAULT_NOTICE;
        }
        else {
            const furnace_hmi_ui_lab_device_component_t *part =
                &explorer->device_catalogue.components[ref->component];
            const furnace_hmi_ui_lab_device_task_t *task = &part->tasks[ref->task];
            card.label = part->name;
            card.value = task->name;
            card.caption = task->remaining;
            device_card_status(&card, device_task_state_text(task->task_state),
                               device_task_status(task->task_state));
            card.action = UI_LAB_ACTION_DEVICE_TASK;
            card.action_value = ((int)ref->component << 8) | (int)ref->task;
        }
        if (create_card(explorer, grid, &card) == NULL) {
            return false;
        }
    }
    if (count == 0U &&
        !device_add_card_empty(grid, explorer->device_catalogue.component_count == 0U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_COMPONENTS)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NO_TASKS))) {
        return false;
    }
    return device_add_footer(explorer, body, (uint16_t)count, UI_LAB_DEVICE_CARD_PAGE_SIZE);
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
    const uint16_t count = (uint16_t)device_history_count(explorer);
    lv_obj_t *grid = device_create_card_page(body, count);
    if (grid == NULL) {
        return false;
    }
    const uint16_t pages = (uint16_t)((count + UI_LAB_DEVICE_CARD_PAGE_SIZE - 1U) /
                                      UI_LAB_DEVICE_CARD_PAGE_SIZE);
    if (explorer->device_page >= pages) {
        explorer->device_page = (uint8_t)(pages - 1U);
    }
    const uint16_t first = (uint16_t)(explorer->device_page * UI_LAB_DEVICE_CARD_PAGE_SIZE);
    for (uint16_t index = first; index < count && index < first + UI_LAB_DEVICE_CARD_PAGE_SIZE; ++index) {
        const uint16_t slot = (uint16_t)(index - first);
        const size_t fixed = index - (size_t)explorer->service_record_count;
        const ui_lab_card_t card = {
            .label = index < explorer->service_record_count
                ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_RECORD)
                : fixed == 1U
                    ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_ACKNOWLEDGEMENT)
                    : fixed == 2U
                        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_REPLACEMENT)
                        : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_INSPECTION),
            .value = device_history_title(explorer, index),
            .caption = device_history_detail(explorer, index),
            .edge = UI_LAB_TONE_NEUTRAL,
            .action = UI_LAB_ACTION_DEVICE_HISTORY,
            .action_value = index,
            .value_font = &furnace_hmi_font_semibold_18,
            .column = (uint8_t)(slot % 2U),
            .row = (uint8_t)(slot / 2U),
        };
        if (create_card(explorer, grid, &card) == NULL) {
            return false;
        }
    }
    return device_add_footer(explorer, body, count, UI_LAB_DEVICE_CARD_PAGE_SIZE);
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
    lv_obj_t *grid = create_card_grid(body, 3U, 2U);
    if (grid == NULL) {
        return false;
    }
    char parts[48];
    (void)snprintf(parts, sizeof(parts),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONFIGURED_PARTS_FORMAT),
                   (unsigned)explorer->device_catalogue.component_count);
    const char *values[][2] = {
        { ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MODEL_VALUE),
          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SERIAL_VALUE) },
        { ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FIRMWARE_VERSION),
          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SCOPE_DISPLAY) },
        { ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_FIRMWARE_VERSION),
          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROL_CPU) },
        { explorer->device_catalogue.revision, parts },
        { ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_VALUE),
          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_DOCUMENTS_CONTACTS) },
    };
    /* Three facts over Catalogue and a two-column Support card (web span 2). */
    for (uint8_t index = 0U; index < UI_LAB_DEVICE_INFO_COUNT; ++index) {
        const ui_lab_card_t card = {
            .label = ui_string(device_info_label_ids[index]),
            .value = values[index][0],
            .caption = values[index][1],
            .edge = UI_LAB_TONE_NEUTRAL,
            .action = UI_LAB_ACTION_DEVICE_INFO,
            .action_value = index,
            .value_font = &furnace_hmi_font_semibold_18,
            .column = (uint8_t)(index % 3U),
            .row = (uint8_t)(index / 3U),
            .column_span = index == UI_LAB_DEVICE_INFO_SUPPORT ? 2U : 1U,
        };
        if (create_card(explorer, grid, &card) == NULL) {
            return false;
        }
    }
    return true;
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
    lv_obj_set_style_pad_column(explorer->content, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->content, 0, LV_PART_MAIN);
    lv_obj_t *rail = create_transparent_container(explorer->content);
    lv_obj_t *body = create_transparent_container(explorer->content);
    if (rail == NULL || body == NULL) {
        return false;
    }
    lv_obj_set_width(rail, nav_column_width(explorer));
    lv_obj_set_height(rail, lv_pct(100));
    lv_obj_set_flex_flow(rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rail, 6, LV_PART_MAIN);
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
    /* List tabs mark staleness in their footer ("Last known"); a banner above
     * five rows and the pager would push the footer off the page. */
    if (explorer->state != NULL &&
        explorer->state->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE &&
        explorer->device_tab == UI_LAB_DEVICE_TAB_OVERVIEW &&
        create_label(body, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_STALE_BANNER),
                     &furnace_hmi_font_regular_14, UI_LAB_MUTED) == NULL) {
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
        return device_build_maintenance(explorer, body);
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
    UI_LAB_SETTINGS_TAB_SYSTEM,
    UI_LAB_SETTINGS_TAB_COUNT,
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
    FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SYSTEM,
};

/* Connections is listed only when the controller reports server/Bluetooth enabled. */
static bool settings_tab_available(const furnace_hmi_ui_lab_explorer_t *explorer, uint8_t tab)
{
    return tab < UI_LAB_SETTINGS_TAB_COUNT &&
           (tab != UI_LAB_SETTINGS_TAB_CONNECTIONS || explorer->settings_connections_enabled);
}

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
           explorer->settings_saved_24_hour != explorer->settings_candidate_24_hour ||
           explorer->settings_saved_date_format != explorer->settings_candidate_date_format ||
           explorer->settings_saved_theme != explorer->settings_candidate_theme ||
           explorer->settings_saved_language != explorer->settings_candidate_language;
}

static void settings_copy_candidate_to_saved(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memcpy(explorer->settings_saved_numeric, explorer->settings_candidate_numeric,
           sizeof(explorer->settings_saved_numeric));
    explorer->settings_saved_reduced_motion = explorer->settings_candidate_reduced_motion;
    explorer->settings_saved_running_view = explorer->settings_candidate_running_view;
    explorer->settings_saved_24_hour = explorer->settings_candidate_24_hour;
    explorer->settings_saved_date_format = explorer->settings_candidate_date_format;
    explorer->settings_saved_theme = explorer->settings_candidate_theme;
    explorer->settings_saved_language = explorer->settings_candidate_language;
}

static void settings_restore_saved(furnace_hmi_ui_lab_explorer_t *explorer)
{
    memcpy(explorer->settings_candidate_numeric, explorer->settings_saved_numeric,
           sizeof(explorer->settings_candidate_numeric));
    explorer->settings_candidate_reduced_motion = explorer->settings_saved_reduced_motion;
    explorer->settings_candidate_running_view = explorer->settings_saved_running_view;
    explorer->settings_candidate_24_hour = explorer->settings_saved_24_hour;
    explorer->settings_candidate_date_format = explorer->settings_saved_date_format;
    explorer->settings_candidate_theme = explorer->settings_saved_theme;
    explorer->settings_candidate_language = explorer->settings_saved_language;
}

/* Theme and language preview while edited: the unsaved choice is shown at
 * once; Discard and Reset show the restored one. A change restyles and
 * relabels everything (GUI context); otherwise only the page is rebuilt. */
static bool settings_apply_presentation(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const furnace_hmi_theme_id_t theme = (furnace_hmi_theme_id_t)explorer->settings_candidate_theme;
    const furnace_hmi_ui_language_t language = (furnace_hmi_ui_language_t)explorer->settings_candidate_language;
    if (ui_lab_theme_selected() == theme && furnace_hmi_ui_language() == language) {
        return rebuild_page(explorer);
    }
    (void)ui_lab_theme_select(theme);
    (void)furnace_hmi_ui_set_language(language);
    return furnace_hmi_ui_lab_explorer_refresh_presentation(explorer);
}

/* Theme and Language are choices only when there is more than one. */
static bool settings_theme_choice_available(void)
{
    return furnace_hmi_theme((furnace_hmi_theme_id_t)1) != NULL;
}

static bool settings_language_choice_available(void)
{
    return furnace_hmi_ui_language_tag((furnace_hmi_ui_language_t)1) != NULL;
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
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SUBTITLE_SYSTEM,
    };
    return ui_string(ids[tab % UI_LAB_SETTINGS_TAB_COUNT]);
}

/* Brightness: ten 10 % steps, 10-100 %. */
#define UI_LAB_SETTINGS_BRIGHTNESS_MIN 10
#define UI_LAB_SETTINGS_BRIGHTNESS_MAX 100
#define UI_LAB_SETTINGS_BRIGHTNESS_STEP 10

/* Choice dialogs: Dim when idle and Default Running view. */
enum {
    UI_LAB_SETTINGS_CHOICE_DIM = 0,
    UI_LAB_SETTINGS_CHOICE_VIEW,
    UI_LAB_SETTINGS_CHOICE_THEME,
    UI_LAB_SETTINGS_CHOICE_LANGUAGE,
    UI_LAB_SETTINGS_CHOICE_DATE_FORMAT,
    UI_LAB_SETTINGS_CHOICE_COUNT,
};

/* Idle dim delays in minutes; 0 is Off. */
static const uint8_t settings_dim_choices[] = { 0U, 1U, 2U, 5U, 10U, 30U };

static void format_dim_minutes(char *buffer, size_t capacity, int32_t minutes)
{
    if (minutes <= 0) {
        (void)snprintf(buffer, capacity, "%s", ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_OFF));
    }
    else {
        (void)snprintf(buffer, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MINUTES_FORMAT),
                       (unsigned)minutes);
    }
}

static uint8_t settings_choice_count(uint8_t kind)
{
    switch (kind) {
    case UI_LAB_SETTINGS_CHOICE_DIM:
        return (uint8_t)(sizeof(settings_dim_choices) / sizeof(settings_dim_choices[0]));
    case UI_LAB_SETTINGS_CHOICE_THEME:
        return (uint8_t)FURNACE_HMI_THEME_COUNT;
    case UI_LAB_SETTINGS_CHOICE_LANGUAGE:
        return (uint8_t)FURNACE_HMI_UI_LANGUAGE_COUNT;
    case UI_LAB_SETTINGS_CHOICE_DATE_FORMAT:
        return 5U;
    default:
        return 3U;
    }
}

static void settings_apply_choice(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t kind,
    uint8_t index
)
{
    if (index >= settings_choice_count(kind)) {
        return;
    }
    switch (kind) {
    case UI_LAB_SETTINGS_CHOICE_DIM:
        explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM] = settings_dim_choices[index];
        break;
    case UI_LAB_SETTINGS_CHOICE_THEME:
        explorer->settings_candidate_theme = index;
        break;
    case UI_LAB_SETTINGS_CHOICE_LANGUAGE:
        explorer->settings_candidate_language = index;
        break;
    case UI_LAB_SETTINGS_CHOICE_DATE_FORMAT:
        explorer->settings_candidate_date_format = index;
        break;
    default:
        explorer->settings_candidate_running_view = index;
        break;
    }
}

/* ---- Simulated controller clock ------------------------------------------- */
enum {
    UI_LAB_CLOCK_DAY = 0,
    UI_LAB_CLOCK_MONTH,
    UI_LAB_CLOCK_YEAR,
    UI_LAB_CLOCK_HOUR,
    UI_LAB_CLOCK_MINUTE,
    UI_LAB_CLOCK_AM_PM,
    UI_LAB_CLOCK_FIELD_COUNT,
};

#define UI_LAB_CLOCK_YEAR_MIN 2020
#define UI_LAB_CLOCK_YEAR_MAX 2099

static uint8_t clock_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };
    const bool leap = (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
    if (month == 2U && leap) {
        return 29U;
    }
    return month >= 1U && month <= 12U ? days[month - 1U] : 31U;
}

/* Day of the week, 0 = Sunday (Sakamoto's method). */
static uint8_t clock_weekday(const furnace_hmi_ui_lab_clock_t *clock)
{
    static const uint8_t offsets[] = { 0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U };
    const unsigned year = clock->month < 3U ? clock->year - 1U : clock->year;
    return (uint8_t)((year + year / 4U - year / 100U + year / 400U +
                      offsets[(clock->month + 11U) % 12U] + clock->day) % 7U);
}

static uint8_t clock_wrap(int value, int minimum, int maximum)
{
    return (uint8_t)(value < minimum ? maximum : value > maximum ? minimum : value);
}

/* Step one field of the dialog draft; a shorter month or a non-leap year pulls
 * the day back (31 Jan -> 28 Feb). Hours, minutes and days wrap; years clamp. */
static void clock_step(furnace_hmi_ui_lab_clock_t *clock, uint8_t field, int direction)
{
    switch (field) {
    case UI_LAB_CLOCK_DAY:
        clock->day = clock_wrap(clock->day + direction, 1, clock_days_in_month(clock->year, clock->month));
        break;
    case UI_LAB_CLOCK_MONTH:
        clock->month = clock_wrap(clock->month + direction, 1, 12);
        break;
    case UI_LAB_CLOCK_YEAR: {
        const int year = (int)clock->year + direction;
        clock->year = (uint16_t)(year < UI_LAB_CLOCK_YEAR_MIN ? UI_LAB_CLOCK_YEAR_MIN
                                 : year > UI_LAB_CLOCK_YEAR_MAX ? UI_LAB_CLOCK_YEAR_MAX : year);
        break;
    }
    case UI_LAB_CLOCK_HOUR:
        clock->hour = clock_wrap(clock->hour + direction, 0, 23);
        break;
    case UI_LAB_CLOCK_MINUTE:
        clock->minute = clock_wrap(clock->minute + direction, 0, 59);
        break;
    case UI_LAB_CLOCK_AM_PM:
        clock->hour = (uint8_t)((clock->hour + 12U) % 24U);
        break;
    default:
        break;
    }
    const uint8_t last = clock_days_in_month(clock->year, clock->month);
    if (clock->day > last) {
        clock->day = last;
    }
}

static const char *clock_month_text(uint8_t month)
{
    static const furnace_hmi_ui_string_id_t ids[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_JAN, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_FEB,
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_MAR, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_APR,
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_MAY, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_JUN,
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_JUL, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_AUG,
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_SEP, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_OCT,
        FURNACE_HMI_UI_STRING_UI_LAB_MONTH_NOV, FURNACE_HMI_UI_STRING_UI_LAB_MONTH_DEC,
    };
    return ui_string(ids[(month + 11U) % 12U]);
}

static void format_clock_hour(char *buffer, size_t capacity, uint8_t hour, bool twenty_four_hour)
{
    if (twenty_four_hour) {
        (void)snprintf(buffer, capacity, "%02u", (unsigned)hour);
    }
    else {
        (void)snprintf(buffer, capacity, "%u", (unsigned)(hour % 12U == 0U ? 12U : hour % 12U));
    }
}

static void format_clock_time(
    char *buffer,
    size_t capacity,
    const furnace_hmi_ui_lab_clock_t *clock,
    bool twenty_four_hour
)
{
    char hour[8];
    char minute[8];
    format_clock_hour(hour, sizeof(hour), clock->hour, twenty_four_hour);
    (void)snprintf(minute, sizeof(minute), "%02u", (unsigned)clock->minute);
    const char *const names[] = { "hour", "minute", "period" };
    const char *const values[] = {
        hour, minute,
        ui_string(clock->hour < 12U ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_AM
                                    : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PM),
    };
    format_pattern(buffer, capacity,
                   ui_string(twenty_four_hour ? FURNACE_HMI_UI_STRING_UI_LAB_TIME_24H_PATTERN
                                              : FURNACE_HMI_UI_STRING_UI_LAB_TIME_12H_PATTERN),
                   names, values, sizeof(names) / sizeof(names[0]));
}

static void format_date_as(char *buffer, size_t capacity, const furnace_hmi_ui_lab_clock_t *clock, uint8_t format);

/* Web clockDateText: "{weekday} {date}", the date in the chosen format. */
static void format_clock_date(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity,
                              const furnace_hmi_ui_lab_clock_t *clock)
{
    static const furnace_hmi_ui_string_id_t weekdays[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_SUNDAY, FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_MONDAY,
        FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_TUESDAY, FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_WEDNESDAY,
        FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_THURSDAY, FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_FRIDAY,
        FURNACE_HMI_UI_STRING_UI_LAB_WEEKDAY_SATURDAY,
    };
    char date[48];
    format_date_as(date, sizeof(date), clock, explorer->settings_candidate_date_format);
    const char *const names[] = { "weekday", "date" };
    const char *const values[] = { ui_string(weekdays[clock_weekday(clock)]), date };
    format_pattern(buffer, capacity, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DATE_LONG_PATTERN),
                   names, values, sizeof(names) / sizeof(names[0]));
}

static lv_obj_t *create_settings_category_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    uint8_t tab
)
{
    lv_obj_t *button = create_page_button(explorer, parent, ui_string(settings_tab_ids[tab]),
                                           UI_LAB_TONE_QUIET, UI_LAB_ACTION_SETTINGS_TAB, tab);
    if (button == NULL) {
        return NULL;
    }
    static const furnace_hmi_icon_t settings_tab_icons[] = {
        FURNACE_HMI_ICON_SUN, FURNACE_HMI_ICON_CLOCK, FURNACE_HMI_ICON_SIGNAL, FURNACE_HMI_ICON_POWER,
    };
    const bool active = explorer->settings_tab == tab;
    style_rail_button(button, settings_tab_icons[tab % (sizeof(settings_tab_icons) / sizeof(settings_tab_icons[0]))]);
    ui_lab_theme_button(button, active ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_QUIET,
                        UI_LAB_EDGE_RIGHT);
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
    ui_lab_theme_background(overlay, FURNACE_HMI_COLOR_SCRIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
    remove_scroll(overlay);

    lv_obj_t *dialog = create_panel(overlay, false);
    if (dialog == NULL) {
        clear_modal(explorer);
        return false;
    }
    ui_lab_theme_dialog(dialog, false);
    lv_obj_set_width(dialog, dialog_width(explorer));
    lv_obj_set_height(dialog, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(dialog, UI_LAB_DIALOG_PADDING, LV_PART_MAIN);
    lv_obj_set_style_pad_row(dialog, UI_LAB_DIALOG_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(dialog, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(dialog);
    lv_obj_t *title = create_label(dialog, title_text, &furnace_hmi_font_semibold_20, UI_LAB_TEXT);
    lv_obj_t *detail = create_label(dialog, detail_text, &furnace_hmi_font_regular_14, UI_LAB_TEXT);
    lv_obj_t *actions = create_transparent_container(dialog);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    if (title == NULL || detail == NULL || actions == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(detail, lv_pct(100));
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(actions, 8, LV_PART_MAIN);
    *dialog_out = dialog;
    *actions_out = actions;
    return true;
}

/* A dialog's corner X (web .dialog-close): closes without doing anything else.
 * The title keeps clear of it. */
static bool add_dialog_close(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *dialog)
{
    lv_obj_t *title = lv_obj_get_child(dialog, 0);
    lv_obj_t *close = create_modal_button(explorer, dialog, "", UI_LAB_TONE_TILE, UI_LAB_ACTION_CLOSE_MODAL, 0);
    if (title == NULL || close == NULL ||
        !add_button_icon(close, FURNACE_HMI_ICON_CLOSE, UI_LAB_ICON_ONLY, UI_LAB_ICON_ONLY_SIZE)) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_add_flag(close, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(close, 44, 44);
    lv_obj_set_style_min_height(close, 44, LV_PART_MAIN);
    lv_obj_align(close, LV_ALIGN_TOP_RIGHT, UI_LAB_DIALOG_PADDING - 12, 12 - UI_LAB_DIALOG_PADDING);
    lv_obj_set_width(title, lv_pct(100));
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_pad_right(title, 52, LV_PART_MAIN);
    return true;
}

static lv_obj_t *create_settings_modal_button(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const char *text,
    ui_lab_tone_t tone,
    ui_lab_action_t action,
    int value
)
{
    lv_obj_t *button = create_modal_button(explorer, parent, text, tone, action, value);
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
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), UI_LAB_TONE_PRIMARY);
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
    lv_obj_t *dialog;
    lv_obj_t *actions;
    if (!settings_modal_shell(explorer, title, detail, &dialog, &actions) || !add_dialog_close(explorer, dialog)) {
        return false;
    }
    /* Maintenance done asks the controller to record it; not without current data. */
    lv_obj_t *done = create_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_DONE),
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_DEVICE_MAINTENANCE_DONE,
        (int)(((unsigned)component_index << 8) | task_index));
    if (done == NULL) {
        clear_modal(explorer);
        return false;
    }
    size_dialog_button(done);
    centre_lone_dialog_button(actions);
    if (!state_is_current(explorer->state)) {
        lv_obj_add_state(done, LV_STATE_DISABLED);
    }
    return true;
}

/* Date formats (web DATE_FORMATS): the catalogue pattern of each, in preference order. */
static const furnace_hmi_ui_string_id_t date_format_patterns[] = {
    FURNACE_HMI_UI_STRING_UI_LAB_DATE_SHORT_PATTERN, FURNACE_HMI_UI_STRING_UI_LAB_DATE_DOTS_PATTERN,
    FURNACE_HMI_UI_STRING_UI_LAB_DATE_SLASH_PATTERN, FURNACE_HMI_UI_STRING_UI_LAB_DATE_US_PATTERN,
    FURNACE_HMI_UI_STRING_UI_LAB_DATE_ISO_PATTERN,
};
#define UI_LAB_DATE_FORMAT_COUNT (sizeof(date_format_patterns) / sizeof(date_format_patterns[0]))

/* Web dateText: a date in the given format. */
static void format_date_as(char *buffer, size_t capacity, const furnace_hmi_ui_lab_clock_t *clock, uint8_t format)
{
    char day[8];
    char two_day[8];
    char two_month[8];
    char year[8];
    (void)snprintf(day, sizeof(day), "%u", (unsigned)clock->day);
    (void)snprintf(two_day, sizeof(two_day), "%02u", (unsigned)clock->day);
    (void)snprintf(two_month, sizeof(two_month), "%02u", (unsigned)clock->month);
    (void)snprintf(year, sizeof(year), "%u", (unsigned)clock->year);
    const char *const names[] = { "day", "month", "year", "dd", "mm" };
    const char *const values[] = { day, clock_month_text(clock->month), year, two_day, two_month };
    format_pattern(buffer, capacity,
                   ui_string(date_format_patterns[format < UI_LAB_DATE_FORMAT_COUNT ? format : 0U]),
                   names, values, sizeof(names) / sizeof(names[0]));
}

/* A date in the chosen (edited) date format. */
static void format_short_date(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity,
                              const furnace_hmi_ui_lab_clock_t *clock)
{
    format_date_as(buffer, capacity, clock, explorer->settings_candidate_date_format);
}

/* The controller records the maintenance (web completeMaintenance): the task
 * starts a new interval (a condition-based task stays condition-based), its
 * last baseline is today, and the record leads Service history. */
static void device_complete_task(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t component_index, uint8_t task_index)
{
    if (!state_is_current(explorer->state) || component_index >= explorer->device_catalogue.component_count) {
        return;
    }
    furnace_hmi_ui_lab_device_component_t *part = &explorer->device_catalogue.components[component_index];
    if (task_index >= part->task_count) {
        return;
    }
    furnace_hmi_ui_lab_device_task_t *task = &part->tasks[task_index];
    char date[48];
    format_short_date(explorer, date, sizeof(date), &explorer->clock);
    if (task->task_state != FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION) {
        task->task_state = FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE;
        copy_text_bounded(task->state, sizeof(task->state), "ok");
        (void)snprintf(task->remaining, sizeof(task->remaining),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_NEXT_IN_FORMAT), task->interval);
    }
    copy_text_bounded(task->last, sizeof(task->last), date);
    {
        furnace_hmi_ui_lab_log_event_t done = log_event(log_now(explorer), UI_LAB_LOG_MAINTENANCE_DONE);
        done.component = component_index;
        done.task = task_index;
        log_add(explorer, &done);
    }
    const size_t kept = explorer->service_record_count < FURNACE_HMI_UI_LAB_SERVICE_RECORDS
        ? explorer->service_record_count : FURNACE_HMI_UI_LAB_SERVICE_RECORDS - 1U;
    memmove(&explorer->service_records[1], &explorer->service_records[0], kept * sizeof(explorer->service_records[0]));
    furnace_hmi_ui_lab_service_record_t *record = &explorer->service_records[0];
    (void)snprintf(record->title, sizeof(record->title),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_TASK_TITLE_FORMAT), part->name, task->name);
    (void)snprintf(record->detail, sizeof(record->detail),
                   ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_DONE_DETAIL_FORMAT), date, part->usage);
    explorer->service_record_count = (uint8_t)(kept + 1U);
    char detail[2 * FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY + 48U];
    (void)snprintf(detail, sizeof(detail), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_RECORDED_DETAIL_FORMAT),
                   part->name, task->name);
    clear_modal(explorer);
    (void)rebuild_page(explorer);
    (void)settings_show_message(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_MAINTENANCE_RECORDED), detail);
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
    if (index >= device_history_count(explorer)) {
        return false;
    }
    return settings_show_message(
        explorer, device_history_title(explorer, index),
        index < explorer->service_record_count
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_HISTORY_DONE_TEXT)
            : ui_string(device_history_text_ids[index - explorer->service_record_count]));
}

/* Support (web supportDialog): the scan hint over the support QR code, drawn
 * in qr-ink on a qr-plate with its quiet zone; the corner X closes it. */
static bool device_show_support(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *dialog;
    lv_obj_t *actions;
    if (!settings_modal_shell(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_SUPPORT_DETAIL), &dialog, &actions) ||
        !add_dialog_close(explorer, dialog)) {
        return false;
    }
    lv_obj_t *row = create_transparent_container(dialog);
    lv_obj_t *plate = row != NULL ? lv_obj_create(row) : NULL;
    lv_obj_t *code = plate != NULL ? lv_image_create(plate) : NULL;
    if (code == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_move_to_index(row, (int32_t)lv_obj_get_index(actions));
    lv_obj_add_flag(actions, LV_OBJ_FLAG_HIDDEN);
    ui_lab_theme_background(plate, FURNACE_HMI_COLOR_QR_PLATE, LV_PART_MAIN);
    lv_obj_set_style_border_width(plate, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(plate, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(plate, 12, LV_PART_MAIN);
    lv_obj_set_size(plate, (int32_t)furnace_hmi_support_qr.header.w + 24, (int32_t)furnace_hmi_support_qr.header.h + 24);
    remove_scroll(plate);
    lv_image_set_src(code, &furnace_hmi_support_qr);
    lv_obj_set_style_image_recolor(code, ui_lab_color(FURNACE_HMI_COLOR_QR_INK), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(code, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(code);
    return true;
}

static bool device_show_info(
    furnace_hmi_ui_lab_explorer_t *explorer,
    uint8_t index
)
{
    if (index >= sizeof(device_info_detail_ids) / sizeof(device_info_detail_ids[0])) {
        return false;
    }
    if (index == UI_LAB_DEVICE_INFO_SUPPORT) {
        return device_show_support(explorer);
    }
    return settings_show_message(
        explorer, ui_string(device_info_label_ids[index]),
        ui_string(device_info_detail_ids[index]));
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
        UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_UNSAVED_STAY, 0);
    lv_obj_t *discard = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD),
        UI_LAB_TONE_DANGER, UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB,
        explorer->settings_pending_tab);
    lv_obj_t *save = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE),
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_SETTINGS_SAVE_TO_TAB,
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
        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_CLOSE_MODAL, 0);
    lv_obj_t *display = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISPLAY_PREFERENCES),
        UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 0);
    lv_obj_t *program = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_FORMAT_PREFERENCES),
        UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 1);
    lv_obj_t *both = create_settings_modal_button(
        explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BOTH_PREFERENCES),
        UI_LAB_TONE_DANGER, UI_LAB_ACTION_SETTINGS_RESET_SCOPE, 2);
    return cancel != NULL && display != NULL && program != NULL && both != NULL;
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
                       UI_LAB_ACTION_SETTINGS_RESTART_CONFIRM, ui_string(action), UI_LAB_TONE_DANGER);
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
    /* The in-sentence form of the scope ("Reset display preferences?"). */
    const char *scope_text = scope == 0U
        ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SCOPE_DISPLAY)
        : scope == 1U
            ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SCOPE_FORMATS)
            : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SCOPE_BOTH);
    (void)snprintf(title, sizeof(title), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DIALOG_TITLE), scope_text);
    const furnace_hmi_ui_string_id_t defaults_id = scope == 0U
        ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_DISPLAY
        : scope == 1U
            ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_FORMATS
            : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DEFAULTS_BOTH;
    (void)snprintf(detail, sizeof(detail), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_DETAIL_FORMAT),
                   ui_string(defaults_id), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_CONFIRM));
    explorer->settings_pending_scope = scope;
    return open_dialog(explorer, title, detail, UI_LAB_ACTION_SETTINGS_RESET_CONFIRM,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_PREFERENCES), UI_LAB_TONE_DANGER);
}

/* Delete programs (web deletePrograms): every listed program goes, after one
 * confirmation that names them; a single program is deleted from its details. */
static bool settings_open_delete_dialog(furnace_hmi_ui_lab_explorer_t *explorer)
{
    uint16_t pending = 0U;
    for (size_t slot = 0U; slot < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++slot) {
        if (program_listed(explorer, slot)) {
            pending |= (uint16_t)(1U << slot);
        }
    }
    if (pending == 0U) {
        return settings_show_message(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_TITLE),
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NO_PROGRAMS));
    }
    char names[FURNACE_HMI_UI_LAB_MAX_PROGRAMS * (FURNACE_HMI_UI_LAB_NAME_CAPACITY + 2U)];
    size_t used = 0U;
    unsigned count = 0U;
    names[0] = '\0';
    for (size_t slot = 0U; slot < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++slot) {
        if ((pending & (1U << slot)) == 0U) {
            continue;
        }
        const int written = snprintf(names + used, sizeof(names) - used, "%s%s",
                                     count > 0U ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LIST_SEPARATOR) : "",
                                     program_name_text(explorer, slot));
        used += written > 0 && (size_t)written < sizeof(names) - used ? (size_t)written : 0U;
        ++count;
    }
    char title[64];
    char detail[sizeof(names) + 96U];
    (void)snprintf(title, sizeof(title), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_COUNT_TITLE_FORMAT), count);
    (void)snprintf(detail, sizeof(detail), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_NAMES_FORMAT), names);
    explorer->settings_delete_pending = pending;
    return open_dialog(explorer, title, detail, UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM,
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ALL), UI_LAB_TONE_DANGER);
}

/* Save bar under Display and Date & time (web `.actions`). */
static bool settings_build_footer(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *parent)
{
    lv_obj_t *actions = create_transparent_container(parent);
    mark_edge_row(actions, UI_LAB_EDGE_TOP);
    if (actions == NULL) {
        return false;
    }
    lv_obj_set_width(actions, lv_pct(100));
    lv_obj_set_height(actions, 56);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_t *status = create_label(
        actions,
        ui_string(settings_dirty(explorer) ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_UNSAVED
                                           : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_STATUS_DISPLAY),
        &furnace_hmi_font_regular_13, UI_LAB_MUTED);
    lv_obj_t *discard = create_page_button(explorer, actions,
                                            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DISCARD),
                                            UI_LAB_TONE_DANGER, UI_LAB_ACTION_SETTINGS_DISCARD, 0);
    lv_obj_t *save = create_page_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVE),
                                        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_SETTINGS_SAVE, 0);
    if (status == NULL || discard == NULL || save == NULL) {
        return false;
    }
    lv_obj_set_width(status, 0);
    lv_obj_set_flex_grow(status, 1);
    lv_label_set_long_mode(status, LV_LABEL_LONG_DOT);
    lv_obj_set_width(discard, 82);
    lv_obj_set_width(save, 72);
    if (!settings_dirty(explorer)) {
        lv_obj_add_state(discard, LV_STATE_DISABLED);
        lv_obj_add_state(save, LV_STATE_DISABLED);
    }
    return true;
}

/* Brightness across the top of Display: -, ten tappable steps, +, with no
 * number shown. Lit steps brighten from left to right; the heading sits just
 * above the bars and the pair is centred in the card (web `.gauge`). */
static bool settings_add_brightness_card(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *grid)
{
    const int32_t brightness = explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS];
    lv_obj_t *card = create_card_tile(grid, 0U, 0U, 3U, 1U);
    if (card == NULL) {
        return false;
    }
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 10, LV_PART_MAIN);
    const ui_lab_card_t head = {
        .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BRIGHTNESS),
        .icon = FURNACE_HMI_ICON_SUN,
        .has_icon = true,
    };
    lv_obj_t *head_row = create_card_head(card, &head);
    lv_obj_t *gauge = create_transparent_container(card);
    if (head_row == NULL || gauge == NULL) {
        return false;
    }
    lv_obj_set_width(gauge, lv_pct(100));
    lv_obj_set_height(gauge, 44);
    lv_obj_set_flex_flow(gauge, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(gauge, 6, LV_PART_MAIN);
    lv_obj_t *minus = create_page_button(explorer, gauge, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE),
                                         UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_BRIGHTNESS_STEP, -1);
    lv_obj_t *steps = create_transparent_container(gauge);
    if (minus == NULL || steps == NULL) {
        return false;
    }
    lv_obj_set_size(minus, 56, lv_pct(100));
    lv_obj_set_style_min_height(minus, 0, LV_PART_MAIN);
    lv_obj_set_width(steps, 0);
    lv_obj_set_flex_grow(steps, 1);
    lv_obj_set_height(steps, lv_pct(100));
    lv_obj_set_flex_flow(steps, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(steps, 4, LV_PART_MAIN);
    for (int32_t level = UI_LAB_SETTINGS_BRIGHTNESS_MIN; level <= UI_LAB_SETTINGS_BRIGHTNESS_MAX;
         level += UI_LAB_SETTINGS_BRIGHTNESS_STEP) {
        lv_obj_t *step = lv_obj_create(steps);
        if (step == NULL) {
            return false;
        }
        lv_obj_set_width(step, 0);
        lv_obj_set_flex_grow(step, 1);
        lv_obj_set_height(step, lv_pct(100));
        lv_obj_set_style_radius(step, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(step, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(step, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(step, 1, LV_PART_MAIN);
        ui_lab_theme_border(step, FURNACE_HMI_COLOR_HAIRLINE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(step, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(step, LV_GRAD_DIR_NONE, LV_PART_MAIN);
        if (level <= brightness) {
            /* 30 % opacity for the first step up to full amber for the last. */
            const int32_t opacity = 77 + (178 * (level - UI_LAB_SETTINGS_BRIGHTNESS_MIN)) /
                (UI_LAB_SETTINGS_BRIGHTNESS_MAX - UI_LAB_SETTINGS_BRIGHTNESS_MIN);
            lv_obj_set_style_bg_color(step, lv_color_mix(UI_LAB_THEME_AMBER, UI_LAB_THEME_TILE_LO,
                                                         (lv_opa_t)opacity), LV_PART_MAIN);
        }
        else {
            lv_obj_set_style_bg_color(step, UI_LAB_THEME_WELL, LV_PART_MAIN);
        }
        remove_scroll(step);
        if (!make_card_interactive(explorer, step, UI_LAB_TONE_FLAT,
                                   UI_LAB_ACTION_SETTINGS_BRIGHTNESS_SET, (int)level)) {
            return false;
        }
    }
    lv_obj_t *plus = create_page_button(explorer, gauge, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE),
                                        UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_BRIGHTNESS_STEP, 1);
    if (plus == NULL) {
        return false;
    }
    lv_obj_set_size(plus, 56, lv_pct(100));
    lv_obj_set_style_min_height(plus, 0, LV_PART_MAIN);
    if (brightness <= UI_LAB_SETTINGS_BRIGHTNESS_MIN) {
        lv_obj_add_state(minus, LV_STATE_DISABLED);
    }
    if (brightness >= UI_LAB_SETTINGS_BRIGHTNESS_MAX) {
        lv_obj_add_state(plus, LV_STATE_DISABLED);
    }
    return true;
}

static bool settings_add_cards(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *grid,
    const ui_lab_card_t *cards,
    size_t count
)
{
    for (size_t index = 0U; index < count; ++index) {
        if (create_card(explorer, grid, &cards[index]) == NULL) {
            return false;
        }
    }
    return true;
}

static bool settings_build_display(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *body)
{
    lv_obj_t *grid = create_card_grid(body, 3U, 3U);
    if (grid == NULL || !settings_add_brightness_card(explorer, grid)) {
        return false;
    }
    char dim[24];
    format_dim_minutes(dim, sizeof(dim), explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM]);
    const ui_lab_card_t cards[] = {
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_WHEN_IDLE), .value = dim,
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_CAPTION),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_CHOICE,
          .action_value = UI_LAB_SETTINGS_CHOICE_DIM, .column = 0U, .row = 1U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_REDUCED_MOTION),
          .value = ui_string(explorer->settings_candidate_reduced_motion
                                 ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ON
                                 : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_OFF),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MOTION_CAPTION),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION,
          .column = 1U, .row = 1U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DEFAULT_RUNNING_VIEW),
          .value = settings_running_view_text(explorer->settings_candidate_running_view),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VIEW_CAPTION),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_CHOICE,
          .action_value = UI_LAB_SETTINGS_CHOICE_VIEW, .column = 2U, .row = 1U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_LANGUAGE),
          .value = furnace_hmi_ui_language_name((furnace_hmi_ui_language_t)explorer->settings_candidate_language),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_LANGUAGE_CAPTION),
          .edge = settings_language_choice_available() ? UI_LAB_TONE_TILE : UI_LAB_TONE_FLAT,
          .action = UI_LAB_ACTION_SETTINGS_SHOW_CHOICE, .action_value = UI_LAB_SETTINGS_CHOICE_LANGUAGE,
          .column = 0U, .row = 2U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNITS),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNITS_VALUE),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNITS_CAPTION),
          .edge = UI_LAB_TONE_FLAT, .column = 1U, .row = 2U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_THEME),
          .value = ui_string(furnace_hmi_theme((furnace_hmi_theme_id_t)explorer->settings_candidate_theme)->name),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_THEME_CAPTION),
          .edge = settings_theme_choice_available() ? UI_LAB_TONE_TILE : UI_LAB_TONE_FLAT,
          .action = UI_LAB_ACTION_SETTINGS_SHOW_CHOICE, .action_value = UI_LAB_SETTINGS_CHOICE_THEME,
          .column = 2U, .row = 2U },
    };
    return settings_add_cards(explorer, grid, cards, sizeof(cards) / sizeof(cards[0])) &&
           settings_build_footer(explorer, body);
}

static bool settings_build_time(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *body)
{
    lv_obj_t *grid = create_card_grid(body, 3U, 2U);
    if (grid == NULL) {
        return false;
    }
    char time[24];
    char date[48];
    format_clock_time(time, sizeof(time), &explorer->clock, explorer->settings_candidate_24_hour);
    format_clock_date(explorer, date, sizeof(date), &explorer->clock);
    char short_date[48];
    format_short_date(explorer, short_date, sizeof(short_date), &explorer->clock);
    const ui_lab_card_t cards[] = {
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_AND_TIME), .value = time,
          .caption = date, .tag = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET),
          .tag_color = UI_LAB_MUTED, .icon = FURNACE_HMI_ICON_CLOCK, .has_icon = true,
          .tag_icon = FURNACE_HMI_ICON_PENCIL, .has_tag_icon = true,
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_CLOCK,
          .column = 0U, .row = 0U, .column_span = 2U, .row_span = 2U,
          .value_font = &furnace_hmi_font_semibold_48,
          .caption_font = &furnace_hmi_font_regular_20, .centred = true },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_TIME_FORMAT),
          .value = ui_string(explorer->settings_candidate_24_hour
                                 ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_24_HOUR
                                 : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_12_HOUR),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_TIME_FORMAT_CAPTION),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT,
          .column = 2U, .row = 0U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT), .value = short_date,
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT_CAPTION),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_CHOICE,
          .action_value = UI_LAB_SETTINGS_CHOICE_DATE_FORMAT, .column = 2U, .row = 1U },
    };
    return settings_add_cards(explorer, grid, cards, sizeof(cards) / sizeof(cards[0])) &&
           settings_build_footer(explorer, body);
}

static bool settings_build_connections(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *body)
{
    lv_obj_t *grid = create_card_grid(body, 2U, 2U);
    if (grid == NULL) {
        return false;
    }
    const furnace_hmi_state_availability_t availability = explorer->state != NULL
        ? explorer->state->availability : FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    const ui_lab_card_t cards[] = {
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FURNACE_CONTROLLER),
          .value = ui_string(availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT
                                 ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONNECTED
                                 : availability == FURNACE_HMI_STATE_AVAILABILITY_STALE
                                     ? FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE
                                     : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_UNAVAILABLE),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_CONTROL_LINK_CAPTION),
          .icon = FURNACE_HMI_ICON_FURNACE, .has_icon = true, .edge = UI_LAB_TONE_FLAT,
          .column = 0U, .row = 0U, .column_span = 2U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SERVER),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NOT_CONNECTED),
          .icon = FURNACE_HMI_ICON_SIGNAL, .has_icon = true, .edge = UI_LAB_TONE_TILE,
          .action = UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP, .action_value = 0,
          .value_font = &furnace_hmi_font_semibold_18, .column = 0U, .row = 1U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BLUETOOTH),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_NOT_CONNECTED),
          .icon = FURNACE_HMI_ICON_SIGNAL, .has_icon = true, .edge = UI_LAB_TONE_TILE,
          .action = UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP, .action_value = 1,
          .value_font = &furnace_hmi_font_semibold_18, .column = 1U, .row = 1U },
    };
    return settings_add_cards(explorer, grid, cards, sizeof(cards) / sizeof(cards[0]));
}

static bool settings_build_system(furnace_hmi_ui_lab_explorer_t *explorer, lv_obj_t *body)
{
    lv_obj_t *grid = create_card_grid(body, 2U, 2U);
    if (grid == NULL) {
        return false;
    }
    const ui_lab_card_t cards[] = {
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SCOPE_LOCAL),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_SETTINGS),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_ROW_DETAIL),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_RESET,
          .value_font = &furnace_hmi_font_semibold_18, .column = 0U, .row = 0U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SCOPE_LIBRARY),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_PROGRAMS),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DELETE_ROW_DETAIL),
          .edge = UI_LAB_TONE_DANGER, .action = UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS,
          .value_font = &furnace_hmi_font_semibold_18, .column = 1U, .row = 0U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SCOPE_DISPLAY),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_ROW_DETAIL),
          .edge = UI_LAB_TONE_TILE, .action = UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY,
          .value_font = &furnace_hmi_font_semibold_18, .column = 0U, .row = 1U },
        { .label = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_FURNACE_CONTROLLER),
          .value = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ATTEMPT_FURNACE_RESTART),
          .caption = ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_ATTEMPT_RESTART_ROW_DETAIL),
          .edge = UI_LAB_TONE_DANGER, .action = UI_LAB_ACTION_SETTINGS_ASSESS_RESTART,
          .value_font = &furnace_hmi_font_semibold_18, .column = 1U, .row = 1U },
    };
    return settings_add_cards(explorer, grid, cards, sizeof(cards) / sizeof(cards[0]));
}

/* Insert a dialog block above the dialog's action row. */
static lv_obj_t *settings_dialog_block(lv_obj_t *dialog, lv_obj_t *actions)
{
    lv_obj_t *block = create_transparent_container(dialog);
    if (block == NULL) {
        return NULL;
    }
    lv_obj_move_to_index(block, lv_obj_get_index(actions));
    lv_obj_set_width(block, lv_pct(100));
    lv_obj_set_height(block, LV_SIZE_CONTENT);
    return block;
}

/* Choice dialog (web chooseOption): one tile per value in rows of three,
 * the current value selected; tapping a tile applies it to the unsaved form. */
static bool settings_open_choice(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t kind)
{
    static const furnace_hmi_ui_string_id_t titles[UI_LAB_SETTINGS_CHOICE_COUNT] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_WHEN_IDLE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DEFAULT_RUNNING_VIEW,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_THEME,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_LANGUAGE,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT,
    };
    static const furnace_hmi_ui_string_id_t details[UI_LAB_SETTINGS_CHOICE_COUNT] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DIM_CHOICE_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_VIEW_CHOICE_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_THEME_CHOICE_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_LANGUAGE_CHOICE_DETAIL,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_FORMAT_CHOICE_DETAIL,
    };
    if (kind >= UI_LAB_SETTINGS_CHOICE_COUNT) {
        return false;
    }
    lv_obj_t *dialog = NULL;
    lv_obj_t *actions = NULL;
    if (!settings_modal_shell(explorer, ui_string(titles[kind]), ui_string(details[kind]), &dialog, &actions)) {
        return false;
    }
    explorer->settings_choice_kind = kind;
    lv_obj_t *choices = settings_dialog_block(dialog, actions);
    if (choices == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_flex_flow(choices, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(choices, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_row(choices, 8, LV_PART_MAIN);
    lv_obj_update_layout(dialog);
    const int32_t width = (lv_obj_get_content_width(choices) - 16) / 3;
    for (uint8_t index = 0U; index < settings_choice_count(kind); ++index) {
        char text[48];
        bool selected;
        switch (kind) {
        case UI_LAB_SETTINGS_CHOICE_DIM:
            format_dim_minutes(text, sizeof(text), settings_dim_choices[index]);
            selected = explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM] ==
                       (int32_t)settings_dim_choices[index];
            break;
        case UI_LAB_SETTINGS_CHOICE_THEME:
            copy_text_bounded(text, sizeof(text),
                              ui_string(furnace_hmi_theme((furnace_hmi_theme_id_t)index)->name));
            selected = explorer->settings_candidate_theme == index;
            break;
        case UI_LAB_SETTINGS_CHOICE_LANGUAGE:
            /* Each language is named in its own language. */
            copy_text_bounded(text, sizeof(text), furnace_hmi_ui_language_name((furnace_hmi_ui_language_t)index));
            selected = explorer->settings_candidate_language == index;
            break;
        case UI_LAB_SETTINGS_CHOICE_DATE_FORMAT:
            /* Each format shows today's date written that way. */
            format_date_as(text, sizeof(text), &explorer->clock, index);
            selected = explorer->settings_candidate_date_format == index;
            break;
        default:
            copy_text_bounded(text, sizeof(text), settings_running_view_text(index));
            selected = explorer->settings_candidate_running_view == index;
            break;
        }
        lv_obj_t *choice = create_modal_button(explorer, choices, text,
                                               selected ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_TILE,
                                               UI_LAB_ACTION_SETTINGS_CHOOSE, index);
        if (choice == NULL) {
            clear_modal(explorer);
            return false;
        }
        lv_obj_set_size(choice, width, 56);
    }
    lv_obj_t *cancel = create_modal_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
                                           UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_CLOSE_MODAL, 0);
    if (cancel == NULL) {
        clear_modal(explorer);
        return false;
    }
    size_dialog_button(cancel);
    centre_lone_dialog_button(actions);
    return true;
}

/* Set date & time (web openClockDialog): a column per field with + above and
 * - below its value, a weekday preview, Cancel and Set date & time. The
 * request is separate from the preference Save; the controller answers it.
 * Every step rebuilds the dialog from explorer->clock_draft. */
static bool settings_open_clock(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_t *dialog = NULL;
    lv_obj_t *actions = NULL;
    if (!settings_modal_shell(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET_DATE_TIME),
                              "", &dialog, &actions)) {
        return false;
    }
    lv_obj_t *detail = lv_obj_get_child(dialog, 1);
    if (detail != NULL) {
        lv_obj_add_flag(detail, LV_OBJ_FLAG_HIDDEN);
    }
    const bool twenty_four_hour = explorer->settings_candidate_24_hour;
    const furnace_hmi_ui_lab_clock_t *draft = &explorer->clock_draft;
    const uint8_t field_count = twenty_four_hour ? UI_LAB_CLOCK_AM_PM : UI_LAB_CLOCK_FIELD_COUNT;
    lv_obj_t *picker = settings_dialog_block(dialog, actions);
    if (picker == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_flex_flow(picker, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(picker, 6, LV_PART_MAIN);
    static const furnace_hmi_ui_string_id_t names[] = {
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DAY, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MONTH,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_YEAR, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_HOUR,
        FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_MINUTE, FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_AM_PM,
    };
    for (uint8_t field = 0U; field < field_count; ++field) {
        char value[16];
        switch (field) {
        case UI_LAB_CLOCK_DAY:
            (void)snprintf(value, sizeof(value), "%u", (unsigned)draft->day);
            break;
        case UI_LAB_CLOCK_MONTH:
            (void)snprintf(value, sizeof(value), "%s", clock_month_text(draft->month));
            break;
        case UI_LAB_CLOCK_YEAR:
            (void)snprintf(value, sizeof(value), "%u", (unsigned)draft->year);
            break;
        case UI_LAB_CLOCK_HOUR:
            format_clock_hour(value, sizeof(value), draft->hour, twenty_four_hour);
            break;
        case UI_LAB_CLOCK_MINUTE:
            (void)snprintf(value, sizeof(value), "%02u", (unsigned)draft->minute);
            break;
        default:
            (void)snprintf(value, sizeof(value), "%s",
                           ui_string(draft->hour < 12U ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_AM
                                                       : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PM));
            break;
        }
        lv_obj_t *column = create_transparent_container(picker);
        if (column == NULL) {
            clear_modal(explorer);
            return false;
        }
        lv_obj_set_size(column, 0, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(column, 1);
        lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(column, 4, LV_PART_MAIN);
        lv_obj_t *name = create_label(column, ui_string(names[field]), &furnace_hmi_font_regular_13, UI_LAB_MUTED);
        lv_obj_t *up = create_modal_button(explorer, column, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE),
                                           UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_CLOCK_STEP,
                                           (int)((field << 1U) | 1U));
        lv_obj_t *well = lv_obj_create(column);
        lv_obj_t *down = create_modal_button(explorer, column, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DECREASE),
                                             UI_LAB_TONE_TILE, UI_LAB_ACTION_SETTINGS_CLOCK_STEP,
                                             (int)(field << 1U));
        if (name == NULL || up == NULL || well == NULL || down == NULL) {
            clear_modal(explorer);
            return false;
        }
        lv_obj_set_width(name, lv_pct(100));
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_size(up, lv_pct(100), 44);
        lv_obj_set_size(down, lv_pct(100), 44);
        lv_obj_set_style_min_height(up, 0, LV_PART_MAIN);
        lv_obj_set_style_min_height(down, 0, LV_PART_MAIN);
        /* Value well: recessed face, soft amber bottom edge (web `.clock-column strong`). */
        lv_obj_set_size(well, lv_pct(100), 44);
        lv_obj_set_style_radius(well, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(well, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(well, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(well, UI_LAB_THEME_WELL, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(well, LV_GRAD_DIR_NONE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(well, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(well, 2, LV_PART_MAIN);
        lv_obj_set_style_border_side(well, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
        ui_lab_theme_border(well, FURNACE_HMI_COLOR_AMBER_SOFT, LV_PART_MAIN);
        remove_scroll(well);
        lv_obj_t *text = create_label(well, value, &furnace_hmi_font_semibold_20, UI_LAB_TEXT);
        if (text == NULL) {
            clear_modal(explorer);
            return false;
        }
        lv_obj_center(text);
    }
    char time[24];
    char date[48];
    char preview[96];
    format_clock_time(time, sizeof(time), draft, twenty_four_hour);
    format_clock_date(explorer, date, sizeof(date), draft);
    (void)snprintf(preview, sizeof(preview), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DATE_TIME_PREVIEW_FORMAT),
                   date, time);
    lv_obj_t *preview_block = settings_dialog_block(dialog, actions);
    lv_obj_t *preview_label = preview_block == NULL ? NULL
        : create_label(preview_block, preview, &furnace_hmi_font_regular_15, UI_LAB_TEXT);
    lv_obj_t *cancel = create_modal_button(explorer, actions, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CANCEL),
                                           UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_CLOSE_MODAL, 0);
    lv_obj_t *set = create_modal_button(explorer, actions,
                                        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SET_DATE_TIME),
                                        UI_LAB_TONE_PRIMARY, UI_LAB_ACTION_SETTINGS_CLOCK_SET, 0);
    if (preview_label == NULL || cancel == NULL || set == NULL) {
        clear_modal(explorer);
        return false;
    }
    lv_obj_set_width(preview_label, lv_pct(100));
    lv_obj_set_style_text_align(preview_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    size_dialog_button(cancel);
    size_dialog_button(set);
    return true;
}

static bool build_settings(furnace_hmi_ui_lab_explorer_t *explorer)
{
    lv_obj_set_flex_flow(explorer->content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(explorer->content, UI_LAB_SPACE, LV_PART_MAIN);
    lv_obj_set_style_pad_row(explorer->content, 0, LV_PART_MAIN);

    lv_obj_t *rail = create_transparent_container(explorer->content);
    lv_obj_t *body = create_transparent_container(explorer->content);
    if (rail == NULL || body == NULL) {
        return false;
    }
    lv_obj_set_width(rail, nav_column_width(explorer));
    lv_obj_set_height(rail, lv_pct(100));
    lv_obj_set_flex_flow(rail, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(rail, 6, LV_PART_MAIN);
    lv_obj_set_width(body, 0);
    lv_obj_set_height(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(body, 8, LV_PART_MAIN);

    if (!settings_tab_available(explorer, explorer->settings_tab)) {
        explorer->settings_tab = UI_LAB_SETTINGS_TAB_DISPLAY;
    }
    for (uint8_t tab = 0U; tab < UI_LAB_SETTINGS_TAB_COUNT; ++tab) {
        if (!settings_tab_available(explorer, tab)) {
            continue;
        }
        if (create_settings_category_button(explorer, rail, tab) == NULL) {
            return false;
        }
    }
    if (create_label(body, ui_string(settings_tab_ids[explorer->settings_tab]),
                     &furnace_hmi_font_semibold_20, UI_LAB_TEXT) == NULL ||
        create_label(body, settings_tab_subtitle(explorer->settings_tab),
                     &furnace_hmi_font_regular_13, UI_LAB_MUTED) == NULL) {
        return false;
    }
    switch (explorer->settings_tab) {
    case UI_LAB_SETTINGS_TAB_DISPLAY:
        return settings_build_display(explorer, body);
    case UI_LAB_SETTINGS_TAB_TIME:
        return settings_build_time(explorer, body);
    case UI_LAB_SETTINGS_TAB_CONNECTIONS:
        return settings_build_connections(explorer, body);
    case UI_LAB_SETTINGS_TAB_SYSTEM:
        return settings_build_system(explorer, body);
    default:
        return false;
    }
}

static bool build_page_body(furnace_hmi_ui_lab_explorer_t *explorer, furnace_hmi_ui_lab_page_t page);

static bool rebuild_page(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->root == NULL) {
        return false;
    }
    clear_modal(explorer);
    if (explorer->stage_dialog != NULL && lv_obj_is_valid(explorer->stage_dialog)) {
        lv_obj_delete(explorer->stage_dialog);
    }
    explorer->stage_dialog = NULL;
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR && explorer->stage_session_open) {
        stage_session_sync(explorer);
    }
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
    explorer->manual_readings = NULL;
    explorer->manual_marker = NULL;
    explorer->manual_marker_index = -1;
    explorer->trajectory_legend_label = NULL;
    explorer->cooling_toggle = NULL;
    explorer->measured_cooling_point_count = 0U;
    explorer->trajectory_time_limit_seconds = 0;
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
    /* The stage editor shows the page it was opened from beneath its dialog. */
    const furnace_hmi_ui_lab_page_t shown = explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR
        ? explorer->stage_editor_origin : explorer->page;
    if (explorer->nav != NULL) {
        update_nav_selection(explorer);
        if (shown == FURNACE_HMI_UI_LAB_PAGE_RUNNING ||
            (shown == FURNACE_HMI_UI_LAB_PAGE_MANUAL && (explorer->manual_running || explorer->manual_cooling)) ||
            shown == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR ||
            shown == FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR) {
            lv_obj_add_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(explorer->nav, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_to_index(explorer->nav, -1);
        }
    }

    lv_obj_update_layout(explorer->root);
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
        if (shown == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR || !build_page_body(explorer, shown)) {
            return false;
        }
        lv_obj_update_layout(explorer->root);
        return build_stage_dialog(explorer);
    }
    return build_page_body(explorer, explorer->page);
}

static bool build_page_body(furnace_hmi_ui_lab_explorer_t *explorer, furnace_hmi_ui_lab_page_t page)
{
   switch (page) {
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
    case FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR:
        return false;
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
    ui_lab_theme_light_strip(explorer->state_strip_line, explorer->state_strip_glow,
                             explorer->manual_light_on);
    return true;
}

/* The shown run is one the operator stopped (web demoRun.stopped). */
static bool run_was_stopped(const furnace_hmi_ui_lab_explorer_t *explorer)
{
    const furnace_hmi_dashboard_state_t *state = explorer->state;
    return explorer->run_stopped && state != NULL && state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM &&
        state->program_id == explorer->stopped_program_id;
}

/* Hand an operator request to the host for the controller (web controllerRequest). */
static void post_request(furnace_hmi_ui_lab_explorer_t *explorer, furnace_hmi_ui_lab_request_kind_t kind)
{
    memset(&explorer->request, 0, sizeof(explorer->request));
    explorer->request.kind = kind;
    if (kind == FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM) {
        copy_text_bounded(explorer->request.program_name, sizeof(explorer->request.program_name),
                          program_name_text(explorer, explorer->selected_program));
        explorer->request.program = *selected_program_entry(explorer);
        explorer->running_program = explorer->selected_program;
    }
}

static bool text_is_blank(const char *text)
{
    for (; text != NULL && *text != '\0'; ++text) {
        if (!isspace((unsigned char)*text)) {
            return false;
        }
    }
    return true;
}

/* Web programEditorErrors: the first problem with the open program draft. */
static bool program_editor_error(const furnace_hmi_ui_lab_explorer_t *explorer, char *buffer, size_t capacity)
{
    buffer[0] = '\0';
    const size_t count = editor_stage_count(explorer);
    int64_t total = 0;
    for (size_t index = 0U; index < count; ++index) {
        const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
        total += stage.duration_minutes > 0 ? stage.duration_minutes : 0;
    }
    furnace_hmi_ui_string_id_t problem = FURNACE_HMI_UI_STRING_COUNT;
    if (text_is_blank(explorer->program_editor_name)) {
        problem = FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_NAME;
    }
    else if (count == 0U) {
        problem = FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_NO_STAGES;
    }
    else if (total > 48 * 60) {
        problem = FURNACE_HMI_UI_STRING_UI_LAB_STAGE_ERROR_TOTAL;
    }
    if (problem != FURNACE_HMI_UI_STRING_COUNT) {
        (void)snprintf(buffer, capacity, "%s", ui_string(problem));
        return true;
    }
    for (size_t index = 0U; index < count; ++index) {
        const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
        const bool heating = fixture_stage_kind(&stage) == UI_LAB_DRAFT_STAGE_HEATING;
        if (stage.target_c < 0 || stage.target_c > UI_LAB_MAX_PROGRAM_TEMPERATURE_C) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_TARGET_FORMAT;
        }
        else if (heating && (stage.delta_c_per_minute < 1 || stage.delta_c_per_minute > UI_LAB_MAX_PROGRAM_RATE_TENTHS)) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_RATE_FORMAT;
        }
        else if (stage.duration_minutes < 1) {
            problem = FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_ERROR_DURATION_FORMAT;
        }
        if (problem != FURNACE_HMI_UI_STRING_COUNT) {
            (void)snprintf(buffer, capacity, ui_string(problem), (unsigned int)index + 1U);
            return true;
        }
    }
    return false;
}

/* Save the program editor's draft into the library (web saveProgramEditor):
 * a new program takes a free slot, an edited one keeps its place. Opens the
 * saved program's details. */
static void commit_program_editor(furnace_hmi_ui_lab_explorer_t *explorer)
{
    char error[112];
    if (program_editor_error(explorer, error, sizeof(error))) {
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_NOT_ACCEPTED), error,
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
        return;
    }
    size_t slot = explorer->selected_program;
    if (explorer->program_editor_new) {
        slot = FURNACE_HMI_UI_LAB_MAX_PROGRAMS;
        for (size_t index = 0U; index < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++index) {
            if (!explorer->programs[index].present) {
                slot = index;
                break;
            }
        }
        if (slot >= FURNACE_HMI_UI_LAB_MAX_PROGRAMS) {
            (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_NOT_ACCEPTED),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_LIBRARY_FULL),
                              UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
            return;
        }
    }
    furnace_hmi_ui_lab_program_t saved;
    memset(&saved, 0, sizeof(saved));
    saved.present = true;
    saved.library = !explorer->program_editor_new && explorer->programs[slot].library;
    saved.name_string = saved.library ? explorer->programs[slot].name_string : 0U;
    const size_t count = editor_stage_count(explorer);
    saved.stage_count = (uint8_t)(count > FURNACE_HMI_UI_LAB_MAX_STAGES ? FURNACE_HMI_UI_LAB_MAX_STAGES : count);
    for (size_t index = 0U; index < saved.stage_count; ++index) {
        const ui_lab_stage_fixture_t stage = program_stage(explorer, index);
        const bool hold = fixture_stage_kind(&stage) == UI_LAB_DRAFT_STAGE_HOLD;
        saved.stages[index] = (furnace_hmi_ui_lab_program_stage_t) {
            hold ? FURNACE_HMI_UI_LAB_STAGE_HOLD : FURNACE_HMI_UI_LAB_STAGE_HEATING,
            stage.target_c, hold ? 0 : stage.delta_c_per_minute, stage.duration_minutes
        };
    }
    char name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    copy_text_bounded(name, sizeof(name), explorer->program_editor_name);
    explorer->programs[slot] = saved;
    /* A library program keeps its catalogue name unless the operator renamed it. */
    if (saved.library && strcmp(name, ui_string((furnace_hmi_ui_string_id_t)saved.name_string)) == 0) {
        explorer->program_name_overrides[slot][0] = '\0';
    }
    else {
        copy_text_bounded(explorer->program_name_overrides[slot], FURNACE_HMI_UI_LAB_NAME_CAPACITY, name);
    }
    log_add_named(explorer, UI_LAB_LOG_PROGRAM_SAVED, program_name_text(explorer, slot));
    clear_staged_session(explorer);
    clear_modal(explorer);
    explorer->selected_program = (uint8_t)slot;
    explorer->program_stage_page = 0U;
    explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL;
    (void)rebuild_page(explorer);
}

/* Remove a program from the library, its favourite and its recent entry (the
 * controller's answer to a delete request; the simulated one accepts it). */
static bool remove_program(furnace_hmi_ui_lab_explorer_t *explorer, uint8_t program)
{
    if (!program_listed(explorer, program)) {
        return false;
    }
    log_add_named(explorer, UI_LAB_LOG_PROGRAM_DELETED, program_name_text(explorer, program));
    if (program_is_favourite(explorer, program)) {
        (void)toggle_program_favourite(explorer, program);
    }
    size_t kept = 0U;
    for (size_t index = 0U; index < explorer->recent_count; ++index) {
        if (explorer->quick_launch_recent[index] != program) {
            explorer->quick_launch_recent[kept++] = explorer->quick_launch_recent[index];
        }
    }
    explorer->recent_count = (uint8_t)kept;
    memset(&explorer->programs[program], 0, sizeof(explorer->programs[program]));
    explorer->program_name_overrides[program][0] = '\0';
    if (explorer->selected_program == program) {
        explorer->selected_program = (uint8_t)listed_program_at(explorer, 0U) % FURNACE_HMI_UI_LAB_MAX_PROGRAMS;
    }
    explorer->program_page = 0U;
    return true;
}

/* Delete on Program details: remove it and show the library. */
static void delete_selected_program(furnace_hmi_ui_lab_explorer_t *explorer)
{
    const uint8_t program = explorer->selected_program;
    if (!program_listed(explorer, program)) {
        return;
    }
    char detail[96];
    (void)snprintf(detail, sizeof(detail), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DELETED_DETAIL_FORMAT),
                   program_name_text(explorer, program));
    (void)remove_program(explorer, program);
    clear_modal(explorer);
    explorer->page = FURNACE_HMI_UI_LAB_PAGE_PROGRAMS;
    (void)rebuild_page(explorer);
    (void)settings_show_message(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DELETED), detail);
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
        : (uint8_t)program_authored_stage_count(selected_program_entry(explorer));
    copy_text_bounded(
        explorer->program_editor_name,
        sizeof(explorer->program_editor_name),
        new_program ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NEW_PROGRAM_NAME)
                    : program_name_text(explorer, explorer->selected_program));
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
        if (value >= 0 && program_listed(explorer, (size_t)value)) {
            explorer->selected_program = (uint8_t)value;
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING);
        break;
    case UI_LAB_ACTION_CONFIRM_START: {
        /* Program details ask before requesting a start (web confirmStart). */
        const furnace_hmi_ui_lab_program_t *program = selected_program_entry(explorer);
        char title[96];
        char detail[96];
        (void)snprintf(title, sizeof(title),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM_TITLE_FORMAT),
                       program_name_text(explorer, explorer->selected_program));
        (void)snprintf(detail, sizeof(detail),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM_DETAIL_FORMAT),
                       (unsigned int)program_authored_stage_count(program),
                       program_duration(program), program_maximum(program));
        (void)open_dialog(explorer, title, detail, UI_LAB_ACTION_RUNNING,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM), UI_LAB_TONE_PRIMARY);
        break;
    }
    case UI_LAB_ACTION_RUNNING:
        /* Start request; Running opens when the controller reports the run. */
        clear_modal(explorer);
        if (program_listed(explorer, explorer->selected_program)) {
            record_recent_program(explorer, explorer->selected_program);
            post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM);
        }
        break;
    case UI_LAB_ACTION_PROGRAM_DETAIL:
        if (value >= 0 && program_listed(explorer, (size_t)value)) {
            explorer->selected_program = (uint8_t)value;
        }
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL);
        break;
    case UI_LAB_ACTION_PROGRAM_STAGES:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES);
        break;
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
        log_add_named(explorer, UI_LAB_LOG_MANUAL_STARTED, "");
        explorer->manual_running = true;
        explorer->manual_paused = false;
        (void)update_manual_run_state(explorer);
        break;
    case UI_LAB_ACTION_MANUAL_STOP:
        /* The controller stops heating and cools automatically; the session stays on screen. */
        log_add_named(explorer, UI_LAB_LOG_MANUAL_STOPPED, "");
        explorer->manual_running = false;
        explorer->manual_paused = false;
        explorer->manual_cooling = true;
        explorer->manual_cooling_seconds = 0U;
        (void)update_manual_run_state(explorer);
        break;
    case UI_LAB_ACTION_MANUAL_EXIT:
        explorer->manual_cooling = false;
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_HOME);
        break;
    case UI_LAB_ACTION_MANUAL_NEW:
        /* A new session on the Manual page with the same target and rate. */
        explorer->manual_cooling = false;
        (void)rebuild_page(explorer);
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
    case UI_LAB_ACTION_TOGGLE_COOLING:
        /* Show all measured cooling, or return to the start-of-cooling window. */
        explorer->running_cooling_full = !explorer->running_cooling_full;
        explorer->trajectory_domain_initialized = false;
        (void)rebuild_page(explorer);
        if (explorer->trajectory != NULL) {
            (void)update_trajectory(explorer);
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
    case UI_LAB_ACTION_DEVICE_MAINTENANCE_DONE:
        if (value >= 0) {
            device_complete_task(explorer, (uint8_t)((unsigned)value >> 8), (uint8_t)((unsigned)value & 0xFFU));
        }
        break;
    case UI_LAB_ACTION_DEVICE_HISTORY:
        (void)device_show_history(explorer, (uint8_t)value);
        break;
    case UI_LAB_ACTION_DEVICE_LOG_EVENT:
        if (value >= 0 && value < (int)explorer->log_count) {
            const furnace_hmi_ui_lab_log_event_t *event = &explorer->log_events[value];
            const furnace_hmi_ui_lab_clock_t at = civil_from_minute(event->minute);
            char text[2 * FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
            char date[48];
            char time_text[24];
            char detail[160];
            log_text(explorer, event, text, sizeof(text));
            format_short_date(explorer, date, sizeof(date), &at);
            log_time_text(explorer, event->minute, time_text, sizeof(time_text));
            (void)snprintf(detail, sizeof(detail), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_LOG_EVENT_DETAIL_FORMAT),
                           date, time_text,
                           ui_string(log_types[event->type % UI_LAB_LOG_TYPE_COUNT].display
                                         ? FURNACE_HMI_UI_STRING_UI_LAB_SOURCE_HMI
                                         : FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_CONTROL_CPU));
            (void)settings_show_message(explorer, text, detail);
        }
        break;
    case UI_LAB_ACTION_LOG_FILTER:
        if (value >= 0 && value < UI_LAB_LOG_FILTER_COUNT) {
            explorer->log_filter = (uint8_t)value;
            explorer->device_page = 0U;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_DEVICE_LOG_ALARMS:
        explorer->device_tab = UI_LAB_DEVICE_TAB_COMPONENTS;
        explorer->log_filter = UI_LAB_LOG_ALARMS;
        explorer->device_page = 0U;
        explorer->device_component_selected = false;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS:
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_SETTINGS);
        break;
    case UI_LAB_ACTION_SETTINGS_TAB:
        if (value < 0 || !settings_tab_available(explorer, (uint8_t)value) ||
            (uint8_t)value == explorer->settings_tab) {
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
    case UI_LAB_ACTION_SETTINGS_BRIGHTNESS_SET:
    case UI_LAB_ACTION_SETTINGS_BRIGHTNESS_STEP: {
        int32_t *brightness = &explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS];
        const int32_t next = action == UI_LAB_ACTION_SETTINGS_BRIGHTNESS_SET
            ? value : *brightness + value * UI_LAB_SETTINGS_BRIGHTNESS_STEP;
        *brightness = next < UI_LAB_SETTINGS_BRIGHTNESS_MIN ? UI_LAB_SETTINGS_BRIGHTNESS_MIN
            : next > UI_LAB_SETTINGS_BRIGHTNESS_MAX ? UI_LAB_SETTINGS_BRIGHTNESS_MAX : next;
        (void)rebuild_page(explorer);
        break;
    }
    case UI_LAB_ACTION_SETTINGS_TOGGLE_MOTION:
        explorer->settings_candidate_reduced_motion = !explorer->settings_candidate_reduced_motion;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_CHOICE:
        if (value >= 0 && value < UI_LAB_SETTINGS_CHOICE_COUNT) {
            (void)settings_open_choice(explorer, (uint8_t)value);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_CHOOSE:
        if (value >= 0) {
            settings_apply_choice(explorer, explorer->settings_choice_kind, (uint8_t)value);
        }
        clear_modal(explorer);
        (void)settings_apply_presentation(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_TOGGLE_TIME_FORMAT:
        explorer->settings_candidate_24_hour = !explorer->settings_candidate_24_hour;
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_CLOCK:
        explorer->clock_draft = explorer->clock;
        (void)settings_open_clock(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_CLOCK_STEP:
        if (value >= 0) {
            clock_step(&explorer->clock_draft, (uint8_t)((unsigned)value >> 1),
                       ((unsigned)value & 1U) != 0U ? 1 : -1);
            (void)settings_open_clock(explorer);
        }
        break;
    case UI_LAB_ACTION_SETTINGS_CLOCK_SET:
        /* The simulated controller accepts every request. */
        explorer->clock = explorer->clock_draft;
        clear_modal(explorer);
        (void)rebuild_page(explorer);
        (void)settings_show_message(
            explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_TIME_SET_TITLE),
            ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_DATE_TIME_SET_DETAIL));
        break;
    case UI_LAB_ACTION_SETTINGS_SHOW_CONNECTION_SETUP: {
        char title[64];
        (void)snprintf(title, sizeof(title), ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP_TITLE_FORMAT),
                       ui_string(value == 0 ? FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SERVER
                                            : FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_BLUETOOTH));
        (void)settings_show_message(
            explorer,
            title,
            value == 0 ? ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP_SERVER)
                       : ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SETUP_BLUETOOTH));
        break;
    }
    case UI_LAB_ACTION_SETTINGS_SAVE:
        settings_copy_candidate_to_saved(explorer);
        log_add_named(explorer, UI_LAB_LOG_SETTINGS_SAVED, "");
        (void)rebuild_page(explorer);
        (void)settings_show_message(explorer,
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVED_TITLE),
                                     ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_SAVED_DETAIL));
        break;
    case UI_LAB_ACTION_SETTINGS_DISCARD:
        settings_restore_saved(explorer);
        (void)settings_apply_presentation(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_UNSAVED_STAY:
        clear_modal(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_DISCARD_TO_TAB:
        settings_restore_saved(explorer);
        clear_modal(explorer);
        (void)settings_apply_presentation(explorer);
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
        log_add_named(explorer, UI_LAB_LOG_SETTINGS_SAVED, "");
        clear_modal(explorer);
        if ((uint8_t)value == UINT8_MAX) {
            (void)furnace_hmi_ui_lab_explorer_navigate(explorer, FURNACE_HMI_UI_LAB_PAGE_DEVICE);
        }
        else {
            explorer->settings_tab = (uint8_t)value;
            (void)rebuild_page(explorer);
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
            explorer->settings_candidate_numeric[UI_LAB_SETTINGS_NUMBER_DIM] = 0;
            explorer->settings_candidate_reduced_motion = false;
            explorer->settings_candidate_running_view = 1U;
            explorer->settings_candidate_theme = 0U;
            explorer->settings_candidate_language = 0U;
        }
        if (scope == 1U || scope == 2U) {
            explorer->settings_candidate_24_hour = true;
            explorer->settings_candidate_date_format = 0U;
        }
        settings_copy_candidate_to_saved(explorer);
        log_add_named(explorer, UI_LAB_LOG_SETTINGS_SAVED, "");
        clear_modal(explorer);
        (void)settings_apply_presentation(explorer);
        (void)settings_show_message(explorer,
                                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_TITLE),
                                    ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESET_COMPLETE));
        break;
    }
    case UI_LAB_ACTION_SETTINGS_SHOW_DELETE_PROGRAMS:
        (void)settings_open_delete_dialog(explorer);
        break;
    case UI_LAB_ACTION_SETTINGS_DELETE_CONFIRM: {
        /* The simulated controller accepts the deletion. */
        unsigned removed = 0U;
        for (size_t slot = 0U; slot < FURNACE_HMI_UI_LAB_MAX_PROGRAMS; ++slot) {
            if ((explorer->settings_delete_pending & (1U << slot)) != 0U && remove_program(explorer, (uint8_t)slot)) {
                ++removed;
            }
        }
        explorer->settings_delete_pending = 0U;
        char detail[96];
        (void)snprintf(detail, sizeof(detail),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAMS_DELETED_DETAIL_FORMAT), removed);
        clear_modal(explorer);
        (void)rebuild_page(explorer);
        (void)settings_show_message(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_PROGRAMS_DELETED), detail);
        break;
    }
    case UI_LAB_ACTION_SETTINGS_SHOW_RESTART_DISPLAY:
        (void)open_dialog(explorer,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_TITLE),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_DETAIL),
                          UI_LAB_ACTION_SETTINGS_RESTART_DISPLAY_CONFIRM,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_RESTART_DISPLAY_CONFIRM),
                          UI_LAB_TONE_DANGER);
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
            (listed_program_count(explorer) + 5U) / 6U) {
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
            (program_authored_stage_count(selected_program_entry(explorer)) + 8U) / 9U) {
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
        if (explorer->selected_stage + 1U < stage_session_count(explorer)) {
            store_selected_stage_draft(explorer);
            load_stage_draft(explorer, explorer->selected_stage + 1U);
        }
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_SHOW_NOTICES:
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_NOTIFICATIONS),
                          "",
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
        break;
    case UI_LAB_ACTION_SHOW_SERVICE:
        (void)furnace_hmi_ui_lab_explorer_show_service_warning(explorer);
        break;
    case UI_LAB_ACTION_SHOW_DELETE:
    {
        char detail[96];
        (void)snprintf(detail, sizeof(detail),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_PROGRAM_DETAIL_FORMAT),
                       program_name_text(explorer, explorer->selected_program));
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_CONFIRM_DELETE),
                          detail, UI_LAB_ACTION_DELETE_REQUESTED,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT), UI_LAB_TONE_DANGER);
        break;
    }
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
        /* Web saveProgramEditor: Save program saves at once. */
        if (explorer->program_editor_active) {
            commit_program_editor(explorer);
        }
        break;
    case UI_LAB_ACTION_LEAVE_PROGRAM_EDITOR:
        if (explorer->program_editor_dirty) {
            (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_UNSAVED_PROGRAM_DETAIL),
                              UI_LAB_ACTION_CONFIRM_DISCARD,
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_TONE_DANGER);
        }
        else if (explorer->program_editor_active) {
            const furnace_hmi_ui_lab_page_t return_page = explorer->program_editor_return_page;
            clear_staged_session(explorer);
            explorer->page = return_page;
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_SHOW_DELETE_STAGE: {
        char title[48];
        (void)snprintf(title, sizeof(title),
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_STAGE_TITLE_FORMAT),
                       (unsigned int)explorer->selected_stage + 1U);
        (void)open_dialog(explorer, title, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_STAGE_DETAIL),
                          UI_LAB_ACTION_DELETE_STAGE, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_STAGE),
                          UI_LAB_TONE_DANGER);
        break;
    }
    case UI_LAB_ACTION_DELETE_STAGE:
        clear_modal(explorer);
        stage_session_delete(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_ADD_STAGE_AFTER:
        stage_session_add_after(explorer);
        (void)rebuild_page(explorer);
        break;
    case UI_LAB_ACTION_EDIT_STAGE_POSITION:
        (void)keyboard_open_number(
            explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_MOVE_STAGE_TITLE),
            (int32_t)explorer->selected_stage + 1, UI_LAB_KEYBOARD_TARGET_STAGE_POSITION,
            explorer->selected_stage, 1, (int32_t)explorer->staged_stage_count, 1, false);
        break;
    case UI_LAB_ACTION_SHOW_DISCARD:
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_STAGE_TITLE),
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_STAGE_DETAIL),
                              UI_LAB_ACTION_CONFIRM_DISCARD,
                              ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_TONE_DANGER);
            break;
        }
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES),
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_DETAIL),
                          UI_LAB_ACTION_CONFIRM_DISCARD,
                          ui_string(FURNACE_HMI_UI_STRING_UI_LAB_DISCARD_CHANGES), UI_LAB_TONE_DANGER);
        break;
    case UI_LAB_ACTION_SHOW_RUN_DETAILS: {
        char detail[256];
        snprintf(detail, sizeof(detail), "%s\n%s", explorer->state->program_name, explorer->state->stage_name);
        (void)open_dialog(explorer, ui_string(FURNACE_HMI_UI_STRING_UI_LAB_RUN_DETAILS),
                          detail,
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
        break;
    }
    case UI_LAB_ACTION_ADD_STAGE:
        if (explorer->program_editor_active) {
            if (explorer->staged_stage_count < FURNACE_HMI_UI_LAB_MAX_STAGES) {
                explorer->program_editor_dirty = true;
                explorer->selected_stage = explorer->staged_stage_count;
                explorer->staged_stage_count++;
                explorer->draft_target_c = 120;
                explorer->draft_rate_tenths_c_per_minute = 10;
                explorer->draft_duration_minutes = 95;
                explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
                explorer->draft_cooling_rate_enabled = false;
                explorer->staged_name_valid[explorer->selected_stage] = false;
                store_selected_stage_draft(explorer);
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
        explorer->draft_target_c = program_stage(explorer, 0U).target_c;
        explorer->draft_rate_tenths_c_per_minute = program_stage(explorer, 0U).delta_c_per_minute;
        explorer->draft_duration_minutes = program_stage(explorer, 0U).duration_minutes;
        explorer->draft_stage_kind = UI_LAB_DRAFT_STAGE_HEATING;
        explorer->draft_cooling_rate_enabled = false;
        explorer->draft_heating_duration_drives_rate = false;
        update_heating_relationship(explorer);
        (void)furnace_hmi_ui_lab_explorer_navigate(explorer,
                                                    FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR);
        break;
    case UI_LAB_ACTION_RETURN_HOME:
        /* A completed or stopped run is closed by the controller; Home follows its report. */
        if (explorer->state->run_state == FURNACE_HMI_RUN_STATE_COMPLETE || run_was_stopped(explorer)) {
            explorer->run_stopped = false;
            post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_CLOSE_RUN);
            break;
        }
        /* Only a run the controller has ended (fault) may be left this way. */
        if (explorer->state->run_state == FURNACE_HMI_RUN_STATE_FAULT) {
            clear_modal(explorer);
            explorer->page = FURNACE_HMI_UI_LAB_PAGE_HOME;
            explorer->trajectory_domain_initialized = false;
            update_nav_selection(explorer);
            (void)rebuild_page(explorer);
        }
        break;
    case UI_LAB_ACTION_PAUSE_PREVIEW:
        post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_PAUSE);
        break;
    case UI_LAB_ACTION_RESUME_PREVIEW:
        post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_RESUME);
        break;
    case UI_LAB_ACTION_STOP_PREVIEW:
        /* A program run is cut where it stands and cools on Running (web stopProgramRun). */
        clear_modal(explorer);
        if (explorer->state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && !run_was_stopped(explorer) &&
            (explorer->state->run_state == FURNACE_HMI_RUN_STATE_RUNNING ||
             explorer->state->run_state == FURNACE_HMI_RUN_STATE_PAUSED)) {
            explorer->run_stopped = true;
            explorer->stopped_program_id = explorer->state->program_id;
            log_add_named(explorer, UI_LAB_LOG_PROGRAM_STOPPED, explorer->state->program_name);
        }
        post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_STOP);
        break;
    case UI_LAB_ACTION_START_AGAIN:
        /* The same program from the start (web startProgramAgain). */
        if (run_was_stopped(explorer) && program_listed(explorer, explorer->running_program)) {
            explorer->selected_program = explorer->running_program;
            explorer->run_stopped = false;
            record_recent_program(explorer, explorer->selected_program);
            post_request(explorer, FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM);
        }
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
                              UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
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
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR && explorer->stage_session_open) {
            close_stage_session(explorer, false);
            break;
        }
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
        if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR && explorer->stage_session_open) {
            /* Web saveStageEditor: saved directly; Save is disabled while any
             * stage is invalid. */
            char error[112];
            stage_session_sync(explorer);
            if (!stage_session_error(explorer, error, sizeof(error))) {
                close_stage_session(explorer, true);
            }
            break;
        }
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
                          UI_LAB_ACTION_CLOSE_MODAL, NULL, UI_LAB_TONE_PRIMARY);
        break;
    case UI_LAB_ACTION_DELETE_REQUESTED:
        delete_selected_program(explorer);
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
    /* A Program run that ended in a fault opens on Running so the graph shows where it stopped. */
    explorer->page = ((state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM ||
                       state->mode == FURNACE_HMI_MACHINE_MODE_MANUAL) &&
                      (state->run_state == FURNACE_HMI_RUN_STATE_RUNNING || state->run_state == FURNACE_HMI_RUN_STATE_PAUSED)) ||
        (state->mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && state->run_state == FURNACE_HMI_RUN_STATE_FAULT)
        ? FURNACE_HMI_UI_LAB_PAGE_RUNNING : FURNACE_HMI_UI_LAB_PAGE_HOME;
    load_library_programs(explorer);
    explorer->running_view_mode = 1U;
    explorer->settings_tab = UI_LAB_SETTINGS_TAB_DISPLAY;
    explorer->settings_pending_tab = UI_LAB_SETTINGS_TAB_DISPLAY;
    explorer->clock = (furnace_hmi_ui_lab_clock_t){ 2026U, 9U, 14U, 14U, 32U };
    explorer->clock_draft = explorer->clock;
    explorer->settings_connections_enabled = false;
    /* The controller's event log, and this display starting. */
    log_build(explorer);
    log_add_named(explorer, UI_LAB_LOG_DISPLAY_STARTED, "");
    explorer->settings_restart_assessment = 0U;
    explorer->settings_restart_result = 0U;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_RATE] = UI_LAB_MAX_PROGRAM_RATE_TENTHS;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_TEMPERATURE] = UI_LAB_MAX_PROGRAM_TEMPERATURE_C;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_DURATION] = 720;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_BRIGHTNESS] = 80;
    explorer->settings_saved_numeric[UI_LAB_SETTINGS_NUMBER_DIM] = 0;
    memcpy(explorer->settings_candidate_numeric, explorer->settings_saved_numeric,
           sizeof(explorer->settings_candidate_numeric));
    explorer->settings_saved_running_view = 1U;
    explorer->settings_candidate_running_view = 1U;
    explorer->settings_saved_24_hour = true;
    explorer->settings_candidate_24_hour = true;
    /* Theme and language are global: start from what is shown now. */
    explorer->settings_saved_theme = (uint8_t)ui_lab_theme_selected();
    explorer->settings_candidate_theme = explorer->settings_saved_theme;
    explorer->settings_saved_language = (uint8_t)furnace_hmi_ui_language();
    explorer->settings_candidate_language = explorer->settings_saved_language;
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
    {
        const ui_lab_stage_fixture_t initial = program_stage(explorer, 2U);
        explorer->draft_target_c = initial.target_c;
        explorer->draft_rate_tenths_c_per_minute = initial.delta_c_per_minute;
        explorer->draft_duration_minutes = initial.duration_minutes;
        explorer->draft_stage_kind = (uint8_t)fixture_stage_kind(&initial);
    }
    /* Web demoRecent: the programs run most recently, newest first. */
    static const uint8_t initial_recent[] = { 1U, 0U };
    memcpy(explorer->quick_launch_recent, initial_recent, sizeof(initial_recent));
    explorer->recent_count = (uint8_t)sizeof(initial_recent);

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
    lv_obj_set_style_radius(explorer->root, 0, LV_PART_MAIN);
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
    lv_obj_set_size(explorer->state_strip_line, parent_width, 3);
    lv_obj_set_pos(explorer->state_strip_line, 0, 0);
    lv_obj_set_style_border_width(explorer->state_strip_line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(explorer->state_strip_line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->state_strip_line, 0, LV_PART_MAIN);
    remove_scroll(explorer->state_strip_line);
    /* Invisible centred bar whose soft shadow is the light-on glow. */
    explorer->state_strip_glow = lv_obj_create(explorer->state_strip);
    if (explorer->state_strip_glow == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_size(explorer->state_strip_glow, parent_width / 2, 1);
    lv_obj_set_pos(explorer->state_strip_glow, parent_width / 4, 0);
    lv_obj_set_style_radius(explorer->state_strip_glow, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(explorer->state_strip_glow, 0, LV_PART_MAIN);
    lv_obj_remove_flag(explorer->state_strip_glow, LV_OBJ_FLAG_CLICKABLE);
    remove_scroll(explorer->state_strip_glow);
    explorer->header_actions[0].explorer = explorer;
    explorer->header_actions[0].action = UI_LAB_ACTION_TOGGLE_LIGHT;
    explorer->header_actions[0].value = 0;
    lv_obj_add_event_cb(explorer->state_strip, action_callback, LV_EVENT_CLICKED,
                        &explorer->header_actions[0]);

    /* Bottom navigation: four separate tiles on the page, 8 px apart. */
    explorer->nav = create_transparent_container(explorer->root);
    if (explorer->nav == NULL) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_set_width(explorer->nav, lv_pct(100));
    lv_obj_set_height(explorer->nav, UI_LAB_BUTTON_MIN_HEIGHT);
    lv_obj_set_flex_flow(explorer->nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(explorer->nav, UI_LAB_SPACE, LV_PART_MAIN);
    const ui_lab_action_t nav_actions[] = {
        UI_LAB_ACTION_HOME, UI_LAB_ACTION_PROGRAMS, UI_LAB_ACTION_DEVICE, UI_LAB_ACTION_SETTINGS,
    };
    const furnace_hmi_icon_t nav_icons[] = {
        FURNACE_HMI_ICON_HOME, FURNACE_HMI_ICON_PROGRAM, FURNACE_HMI_ICON_FURNACE, FURNACE_HMI_ICON_SLIDERS,
    };
    for (size_t index = 0; index < sizeof(nav_texts) / sizeof(nav_texts[0]); ++index) {
        lv_obj_t *button = create_action_button(
            explorer->nav, ui_string(nav_texts[index]), UI_LAB_TONE_QUIET, &explorer->nav_actions[index], explorer,
            nav_actions[index], 0
        );
        if (button == NULL) {
            furnace_hmi_ui_lab_explorer_destroy(explorer);
            return false;
        }
        lv_obj_set_flex_grow(button, 1);
        lv_obj_set_height(button, UI_LAB_BUTTON_MIN_HEIGHT);
        (void)add_button_icon(button, nav_icons[index], UI_LAB_ICON_START, UI_LAB_ICON_SIZE);
        lv_obj_set_style_pad_column(button, 10, LV_PART_MAIN);
    }
    update_nav_selection(explorer);
    if (!update_header(explorer) || !rebuild_page(explorer)) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    lv_obj_update_layout(explorer->root);
    /* An active fault in the first snapshot is presented once, like a new report. */
    if (state_has_fault_alert(explorer->state) && !open_alert(explorer, UI_LAB_ALERT_FAULT)) {
        furnace_hmi_ui_lab_explorer_destroy(explorer);
        return false;
    }
    return true;
}

bool furnace_hmi_ui_lab_explorer_refresh_presentation(furnace_hmi_ui_lab_explorer_t *explorer)
{
    if (explorer == NULL || explorer->root == NULL || explorer->nav == NULL) {
        return false;
    }
    /* Built once at creation: the root, the light strip and the navigation. */
    lv_obj_set_style_bg_color(explorer->root, UI_LAB_BACKGROUND, LV_PART_MAIN);
    for (uint32_t index = 0U; index < sizeof(nav_texts) / sizeof(nav_texts[0]); ++index) {
        lv_obj_t *button = lv_obj_get_child(explorer->nav, (int32_t)index);
        const uint32_t count = button == NULL ? 0U : lv_obj_get_child_count(button);
        for (uint32_t child = 0U; child < count; ++child) {
            lv_obj_t *label = lv_obj_get_child(button, (int32_t)child);
            if (lv_obj_check_type(label, &lv_label_class)) {
                lv_label_set_text(label, ui_string(nav_texts[index]));
            }
        }
    }
    update_nav_selection(explorer);
    return update_header(explorer) && rebuild_page(explorer);
}

static void update_value_label(lv_obj_t *label, const furnace_hmi_i32_value_t *value, furnace_hmi_ui_string_id_t format)
{
    if (label == NULL) { return; }
    char text[48]; format_observation_value(text, sizeof(text), value, format); lv_label_set_text(label, text);
}

static bool update_observation(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_dashboard_state_t *state);

/* Which alert, if any, a new observation raises: a newly reported fault, the
 * link lost, or the link restored while the link-lost alert is still open. */
static uint8_t alert_for_transition(const furnace_hmi_ui_lab_explorer_t *explorer,
                                    const furnace_hmi_dashboard_state_t *next)
{
    const furnace_hmi_dashboard_state_t *previous = &explorer->observation;
    if (state_has_fault_alert(next) &&
        (!state_has_fault_alert(previous) || previous->fault_kind != next->fault_kind)) {
        return UI_LAB_ALERT_FAULT;
    }
    if (previous->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT &&
        next->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE) {
        return UI_LAB_ALERT_LINK_LOST;
    }
    if (previous->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE &&
        next->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT &&
        explorer->modal != NULL && explorer->alert_open == UI_LAB_ALERT_LINK_LOST) {
        return UI_LAB_ALERT_LINK_RESTORED;
    }
    return UI_LAB_ALERT_NONE;
}

bool furnace_hmi_ui_lab_explorer_update(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_dashboard_state_t *state)
{
    if (explorer == NULL || explorer->root == NULL || state == NULL || !furnace_hmi_dashboard_state_is_well_formed(state)) { return false; }
    const uint8_t alert = alert_for_transition(explorer, state);
    if (!update_observation(explorer, state)) { return false; }
    return alert == UI_LAB_ALERT_NONE || open_alert(explorer, alert);
}

static bool update_observation(furnace_hmi_ui_lab_explorer_t *explorer, const furnace_hmi_dashboard_state_t *state)
{
    const bool new_run = state->controller_session_epoch != explorer->observation.controller_session_epoch ||
        state->program_id != explorer->observation.program_id || state->program_revision != explorer->observation.program_revision ||
        state->elapsed_seconds.value < explorer->observation.elapsed_seconds.value;
    const bool structure_changed = state->availability != explorer->observation.availability ||
        state->mode != explorer->observation.mode || state->run_state != explorer->observation.run_state ||
        state->stage_index != explorer->observation.stage_index || new_run;
    /* A cooling Manual session counts its cooling time on each observation (one per second). */
    if (explorer->manual_cooling) {
        ++explorer->manual_cooling_seconds;
    }
    log_observation(explorer, &explorer->observation, state);
    explorer->observation = *state; normalize_observation_validity(&explorer->observation);
    explorer->state = &explorer->observation; state = explorer->state;
    if (new_run) { explorer->trajectory_domain_initialized = false; explorer->running_cooling_full = false; }
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
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
        int64_t window_end = 0;
        bool overflows = false;
        const bool want_toggle = explorer->running_view_mode != 2U &&
            running_cooling_window(explorer, &window_end, &overflows) && overflows;
        if (explorer->cooling_toggle != NULL) {
            show_cooling_toggle(explorer, want_toggle);
        }
        else if (want_toggle) {
            lv_obj_t *modal = explorer->modal; size_t modal_count = explorer->modal_action_count;
            explorer->modal = NULL;
            const bool rebuilt = rebuild_page(explorer);
            explorer->modal = modal; explorer->modal_action_count = modal_count;
            if (modal != NULL) { lv_obj_move_to_index(modal, -1); }
            if (!rebuilt) { return false; }
        }
    }
    if (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL) {
        return update_manual_run_state(explorer);
    }
    update_value_label(explorer->home_temperature, &state->current_temperature_c, FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
    update_value_label(explorer->home_target, &state->target_temperature_c, FURNACE_HMI_UI_STRING_UI_LAB_VALUE_FORMAT_NUMBER);
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
        (explorer->page == FURNACE_HMI_UI_LAB_PAGE_MANUAL && (explorer->manual_running || explorer->manual_cooling) &&
         page != explorer->page) ||
        (explorer->page == FURNACE_HMI_UI_LAB_PAGE_RUNNING &&
        page != FURNACE_HMI_UI_LAB_PAGE_RUNNING)) {
        return false;
    }
    if (explorer->page != page) {
        explorer->trajectory_domain_initialized = false;
        if (page == FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR) {
            begin_stage_session(explorer, explorer->page);
        }
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
                       ui_string(FURNACE_HMI_UI_STRING_UI_LAB_ACKNOWLEDGE_PREVIEW), UI_LAB_TONE_PRIMARY);
}

bool furnace_hmi_ui_lab_explorer_take_request(
    furnace_hmi_ui_lab_explorer_t *explorer,
    furnace_hmi_ui_lab_request_t *request
)
{
    if (explorer == NULL || request == NULL || explorer->request.kind == FURNACE_HMI_UI_LAB_REQUEST_NONE) {
        return false;
    }
    *request = explorer->request;
    memset(&explorer->request, 0, sizeof(explorer->request));
    return true;
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
