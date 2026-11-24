/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Finding and tapping widgets of the operator screens by what they show, as an
 * operator does. Shared by the screen checks (operator_content_checks.c, on
 * the desktop and Zephyr) and the end-to-end operator workflows
 * (tests/zephyr/operator_workflows, Phase 21 M21-8). Only the GUI context
 * calls them (ADR-0005).
 */

#ifndef FURNACE_HMI_SCREEN_PROBE_H
#define FURNACE_HMI_SCREEN_PROBE_H

#include <stdbool.h>

#include <lvgl.h>

/** True if a label in the tree under object contains text. */
bool furnace_hmi_probe_tree_contains(const lv_obj_t *object, const char *text);

/** The first button in the tree under object, or NULL. */
lv_obj_t *furnace_hmi_probe_first_button(lv_obj_t *object);

/** The button under object whose label contains text, or NULL. */
lv_obj_t *furnace_hmi_probe_button_with_text(lv_obj_t *object, const char *text);

/** The label under object that reads exactly text, or NULL. */
lv_obj_t *furnace_hmi_probe_label_with_text(lv_obj_t *object, const char *text);

/** The clickable card holding the label that reads text (Settings cards are not buttons). */
lv_obj_t *furnace_hmi_probe_card_with_text(lv_obj_t *root, const char *text);

/** A few LVGL timer passes, so that the screens react. */
void furnace_hmi_probe_settle(void);

/** A tap: LV_EVENT_CLICKED on button, then settle. Nothing for NULL. */
void furnace_hmi_probe_click(lv_obj_t *button);

/** True if button exists, is shown and is not disabled. */
bool furnace_hmi_probe_enabled(const lv_obj_t *button);

#endif
