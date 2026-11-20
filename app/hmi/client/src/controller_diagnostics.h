/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Internal to the controller client: the reading of the controller's health
 * and event history (Phase 21 M21-7c). The client owns the link; this module
 * says which read is due and takes the answers. Single-threaded, nothing blocks.
 */

#ifndef FURNACE_HMI_CONTROLLER_DIAGNOSTICS_INTERNAL_H
#define FURNACE_HMI_CONTROLLER_DIAGNOSTICS_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace/wire_messages.h>
#include <furnace_hmi/controller_diagnostics.h>

/** Nothing read; no read in progress. */
void furnace_hmi_diagnostics_init(furnace_hmi_controller_diagnostics_t *d);

/** Another controller session: its health and history go. */
void furnace_hmi_diagnostics_discard(furnace_hmi_controller_diagnostics_t *d);

/** Ask for a history read; one in progress goes on. */
void furnace_hmi_diagnostics_request_history(furnace_hmi_controller_diagnostics_t *d);

/**
 * Called at every poll. Follows the link (a link that is not current stops a
 * read and makes the health stale) and, while connected, gives the next read
 * that is due: out's type and body, without the envelope. Returns false when
 * nothing is due. After sending, the client calls furnace_hmi_diagnostics_sent.
 */
bool furnace_hmi_diagnostics_due(furnace_hmi_controller_diagnostics_t *d, uint32_t now_ms,
                                 bool connected, struct furnace_wire_message *out);

/** The read given by furnace_hmi_diagnostics_due was sent under request_id. */
void furnace_hmi_diagnostics_sent(furnace_hmi_controller_diagnostics_t *d,
                                  const struct furnace_wire_message *out, uint32_t request_id,
                                  uint32_t now_ms);

/** A HEALTH report of the current controller session (the client checked the session). */
void furnace_hmi_diagnostics_on_health(furnace_hmi_controller_diagnostics_t *d,
                                       const struct furnace_wire_health *h, uint32_t now_ms);

void furnace_hmi_diagnostics_on_history_page(furnace_hmi_controller_diagnostics_t *d,
                                             const struct furnace_wire_event_history_page *p,
                                             uint32_t now_ms);

/** COMMAND_REJECTED of request_id: true when it was a diagnostic read (asked again later). */
bool furnace_hmi_diagnostics_on_rejected(furnace_hmi_controller_diagnostics_t *d,
                                         uint32_t request_id);

#endif /* FURNACE_HMI_CONTROLLER_DIAGNOSTICS_INTERNAL_H */
