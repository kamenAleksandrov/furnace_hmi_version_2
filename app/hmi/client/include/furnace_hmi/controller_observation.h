/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_OBSERVATION_H
#define FURNACE_HMI_CONTROLLER_OBSERVATION_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/dashboard_model.h>
#include <furnace_hmi/hmi_state_availability.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FURNACE_HMI_CONTROLLER_SESSION_ID_SIZE 16U
#define FURNACE_HMI_CONTROLLER_MODEL_ID_CAPACITY 32U
/** The controller's program_name field (wire format: 124 bytes of UTF-8). */
#define FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY 124U
/** The controller's fault table holds up to 27 entries (controller interface, FAULT_LIST). */
#define FURNACE_HMI_CONTROLLER_MAX_FAULTS 27U
/** status_age_ms before the first STATUS. */
#define FURNACE_HMI_CONTROLLER_AGE_NONE UINT32_MAX

/** The controller's `measured` validity codes (wire format §4); 0 is never valid. */
#define FURNACE_HMI_CONTROLLER_MEASURED_VALID 1U
#define FURNACE_HMI_CONTROLLER_MEASURED_STALE 2U
#define FURNACE_HMI_CONTROLLER_MEASURED_UNAVAILABLE 3U

/** Where the client stands with the controller. */
typedef enum {
    /** No accepted HELLO yet: HELLO is being sent. */
    FURNACE_HMI_LINK_CONNECTING = 0,
    /** The controller refused this protocol version. */
    FURNACE_HMI_LINK_INCOMPATIBLE,
    /** Connected; capabilities and snapshot not yet complete. */
    FURNACE_HMI_LINK_SYNCHRONIZING,
    /** Synchronized, and the newest STATUS is within the controller's link_silence_ms. */
    FURNACE_HMI_LINK_CONNECTED,
    /** Was connected; STATUS is overdue. HELLO is being sent again. */
    FURNACE_HMI_LINK_SILENT,
    FURNACE_HMI_LINK_COUNT,
} furnace_hmi_link_state_t;

/** Lifecycle of the client's one request, SNAPSHOT_REQUEST (PROTO-001). */
typedef enum {
    FURNACE_HMI_REQUEST_NONE = 0,
    FURNACE_HMI_REQUEST_PENDING,
    /** Answered with SNAPSHOT_END carrying its request ID. */
    FURNACE_HMI_REQUEST_COMPLETED,
    /** Answered with COMMAND_REJECTED; reject_reason holds the controller's code. */
    FURNACE_HMI_REQUEST_REJECTED,
    /** The controller session changed while it was pending: no result will come. */
    FURNACE_HMI_REQUEST_UNRESOLVED,
    FURNACE_HMI_REQUEST_COUNT,
} furnace_hmi_request_state_t;

/** One fault entry of the controller's FAULT_LIST; codes as the controller sends them. */
typedef struct {
    uint8_t fault_id;
    uint8_t source;
    uint8_t fault_class;
    bool condition_active;
    bool latched;
    bool acknowledged;
} furnace_hmi_controller_fault_t;

/**
 * What the HMI knows of the controller, as plain data (ADR-0003: no codec
 * structure). Every value comes from a controller frame, and controller codes
 * are kept as received; their meaning is the controller interface's. Values
 * belong to the controller session in session_id and are discarded when it
 * changes (PROTO-008).
 */
