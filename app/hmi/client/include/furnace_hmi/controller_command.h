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
    FURNACE_HMI_COMMAND_COUNT,
} furnace_hmi_command_kind_t;

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
     * FAULT_ACKNOWLEDGE_RESULT result, or FAULT_RECOVERY_RESULT reply.
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
