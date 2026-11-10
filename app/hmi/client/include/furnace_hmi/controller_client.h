/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_CLIENT_H
#define FURNACE_HMI_CONTROLLER_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace/wire.h>
#include <furnace/wire_messages.h>
#include <furnace_hmi/byte_port.h>
#include <furnace_hmi/controller_observation.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HELLO repetition while no answer arrives. A provisional HMI value (Phase 21 plan, M21-3). */
#define FURNACE_HMI_CLIENT_HELLO_RETRY_MS 1000U
/** A synchronization without SNAPSHOT_END for this long asks for the snapshot itself. */
#define FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS 3000U
/** Bytes one poll reads at most, so that a poll is bounded work. */
#define FURNACE_HMI_CLIENT_POLL_BYTES 512U

typedef struct {
    furnace_hmi_byte_port_t port;
    /**
     * A new client session ID, never 0, different at each HMI boot (controller
     * interface, client_session_id). Called at init and when the controller
     * shows that an ID was reused.
     */
    uint32_t (*new_client_session_id)(void *context);
    void *id_context;
} furnace_hmi_controller_client_config_t;

/**
 * The HMI's client of the controller link (WI-H06): session, synchronization,
 * freshness and the snapshot request. Portable and single-threaded: one
 * execution context calls poll; nothing blocks. It sends only HELLO, HEARTBEAT,
 * SNAPSHOT_REQUEST and CAPABILITIES_REQUEST: no command that changes the
 * controller. A caller-owned, statically allocated object.
 */
typedef struct {
    furnace_hmi_controller_client_config_t config;
    furnace_hmi_controller_observation_t observation;
    uint32_t generation;

    struct furnace_wire_rx rx;
    struct furnace_wire_tx tx;
    /* The message being handled, and the one being sent: separate, so a handler can answer. */
    struct furnace_wire_message message;
    struct furnace_wire_message out;

    uint32_t now_ms;
    uint32_t last_status_ms;
    uint32_t last_hello_ms;
    uint32_t last_send_ms;
    uint32_t sync_started_ms;
    bool hello_sent;
    uint32_t next_request_id;

    /* The snapshot being received. */
    bool snapshot_active;
    uint8_t pending_fault_count;
    furnace_hmi_controller_fault_t pending_faults[FURNACE_HMI_CONTROLLER_MAX_FAULTS];
    uint32_t newest_fault_event_seen;
    bool event_sequences_known;
} furnace_hmi_controller_client_t;

/** Start a client: a new client session, no controller known, HELLO due at once. */
void furnace_hmi_controller_client_init(
    furnace_hmi_controller_client_t *client,
    const furnace_hmi_controller_client_config_t *config,
    uint32_t now_ms
);

/**
 * Read what has arrived (at most FURNACE_HMI_CLIENT_POLL_BYTES), handle each
 * frame, and send what is due. now_ms is a monotonic millisecond clock; it may
 * wrap.
 */
void furnace_hmi_controller_client_poll(furnace_hmi_controller_client_t *client, uint32_t now_ms);

/** The current observation; valid until the next poll. */
const furnace_hmi_controller_observation_t *furnace_hmi_controller_client_observation(
    const furnace_hmi_controller_client_t *client
);

/** Changes whenever the observation changes, except status_age_ms alone. */
uint32_t furnace_hmi_controller_client_generation(const furnace_hmi_controller_client_t *client);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_CLIENT_H */
