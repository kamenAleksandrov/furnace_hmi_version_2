/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_LAB_EXPLORER_H
#define FURNACE_HMI_UI_LAB_EXPLORER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <furnace_hmi/dashboard_model.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FURNACE_HMI_UI_LAB_PAGE_HOME = 0,
    FURNACE_HMI_UI_LAB_PAGE_MANUAL,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING,
    FURNACE_HMI_UI_LAB_PAGE_RUNNING,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAMS,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_EDITOR,
    FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL,
    FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR,
    FURNACE_HMI_UI_LAB_PAGE_DEVICE,
    FURNACE_HMI_UI_LAB_PAGE_SETTINGS,
    FURNACE_HMI_UI_LAB_PAGE_COUNT,
} furnace_hmi_ui_lab_page_t;

#define FURNACE_HMI_UI_LAB_MAX_PROGRAMS 12U
#define FURNACE_HMI_UI_LAB_MAX_STAGES 8U
#define FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS 3U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT 6U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT 7U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT 8U
#define FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS 12U
#define FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS 4U
#define FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY 192U
/* Full-width top accent/light-request strip. It equals the 12 px side and
 * bottom borders, so content starts 12 px below the top edge. */
#define FURNACE_HMI_UI_LAB_LIGHT_TARGET_HEIGHT 12
/* Running reading line: label/value pairs between the run visual and actions. */
#define FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT 5U
#define FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY 40U
#define FURNACE_HMI_UI_LAB_NAME_CAPACITY 33U
#define FURNACE_HMI_UI_LAB_KEYBOARD_VALUE_CAPACITY 64U
#define FURNACE_HMI_UI_LAB_KEYBOARD_BUTTON_COUNT 64U

typedef enum {
    FURNACE_HMI_UI_LAB_DEVICE_TASK_UP_TO_DATE = 0,
    FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE_SOON,
    FURNACE_HMI_UI_LAB_DEVICE_TASK_DUE,
    FURNACE_HMI_UI_LAB_DEVICE_TASK_CONDITION,
    FURNACE_HMI_UI_LAB_DEVICE_TASK_UNKNOWN,
} furnace_hmi_ui_lab_device_task_state_t;

