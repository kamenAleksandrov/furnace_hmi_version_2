/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_COMMAND_H
#define FURNACE_HMI_CONTROLLER_COMMAND_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The operator commands the HMI sends (Phase 21 M21-5; controller interface, Messages). */
typedef enum {
    FURNACE_HMI_COMMAND_NONE = 0,
    /** Asks for the fault record write that a Start needs; cannot cause heating. */
    FURNACE_HMI_COMMAND_START_PREPARE,
    FURNACE_HMI_COMMAND_MANUAL_START,
    FURNACE_HMI_COMMAND_MANUAL_RETARGET,
    FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH,
    FURNACE_HMI_COMMAND_PAUSE,
    FURNACE_HMI_COMMAND_RESUME,
    FURNACE_HMI_COMMAND_CONTINUE,
    FURNACE_HMI_COMMAND_STOP,
    FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE,
    FURNACE_HMI_COMMAND_FAULT_RECOVERY,
    /* M21-7a: library changes (PROGRAM_SAVE, PROGRAM_DELETE) and the program Start transaction. */
    FURNACE_HMI_COMMAND_PROGRAM_SAVE,
    FURNACE_HMI_COMMAND_PROGRAM_DELETE,
    FURNACE_HMI_COMMAND_PROGRAM_START,
    FURNACE_HMI_COMMAND_START_CONFIRM,
    FURNACE_HMI_COMMAND_START_CANCEL,
    FURNACE_HMI_COMMAND_COUNT,
} furnace_hmi_command_kind_t;

/** The controller's program name: UTF-8, at most 124 bytes (wire format). */
#define FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY 124U
/** Stages of one program on the wire (PROGRAM_DATA, PROGRAM_SAVE). */
#define FURNACE_HMI_CONTROLLER_MAX_STAGES 18U
/** The controller's stage kinds (controller interface: Program library). */
#define FURNACE_HMI_CONTROLLER_STAGE_HEATING 1U
#define FURNACE_HMI_CONTROLLER_STAGE_HOLD 2U

/** One program stage as the controller holds it: Heating has a target and rate, Hold a duration. */
typedef struct {
    uint8_t kind;
    int32_t target_decicelsius;
    int32_t rate_decicelsius_per_min;
    uint32_t duration_minutes;
} furnace_hmi_controller_stage_t;

/** One operator command as plain data (ADR-0003); fields beyond the kind's are ignored. */
typedef struct {
    furnace_hmi_command_kind_t kind;
    /** Retarget, length, Pause, Resume, Continue, Stop: the run the operator sees. */
    uint32_t run_id;
    /** Manual Start and retarget, in 0.1 °C and 0.1 °C/min. */
    int32_t target_decicelsius;
    int32_t rate_decicelsius_per_min;
    /** Manual Start and length. */
    uint32_t session_minutes;
    /** Fault acknowledgement and recovery: the entry, as FAULT_LIST names it. */
    uint8_t fault_id;
    uint8_t source;
    /**
     * Save: the program (0 creates) and the revision it was edited from (0 when
     * creating). Delete, program Start and Confirm: the program and the exact
     * revision the operator saw.
     */
    uint32_t program_id;
    uint32_t program_revision;
    /** Save: the controller's estimate the operator acknowledged (0 none). */
    uint32_t acknowledged_duration_s;
    /** Save: the whole program, name NUL-terminated. */
    char name[FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U];
    uint8_t stage_count;
    furnace_hmi_controller_stage_t stages[FURNACE_HMI_CONTROLLER_MAX_STAGES];
    /** Confirm and cancel: the transaction; Confirm: exactly the proposal shown. */
    uint32_t transaction_id;
    uint32_t proposal_revision;
    uint8_t proposed_stage;
    /** Confirm: the duration the operator accepted, when the proposal requires it. */
    uint32_t accepted_duration_s;
} furnace_hmi_command_t;

/** Where a command stands. Only the controller's result makes it accepted or rejected (API-03). */
typedef enum {
    FURNACE_HMI_COMMAND_IDLE = 0,
    /** Sent, or being sent again under the same request_id; no result yet. */
    FURNACE_HMI_COMMAND_PENDING,
    /** The controller applied it: its result message said so. */
    FURNACE_HMI_COMMAND_ACCEPTED,
    /** The controller's result refused it; reason holds the controller's code. */
    FURNACE_HMI_COMMAND_REJECTED,
    /** COMMAND_REJECTED: the controller's service refused it before the core; no effect. */
    FURNACE_HMI_COMMAND_REFUSED,
    /** The outcome is not known: the state comes from STATUS. unresolved_cause says why. */
    FURNACE_HMI_COMMAND_UNRESOLVED,
    /**
     * Program Start only: the controller answered with a proposal that awaits
     * the operator's Confirm or Cancel (observation.start_proposal). Nothing heats.
     */
    FURNACE_HMI_COMMAND_PROPOSED,
    FURNACE_HMI_COMMAND_STATE_COUNT,
} furnace_hmi_command_state_t;

