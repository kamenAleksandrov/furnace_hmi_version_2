/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_CLIENT_H
#define FURNACE_HMI_CONTROLLER_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace/wire.h>
#include <furnace/wire_messages.h>
#include <furnace_hmi/byte_port.h>
#include <furnace_hmi/controller_command.h>
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
/*
 * Bounds on the controller's link intervals (CAPABILITIES), provisional HMI
 * values: outside them the capabilities are refused and the link never becomes
 * current. link_silence_ms must also exceed status_interval_ms.
 */
#define FURNACE_HMI_CLIENT_MAX_STATUS_INTERVAL_MS 60000U
#define FURNACE_HMI_CLIENT_MAX_HEARTBEAT_INTERVAL_MS 60000U
#define FURNACE_HMI_CLIENT_MAX_LINK_SILENCE_MS 600000U
/*
 * An unanswered command is sent again under the same request_id after this
 * long, up to the attempts below; provisional HMI values (plan, M21-5 design).
 */
#define FURNACE_HMI_CLIENT_COMMAND_RETRY_MS 1000U
#define FURNACE_HMI_CLIENT_COMMAND_ATTEMPTS 3U
#define FURNACE_HMI_CLIENT_STOP_ATTEMPTS 10U

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

/* A command being delivered: what is resent under the same request_id. */
typedef struct {
    furnace_hmi_command_t command;
    uint32_t last_sent_ms;
} furnace_hmi_controller_client_delivery_t;

/**
 * The HMI's client of the controller link (WI-H06): session, synchronization,
 * freshness and the snapshot request; since M21-5 the operator's commands,
 * which it sends only when submitted. Portable and single-threaded: one
 * execution context calls poll and submit; nothing blocks. A caller-owned,
 * statically allocated object.
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
    uint32_t snapshot_requested_ms;
    bool hello_sent;
    uint32_t next_request_id;
    uint32_t capabilities_request_id;
    /** The controller answered a request of this client session: its ID is not a reused one. */
    bool client_session_confirmed;

    /*
     * The status_sequence held when the link fell silent. After reconnecting to
     * the same session ID, nothing of that session can be older: something older
     * is another session that drew the same ID.
     */
    bool reconnect_baseline_active;
    uint32_t reconnect_baseline;

    /* The snapshot being received, and its FAULT_LIST pages. */
    bool snapshot_active;
    bool pending_fault_pages_ok;
    bool pending_fault_pages_seen;
    uint8_t pending_fault_total;
    uint32_t pending_fault_sequence;
    uint32_t pending_fault_request_id;
    uint8_t pending_fault_count;
    furnace_hmi_controller_fault_t pending_faults[FURNACE_HMI_CONTROLLER_MAX_FAULTS];
    /** The newest FAULT_EVENT since the snapshot was asked for. */
    uint32_t newest_fault_event_seen;
    bool event_sequences_known;

    /* The commands being delivered, one per slot (observation.command and .stop). */
    furnace_hmi_controller_client_delivery_t command_delivery;
    furnace_hmi_controller_client_delivery_t stop_delivery;
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

/**
 * Send an operator command now, in its slot (Stop or ordinary), and report it
 * as that slot's outcome with the caller's submission number. Refused before
 * sending, as observation.command_refusal, when the link is not current, the
 * slot's previous command is pending, or the kind is not a command; the slot is
 * then unchanged. Returns FURNACE_HMI_COMMAND_LOCAL_NONE when sent.
 */
furnace_hmi_command_local_t furnace_hmi_controller_client_submit(
    furnace_hmi_controller_client_t *client,
    const furnace_hmi_command_t *command,
    uint32_t submission
);

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
