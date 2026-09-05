/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_DASHBOARD_VIEW_H
#define FURNACE_HMI_DASHBOARD_VIEW_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/dashboard_model.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;
    lv_obj_t *session_label;
    lv_obj_t *run_badge;
    lv_obj_t *program_value;
    lv_obj_t *stage_value;
    lv_obj_t *temperature_value;
    lv_obj_t *target_value;
    lv_obj_t *timing_value;
    lv_obj_t *demand_value;
    lv_obj_t *power_value;
    lv_obj_t *fan_value;
    lv_obj_t *door_value;
    lv_obj_t *service_value;
    lv_obj_t *visual_status;
    lv_obj_t *navigation_feedback;
    lv_obj_t *chart;
    lv_chart_series_t *planned_series;
    lv_chart_series_t *measured_series;
    int32_t planned_values[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    int32_t measured_values[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
} furnace_hmi_dashboard_view_t;

/** Create a read-only dashboard under the LVGL-owned parent. */
bool furnace_hmi_dashboard_view_create(
    furnace_hmi_dashboard_view_t *view,
    lv_obj_t *parent,
    const furnace_hmi_dashboard_state_t *state
);

/** Apply a current observation without changing LVGL ownership. */
bool furnace_hmi_dashboard_view_update(
    furnace_hmi_dashboard_view_t *view,
    const furnace_hmi_dashboard_state_t *state
);

/** Delete the view and clear its handles. */
void furnace_hmi_dashboard_view_destroy(furnace_hmi_dashboard_view_t *view);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_DASHBOARD_VIEW_H */
