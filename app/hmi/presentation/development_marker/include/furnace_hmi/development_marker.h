/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_DEVELOPMENT_MARKER_H
#define FURNACE_HMI_DEVELOPMENT_MARKER_H

#include <stdbool.h>

#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a read-only footer naming a development build (``text``, for example the
 * desktop program or the simulated controller link) as the last child of
 * ``parent``. It has no input behavior and is deleted with its parent. Returns
 * false when an LVGL constructor reports NULL; nothing is left behind then.
 * Call from the LVGL-owning GUI context.
 */
bool furnace_hmi_development_marker_create(lv_obj_t *parent, furnace_hmi_ui_string_id_t text);

/**
 * The same marker as a small tag on LVGL's top layer, at the top right, over
 * screens whose own layout has no room for a footer (the operator screens).
 * It takes no input and stays until deleted. Call from the GUI context.
 */
lv_obj_t *furnace_hmi_development_tag_create(furnace_hmi_ui_string_id_t text);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_DEVELOPMENT_MARKER_H */
