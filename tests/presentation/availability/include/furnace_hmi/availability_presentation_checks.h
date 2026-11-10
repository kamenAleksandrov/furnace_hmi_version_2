/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_AVAILABILITY_PRESENTATION_CHECKS_H
#define FURNACE_HMI_AVAILABILITY_PRESENTATION_CHECKS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check the state-availability presentation on ``display``: the copy for each
 * state, fail-closed mapping, the read-only object tree, the 640 x 360 bounds,
 * and handle lifetime. One definition for every build that runs it: the
 * desktop program's ``--presentation-smoke-test`` and the Zephyr test.
 *
 * Changes the display resolution to 640 x 360 and leaves the active screen
 * empty. Call from the LVGL-owning GUI context after LVGL is initialized. It
 * services LVGL timers without delays, so it does not depend on the platform's
 * LVGL delay callback.
 *
 * Returns 0 when every check passes, otherwise the desktop program's exit code
 * for the first failure, with ``*failure`` set to its description.
 */
int furnace_hmi_availability_presentation_check(lv_display_t *display, const char **failure);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_AVAILABILITY_PRESENTATION_CHECKS_H */
