/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The GUI execution context's work between LVGL timer passes (Phase 21 M21-8):
 * the controller link's newest observation, library, limits, clock and
 * diagnostics into the operator screens, and the screens' requests to the
 * link. The application (gui_main.c) and the end-to-end operator workflows
 * (tests/zephyr/operator_workflows) run this same code, so the tested path is
 * the application's. Only the GUI context calls it (ADR-0005).
 */

#ifndef FURNACE_HMI_ZEPHYR_GUI_HOST_H
#define FURNACE_HMI_ZEPHYR_GUI_HOST_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/controller_library.h>
#include <furnace_hmi/controller_observation.h>
#include <furnace_hmi/dashboard_model.h>
#include <furnace_hmi/diagnostics_model.h>
#include <furnace_hmi/operator_screens.h>
#include <lvgl.h>

typedef struct furnace_hmi_gui_host {
    furnace_hmi_operator_screens_t screens;
    furnace_hmi_operator_content_t no_content;
    furnace_hmi_controller_observation_t observation;
    furnace_hmi_dashboard_state_t status;
    /* The controller's library (M21-7b): the link's copy and the screens' form of it. */
    furnace_hmi_controller_library_t library;
    furnace_hmi_operator_library_t shown_library;
    /* The controller's health and history (M21-7c), shown by the screens as they are. */
    furnace_hmi_controller_health_t health;
    furnace_hmi_controller_history_t history;
    /* M21-7d: the controller's program limits and planning reference as the screens use them. */
    furnace_hmi_operator_program_limits_t limits;
    furnace_hmi_operator_planning_reference_t reference;
    uint32_t seen;
    uint32_t library_seen;
    uint32_t diagnostics_seen;
    uint32_t last_taken;
    bool watchdog_stale;
} furnace_hmi_gui_host_t;

/** Creates the screens on `parent`, showing nothing read yet. False if they cannot be created. */
bool furnace_hmi_gui_host_init(furnace_hmi_gui_host_t *host, lv_obj_t *parent, uint32_t now_ms);

/** One pass at uptime `now_ms`: the link's news into the screens, the screens' requests out. */
void furnace_hmi_gui_host_pass(furnace_hmi_gui_host_t *host, uint32_t now_ms);

#endif
