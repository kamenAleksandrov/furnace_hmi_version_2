/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_STATE_AVAILABILITY_VIEW_H
#define FURNACE_HMI_STATE_AVAILABILITY_VIEW_H

#include <stdbool.h>

#include <furnace_hmi/state_availability.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LVGL objects owned by one state-availability presentation instance.
 *
 * Only the designated GUI execution context may create, inspect, or mutate
 * this object or the referenced LVGL objects.
 */
typedef struct {
    lv_obj_t *root;
    lv_obj_t *status_bar;
    lv_obj_t *status_badge;
    lv_obj_t *heading;
    lv_obj_t *detail;
    furnace_hmi_state_availability_t rendered_availability;
} furnace_hmi_state_availability_view_t;

/**
 * Create a read-only availability view under ``parent``.
 *
 * ``view`` must be zero-initialized before its first use. A second create is
 * rejected until the active view is destroyed. The handle has a stable address:
 * do not copy or move it while the object tree exists, and destroy the view
 * before the handle's storage lifetime ends.
 *
 * A NULL, zero-initialized, or invalid state fails closed to UNAVAILABLE.
 * Returns false when a required pointer is NULL or a directly called LVGL
 * object constructor reports NULL; any partial object tree is removed before
 * returning. Pinned LVGL internal allocations and style setters do not expose
 * recoverable failure, so their out-of-memory behavior follows the configured
 * LVGL fatal assertion policy pending ADR-0008. This function must be called by
 * the LVGL-owning GUI execution context.
 * The current presentation is verified at viewports of 640 x 360 or larger.
 */
bool furnace_hmi_state_availability_view_create(
    furnace_hmi_state_availability_view_t *view,
    lv_obj_t *parent,
    const furnace_hmi_state_availability_view_state_t *state
);

/**
 * Delete an active view and clear its handle.
 *
 * Root deletion, including deletion through the parent, clears the handle.
 * A later destroy is therefore a safe no-op and cannot delete an unrelated
 * object whose address was reused. The call must still occur in the
 * LVGL-owning GUI context.
 */
void furnace_hmi_state_availability_view_destroy(
    furnace_hmi_state_availability_view_t *view
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_STATE_AVAILABILITY_VIEW_H */
