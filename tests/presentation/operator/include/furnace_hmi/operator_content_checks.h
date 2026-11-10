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

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_OPERATOR_CONTENT_CHECKS_H */
