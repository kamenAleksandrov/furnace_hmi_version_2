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
    FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR,
    FURNACE_HMI_UI_LAB_PAGE_DEVICE,
    FURNACE_HMI_UI_LAB_PAGE_SETTINGS,
    FURNACE_HMI_UI_LAB_PAGE_COUNT,
} furnace_hmi_ui_lab_page_t;

#define FURNACE_HMI_UI_LAB_MAX_PROGRAMS 12U
#define FURNACE_HMI_UI_LAB_MAX_STAGES 8U
#define FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS 3U
#define FURNACE_HMI_UI_LAB_MAX_RECENT_PROGRAMS 6U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT 5U
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
#define FURNACE_HMI_UI_LAB_SERVICE_RECORDS 8U
#define FURNACE_HMI_UI_LAB_LOG_CAPACITY 160U

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

/* One copy of a stage-editor session's stages (see the explorer's stage
 * session helpers): the raw staged arrays plus bookkeeping. */
typedef struct {
    int32_t targets[FURNACE_HMI_UI_LAB_MAX_STAGES];
    int32_t rates[FURNACE_HMI_UI_LAB_MAX_STAGES];
    int32_t durations[FURNACE_HMI_UI_LAB_MAX_STAGES];
    int32_t starts[FURNACE_HMI_UI_LAB_MAX_STAGES];
    uint8_t kinds[FURNACE_HMI_UI_LAB_MAX_STAGES];
    bool cooling_rate_enabled[FURNACE_HMI_UI_LAB_MAX_STAGES];
    bool valid[FURNACE_HMI_UI_LAB_MAX_STAGES];
    bool name_valid[FURNACE_HMI_UI_LAB_MAX_STAGES];
    char names[FURNACE_HMI_UI_LAB_MAX_STAGES][FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    uint8_t program;
    uint8_t count;
    bool active;
} furnace_hmi_ui_lab_stage_set_t;

/* One authored stage: Heating ramps to `target_c` at `rate_tenths_c_per_minute`;
 * Hold keeps the previous target for `minutes`. Cooling is controller-owned
 * and never authored. */
typedef enum {
    FURNACE_HMI_UI_LAB_STAGE_HEATING = 0,
    FURNACE_HMI_UI_LAB_STAGE_HOLD = 1,
} furnace_hmi_ui_lab_stage_kind_t;

typedef struct {
    uint8_t kind;
    int32_t target_c;
    int32_t rate_tenths_c_per_minute;
    int32_t minutes;
} furnace_hmi_ui_lab_program_stage_t;

/* A program as the controller's library holds it for this session. Library
 * programs take their name from the string catalogue (`name_string`); a
 * program the operator created or renamed carries its own name in
 * `program_name_overrides`. Nothing here persists. */
typedef struct {
    bool present;
    bool library;
    uint16_t name_string;
    uint8_t stage_count;
    furnace_hmi_ui_lab_program_stage_t stages[FURNACE_HMI_UI_LAB_MAX_STAGES];
} furnace_hmi_ui_lab_program_t;

/* An operator request for the controller. The explorer only asks; the host
 * hands the request to the controller (here the simulator) and the result
 * arrives as a new observation through furnace_hmi_ui_lab_explorer_update(). */
typedef enum {
    FURNACE_HMI_UI_LAB_REQUEST_NONE = 0,
    FURNACE_HMI_UI_LAB_REQUEST_START_PROGRAM,
    FURNACE_HMI_UI_LAB_REQUEST_PAUSE,
    FURNACE_HMI_UI_LAB_REQUEST_RESUME,
    FURNACE_HMI_UI_LAB_REQUEST_STOP,
    /* Close a stopped, cooling or completed run (Return Home). */
    FURNACE_HMI_UI_LAB_REQUEST_CLOSE_RUN,
} furnace_hmi_ui_lab_request_kind_t;

typedef struct {
    furnace_hmi_ui_lab_request_kind_t kind;
    /* START_PROGRAM only: the program as the operator confirmed it. */
    char program_name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    furnace_hmi_ui_lab_program_t program;
} furnace_hmi_ui_lab_request_t;

/* A service record the controller confirmed in this session (Maintenance done). */
typedef struct {
    char title[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
    char detail[FURNACE_HMI_UI_LAB_DEVICE_TEXT_CAPACITY];
} furnace_hmi_ui_lab_service_record_t;

/* One Device log event (the controller's event log). */
typedef struct {
    int32_t minute;     /**< Wall-clock minutes since 1970-01-01 on the controller clock. */
    uint8_t type;       /**< ui_lab log event type. */
    uint8_t program;    /**< Delivered library program, or UINT8_MAX when `name` holds the program. */
    uint8_t fault;      /**< furnace_hmi_fault_kind_t of an alarm. */
    uint8_t component;  /**< Maintenance events: catalogue component and task. */
    uint8_t task;
    char name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
} furnace_hmi_ui_lab_log_event_t;

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
/** Re-apply the active theme (ui_lab_theme_select) and language to
 * everything on screen: the chrome built at creation, then the current page.
 * Open dialogs and keyboards close; the page and its state are kept. Call only
 * from the GUI context, after changing the theme or language. */
bool furnace_hmi_ui_lab_explorer_refresh_presentation(furnace_hmi_ui_lab_explorer_t *explorer);

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

/** Take the operator's pending controller request, if any; false when none. */
bool furnace_hmi_ui_lab_explorer_take_request(
    furnace_hmi_ui_lab_explorer_t *explorer,
    furnace_hmi_ui_lab_request_t *request
);

/* The opaque storage is intentionally fixed-size so the host laboratory does
 * not need a separate allocator policy. Its fields remain private to the lab.
 */
/** Calendar date and wall-clock time (simulated controller clock). */
typedef struct {
    uint16_t year;
    uint8_t month;  /**< 1-12 */
    uint8_t day;    /**< 1-31 */
    uint8_t hour;   /**< 0-23 */
    uint8_t minute; /**< 0-59 */
} furnace_hmi_ui_lab_clock_t;

struct furnace_hmi_ui_lab_explorer {
    lv_obj_t *root;
    lv_obj_t *state_strip;
    lv_obj_t *state_strip_line;
    lv_obj_t *state_strip_glow;
    lv_obj_t *header_title;
    lv_obj_t *state_badge;
    lv_obj_t *service_badge;
    lv_obj_t *content;
    lv_obj_t *nav;
    lv_obj_t *modal;
    /* Which controller alert the open modal shows (0 when it is not an alert). */
    uint8_t alert_open;
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
    /* After Stop the controller cools automatically: Exit and Start new replace
     * Pause and Stop, and the steppers are off (web leaveManualCooling). */
    lv_obj_t *manual_exit_button;
    lv_obj_t *manual_new_button;
    lv_obj_t *manual_steppers;
    lv_obj_t *grid_lines[FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT +
                         FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT];
    lv_obj_t *trajectory_stage_nodes[FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT];
    lv_obj_t *measured_segments[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - 1U];
    lv_point_precise_t planned_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    lv_point_precise_t cooling_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    uint32_t cooling_point_count;
    /* Measured readings after curing ended: drawn blue and dashed. */
    lv_point_precise_t measured_cooling_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    uint32_t measured_cooling_point_count;
    /* 0, or the end of the visible time window (Running shows only the start
     * of automatic cooling unless the operator shows all of it). */
    int64_t trajectory_time_limit_seconds;
    lv_obj_t *trajectory_legend_label;
    lv_obj_t *cooling_toggle;
    bool running_cooling_full;
    lv_obj_t *manual_readings;
    lv_obj_t *manual_marker;
    int32_t manual_marker_index;
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
    uint8_t quick_launch_recent[FURNACE_HMI_UI_LAB_MAX_RECENT_PROGRAMS];
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
    bool manual_cooling;
    uint32_t manual_cooling_seconds;
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
    /* Stage editor dialog: drawn over the page it was opened from. The
     * session copies every stage; Discard restores `stage_backup`, Save
     * compares against `stage_baseline` to decide whether the draft changed. */
    lv_obj_t *stage_dialog;
    furnace_hmi_ui_lab_page_t stage_editor_origin;
    bool stage_session_open;
    int32_t stage_starts[FURNACE_HMI_UI_LAB_MAX_STAGES];
    furnace_hmi_ui_lab_stage_set_t stage_backup;
    furnace_hmi_ui_lab_stage_set_t stage_baseline;
    char program_editor_name[FURNACE_HMI_UI_LAB_NAME_CAPACITY];
    int pending_action;
    int pending_action_value;
    bool action_pending;
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
    lv_obj_t *keyboard_input;
    lv_obj_t *keyboard_hint;
    lv_obj_t *keyboard_count;
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
    uint8_t settings_choice_kind;
    /* Server/Bluetooth enabled by the controller; Connections is listed only then. */
    bool settings_connections_enabled;
    uint8_t settings_restart_assessment;
    uint8_t settings_restart_result;
    int32_t settings_saved_numeric[5];
    int32_t settings_candidate_numeric[5];
    bool settings_saved_reduced_motion;
    bool settings_candidate_reduced_motion;
    uint8_t settings_saved_running_view;
    uint8_t settings_candidate_running_view;
    /* Date format preference (0 "6 Oct 2026", 1 dots, 2 slashes, 3 month first, 4 ISO). */
    uint8_t settings_saved_date_format;
    uint8_t settings_candidate_date_format;
    /* Settings > Delete programs: the programs the confirmation named. */
    uint16_t settings_delete_pending;
    bool settings_saved_24_hour;
    bool settings_candidate_24_hour;
    /* Display preferences: furnace_hmi_theme_id_t and furnace_hmi_ui_language_t. */
    uint8_t settings_saved_theme;
    uint8_t settings_candidate_theme;
    uint8_t settings_saved_language;
    uint8_t settings_candidate_language;
    /* Simulated controller clock and the Set date & time dialog draft. */
    furnace_hmi_ui_lab_clock_t clock;
    furnace_hmi_ui_lab_clock_t clock_draft;
    furnace_hmi_ui_lab_device_catalogue_t device_catalogue;
    uint8_t device_tab;
    uint8_t device_filter;
    uint8_t device_page;
    uint8_t device_selected_component;
    uint8_t device_selected_task;
    bool device_component_selected;
    /* Program library for this session: the delivered library programs, then
     * programs the operator created. `library_count` is how many delivered
     * programs are listed (all of them, or none). */
    furnace_hmi_ui_lab_program_t programs[FURNACE_HMI_UI_LAB_MAX_PROGRAMS];
    /* The program a confirmed start or a run belongs to. */
    uint8_t running_program;
    /* The operator stopped the run with this controller program id: it cools
     * on Running with Start again and Return Home. */
    bool run_stopped;
    uint32_t stopped_program_id;
    furnace_hmi_ui_lab_request_t request;
    /* Service records confirmed this session, newest first, before the
     * controller's earlier records in Service history. */
    furnace_hmi_ui_lab_service_record_t service_records[FURNACE_HMI_UI_LAB_SERVICE_RECORDS];
    uint8_t service_record_count;
    /* Device log: one month of history and this session's events, newest first. */
    furnace_hmi_ui_lab_log_event_t log_events[FURNACE_HMI_UI_LAB_LOG_CAPACITY];
    uint16_t log_count;
    uint8_t log_filter;
};

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_LAB_EXPLORER_H */