typedef enum {
    FURNACE_HMI_COMMAND_UNRESOLVED_NONE = 0,
    /** No answer after every attempt. */
    FURNACE_HMI_COMMAND_UNRESOLVED_NO_ANSWER,
    /** The link fell silent while it was pending. */
    FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
    /** Another controller session: a restart, which ends every run. */
    FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED,
    /** The controller reported DUPLICATE_REQUEST on a retry: it may or may not have run. */
    FURNACE_HMI_COMMAND_UNRESOLVED_DUPLICATE,
    /** STOP_UNRESOLVED: heater-off and fan-on asserted, the Stop's own outcome unknown. */
    FURNACE_HMI_COMMAND_UNRESOLVED_STOP,
    /** The HMI opened a new client session; a result goes only to the one that sent it. */
    FURNACE_HMI_COMMAND_UNRESOLVED_CLIENT_SESSION_CHANGED,
    /** A library change answered UNRESOLVED: it may still take effect (the library is listed again). */
    FURNACE_HMI_COMMAND_UNRESOLVED_BY_CONTROLLER,
    FURNACE_HMI_COMMAND_UNRESOLVED_COUNT,
} furnace_hmi_command_unresolved_t;

/** Why the HMI refused a command before sending it; nothing was sent. */
typedef enum {
    /** Not refused: sent. */
    FURNACE_HMI_COMMAND_LOCAL_NONE = 0,
    /** The link is not current (CONNECTED and synchronized). */
    FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT,
    /** A command of the same slot is still pending. */
    FURNACE_HMI_COMMAND_LOCAL_BUSY,
    /** Not a command kind the client sends. */
    FURNACE_HMI_COMMAND_LOCAL_INVALID,
    /** A Stop handed over before it was sent: the earlier command is dropped, never sent after it. */
    FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP,
    FURNACE_HMI_COMMAND_LOCAL_COUNT,
} furnace_hmi_command_local_t;

/** The latest command of one slot and its outcome, with controller codes as received. */
typedef struct {
    /** The submitter's number for it (0: none); a screen matches its own command by it. */
    uint32_t submission;
    furnace_hmi_command_kind_t kind;
    furnace_hmi_command_state_t state;
    uint32_t request_id;
    /** Sends so far, the first included. */
    uint8_t attempts;
    furnace_hmi_command_unresolved_t unresolved_cause;
    /**
     * The controller's reason: RUN_COMMAND_* reason, COMMAND_REJECTED reason,
     * FAULT_ACKNOWLEDGE_RESULT result, FAULT_RECOVERY_RESULT reply,
     * PROGRAM_SAVE_RESULT or PROGRAM_DELETE_RESULT result, or START_REJECTED reason.
     */
    uint8_t reason;
    /** RUN_COMMAND_*: pause-reason codes that block a Resume or Continue. */
    uint32_t blocking_conditions;
    /** RUN_COMMAND_*: the run it applied to; for Manual Start the new session's ID. */
    uint32_t run_id;
    /** FAULT_RECOVERY_RESULT: the entry was found. */
    bool fault_found;
    /** START_PREPARE_RESULT: the readiness when it was applied. */
    uint8_t store_readiness;
    /** Save and delete results: the program, and a save's new revision when SAVED. */
    uint32_t program_id;
    uint32_t program_revision;
    /** PROGRAM_SAVE_RESULT: the first validation error (core/program codes). */
    uint8_t error_reason;
    uint8_t error_field;
    uint8_t error_stage;
    /** PROGRAM_SAVE_RESULT: the planning estimate and its reference (0.1 °C). */
    uint32_t curing_estimate_s;
    int32_t planning_reference_decicelsius;
    /** Program Start and Confirm: the transaction; START_ACCEPTED: the run is run_id. */
    uint32_t transaction_id;
} furnace_hmi_command_outcome_t;

/** The latest command the HMI refused before sending; it never replaces a slot's outcome. */
typedef struct {
    uint32_t submission;
    furnace_hmi_command_kind_t kind;
    furnace_hmi_command_local_t reason;
} furnace_hmi_command_refusal_t;

/** True for the kinds that use the Stop slot. */
bool furnace_hmi_command_is_stop(furnace_hmi_command_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_COMMAND_H */
