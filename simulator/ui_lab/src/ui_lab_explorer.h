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
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING,
    FURNACE_HMI_UI_LAB_PAGE_RUNNING,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAMS,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_DETAIL,
    FURNACE_HMI_UI_LAB_PAGE_PROGRAM_STAGES,
    FURNACE_HMI_UI_LAB_PAGE_STAGE_DETAIL,
    FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR,
    FURNACE_HMI_UI_LAB_PAGE_DEVICE,
    FURNACE_HMI_UI_LAB_PAGE_SETTINGS,
    FURNACE_HMI_UI_LAB_PAGE_COUNT,
} furnace_hmi_ui_lab_page_t;

#define FURNACE_HMI_UI_LAB_MAX_PROGRAMS 12U
#define FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS 3U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT 6U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT 7U
#define FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT 5U

typedef struct furnace_hmi_ui_lab_explorer furnace_hmi_ui_lab_explorer_t;

typedef struct {
    furnace_hmi_ui_lab_explorer_t *explorer;
    int action;
    int value;
} furnace_hmi_ui_lab_action_slot_t;

/** Create the host-only visual explorer under the LVGL owner context. */
bool furnace_hmi_ui_lab_explorer_create(
    furnace_hmi_ui_lab_explorer_t *explorer,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state
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
    lv_obj_t *header_title;
    lv_obj_t *state_badge;
    lv_obj_t *service_badge;
    lv_obj_t *content;
    lv_obj_t *nav;
    lv_obj_t *modal;
    lv_obj_t *home_temperature;
    lv_obj_t *home_target;
    lv_obj_t *home_stage;
    lv_obj_t *home_remaining;
    lv_obj_t *home_state_detail;
    lv_obj_t *trajectory;
    lv_obj_t *planned_line;
    lv_obj_t *trajectory_minimum_label;
    lv_obj_t *trajectory_maximum_label;
    lv_obj_t *trajectory_time_axis_label;
    lv_obj_t *trajectory_end_time_label;
    lv_obj_t *trajectory_middle_temperature_label;
    lv_obj_t *trajectory_middle_time_label;
    lv_obj_t *grid_lines[FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT +
                         FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT];
    lv_obj_t *trajectory_stage_nodes[FURNACE_HMI_UI_LAB_TRAJECTORY_STAGE_NODE_COUNT];
    lv_obj_t *measured_segments[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - 1U];
    lv_point_precise_t planned_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    lv_point_precise_t measured_segment_points[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - 1U][2];
    lv_point_precise_t grid_line_points[FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_HORIZONTAL_COUNT +
                                        FURNACE_HMI_UI_LAB_TRAJECTORY_GRID_VERTICAL_COUNT][2];
    const furnace_hmi_dashboard_state_t *state;
    furnace_hmi_dashboard_state_t preview_graph_state;
    furnace_hmi_ui_lab_page_t page;
    uint8_t program_page;
    uint8_t program_stage_page;
    uint8_t selected_program;
    uint8_t selected_stage;
    uint8_t running_view_mode;
    uint8_t quick_launch_favourites[FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS];
    uint8_t quick_launch_recent[FURNACE_HMI_UI_LAB_MAX_QUICK_LAUNCH_ITEMS];
    uint8_t favourite_count;
    uint8_t recent_count;
    int32_t draft_target_c;
    /* Tenths of a degree Celsius per minute; keeps the host control precise
     * without making this presentation-only unit a wire-format choice. */
    int32_t draft_rate_tenths_c_per_minute;
    int32_t draft_duration_minutes;
    uint8_t draft_stage_kind;
    bool draft_cooling_rate_enabled;
    bool draft_heating_duration_drives_rate;
    int pending_action;
    int pending_action_value;
    bool action_pending;
    bool nav_selection_animation_pending;
    bool trajectory_uses_program_preview;
    bool trajectory_domain_initialized;
    int32_t trajectory_domain_maximum_seconds;
    int32_t trajectory_domain_minimum_temperature_c;
    int32_t trajectory_domain_maximum_temperature_c;
    furnace_hmi_ui_lab_action_slot_t header_actions[2];
    furnace_hmi_ui_lab_action_slot_t nav_actions[4];
    furnace_hmi_ui_lab_action_slot_t page_actions[64];
    furnace_hmi_ui_lab_action_slot_t modal_actions[8];
    size_t page_action_count;
    size_t modal_action_count;
};

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_LAB_EXPLORER_H */