typedef struct {
    furnace_hmi_link_state_t link;
    /** CURRENT only when the link is CONNECTED; STALE when kept data is overdue. */
    furnace_hmi_state_availability_t availability;
    /** Milliseconds since the newest STATUS; FURNACE_HMI_CONTROLLER_AGE_NONE before one. */
    uint32_t status_age_ms;
    uint32_t client_session_id;

    /* Identity, from HELLO_ACK. */
    bool identity_known;
    uint8_t session_id[FURNACE_HMI_CONTROLLER_SESSION_ID_SIZE];
    uint32_t firmware_id;
    uint32_t boot_count;
    /** The controller's protocol version, from the HELLO_ACK header (also when refused). */
    bool controller_version_known;
    uint8_t controller_proto_major;
    uint8_t controller_proto_minor;
    /** The HELLO_ACK result code of the last answer. */
    uint8_t hello_result;
    /** Controller session changes seen since the client started. */
    uint32_t controller_session_changes;

    /* CAPABILITIES of this session. */
    bool capabilities_valid;
    char model_id[FURNACE_HMI_CONTROLLER_MODEL_ID_CAPACITY + 1U];
    bool configured;
    uint8_t config_state;
    uint32_t config_revision;
    uint32_t profile_revision;
    uint32_t status_interval_ms;
    uint32_t heartbeat_interval_ms;
    uint32_t link_silence_ms;

    /* The newest STATUS of this session. */
    bool status_valid;
    uint32_t status_sequence;
    int32_t temperature_decicelsius;
    uint8_t temperature_validity;
    uint32_t temperature_age_ms;
    uint8_t measurement_state;
    uint32_t measurement_qualified_s;
    uint8_t operating_mode;
    uint8_t run_state;
    uint8_t session_kind;
    uint32_t run_id;
    uint32_t program_id;
    uint32_t program_revision;
    uint8_t stage_index;
    uint8_t stage_count;
    uint8_t stage_kind;
    char program_name[FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U];
    uint32_t run_elapsed_s;
    /** The final cooling: NONE 0, ACTIVE 1, COMPLETE 2, ENDED 3 (controller interface). */
    uint8_t cooling;
    uint32_t cooling_s;
    bool setpoint_valid;
    int32_t setpoint_decicelsius;
    bool heating_requested;
    bool heating_inhibited;
    uint8_t latched_class;
    uint32_t inhibit_ids;
    uint8_t unacknowledged_faults;
    bool session_fresh;
    uint8_t store_readiness;
    bool execution_active;
    bool heating_authorized;
    /** The controller's lifecycle decides navigation, never a local flag (interface: Lifecycle). */
    bool navigation_permitted;
    uint8_t fan_feedback;
    uint8_t door_state;
    bool emergency_active;

    /* FAULT_LIST of the newest complete snapshot. */
    bool faults_valid;
    /** A fault changed, or a snapshot was lost or incomplete: the list is being asked for again. */
    bool faults_outdated;
    uint8_t fault_count;
    furnace_hmi_controller_fault_t faults[FURNACE_HMI_CONTROLLER_MAX_FAULTS];

    /* Event sequences: the newest applied RUN_EVENT and FAULT_EVENT. */
    uint32_t run_event_sequence;
    uint32_t fault_event_sequence;

    /* The snapshot request. */
    furnace_hmi_request_state_t snapshot_request;
    uint32_t snapshot_request_id;
    uint8_t snapshot_reject_reason;

    /* Link counters; saturating. */
    uint32_t frames_received;
    uint32_t frames_dropped;
    uint32_t frames_ignored;
    uint32_t snapshots_requested;
    uint32_t event_gaps;
    /** After a reconnect, a STATUS or snapshot older than the status held: a repeated session ID. */
    uint32_t status_regressions;
    /** Snapshots whose FAULT_LIST pages were missing or inconsistent: not applied. */
    uint32_t snapshots_incomplete;
    /** CAPABILITIES with link intervals outside the HMI's bounds: refused. */
    uint32_t capabilities_rejected;
    uint32_t write_shortfalls;
} furnace_hmi_controller_observation_t;

/**
 * Validity of the measured temperature for presentation (REQ-NFR-006): CURRENT
 * only when the controller marks it valid and the link is current; STALE when
 * the controller marks it stale, or a valid value is kept while the link is
 * stale; otherwise UNAVAILABLE.
 */
furnace_hmi_value_validity_t furnace_hmi_controller_temperature_validity(
    const furnace_hmi_controller_observation_t *observation
);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_OBSERVATION_H */