typedef struct {
    char id[FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY];
    char name[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char basis[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char interval[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char remaining[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char state[FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY];
    char last[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char procedure[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    furnace_hmi_ui_lab_device_task_state_t task_state;
} furnace_hmi_ui_lab_device_task_t;

typedef struct {
    char id[FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY];
    char name[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char part[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char location[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char usage[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char source[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char instance[FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY];
    uint8_t task_count;
    furnace_hmi_ui_lab_device_task_t tasks[FURNACE_HMI_UI_LAB_DEVICE_MAX_TASKS];
} furnace_hmi_ui_lab_device_component_t;

typedef struct {
    char revision[FURNACE_HMI_UI_LAB_DEVICE_ID_CAPACITY];
    uint8_t component_count;
    furnace_hmi_ui_lab_device_component_t components[FURNACE_HMI_UI_LAB_DEVICE_MAX_COMPONENTS];
} furnace_hmi_ui_lab_device_catalogue_t;

typedef struct furnace_hmi_ui_lab_explorer furnace_hmi_ui_lab_explorer_t;

typedef struct {
    furnace_hmi_ui_lab_explorer_t *explorer;
    int action;
    int value;
} furnace_hmi_ui_lab_action_slot_t;

typedef struct {
    furnace_hmi_ui_lab_explorer_t *explorer;
    uint8_t action;
    char text[16];
} furnace_hmi_ui_lab_keyboard_button_t;

/** Create the host-only visual explorer under the LVGL owner context. */
bool furnace_hmi_ui_lab_explorer_create(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state,
    const furnace_hmi_ui_lab_device_catalogue_t *device_catalogue
);

/** Load the bounded developer-editable catalogue before GUI ownership starts. */
bool furnace_hmi_ui_lab_device_catalogue_load(
    furnace_hmi_ui_lab_device_catalogue_t *catalogue,
    const char *path
);

/** Refresh controller-observation labels without treating simulation as truth. */
bool furnace_hmi_ui_lab_explorer_update(
    furnace_hmi_ui_lab_explorer_t *explorer,
    const furnace_hmi_dashboard_state_t *state
);

/** Change the visible lab page. Detail pages are also available to smoke tests. */
bool furnace_hmi_ui_lab_explorer_navigate(
    furnace_hmi_ui_lab_explorer_t *explorer,
    furnace_hmi_ui_lab_page_t page
);

/** Present the controller-owned service acknowledgement preview. */
bool furnace_hmi_ui_lab_explorer_show_service_warning(
    furnace_hmi_ui_lab_explorer_t *explorer
);

/** Delete the lab tree and reset the supplied storage. */
void furnace_hmi_ui_lab_explorer_destroy(furnace_hmi_ui_lab_explorer_t *explorer);

/* The opaque storage is intentionally fixed-size so the host laboratory does
 * not need a separate allocator policy. Its fields remain private to the lab.
 */
struct furnace_hmi_ui_lab_explorer {
    lv_obj_t *root;
    lv_obj_t *state_strip;
    lv_obj_t *state_strip_line;
    lv_obj_t *header_title;
    lv_obj_t *state_badge;
    lv_obj_t *service_badge;
    lv_obj_t *content;
    lv_obj_t *nav;
    lv_obj_t *modal;
    furnace_hmi_dashboard_state_t observation;
    lv_obj_t *device_values[4];
    lv_obj_t *live_door;
    lv_obj_t *live_fan;
    lv_obj_t *home_temperature;
    lv_obj_t *home_target;
    lv_obj_t *home_stage;
    lv_obj_t *home_remaining;
    lv_obj_t *home_state_detail;
    lv_obj_t *trajectory;
    lv_obj_t *planned_line;
    lv_obj_t *cooling_overlay;
    lv_obj_t *trajectory_minimum_label;
    lv_obj_t *trajectory_maximum_label;
    lv_obj_t *trajectory_time_axis_label;
    lv_obj_t *trajectory_end_time_label;
    lv_obj_t *trajectory_middle_temperature_label;
    lv_obj_t *trajectory_middle_time_label;
    lv_obj_t *manual_prediction_label;
    lv_obj_t *manual_target_value;
    lv_obj_t *manual_rate_value;
    lv_obj_t *manual_status_label;
    lv_obj_t *manual_context_label;
    lv_obj_t *manual_start_button;
    lv_obj_t *manual_pause_button;
    lv_obj_t *manual_stop_button;
    lv_obj_t *grid_lines[FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT +
                         FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT];
    lv_obj_t *trajectory_stage_nodes[FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT];
    lv_obj_t *measured_segments[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - 1U];
    lv_point_precise_t planned_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    lv_point_precise_t cooling_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    uint32_t cooling_point_count;
    lv_point_precise_t measured_segment_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - 1U][2];
    lv_point_precise_t grid_line_points[FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT +
                                        FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT][2];
    const furnace_hmi_dashboard_state_t *state;
    furnace_hmi_dashboard_state_t preview_graph_state;
    furnace_hmi_ui_lab_page_t page;
    uint8_t library_count;
    uint8_t program_page;
    uint8_t program_stage_page;
    uint8_t selected_program;
    uint8_t selected_stage;
    uint8_t running_view_mode;
    lv_obj_t *running_reading_names[FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT];
    lv_obj_t *running_reading_values[FURNACE_HMI_UI_LAB_RUNNING_READING_COUNT];
    uint8_t quick_launch_favourites[FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS];
    uint8_t quick_launch_recent[FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS];
    uint8_t favourite_count;
    uint8_t recent_count;
    int32_t draft_target_c;
    /* Tenths of a degree Celsius per minute; keeps the host control precise
     * without making this presentation-only unit a wire-format choice. */
    int32_t draft_rate_tenths_c_per_minute;
    int32_t draft_duration_minutes;
    int32_t manual_target_c;
    int32_t manual_rate_tenths_c_per_minute;
    bool manual_running;
    bool manual_paused;
    bool manual_light_on;
    uint8_t draft_stage_kind;
    bool draft_cooling_rate_enabled;
    bool draft_heating_duration_drives_rate;
    /* A bounded, HMI-local edit session. These values are never sent to the
     * simulator/controller and are discarded unless the user explicitly saves
     * the whole session. */
    int32_t staged_targets[FURNACE_HMI_UI_LAB_MAX_STAGES];
    int32_t staged_rates[FURNACE_HMI_UI_LAB_MAX_STAGES];
    int32_t staged_durations[FURNACE_HMI_UI_LAB_MAX_STAGES];
    uint8_t staged_kinds[FURNACE_HMI_UI_LAB_MAX_STAGES];
    bool staged_cooling_rate_enabled[FURNACE_HMI_UI_LAB_MAX_STAGES];
    bool staged_valid[FURNACE_HMI_UI_LAB_MAX_STAGES];
    uint8_t staged_program;
    uint8_t staged_stage_count;
    bool staged_session_active;
    bool program_editor_active;
    bool program_editor_new;
    /* The open program draft differs from what was loaded (name or stages). */
    bool program_editor_dirty;
    furnace_hmi_ui_lab_page_t program_editor_return_page;
    char program_editor_name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    int pending_action;
    int pending_action_value;
    bool action_pending;
    bool nav_selection_animation_pending;
    bool trajectory_uses_program_preview;
    bool trajectory_domain_initialized;
    int64_t trajectory_domain_maximum_seconds;
    int32_t trajectory_domain_minimum_temperature_c;
    int32_t trajectory_domain_maximum_temperature_c;
    furnace_hmi_ui_lab_action_slot_t header_actions[2];
    furnace_hmi_ui_lab_action_slot_t nav_actions[4];
    furnace_hmi_ui_lab_action_slot_t page_actions[64];
    furnace_hmi_ui_lab_action_slot_t modal_actions[32];
    size_t page_action_count;
    size_t modal_action_count;
    lv_obj_t *settings_editor_spinbox;
    lv_obj_t *keyboard_input;
    lv_obj_t *keyboard_hint;
    lv_obj_t *keyboard_apply;
    lv_obj_t *keyboard_layout;
    furnace_hmi_ui_lab_keyboard_button_t keyboard_buttons[FURNACE_HMI_UI_LAB_KEYBOARD_BUTTON_COUNT];
    size_t keyboard_button_count;
    uint8_t keyboard_mode;
    uint8_t keyboard_target;
    uint8_t keyboard_stage;
    uint8_t keyboard_shift;
    uint8_t keyboard_selected;
    uint8_t keyboard_rate_tenths;
    int32_t keyboard_minimum;
    int32_t keyboard_maximum;
    int32_t keyboard_step;
    char keyboard_value[FURNACE_HMI_UI_LAB_KEYBOARD_VALUE_CAPACITY];
    size_t keyboard_length;
    size_t keyboard_cursor;
    size_t keyboard_anchor;
    char program_name_overrides[FURNACE_HMI_UI_LAB_MAX_PROGRAMS][FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    char staged_names[FURNACE_HMI_UI_LAB_MAX_STAGES][FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    bool staged_name_valid[FURNACE_HMI_UI_LAB_MAX_STAGES];
    uint8_t settings_tab;
    uint8_t settings_pending_tab;
    uint8_t settings_pending_scope;
    uint8_t settings_display_page;
    uint8_t settings_editor_key;
    uint8_t settings_connection_fixture;
    uint8_t settings_restart_assessment;
    uint8_t settings_restart_result;
    int32_t settings_saved_numeric[5];
    int32_t settings_candidate_numeric[5];
    bool settings_saved_reduced_motion;
    bool settings_candidate_reduced_motion;
    uint8_t settings_saved_running_view;
    uint8_t settings_candidate_running_view;
    bool settings_saved_24_hour;
    bool settings_candidate_24_hour;
    furnace_hmi_ui_lab_device_catalogue_t device_catalogue;
    uint8_t device_tab;
    uint8_t device_filter;
    uint8_t device_page;
    uint8_t device_selected_component;
    uint8_t device_selected_task;
    bool device_component_selected;
};

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_LAB_EXPLORER_H */
