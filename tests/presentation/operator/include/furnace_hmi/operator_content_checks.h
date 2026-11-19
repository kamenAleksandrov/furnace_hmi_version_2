/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_OPERATOR_CONTENT_CHECKS_H
#define FURNACE_HMI_OPERATOR_CONTENT_CHECKS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The operator screens with no controller-owned content and a current status
 * whose measured temperature is unavailable, as the Zephyr application has them
 * before the controller provides library, log, clock and limits. Visits Home,
 * Manual, Programs, Device and Settings and reads every label: no simulated
 * content (a delivered program, the lab's date), no forecast from an assumed
 * temperature, and the unavailable presentation where content is missing
 * (Phase 21 M21-2b). One definition for every build that runs it: the desktop
 * program's --operator-content-smoke-test and the Zephyr test.
 *
 * Call from the LVGL-owning GUI context. Leaves the active screen empty.
 * Returns 0 when every check passes, otherwise a code with *failure set.
 */
int furnace_hmi_operator_content_check(lv_display_t *display, const char **failure);

/**
 * The controller's lifecycle on the operator screens: a held (faulted) run
 * stays on Running, a run that still cools is not shown complete and may be
 * left, a decision is named, and a controller Manual session holds Manual.
 * Same contract as furnace_hmi_operator_content_check.
 */
int furnace_hmi_operator_lifecycle_check(lv_display_t *display, const char **failure);

/**
 * Manual against a controller (Phase 21 M21-5b): opening Manual asks for start
 * readiness, Start waits for it and asks for the controller's longest session,
 * the session follows the controller's status, steppers retarget with taps
 * merged while one is pending, Pause, Resume and Stop are requests, and every
 * outcome other than accepted is shown. Same contract as
 * furnace_hmi_operator_content_check.
 */
int furnace_hmi_operator_manual_command_check(lv_display_t *display, const char **failure);

/**
 * Faults and the decision against a controller (Phase 21 M21-5c): the Faults
 * dialog from Manual and from Home, Acknowledge and Recover as requests with
 * the controller's answer in the dialog, no Recover for a protective trip, and
 * the decision panel with its countdown, Continue and Stop. Same contract as
 * furnace_hmi_operator_content_check.
 */
int furnace_hmi_operator_fault_check(lv_display_t *display, const char **failure);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_OPERATOR_CONTENT_CHECKS_H */
