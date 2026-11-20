/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The HMI's controller client (Phase 21 M21-3). The rules are the controller
 * interface's "Session, link and delivery"; the plan's M21-3 design lists how
 * this client applies them.
 */

#include <stddef.h>
#include <string.h>

#include <furnace_hmi/controller_client.h>

#include "controller_diagnostics.h"

/* HELLO_ACK results and COMMAND_REJECTED reasons (controller interface). */
#define HELLO_OK 0U
#define HELLO_VERSION_UNSUPPORTED 1U
#define HELLO_INVALID_CLIENT_SESSION 2U
#define REJECT_NOT_CONNECTED 1U
#define REJECT_STALE_SESSION 2U
#define REJECT_CLIENT_SESSION_MISMATCH 3U
#define REJECT_DUPLICATE_REQUEST 4U
#define REJECT_STOP_UNRESOLVED 13U
#define CLIENT_KIND_HMI 1U
/* FAULT_ACKNOWLEDGE_RESULT and FAULT_RECOVERY_RESULT codes (controller interface). */
#define ACKNOWLEDGE_DONE 0U
#define RECOVERY_RECOVERED 7U
/* Library results (controller interface: Program library). */
#define LIBRARY_OK 0U
#define LIBRARY_STORAGE_UNAVAILABLE 1U
#define DATA_DAMAGED 3U
#define SAVE_SAVED 0U
#define DELETE_DELETED 0U
#define LIBRARY_UNRESOLVED 10U
#define SETTING_PLANNING_REFERENCE 1U
#define SETTING_APPLIED 0U
/* Start transaction codes: the start reason DUPLICATE_REQUEST and the STATUS start states. */
#define START_DUPLICATE_REQUEST 5U
#define START_STATE_CONSUMED 4U
#define START_STATE_CANCELLED 5U
#define START_STATE_INVALIDATED 6U

#define READ_CHUNK 64U

/* The copies between the plain data and the codec assume these capacities (review L6). */
_Static_assert(sizeof(((furnace_hmi_command_t *)0)->name) ==
                   sizeof(((struct furnace_wire_program_save *)0)->name),
               "a save's name is copied whole");
_Static_assert(sizeof(((struct furnace_wire_library_entry *)0)->name) ==
                   FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U,
               "library names fit");
_Static_assert(FURNACE_HMI_CONTROLLER_MAX_STAGES ==
                   sizeof(((struct furnace_wire_program_data *)0)->stages) /
                       sizeof(((struct furnace_wire_program_data *)0)->stages[0]),
               "program stages match the wire");
_Static_assert(FURNACE_HMI_CONTROLLER_MAX_STAGES ==
                   sizeof(((struct furnace_wire_start_proposal *)0)->skipped_stages) /
                       sizeof(((struct furnace_wire_start_proposal *)0)->skipped_stages[0]),
               "skipped stages match the wire");
_Static_assert(FURNACE_HMI_CONTROLLER_MAX_PROPOSAL_WARNINGS ==
                   sizeof(((struct furnace_wire_start_proposal *)0)->warnings),
               "proposal warnings match the wire");
_Static_assert(FURNACE_HMI_CLIENT_LIBRARY_PAGE ==
                   sizeof(((struct furnace_wire_program_list *)0)->entries) /
                       sizeof(((struct furnace_wire_program_list *)0)->entries[0]),
               "a list page as the wire holds it");
_Static_assert(FURNACE_HMI_CONTROLLER_HEALTH_SUBSYSTEMS ==
                   sizeof(((struct furnace_wire_health *)0)->subsystems) /
                       sizeof(((struct furnace_wire_health *)0)->subsystems[0]),
               "health subsystems as the wire holds them");
_Static_assert(FURNACE_HMI_CONTROLLER_HISTORY_PAGE ==
                   sizeof(((struct furnace_wire_event_history_page *)0)->entries) /
                       sizeof(((struct furnace_wire_event_history_page *)0)->entries[0]),
               "a history page as the wire holds it");
_Static_assert(FURNACE_HMI_CONTROLLER_EVENT_PARAMS ==
                   sizeof(((struct furnace_wire_history_entry *)0)->params) /
                       sizeof(((struct furnace_wire_history_entry *)0)->params[0]),
               "entry parameters as the wire holds them");

/* Where the library mirror's reading stands (client.library_step). */
#define LIBRARY_IDLE 0U
#define LIBRARY_LISTING 1U
#define LIBRARY_READING 2U

static void unresolve(furnace_hmi_controller_client_t *c, bool stop,
                      furnace_hmi_command_unresolved_t cause);
static void library_discard(furnace_hmi_controller_client_t *c);
static void end_open_proposal(furnace_hmi_controller_client_t *c);

static void saturating_increment(uint32_t *counter)
{
    if (*counter != UINT32_MAX) {
        (*counter)++;
    }
}

static bool elapsed(uint32_t now, uint32_t since, uint32_t interval)
{
    return (uint32_t)(now - since) >= interval;
}

static void changed(furnace_hmi_controller_client_t *c)
{
    c->generation++;
}

static void set_link(furnace_hmi_controller_client_t *c, furnace_hmi_link_state_t link)
{
    if (c->observation.link != link) {
        c->observation.link = link;
        changed(c);
    }
}

/* ---- sending ------------------------------------------------------------------------------ */

/*
 * The port takes the whole frame or reports a shortfall. The Zephyr UART port
 * writes byte by byte and always takes everything, so there it never counts.
 */
static void send_out(furnace_hmi_controller_client_t *c)
{
    size_t written;

    if (furnace_wire_tx_encode(&c->tx, &c->out) != FURNACE_WIRE_OK) {
        return;
    }
    written = c->config.port.write(c->config.port.context, c->tx.encoded, c->tx.encoded_len);
    if (written != c->tx.encoded_len) {
        saturating_increment(&c->observation.write_shortfalls);
    }
    c->last_send_ms = c->now_ms;
}

static void send_hello(furnace_hmi_controller_client_t *c)
{
    /* Re-opening the session ends the operator's intent; a Stop waits (M21-5 design; review F1). */
    unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST);
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_HELLO;
    c->out.u.hello.client_session_id = c->observation.client_session_id;
    c->out.u.hello.client_kind = CLIENT_KIND_HMI;
    send_out(c);
    c->hello_sent = true;
    c->last_hello_ms = c->now_ms;
}

static void send_heartbeat(furnace_hmi_controller_client_t *c)
{
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_HEARTBEAT;
    c->out.u.heartbeat.client_session_id = c->observation.client_session_id;
    send_out(c);
}

static void new_client_session(furnace_hmi_controller_client_t *c)
{
    uint32_t id = c->config.new_client_session_id(c->config.id_context);

    if (id == 0U || id == c->observation.client_session_id) {
        id = c->observation.client_session_id + 1U;
        id = (id == 0U) ? 1U : id;
    }
    c->observation.client_session_id = id;
    c->next_request_id = 1U;
    c->capabilities_request_id = 0U;
    c->client_session_confirmed = false;
    /* A result goes only to the client session that sent the command. */
    unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_CLIENT_SESSION_CHANGED);
    unresolve(c, true, FURNACE_HMI_COMMAND_UNRESOLVED_CLIENT_SESSION_CHANGED);
    /* Another client session invalidates the transaction (CLIENT_SESSION_CHANGED; review L5). */
    end_open_proposal(c);
    changed(c);
}

/*
 * The next request ID of this client session. request_id never wraps: at
 * 2^32-1 the client starts a new client session with HELLO instead (spec).
 */
static bool take_request_id(furnace_hmi_controller_client_t *c, uint32_t *id)
{
    if (c->next_request_id == UINT32_MAX) {
        new_client_session(c);
        send_hello(c);
        return false;
    }
    *id = c->next_request_id++;
    return true;
}

static bool fill_envelope(furnace_hmi_controller_client_t *c, struct furnace_wire_envelope *env)
{
    if (!take_request_id(c, &env->request_id)) {
        return false;
    }
    (void)memcpy(env->session_id, c->observation.session_id, sizeof(env->session_id));
    env->client_session_id = c->observation.client_session_id;
    return true;
}

static void request_capabilities(furnace_hmi_controller_client_t *c)
{
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_CAPABILITIES_REQUEST;
    if (!fill_envelope(c, &c->out.u.capabilities_request.envelope)) {
        return;
    }
    c->capabilities_request_id = c->out.u.capabilities_request.envelope.request_id;
    send_out(c);
}

/*
 * Ask for a snapshot. A new request supersedes one still pending: only the
 * newer gets SNAPSHOT_END. The fault list counts as outdated until it arrives.
 */
static void request_snapshot(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    if (!o->identity_known) {
        return;
    }
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_SNAPSHOT_REQUEST;
    if (!fill_envelope(c, &c->out.u.snapshot_request.envelope)) {
        return;
    }
    o->snapshot_request = FURNACE_HMI_REQUEST_PENDING;
    o->snapshot_request_id = c->out.u.snapshot_request.envelope.request_id;
    o->snapshot_reject_reason = 0U;
    o->faults_outdated = true;
    saturating_increment(&o->snapshots_requested);
    c->snapshot_active = true;
    c->snapshot_requested_ms = c->now_ms;
    c->pending_fault_pages_seen = false;
    c->newest_fault_event_seen = 0U;
    send_out(c);
    changed(c);
}

/* Ask for a snapshot unless one is already on its way. */
static void refresh(furnace_hmi_controller_client_t *c)
{
    c->observation.faults_outdated = true;
    if (c->observation.snapshot_request != FURNACE_HMI_REQUEST_PENDING) {
        request_snapshot(c);
    }
}

/* ---- operator commands (M21-5; plan, M21-5 design) ---------------------------------------- */

bool furnace_hmi_command_is_stop(furnace_hmi_command_kind_t kind)
{
    return kind == FURNACE_HMI_COMMAND_STOP;
}

static furnace_hmi_command_outcome_t *outcome_of(furnace_hmi_controller_client_t *c, bool stop)
{
    return stop ? &c->observation.stop : &c->observation.command;
}

static furnace_hmi_controller_client_delivery_t *delivery_of(furnace_hmi_controller_client_t *c,
                                                             bool stop)
{
    return stop ? &c->stop_delivery : &c->command_delivery;
}

/* The command as its message, with the envelope of this request_id. */
static bool encode_command(furnace_hmi_controller_client_t *c, const furnace_hmi_command_t *cmd,
                           uint32_t request_id)
{
    struct furnace_wire_message *m = &c->out;
    struct furnace_wire_envelope *env;

    (void)memset(m, 0, sizeof(*m));
    switch (cmd->kind) {
    case FURNACE_HMI_COMMAND_START_PREPARE:
        m->type = FURNACE_WIRE_MSG_START_PREPARE;
        env = &m->u.start_prepare.envelope;
        break;
    case FURNACE_HMI_COMMAND_MANUAL_START:
        m->type = FURNACE_WIRE_MSG_MANUAL_START;
        env = &m->u.manual_start.envelope;
        m->u.manual_start.target = cmd->target_decicelsius;
        m->u.manual_start.rate = cmd->rate_decicelsius_per_min;
        m->u.manual_start.session_minutes = cmd->session_minutes;
        break;
    case FURNACE_HMI_COMMAND_MANUAL_RETARGET:
        m->type = FURNACE_WIRE_MSG_MANUAL_RETARGET;
        env = &m->u.manual_retarget.envelope;
        m->u.manual_retarget.run_id = cmd->run_id;
        m->u.manual_retarget.target = cmd->target_decicelsius;
        m->u.manual_retarget.rate = cmd->rate_decicelsius_per_min;
        break;
    case FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH:
        m->type = FURNACE_WIRE_MSG_MANUAL_SET_LENGTH;
        env = &m->u.manual_set_length.envelope;
        m->u.manual_set_length.run_id = cmd->run_id;
        m->u.manual_set_length.session_minutes = cmd->session_minutes;
        break;
    case FURNACE_HMI_COMMAND_PAUSE:
        m->type = FURNACE_WIRE_MSG_RUN_PAUSE;
        env = &m->u.run_pause.envelope;
        m->u.run_pause.run_id = cmd->run_id;
        break;
    case FURNACE_HMI_COMMAND_RESUME:
        m->type = FURNACE_WIRE_MSG_RESUME_REQUEST;
        env = &m->u.resume_request.envelope;
        m->u.resume_request.run_id = cmd->run_id;
        break;
    case FURNACE_HMI_COMMAND_CONTINUE:
        m->type = FURNACE_WIRE_MSG_RUN_CONTINUE;
        env = &m->u.run_continue.envelope;
        m->u.run_continue.run_id = cmd->run_id;
        break;
    case FURNACE_HMI_COMMAND_STOP:
        m->type = FURNACE_WIRE_MSG_RUN_STOP;
        env = &m->u.run_stop.envelope;
        m->u.run_stop.run_id = cmd->run_id;
        break;
    case FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE:
        m->type = FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE;
        env = &m->u.fault_acknowledge.envelope;
        m->u.fault_acknowledge.fault_id = cmd->fault_id;
        m->u.fault_acknowledge.source = cmd->source;
        break;
    case FURNACE_HMI_COMMAND_FAULT_RECOVERY:
        m->type = FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST;
        env = &m->u.fault_recovery_request.envelope;
        m->u.fault_recovery_request.fault_id = cmd->fault_id;
        m->u.fault_recovery_request.source = cmd->source;
        break;
    case FURNACE_HMI_COMMAND_PROGRAM_SAVE:
        m->type = FURNACE_WIRE_MSG_PROGRAM_SAVE;
        env = &m->u.program_save.envelope;
        m->u.program_save.program_id = cmd->program_id;
        m->u.program_save.base_revision = cmd->program_revision;
        m->u.program_save.acknowledged_duration_s = cmd->acknowledged_duration_s;
        (void)memcpy(m->u.program_save.name, cmd->name, sizeof(m->u.program_save.name));
        m->u.program_save.stages_count = cmd->stage_count;
        for (uint8_t i = 0U; i < cmd->stage_count; i++) {
            m->u.program_save.stages[i].kind = cmd->stages[i].kind;
            m->u.program_save.stages[i].target = cmd->stages[i].target_decicelsius;
            m->u.program_save.stages[i].rate = cmd->stages[i].rate_decicelsius_per_min;
            m->u.program_save.stages[i].duration_minutes = cmd->stages[i].duration_minutes;
        }
        break;
    case FURNACE_HMI_COMMAND_PROGRAM_DELETE:
        m->type = FURNACE_WIRE_MSG_PROGRAM_DELETE;
        env = &m->u.program_delete.envelope;
        m->u.program_delete.program_id = cmd->program_id;
        m->u.program_delete.program_revision = cmd->program_revision;
        break;
    case FURNACE_HMI_COMMAND_PROGRAM_START:
        m->type = FURNACE_WIRE_MSG_START_REQUEST;
        env = &m->u.start_request.envelope;
        m->u.start_request.program_id = cmd->program_id;
        m->u.start_request.program_revision = cmd->program_revision;
        break;
    case FURNACE_HMI_COMMAND_START_CONFIRM:
        m->type = FURNACE_WIRE_MSG_START_CONFIRM;
        env = &m->u.start_confirm.envelope;
        m->u.start_confirm.transaction_id = cmd->transaction_id;
        m->u.start_confirm.proposal_revision = cmd->proposal_revision;
        m->u.start_confirm.program_id = cmd->program_id;
        m->u.start_confirm.program_revision = cmd->program_revision;
        m->u.start_confirm.proposed_stage = cmd->proposed_stage;
        m->u.start_confirm.accepted_duration_s = cmd->accepted_duration_s;
        break;
    case FURNACE_HMI_COMMAND_START_CANCEL:
        m->type = FURNACE_WIRE_MSG_START_CANCEL;
        env = &m->u.start_cancel.envelope;
        m->u.start_cancel.transaction_id = cmd->transaction_id;
        break;
    case FURNACE_HMI_COMMAND_SETTING_CHANGE:
        m->type = FURNACE_WIRE_MSG_SETTING_CHANGE;
        env = &m->u.setting_change.envelope;
        m->u.setting_change.setting = cmd->setting;
        m->u.setting_change.value = cmd->setting_value;
        break;
    case FURNACE_HMI_COMMAND_NONE:
    case FURNACE_HMI_COMMAND_COUNT:
    default:
        return false;
    }
    (void)memcpy(env->session_id, c->observation.session_id, sizeof(env->session_id));
    env->client_session_id = c->observation.client_session_id;
    env->request_id = request_id;
    return true;
}

/* Send a slot's command (again) under its request_id. */
static void send_command(furnace_hmi_controller_client_t *c, bool stop)
{
    furnace_hmi_command_outcome_t *o = outcome_of(c, stop);
    furnace_hmi_controller_client_delivery_t *d = delivery_of(c, stop);

    if (encode_command(c, &d->command, o->request_id)) {
        send_out(c);
        d->last_sent_ms = c->now_ms;
        o->attempts++;
        changed(c);
    }
}

static void unresolve(furnace_hmi_controller_client_t *c, bool stop,
                      furnace_hmi_command_unresolved_t cause)
{
    furnace_hmi_command_outcome_t *o = outcome_of(c, stop);

    if (o->state == FURNACE_HMI_COMMAND_PENDING) {
        o->state = FURNACE_HMI_COMMAND_UNRESOLVED;
        o->unresolved_cause = cause;
        changed(c);
    }
}

/*
 * The pending command of this request_id and kind, if any. Results carry no
 * client session, so the kind is checked too (review F7).
 */
static furnace_hmi_command_outcome_t *pending_command(furnace_hmi_controller_client_t *c,
                                                      uint32_t request_id,
                                                      furnace_hmi_command_kind_t kind)
{
    furnace_hmi_command_outcome_t *o = outcome_of(c, furnace_hmi_command_is_stop(kind));

    if (kind != FURNACE_HMI_COMMAND_NONE && o->state == FURNACE_HMI_COMMAND_PENDING &&
        o->request_id == request_id && o->kind == kind) {
        return o;
    }
    return NULL;
}

/* RUN_COMMAND_* command codes (controller interface). */
static furnace_hmi_command_kind_t kind_of_run_command(uint8_t command)
{
    switch (command) {
    case 1U:
        return FURNACE_HMI_COMMAND_PAUSE;
    case 2U:
        return FURNACE_HMI_COMMAND_RESUME;
    case 3U:
        return FURNACE_HMI_COMMAND_STOP;
    case 4U:
        return FURNACE_HMI_COMMAND_CONTINUE;
    case 6U:
        return FURNACE_HMI_COMMAND_MANUAL_START;
    case 7U:
        return FURNACE_HMI_COMMAND_MANUAL_RETARGET;
    case 8U:
        return FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH;
    default:
        return FURNACE_HMI_COMMAND_NONE;
    }
}

/* The command a COMMAND_REJECTED names by its message type. */
static furnace_hmi_command_kind_t kind_of_message(uint16_t msg_type)
{
    switch (msg_type) {
    case FURNACE_WIRE_MSG_START_PREPARE:
        return FURNACE_HMI_COMMAND_START_PREPARE;
    case FURNACE_WIRE_MSG_MANUAL_START:
        return FURNACE_HMI_COMMAND_MANUAL_START;
    case FURNACE_WIRE_MSG_MANUAL_RETARGET:
        return FURNACE_HMI_COMMAND_MANUAL_RETARGET;
    case FURNACE_WIRE_MSG_MANUAL_SET_LENGTH:
        return FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH;
    case FURNACE_WIRE_MSG_RUN_PAUSE:
        return FURNACE_HMI_COMMAND_PAUSE;
    case FURNACE_WIRE_MSG_RESUME_REQUEST:
        return FURNACE_HMI_COMMAND_RESUME;
    case FURNACE_WIRE_MSG_RUN_CONTINUE:
        return FURNACE_HMI_COMMAND_CONTINUE;
    case FURNACE_WIRE_MSG_RUN_STOP:
        return FURNACE_HMI_COMMAND_STOP;
    case FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE:
        return FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE;
    case FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST:
        return FURNACE_HMI_COMMAND_FAULT_RECOVERY;
    case FURNACE_WIRE_MSG_PROGRAM_SAVE:
        return FURNACE_HMI_COMMAND_PROGRAM_SAVE;
    case FURNACE_WIRE_MSG_PROGRAM_DELETE:
        return FURNACE_HMI_COMMAND_PROGRAM_DELETE;
    case FURNACE_WIRE_MSG_START_REQUEST:
        return FURNACE_HMI_COMMAND_PROGRAM_START;
    case FURNACE_WIRE_MSG_START_CONFIRM:
        return FURNACE_HMI_COMMAND_START_CONFIRM;
    case FURNACE_WIRE_MSG_START_CANCEL:
        return FURNACE_HMI_COMMAND_START_CANCEL;
    case FURNACE_WIRE_MSG_SETTING_CHANGE:
        return FURNACE_HMI_COMMAND_SETTING_CHANGE;
    default:
        return FURNACE_HMI_COMMAND_NONE;
    }
}

/* Save and delete: answered once written, so retried later and fewer times (M21-7 design). */
static bool is_library_change(furnace_hmi_command_kind_t kind)
{
    return kind == FURNACE_HMI_COMMAND_PROGRAM_SAVE || kind == FURNACE_HMI_COMMAND_PROGRAM_DELETE;
}

static void complete(furnace_hmi_controller_client_t *c, furnace_hmi_command_outcome_t *o,
                     bool accepted, uint8_t reason)
{
    o->state = accepted ? FURNACE_HMI_COMMAND_ACCEPTED : FURNACE_HMI_COMMAND_REJECTED;
    o->reason = reason;
    c->client_session_confirmed = true;
    changed(c);
}

static void on_run_command_result(furnace_hmi_controller_client_t *c, bool accepted,
                                  uint32_t request_id, uint8_t command, uint8_t reason,
                                  uint32_t blocking, uint32_t run_id)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, request_id, kind_of_run_command(command));

    if (o != NULL) {
        o->blocking_conditions = blocking;
        o->run_id = run_id;
        complete(c, o, accepted, reason);
    }
}

static void on_fault_acknowledge_result(furnace_hmi_controller_client_t *c,
                                        const struct furnace_wire_fault_acknowledge_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE);

    if (o != NULL) {
        complete(c, o, r->result == ACKNOWLEDGE_DONE, r->result);
    }
}

static void on_fault_recovery_result(furnace_hmi_controller_client_t *c,
                                     const struct furnace_wire_fault_recovery_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_FAULT_RECOVERY);

    if (o != NULL) {
        o->fault_found = r->found;
        /* The reply means nothing when the entry was not found (interface): 0. */
        complete(c, o, r->found && r->reply == RECOVERY_RECOVERED, r->found ? r->reply : 0U);
    }
}

static void on_start_prepare_result(furnace_hmi_controller_client_t *c,
                                    const struct furnace_wire_start_prepare_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_START_PREPARE);

    if (o != NULL) {
        o->store_readiness = r->store_readiness;
        complete(c, o, true, 0U);
    }
}

/* ---- library changes and the program Start (M21-7a; plan, M21-7 design) ------------------ */

static void on_program_save_result(furnace_hmi_controller_client_t *c,
                                   const struct furnace_wire_program_save_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_PROGRAM_SAVE);

    if (o == NULL) {
        return;
    }
    o->program_id = r->program_id;
    o->program_revision = r->program_revision;
    o->error_reason = r->error_reason;
    o->error_field = r->error_field;
    o->error_stage = r->error_stage;
    o->curing_estimate_s = r->curing_estimate_s;
    o->planning_reference_decicelsius = r->planning_reference;
    /* After a change, or one that may still take effect, the library is listed again (review M3). */
    c->library_relist = (r->result == SAVE_SAVED || r->result == LIBRARY_UNRESOLVED);
    if (r->result == LIBRARY_UNRESOLVED) {
        o->reason = r->result;
        c->client_session_confirmed = true;
        unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_BY_CONTROLLER);
        return;
    }
    complete(c, o, r->result == SAVE_SAVED, r->result);
}

static void on_program_delete_result(furnace_hmi_controller_client_t *c,
                                     const struct furnace_wire_program_delete_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_PROGRAM_DELETE);

    if (o == NULL) {
        return;
    }
    o->program_id = r->program_id;
    c->library_relist = (r->result == DELETE_DELETED || r->result == LIBRARY_UNRESOLVED);
    if (r->result == LIBRARY_UNRESOLVED) {
        o->reason = r->result;
        c->client_session_confirmed = true;
        unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_BY_CONTROLLER);
        return;
    }
    complete(c, o, r->result == DELETE_DELETED, r->result);
}

/* ---- the planning reference (M21-7d; controller interface, Settings) ---------------------- */

static void on_settings(furnace_hmi_controller_client_t *c, const struct furnace_wire_settings *s)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    /* SETTINGS names no session: taken only from the session whose identity is known. */
    if (!o->identity_known) {
        saturating_increment(&o->frames_ignored);
        return;
    }
    o->settings_valid = true;
    o->settings_state = s->settings_state;
    o->planning_reference_decicelsius = s->planning_reference;
    changed(c);
}

static void on_setting_result(furnace_hmi_controller_client_t *c,
                              const struct furnace_wire_setting_result *r)
{
    furnace_hmi_command_outcome_t *o =
        pending_command(c, r->request_id, FURNACE_HMI_COMMAND_SETTING_CHANGE);

    if (o == NULL) {
        return;
    }
    /* The value in effect afterwards, whatever the result (interface, Settings). */
    o->setting_value = r->value;
    if (r->setting == SETTING_PLANNING_REFERENCE) {
        c->observation.settings_valid = true;
        c->observation.planning_reference_decicelsius = r->value;
    }
    complete(c, o, r->result == SETTING_APPLIED, r->result);
}

/* The pending Start-transaction command this result answers, of any of the given kinds. */
static furnace_hmi_command_outcome_t *pending_start(furnace_hmi_controller_client_t *c,
                                                    uint32_t request_id, bool request,
                                                    bool confirm, bool cancel)
{
    furnace_hmi_command_outcome_t *o = NULL;

    if (request) {
        o = pending_command(c, request_id, FURNACE_HMI_COMMAND_PROGRAM_START);
    }
    if (o == NULL && confirm) {
        o = pending_command(c, request_id, FURNACE_HMI_COMMAND_START_CONFIRM);
    }
    if (o == NULL && cancel) {
        o = pending_command(c, request_id, FURNACE_HMI_COMMAND_START_CANCEL);
    }
    return o;
}

/* The open proposal of this transaction, if any. */
static furnace_hmi_controller_start_proposal_t *open_proposal(furnace_hmi_controller_client_t *c,
                                                              uint32_t transaction_id)
{
    furnace_hmi_controller_start_proposal_t *p = &c->observation.start_proposal;

    return (p->state == FURNACE_HMI_PROPOSAL_OPEN && p->transaction_id == transaction_id) ? p
                                                                                          : NULL;
}

static void end_proposal(furnace_hmi_controller_client_t *c,
                         furnace_hmi_controller_start_proposal_t *p,
                         furnace_hmi_proposal_state_t state, uint8_t reason)
{
    if (p != NULL) {
        p->state = state;
        p->invalidation_reason = reason;
        changed(c);
    }
}

static void end_open_proposal(furnace_hmi_controller_client_t *c)
{
    end_proposal(c, open_proposal(c, c->observation.start_proposal.transaction_id),
                 FURNACE_HMI_PROPOSAL_ENDED, 0U);
}

/*
 * The answer to the latest Start of this client session, also after that Start
 * became unresolved: the controller's transaction is then still shown (review M2).
 */
static bool answers_latest_start(const furnace_hmi_controller_client_t *c, uint32_t request_id)
{
    const furnace_hmi_command_outcome_t *o = &c->observation.command;

    return request_id != 0U && o->kind == FURNACE_HMI_COMMAND_PROGRAM_START &&
           o->request_id == request_id && o->state == FURNACE_HMI_COMMAND_UNRESOLVED &&
           o->unresolved_cause != FURNACE_HMI_COMMAND_UNRESOLVED_CLIENT_SESSION_CHANGED &&
           o->unresolved_cause != FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED;
}

static void fill_proposal(furnace_hmi_controller_start_proposal_t *p,
                          const struct furnace_wire_start_proposal *w)
{
    p->state = FURNACE_HMI_PROPOSAL_OPEN;
    p->transaction_id = w->transaction_id;
    p->proposal_revision = w->proposal_revision;
    p->measured_decicelsius = w->measured_temperature.value;
    p->measured_validity = w->measured_temperature.validity;
    p->proposed_stage = w->proposed_stage;
    p->skipped_count = w->skipped_stages_count;
    for (uint8_t i = 0U; i < w->skipped_stages_count; i++) {
        p->skipped[i].index = w->skipped_stages[i].index;
        p->skipped[i].kind = w->skipped_stages[i].kind;
        p->skipped[i].target_decicelsius = w->skipped_stages[i].target;
        p->skipped[i].hold_minutes = w->skipped_stages[i].hold_minutes;
    }
    p->remaining_curing_s = w->remaining_curing_s;
    p->duration_ack_required = w->duration_ack_required;
    p->recommended_s = w->recommended_s;
    p->maximum_s = w->maximum_s;
    p->warnings_count = w->warnings_count;
    (void)memcpy(p->warnings, w->warnings, sizeof(p->warnings));
    p->invalidation_reason = 0U;
}

/*
 * Only the answer to the operator's START_REQUEST opens a proposal; later
 * revisions of that transaction replace it (spec: START_PROPOSAL).
 */
static void on_start_proposal(furnace_hmi_controller_client_t *c,
                              const struct furnace_wire_start_proposal *w)
{
    furnace_hmi_controller_start_proposal_t *p = &c->observation.start_proposal;
    furnace_hmi_command_outcome_t *o =
        (w->request_id != 0U) ? pending_start(c, w->request_id, true, false, false) : NULL;

    if (o != NULL) {
        const furnace_hmi_command_t *start = &c->command_delivery.command;

        o->transaction_id = w->transaction_id;
        o->state = FURNACE_HMI_COMMAND_PROPOSED;
        c->client_session_confirmed = true;
        (void)memset(p, 0, sizeof(*p));
        p->program_id = start->program_id;
        p->program_revision = start->program_revision;
        fill_proposal(p, w);
        changed(c);
        return;
    }
    if (answers_latest_start(c, w->request_id) && p->state != FURNACE_HMI_PROPOSAL_OPEN) {
        const furnace_hmi_command_t *start = &c->command_delivery.command;

        (void)memset(p, 0, sizeof(*p));
        p->program_id = start->program_id;
        p->program_revision = start->program_revision;
        fill_proposal(p, w);
        changed(c);
        return;
    }
    p = open_proposal(c, w->transaction_id);
    if (p != NULL && w->proposal_revision > p->proposal_revision) {
        fill_proposal(p, w);
        changed(c);
    }
}

static void on_start_accepted(furnace_hmi_controller_client_t *c,
                              const struct furnace_wire_start_accepted *a)
{
    furnace_hmi_command_outcome_t *o = pending_start(c, a->request_id, true, true, false);

    if (o != NULL) {
        o->run_id = a->run_id;
        o->transaction_id = a->transaction_id;
        complete(c, o, true, 0U);
    }
    end_proposal(c, open_proposal(c, a->transaction_id), FURNACE_HMI_PROPOSAL_ACCEPTED, 0U);
}

static void on_start_rejected(furnace_hmi_controller_client_t *c,
                              const struct furnace_wire_start_rejected *r)
{
    furnace_hmi_command_outcome_t *o = pending_start(c, r->request_id, true, true, true);

    if (o == NULL) {
        return;
    }
    if (r->reason == START_DUPLICATE_REQUEST) {
        /* The start transaction's own duplicate check: the outcome is unknown. */
        o->reason = r->reason;
        c->client_session_confirmed = true;
        unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_DUPLICATE);
        return;
    }
    complete(c, o, false, r->reason);
}

static void on_start_cancelled(furnace_hmi_controller_client_t *c,
                               const struct furnace_wire_start_cancelled *r)
{
    furnace_hmi_command_outcome_t *o = pending_start(c, r->request_id, false, false, true);

    if (o != NULL) {
        o->transaction_id = r->transaction_id;
        complete(c, o, true, 0U);
    }
    end_proposal(c, open_proposal(c, r->transaction_id), FURNACE_HMI_PROPOSAL_CANCELLED, 0U);
}

static void on_start_invalidated(furnace_hmi_controller_client_t *c,
                                 const struct furnace_wire_start_proposal_invalidated *i)
{
    end_proposal(c, open_proposal(c, i->transaction_id), FURNACE_HMI_PROPOSAL_INVALIDATED,
                 i->reason);
}

/*
 * STATUS tells a client that missed an event how the transaction ended. Status
 * is coalesced and may trail the proposal: no transaction or an older one says
 * nothing (plan, M21-7 design).
 */
static void proposal_from_status(furnace_hmi_controller_client_t *c,
                                 const struct furnace_wire_start_status *s)
{
    furnace_hmi_controller_start_proposal_t *p = &c->observation.start_proposal;

    if (p->state != FURNACE_HMI_PROPOSAL_OPEN || s->transaction_id < p->transaction_id) {
        return;
    }
    if (s->transaction_id > p->transaction_id) {
        end_proposal(c, p, FURNACE_HMI_PROPOSAL_ENDED, 0U);
    } else if (s->state == START_STATE_CONSUMED) {
        end_proposal(c, p, FURNACE_HMI_PROPOSAL_ACCEPTED, 0U);
    } else if (s->state == START_STATE_CANCELLED) {
        end_proposal(c, p, FURNACE_HMI_PROPOSAL_CANCELLED, 0U);
    } else if (s->state == START_STATE_INVALIDATED) {
        end_proposal(c, p, FURNACE_HMI_PROPOSAL_INVALIDATED, s->invalidation_reason);
    }
}

/* A save the codec can encode: a NUL-terminated strict UTF-8 name, kinds and fields as the wire allows. */
static bool save_encodable(const furnace_hmi_command_t *cmd)
{
    const void *nul = memchr(cmd->name, '\0', sizeof(cmd->name));

    if (nul == NULL || cmd->stage_count > FURNACE_HMI_CONTROLLER_MAX_STAGES ||
        !furnace_hmi_controller_name_valid(cmd->name,
                                           (uint32_t)((const char *)nul - cmd->name),
                                           FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY)) {
        return false;
    }
    for (uint8_t i = 0U; i < cmd->stage_count; i++) {
        const furnace_hmi_controller_stage_t *s = &cmd->stages[i];

        if (s->kind == FURNACE_HMI_CONTROLLER_STAGE_HEATING) {
            if (s->duration_minutes != 0U) {
                return false;
            }
        } else if (s->kind != FURNACE_HMI_CONTROLLER_STAGE_HOLD || s->target_decicelsius != 0 ||
                   s->rate_decicelsius_per_min != 0) {
            return false;
        }
    }
    return true;
}

/*
 * COMMAND_REJECTED of a pending command: refused by the service, except that a
 * duplicate and STOP_UNRESOLVED leave the outcome unknown (controller interface).
 * A Stop the service refused because this client is not connected stays
 * pending: it is sent again after HELLO, under its request_id (review F3).
 */
static bool command_rejected(furnace_hmi_controller_client_t *c, uint32_t request_id,
                             uint16_t msg_type, uint8_t reason)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, request_id, kind_of_message(msg_type));

    if (o == NULL) {
        return false;
    }
    o->reason = reason;
    if (furnace_hmi_command_is_stop(o->kind) &&
        (reason == REJECT_NOT_CONNECTED || reason == REJECT_CLIENT_SESSION_MISMATCH)) {
        return true;
    }
    if (reason == REJECT_DUPLICATE_REQUEST) {
        o->state = FURNACE_HMI_COMMAND_UNRESOLVED;
        o->unresolved_cause = FURNACE_HMI_COMMAND_UNRESOLVED_DUPLICATE;
    } else if (reason == REJECT_STOP_UNRESOLVED) {
        o->state = FURNACE_HMI_COMMAND_UNRESOLVED;
        o->unresolved_cause = FURNACE_HMI_COMMAND_UNRESOLVED_STOP;
    } else {
        o->state = FURNACE_HMI_COMMAND_REFUSED;
    }
    changed(c);
    return true;
}

/*
 * An unanswered command is sent again under its request_id while the link is
 * connected, until its attempts run out. A Stop is also sent again while the
 * link synchronizes (HELLO_ACK has confirmed the session; review F4), and waits
 * through a silent link for at most FURNACE_HMI_CLIENT_STOP_PATIENCE_MS.
 */
static void on_command_time(furnace_hmi_controller_client_t *c)
{
    const furnace_hmi_link_state_t link = c->observation.link;

    for (int slot = 0; slot < 2; slot++) {
        const bool stop = (slot == 1);
        furnace_hmi_command_outcome_t *o = outcome_of(c, stop);
        const uint32_t since = delivery_of(c, stop)->last_sent_ms;
        const bool library = is_library_change(o->kind);
        const uint8_t attempts = stop      ? FURNACE_HMI_CLIENT_STOP_ATTEMPTS
                                 : library ? FURNACE_HMI_CLIENT_LIBRARY_ATTEMPTS
                                           : FURNACE_HMI_CLIENT_COMMAND_ATTEMPTS;
        const uint32_t retry_ms =
            library ? FURNACE_HMI_CLIENT_LIBRARY_RETRY_MS : FURNACE_HMI_CLIENT_COMMAND_RETRY_MS;
        const bool usable = (link == FURNACE_HMI_LINK_CONNECTED) ||
                            (stop && link == FURNACE_HMI_LINK_SYNCHRONIZING);

        if (o->state != FURNACE_HMI_COMMAND_PENDING) {
            continue;
        }
        if (!usable) {
            if (stop && elapsed(c->now_ms, since, FURNACE_HMI_CLIENT_STOP_PATIENCE_MS)) {
                unresolve(c, stop, FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST);
            }
            continue;
        }
        if (!elapsed(c->now_ms, since, retry_ms)) {
            continue;
        }
        if (o->attempts >= attempts) {
            unresolve(c, stop, FURNACE_HMI_COMMAND_UNRESOLVED_NO_ANSWER);
        } else {
            send_command(c, stop);
        }
    }
}

static void record_refusal(furnace_hmi_controller_client_t *c, const furnace_hmi_command_t *command,
                           uint32_t submission, furnace_hmi_command_local_t reason)
{
    furnace_hmi_command_refusal_t *r = furnace_hmi_command_is_stop(command->kind)
                                           ? &c->observation.stop_refusal
                                           : &c->observation.command_refusal;

    r->submission = submission;
    r->kind = command->kind;
    r->reason = reason;
    changed(c);
}

void furnace_hmi_controller_client_refuse(furnace_hmi_controller_client_t *client,
                                          const furnace_hmi_command_t *command,
                                          uint32_t submission, furnace_hmi_command_local_t reason)
{
    record_refusal(client, command, submission, reason);
}

furnace_hmi_command_local_t furnace_hmi_controller_client_submit(
    furnace_hmi_controller_client_t *client, const furnace_hmi_command_t *command,
    uint32_t submission)
{
    furnace_hmi_controller_client_t *c = client;
    furnace_hmi_command_local_t refusal = FURNACE_HMI_COMMAND_LOCAL_NONE;
    const bool stop = furnace_hmi_command_is_stop(command->kind);
    furnace_hmi_command_outcome_t *o = outcome_of(c, stop);
    uint32_t request_id = 0U;

    if (command->kind <= FURNACE_HMI_COMMAND_NONE || command->kind >= FURNACE_HMI_COMMAND_COUNT ||
        (command->kind == FURNACE_HMI_COMMAND_PROGRAM_SAVE && !save_encodable(command))) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_INVALID;
    } else if (c->observation.link != FURNACE_HMI_LINK_CONNECTED) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT;
    } else if (o->state == FURNACE_HMI_COMMAND_PENDING) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_BUSY;
    } else if (!take_request_id(c, &request_id)) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT; /* a new client session is opening */
    }
    if (refusal != FURNACE_HMI_COMMAND_LOCAL_NONE) {
        record_refusal(c, command, submission, refusal);
        return refusal;
    }
    (void)memset(o, 0, sizeof(*o));
    o->submission = submission;
    o->kind = command->kind;
    o->state = FURNACE_HMI_COMMAND_PENDING;
    o->request_id = request_id;
    delivery_of(c, stop)->command = *command;
    if (command->kind == FURNACE_HMI_COMMAND_PROGRAM_START &&
        c->observation.start_proposal.state != FURNACE_HMI_PROPOSAL_OPEN) {
        /* A new Start: the proposal of an earlier one is no longer shown. */
        (void)memset(&c->observation.start_proposal, 0, sizeof(c->observation.start_proposal));
    }
    send_command(c, stop);
    return FURNACE_HMI_COMMAND_LOCAL_NONE;
}

/* ---- the controller session -------------------------------------------------------------- */

static void discard_session_data(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    o->capabilities_valid = false;
    o->settings_valid = false;
    o->status_valid = false;
    o->faults_valid = false;
    o->faults_outdated = false;
    o->fault_count = 0U;
    o->run_event_sequence = 0U;
    o->fault_event_sequence = 0U;
    if (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) {
        o->snapshot_request = FURNACE_HMI_REQUEST_UNRESOLVED;
    }
    c->event_sequences_known = false;
    c->newest_fault_event_seen = 0U;
    c->pending_fault_pages_seen = false;
    c->reconnect_baseline_active = false;
    /* Another controller session: a restart, which ends every run; no result will come. */
    unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED);
    unresolve(c, true, FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED);
    end_open_proposal(c);
    o->start_transaction_id = 0U;
    o->start_proposal_revision = 0U;
    o->start_state = 0U;
    o->start_invalidation_reason = 0U;
    o->library_revision = 0U;
    library_discard(c);
    furnace_hmi_diagnostics_discard(&c->diagnostics);
    changed(c);
}

static bool is_current_session(const furnace_hmi_controller_client_t *c, const uint8_t *session)
{
    return c->observation.identity_known &&
           memcmp(c->observation.session_id, session, FURNACE_HMI_CONTROLLER_SESSION_ID_SIZE) == 0;
}

static void begin_synchronizing(furnace_hmi_controller_client_t *c)
{
    set_link(c, FURNACE_HMI_LINK_SYNCHRONIZING);
    c->sync_started_ms = c->now_ms;
}

/* Another controller session: its predecessor's data goes, and the new one is synchronized. */
static void adopt_session(furnace_hmi_controller_client_t *c, const uint8_t *session)
{
    if (c->observation.identity_known) {
        saturating_increment(&c->observation.controller_session_changes);
    }
    discard_session_data(c);
    (void)memcpy(c->observation.session_id, session, FURNACE_HMI_CONTROLLER_SESSION_ID_SIZE);
    c->observation.identity_known = true;
    begin_synchronizing(c);
    /* The controller sends CAPABILITIES and a snapshot to a new client or controller session. */
    c->snapshot_active = true;
    c->newest_fault_event_seen = 0U;
}

/*
 * After a reconnect, something older than the status held when the link fell
 * silent: another controller session that drew the same ID (a restart;
 * improbable, not impossible). Everything is discarded and asked for again.
 */
static void repeated_session_id(furnace_hmi_controller_client_t *c)
{
    saturating_increment(&c->observation.status_regressions);
    saturating_increment(&c->observation.controller_session_changes);
    discard_session_data(c);
    begin_synchronizing(c);
    request_capabilities(c);
    request_snapshot(c);
}

static void on_hello_ack(furnace_hmi_controller_client_t *c, const struct furnace_wire_frame *f,
                         const struct furnace_wire_hello_ack *ack)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    o->controller_version_known = true;
    o->controller_proto_major = f->proto_major;
    o->controller_proto_minor = f->proto_minor;
    o->hello_result = ack->result;
    changed(c);
    if (ack->result != HELLO_OK) {
        unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST);
        if (ack->result == HELLO_INVALID_CLIENT_SESSION) {
            new_client_session(c);
        } else {
            set_link(c, FURNACE_HMI_LINK_INCOMPATIBLE);
        }
        return;
    }
    o->firmware_id = ack->firmware_id;
    o->boot_count = ack->boot_count;
    if (!is_current_session(c, ack->session_id)) {
        adopt_session(c, ack->session_id);
        return;
    }
    /* The controller (re)opened the client's session: an ordinary command's intent ends (F1). */
    unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST);
    /* The same controller session: it sends no snapshot to a repeated HELLO, so ask. */
    begin_synchronizing(c);
    if (!o->capabilities_valid) {
        request_capabilities(c);
    }
    request_snapshot(c);
}

/* A refused client reads only the header's version and the result (wire format §3). */
static void on_hello_ack_of_other_version(furnace_hmi_controller_client_t *c,
                                          const struct furnace_wire_frame *f)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    o->controller_version_known = true;
    o->controller_proto_major = f->proto_major;
    o->controller_proto_minor = f->proto_minor;
    o->hello_result = (f->payload_len > 0U) ? f->payload[0] : HELLO_VERSION_UNSUPPORTED;
    set_link(c, FURNACE_HMI_LINK_INCOMPATIBLE);
    changed(c);
}

/* A STATUS or CAPABILITIES naming another session: as after a restart, then HELLO again. */
static void identity_changed(furnace_hmi_controller_client_t *c, const uint8_t *session)
{
    adopt_session(c, session);
    send_hello(c);
}

static bool intervals_acceptable(const struct furnace_wire_capabilities *caps)
{
    return caps->status_interval_ms > 0U &&
           caps->status_interval_ms <= FURNACE_HMI_CLIENT_MAX_STATUS_INTERVAL_MS &&
           caps->heartbeat_interval_ms > 0U &&
           caps->heartbeat_interval_ms <= FURNACE_HMI_CLIENT_MAX_HEARTBEAT_INTERVAL_MS &&
           caps->link_silence_ms > caps->status_interval_ms &&
           caps->link_silence_ms <= FURNACE_HMI_CLIENT_MAX_LINK_SILENCE_MS;
}

static void on_capabilities(furnace_hmi_controller_client_t *c,
                            const struct furnace_wire_capabilities *caps)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    if (!o->identity_known) {
        saturating_increment(&o->frames_ignored);
        return;
    }
    if (!is_current_session(c, caps->session_id)) {
        identity_changed(c, caps->session_id);
    }
    if (caps->request_id != 0U && caps->request_id == c->capabilities_request_id) {
        c->client_session_confirmed = true;
    }
    if (!intervals_acceptable(caps)) {
        saturating_increment(&o->capabilities_rejected);
        changed(c);
        return;
    }
    (void)memset(o->model_id, 0, sizeof(o->model_id));
    (void)memcpy(o->model_id, caps->model_id, sizeof(caps->model_id));
    o->configured = caps->configured;
    o->config_state = caps->config_state;
    o->config_revision = caps->config_revision;
    o->profile_revision = caps->profile_revision;
    o->status_interval_ms = caps->status_interval_ms;
    o->heartbeat_interval_ms = caps->heartbeat_interval_ms;
    o->link_silence_ms = caps->link_silence_ms;
    o->manual_max_minutes = caps->manual_max_minutes;
    o->max_stages = caps->max_stages;
    o->stage_kinds = caps->stage_kinds;
    o->target_min_decicelsius = caps->target_min;
    o->target_max_decicelsius = caps->target_max;
    o->rate_min_decicelsius_per_min = caps->rate_min;
    o->rate_max_decicelsius_per_min = caps->rate_max;
    o->curing_max_minutes = caps->curing_max_minutes;
    o->recommended_curing_minutes = caps->recommended_curing_minutes;
    o->planning_reference_min_decicelsius = caps->planning_reference_min;
    o->planning_reference_max_decicelsius = caps->planning_reference_max;
    o->name_max_code_points = caps->name_max_code_points;
    o->name_max_bytes = caps->name_max_bytes;
    o->library_capacity = caps->library_capacity;
    o->capabilities_valid = true;
    changed(c);
}

static void copy_status(furnace_hmi_controller_observation_t *o, const struct furnace_wire_status *s)
{
    o->status_sequence = s->status_sequence;
    o->temperature_decicelsius = s->measured_temperature.value;
    o->temperature_validity = s->measured_temperature.validity;
    o->temperature_age_ms = s->measured_temperature.age_ms;
    o->measurement_state = s->equipment.measurement_state;
    o->measurement_qualified_s = s->run_status.measurement_qualified_s;
    o->operating_mode = s->operating_mode;
    o->run_state = s->run_status.run_state;
    o->session_kind = s->run_status.session_kind;
    o->run_id = s->run_status.run_id;
    o->program_id = s->run_status.program_id;
    o->program_revision = s->run_status.program_revision;
    o->stage_index = s->run_status.stage_index;
    o->stage_count = s->run_status.stage_count;
    o->stage_kind = s->run_status.stage_kind;
    (void)memset(o->program_name, 0, sizeof(o->program_name));
    (void)memcpy(o->program_name, s->run_status.program_name,
                 sizeof(o->program_name) - 1U < sizeof(s->run_status.program_name)
                     ? sizeof(o->program_name) - 1U
                     : sizeof(s->run_status.program_name));
    o->run_elapsed_s = s->run_status.run_elapsed_s;
    o->cooling = s->run_status.cooling;
    o->cooling_s = s->run_status.cooling_s;
    o->manual_target_decicelsius = s->run_status.manual_target;
    o->manual_rate_decicelsius_per_min = s->run_status.manual_rate;
    o->manual_remaining_s = s->run_status.manual_remaining_s;
    o->setpoint_valid = s->run_status.setpoint_valid;
    o->setpoint_decicelsius = s->run_status.setpoint;
    o->heating_requested = s->run_status.heating_requested;
    o->heating_inhibited = s->fault_status.heating_inhibited;
    o->latched_class = s->fault_status.latched_class;
    o->inhibit_ids = s->fault_status.inhibit_ids;
    o->continuable_ids = s->fault_status.continuable_ids;
    o->resume_allowed = s->fault_status.resume_allowed;
    o->decision_remaining_s = s->run_status.decision_remaining_s;
    o->unacknowledged_faults = s->fault_status.unacknowledged_count;
    o->session_fresh = s->controller.session_fresh;
    o->store_readiness = s->controller.store_readiness;
    o->execution_active = s->lifecycle.execution_active;
    o->heating_authorized = s->lifecycle.heating_authorized;
    o->navigation_permitted = s->lifecycle.navigation_permitted;
    o->settings_changes_permitted = s->lifecycle.settings_changes_permitted;
    o->fan_feedback = s->equipment.fan_feedback;
    o->door_state = s->equipment.door_state;
    o->emergency_active = s->equipment.emergency_active;
    o->library_revision = s->library_revision;
    o->start_transaction_id = s->start.transaction_id;
    o->start_proposal_revision = s->start.proposal_revision;
    o->start_state = s->start.state;
    o->start_invalidation_reason = s->start.invalidation_reason;
    o->status_valid = true;
}

static void on_status(furnace_hmi_controller_client_t *c, const struct furnace_wire_status *s)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    if (!o->identity_known) {
        saturating_increment(&o->frames_ignored);
        return;
    }
    if (!is_current_session(c, s->session_id)) {
        identity_changed(c, s->session_id);
    } else if (c->reconnect_baseline_active && s->status_sequence < c->reconnect_baseline) {
        repeated_session_id(c);
    } else if (o->status_valid && s->status_sequence <= o->status_sequence) {
        /*
         * Not newer than what is shown. The snapshot's STATUS may be older than a
         * periodic one (spec: Snapshot); otherwise an older STATUS asks for a
         * snapshot (spec: Recovery). A gap alone is normal: status is coalesced.
         */
        if (s->status_sequence < o->status_sequence && !c->snapshot_active) {
            refresh(c);
        }
        return;
    }
    copy_status(o, s);
    proposal_from_status(c, &s->start);
    c->last_status_ms = c->now_ms;
    changed(c);
    if (o->link == FURNACE_HMI_LINK_SILENT) {
        /* Contact again without a HELLO: rebuild from a snapshot (reconnect, step 5). */
        begin_synchronizing(c);
        refresh(c);
    }
}

/* Pages must be contiguous, all of one snapshot, and add up to its total. */
static void on_fault_list(furnace_hmi_controller_client_t *c,
                          const struct furnace_wire_fault_list *list)
{
    if (!c->snapshot_active) {
        return;
    }
    if (list->first_index == 0U) {
        c->pending_fault_pages_seen = true;
        c->pending_fault_pages_ok = list->total <= FURNACE_HMI_CONTROLLER_MAX_FAULTS;
        c->pending_fault_total = list->total;
        c->pending_fault_sequence = list->status_sequence;
        c->pending_fault_request_id = list->request_id;
        c->pending_fault_count = 0U;
    } else if (!c->pending_fault_pages_seen || list->first_index != c->pending_fault_count ||
               list->total != c->pending_fault_total ||
               list->status_sequence != c->pending_fault_sequence ||
               list->request_id != c->pending_fault_request_id) {
        c->pending_fault_pages_ok = false;
        return;
    }
    for (uint8_t i = 0U; i < list->entries_count; i++) {
        const struct furnace_wire_fault_entry *e = &list->entries[i];
        furnace_hmi_controller_fault_t *f;

        if (c->pending_fault_count >= FURNACE_HMI_CONTROLLER_MAX_FAULTS) {
            c->pending_fault_pages_ok = false;
            break;
        }
        f = &c->pending_faults[c->pending_fault_count++];
        f->fault_id = e->fault_id;
        f->source = e->source;
        f->fault_class = e->class;
        f->condition_active = e->condition_active;
        f->latched = e->latched;
        f->acknowledged = e->acknowledged;
        f->recovery = e->recovery;
        f->category = e->category;
        f->continuable = e->continuable;
        f->continued = e->continued;
    }
}

static bool fault_pages_complete(const furnace_hmi_controller_client_t *c,
                                 const struct furnace_wire_snapshot_end *end)
{
    return c->pending_fault_pages_seen && c->pending_fault_pages_ok &&
           c->pending_fault_count == c->pending_fault_total &&
           c->pending_fault_sequence == end->status_sequence &&
           c->pending_fault_request_id == end->request_id;
}

static void on_snapshot_end(furnace_hmi_controller_client_t *c,
                            const struct furnace_wire_snapshot_end *end)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    const bool ours = (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) &&
                      (end->request_id == o->snapshot_request_id);

    if (!c->snapshot_active || (end->request_id != 0U && !ours)) {
        return; /* superseded, or not expected */
    }
    if (ours) {
        c->client_session_confirmed = true;
    }
    if (c->reconnect_baseline_active && end->status_sequence < c->reconnect_baseline) {
        repeated_session_id(c);
        return;
    }
    if (!fault_pages_complete(c, end)) {
        /* Not applied; the request timeout asks again. No list is not an empty list. */
        saturating_increment(&o->snapshots_incomplete);
        c->snapshot_active = !ours && (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING);
        c->pending_fault_pages_seen = false;
        o->faults_outdated = true;
        changed(c);
        return;
    }
    if (ours) {
        o->snapshot_request = FURNACE_HMI_REQUEST_COMPLETED;
    }
    (void)memcpy(o->faults, c->pending_faults, sizeof(o->faults));
    o->fault_count = c->pending_fault_count;
    o->faults_valid = true;
    o->faults_outdated = false;
    o->run_event_sequence = end->run_event_sequence;
    o->fault_event_sequence = end->fault_sequence;
    o->library_revision = end->library_revision;
    c->event_sequences_known = true;
    /* The controller's own snapshot (request 0) does not answer a request of ours still pending. */
    c->snapshot_active = !ours && (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING);
    c->pending_fault_pages_seen = false;
    c->reconnect_baseline_active = false;
    changed(c);
    if (o->capabilities_valid && o->status_valid) {
        set_link(c, FURNACE_HMI_LINK_CONNECTED);
    }
    if (c->newest_fault_event_seen > end->fault_sequence) {
        refresh(c); /* a fault changed after the snapshot's instant */
    }
}

static void on_event(furnace_hmi_controller_client_t *c, uint32_t sequence, bool fault)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    uint32_t *last = fault ? &o->fault_event_sequence : &o->run_event_sequence;
    bool gap;

    if (fault && sequence > c->newest_fault_event_seen) {
        c->newest_fault_event_seen = sequence;
    }
    if (!c->event_sequences_known || sequence <= *last) {
        return; /* before the snapshot's base, or already in it */
    }
    gap = (sequence != *last + 1U);
    if (gap) {
        saturating_increment(&o->event_gaps);
    }
    *last = sequence;
    changed(c);
    if (gap || fault) {
        /* The fault list is taken from a snapshot, never inferred from events. */
        refresh(c);
    }
}

static void on_command_rejected(furnace_hmi_controller_client_t *c,
                                const struct furnace_wire_command_rejected *r)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    const bool snapshot = (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) &&
                          (r->request_id == o->snapshot_request_id);
    const bool capabilities = (r->request_id != 0U) && (r->request_id == c->capabilities_request_id);
    /* A refused library read is asked again after its timeout, like a lost answer. */
    const bool library = (c->library_step != LIBRARY_IDLE) && (r->request_id == c->library_request_id);
    /* Likewise a refused diagnostic read. */
    const bool diagnostic = furnace_hmi_diagnostics_on_rejected(&c->diagnostics, r->request_id);
    const bool command = command_rejected(c, r->request_id, r->msg_type, r->reason);

    if (!snapshot && !capabilities && !library && !diagnostic && !command) {
        return;
    }
    if (snapshot) {
        o->snapshot_request = FURNACE_HMI_REQUEST_REJECTED;
        o->snapshot_reject_reason = r->reason;
        c->snapshot_active = false;
    }
    changed(c);
    if (r->reason == REJECT_DUPLICATE_REQUEST && !c->client_session_confirmed) {
        /* A duplicate before any answer: the controller knew this client session ID already. */
        new_client_session(c);
        send_hello(c);
    } else if (r->reason == REJECT_NOT_CONNECTED || r->reason == REJECT_STALE_SESSION ||
               r->reason == REJECT_CLIENT_SESSION_MISMATCH) {
        send_hello(c);
    }
}

/* ---- the library mirror (M21-7a; plan, M21-7 design) ------------------------------------- */

static void library_changed(furnace_hmi_controller_client_t *c)
{
    c->library_generation++;
}

/* Another controller session: nothing of the previous one's library is shown. */
static void library_discard(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_library_t *l = &c->library;

    l->state = FURNACE_HMI_LIBRARY_UNKNOWN;
    l->library_revision = 0U;
    l->updating = false;
    l->count = 0U;
    (void)memset(l->programs, 0, sizeof(l->programs));
    c->library_step = LIBRARY_IDLE;
    c->library_relist = false;
    library_changed(c);
}

/* Send the request of the current step: the page at library_next, or the read of that program. */
static void library_send(furnace_hmi_controller_client_t *c)
{
    struct furnace_wire_envelope *env;

    (void)memset(&c->out, 0, sizeof(c->out));
    if (c->library_step == LIBRARY_LISTING) {
        c->out.type = FURNACE_WIRE_MSG_PROGRAM_LIST_REQUEST;
        env = &c->out.u.program_list_request.envelope;
        c->out.u.program_list_request.first_index = c->library_next;
        c->out.u.program_list_request.max_count = FURNACE_HMI_CLIENT_LIBRARY_PAGE;
    } else {
        c->out.type = FURNACE_WIRE_MSG_PROGRAM_READ_REQUEST;
        env = &c->out.u.program_read_request.envelope;
        c->out.u.program_read_request.program_id =
            c->library_reading.programs[c->library_next].program_id;
    }
    c->library_sent_ms = c->now_ms;
    if (!fill_envelope(c, env)) {
        c->library_step = LIBRARY_IDLE; /* a new client session is opening */
        return;
    }
    c->library_request_id = env->request_id;
    send_out(c);
}

static void library_list_from_start(furnace_hmi_controller_client_t *c)
{
    c->library_step = LIBRARY_LISTING;
    c->library_next = 0U;
    c->library_reading.count = 0U;
    library_send(c);
}

/* The library changed while it was read: everything is read again from index 0. */
static void library_restart(furnace_hmi_controller_client_t *c)
{
    saturating_increment(&c->library.restarts);
    library_changed(c);
    library_list_from_start(c);
}

/* The controller serves no library, or one too large to hold: nothing is shown. */
static void library_settle(furnace_hmi_controller_client_t *c, furnace_hmi_library_state_t state,
                           uint32_t revision)
{
    c->library.state = state;
    /* Held at the newest revision seen, so a STATUS ahead of the page does not list it again (L6). */
    c->library.library_revision = (revision > c->observation.library_revision)
                                      ? revision
                                      : c->observation.library_revision;
    c->library.updating = false;
    c->library.count = 0U;
    c->library_step = LIBRARY_IDLE;
    library_changed(c);
}

/* Read the next program that is not damaged; with every program read, the listing is shown. */
static void library_read_next(furnace_hmi_controller_client_t *c, uint8_t from)
{
    furnace_hmi_controller_library_t *r = &c->library_reading;
    furnace_hmi_controller_library_t *l = &c->library;

    for (uint8_t i = from; i < r->count; i++) {
        if (!r->programs[i].damaged) {
            c->library_step = LIBRARY_READING;
            c->library_next = i;
            library_send(c);
            return;
        }
    }
    l->state = FURNACE_HMI_LIBRARY_CURRENT;
    l->library_revision = r->library_revision;
    l->updating = false;
    l->count = r->count;
    (void)memcpy(l->programs, r->programs, sizeof(l->programs));
    c->library_step = LIBRARY_IDLE;
    library_changed(c);
}

static void copy_name(char *to, const char *from)
{
    (void)memcpy(to, from, FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY);
    to[FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY] = '\0';
}

static bool name_shown(const char *name)
{
    return furnace_hmi_controller_name_valid(name, (uint32_t)strlen(name),
                                             FURNACE_HMI_CONTROLLER_NAME_MAX_CODE_POINTS);
}

static void on_program_list(furnace_hmi_controller_client_t *c,
                            const struct furnace_wire_program_list *l)
{
    furnace_hmi_controller_library_t *r = &c->library_reading;

    if (c->library_step != LIBRARY_LISTING || l->request_id != c->library_request_id) {
        return; /* superseded, or not ours */
    }
    c->client_session_confirmed = true;
    if (l->result != LIBRARY_OK) {
        library_settle(c, FURNACE_HMI_LIBRARY_UNAVAILABLE, l->library_revision);
        return;
    }
    if (c->library_next == 0U) {
        r->library_revision = l->library_revision;
        c->library_total = l->total;
        if (l->total > FURNACE_HMI_CONTROLLER_MAX_PROGRAMS) {
            library_settle(c, FURNACE_HMI_LIBRARY_TOO_LARGE, l->library_revision);
            return;
        }
    } else if (l->library_revision != r->library_revision || l->total != c->library_total) {
        library_restart(c);
        return;
    }
    if (c->library_total == 0U && l->first_index == 0U && l->entries_count == 0U) {
        library_read_next(c, 0U); /* a library without programs (review M1) */
        return;
    }
    if (l->first_index != c->library_next || l->entries_count == 0U ||
        l->entries_count > c->library_total - c->library_next) {
        return; /* not the page asked for: asked again after the timeout */
    }
    for (uint8_t i = 0U; i < l->entries_count; i++) {
        const struct furnace_wire_library_entry *e = &l->entries[i];
        furnace_hmi_controller_program_t *p = &r->programs[c->library_next + i];

        (void)memset(p, 0, sizeof(*p));
        p->program_id = e->program_id;
        p->program_revision = e->program_revision;
        copy_name(p->name, e->name);
        p->name_valid = name_shown(p->name);
        p->damaged = e->damaged;
        p->max_target_decicelsius = e->max_target;
        p->curing_estimate_s = e->curing_estimate_s;
        p->planning_reference_decicelsius = e->planning_reference;
    }
    c->library_next = (uint8_t)(c->library_next + l->entries_count);
    r->count = c->library_next;
    if (c->library_next < c->library_total) {
        library_send(c);
    } else {
        library_read_next(c, 0U);
    }
}

static void on_program_data(furnace_hmi_controller_client_t *c,
                            const struct furnace_wire_program_data *d)
{
    furnace_hmi_controller_program_t *p = &c->library_reading.programs[c->library_next];

    if (c->library_step != LIBRARY_READING || d->request_id != c->library_request_id) {
        return;
    }
    c->client_session_confirmed = true;
    if (d->result == LIBRARY_STORAGE_UNAVAILABLE) {
        library_settle(c, FURNACE_HMI_LIBRARY_UNAVAILABLE, c->library_reading.library_revision);
        return;
    }
    if (d->result == DATA_DAMAGED) {
        p->damaged = true;
        p->stage_count = 0U;
    } else if (d->result == LIBRARY_OK && d->program_id == p->program_id &&
               d->program_revision == p->program_revision) {
        copy_name(p->name, d->name);
        p->name_valid = name_shown(p->name);
        p->curing_estimate_s = d->curing_estimate_s;
        p->planning_reference_decicelsius = d->planning_reference;
        p->stage_count = d->stages_count;
        for (uint8_t i = 0U; i < d->stages_count; i++) {
            p->stages[i].kind = d->stages[i].kind;
            p->stages[i].target_decicelsius = d->stages[i].target;
            p->stages[i].rate_decicelsius_per_min = d->stages[i].rate;
            p->stages[i].duration_minutes = d->stages[i].duration_minutes;
        }
    } else {
        library_restart(c); /* NOT_FOUND or another revision: the library changed */
        return;
    }
    library_read_next(c, (uint8_t)(c->library_next + 1U));
}

/*
 * While the link is current: list when the controller's revision is newer than
 * the one shown, ask again what went unanswered, and mark the listing shown as
 * updating. A link that is not current stops the reading (plan, M21-7 design).
 */
static void library_time(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_library_t *l = &c->library;
    const uint32_t wanted = c->observation.library_revision;
    bool updating;

    if (c->observation.link != FURNACE_HMI_LINK_CONNECTED) {
        c->library_step = LIBRARY_IDLE;
        return;
    }
    /* Revisions only increase within a controller session; a STATUS may trail a page. */
    updating = (l->state == FURNACE_HMI_LIBRARY_CURRENT) && (wanted > l->library_revision);
    if (l->updating != updating) {
        l->updating = updating;
        library_changed(c);
    }
    if (c->library_step != LIBRARY_IDLE) {
        if (elapsed(c->now_ms, c->library_sent_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS)) {
            saturating_increment(&l->retries);
            library_changed(c);
            library_send(c);
        }
    } else if (l->state == FURNACE_HMI_LIBRARY_UNKNOWN || wanted > l->library_revision ||
               c->library_relist) {
        c->library_relist = false;
        library_list_from_start(c);
    }
}

static void on_frame(furnace_hmi_controller_client_t *c, const struct furnace_wire_frame *f)
{
    const struct furnace_wire_message *m = &c->message;

    if (furnace_wire_is_client_message(f->msg_type)) {
        saturating_increment(&c->observation.frames_ignored);
        return;
    }
    if (furnace_wire_payload_decode(f->msg_type, f->payload, f->payload_len, &c->message) !=
        FURNACE_WIRE_OK) {
        saturating_increment(&c->observation.frames_dropped);
        return;
    }
    saturating_increment(&c->observation.frames_received);
    switch (f->msg_type) {
    case FURNACE_WIRE_MSG_HELLO_ACK:
        on_hello_ack(c, f, &m->u.hello_ack);
        break;
    case FURNACE_WIRE_MSG_CAPABILITIES:
        on_capabilities(c, &m->u.capabilities);
        break;
    case FURNACE_WIRE_MSG_STATUS:
        on_status(c, &m->u.status);
        break;
    case FURNACE_WIRE_MSG_FAULT_LIST:
        on_fault_list(c, &m->u.fault_list);
        break;
    case FURNACE_WIRE_MSG_SNAPSHOT_END:
        on_snapshot_end(c, &m->u.snapshot_end);
        break;
    case FURNACE_WIRE_MSG_RUN_EVENT:
        on_event(c, m->u.run_event.sequence, false);
        break;
    case FURNACE_WIRE_MSG_FAULT_EVENT:
        on_event(c, m->u.fault_event.sequence, true);
        break;
    case FURNACE_WIRE_MSG_COMMAND_REJECTED:
        on_command_rejected(c, &m->u.command_rejected);
        break;
    case FURNACE_WIRE_MSG_RUN_COMMAND_ACCEPTED:
        on_run_command_result(c, true, m->u.run_command_accepted.request_id,
                              m->u.run_command_accepted.command,
                              m->u.run_command_accepted.reason,
                              m->u.run_command_accepted.blocking_conditions,
                              m->u.run_command_accepted.run_id);
        break;
    case FURNACE_WIRE_MSG_RUN_COMMAND_REJECTED:
        on_run_command_result(c, false, m->u.run_command_rejected.request_id,
                              m->u.run_command_rejected.command,
                              m->u.run_command_rejected.reason,
                              m->u.run_command_rejected.blocking_conditions,
                              m->u.run_command_rejected.run_id);
        break;
    case FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT:
        on_fault_acknowledge_result(c, &m->u.fault_acknowledge_result);
        break;
    case FURNACE_WIRE_MSG_FAULT_RECOVERY_RESULT:
        on_fault_recovery_result(c, &m->u.fault_recovery_result);
        break;
    case FURNACE_WIRE_MSG_START_PREPARE_RESULT:
        on_start_prepare_result(c, &m->u.start_prepare_result);
        break;
    case FURNACE_WIRE_MSG_PROGRAM_LIST:
        on_program_list(c, &m->u.program_list);
        break;
    case FURNACE_WIRE_MSG_PROGRAM_DATA:
        on_program_data(c, &m->u.program_data);
        break;
    case FURNACE_WIRE_MSG_PROGRAM_SAVE_RESULT:
        on_program_save_result(c, &m->u.program_save_result);
        break;
    case FURNACE_WIRE_MSG_PROGRAM_DELETE_RESULT:
        on_program_delete_result(c, &m->u.program_delete_result);
        break;
    case FURNACE_WIRE_MSG_START_PROPOSAL:
        on_start_proposal(c, &m->u.start_proposal);
        break;
    case FURNACE_WIRE_MSG_START_ACCEPTED:
        on_start_accepted(c, &m->u.start_accepted);
        break;
    case FURNACE_WIRE_MSG_START_REJECTED:
        on_start_rejected(c, &m->u.start_rejected);
        break;
    case FURNACE_WIRE_MSG_START_CANCELLED:
        on_start_cancelled(c, &m->u.start_cancelled);
        break;
    case FURNACE_WIRE_MSG_START_PROPOSAL_INVALIDATED:
        on_start_invalidated(c, &m->u.start_proposal_invalidated);
        break;
    case FURNACE_WIRE_MSG_HEALTH:
        if (is_current_session(c, m->u.health.session_id)) {
            c->client_session_confirmed = true;
            furnace_hmi_diagnostics_on_health(&c->diagnostics, &m->u.health, c->now_ms);
        }
        break;
    case FURNACE_WIRE_MSG_EVENT_HISTORY_PAGE:
        c->client_session_confirmed = true;
        furnace_hmi_diagnostics_on_history_page(&c->diagnostics, &m->u.event_history_page,
                                                c->now_ms);
        break;
    case FURNACE_WIRE_MSG_SETTINGS:
        on_settings(c, &m->u.settings);
        break;
    case FURNACE_WIRE_MSG_SETTING_RESULT:
        on_setting_result(c, &m->u.setting_result);
        break;
    default:
        break; /* PROGRAM_ESTIMATE: not asked for (the controller's stored estimate is shown) */
    }
}

/*
 * The diagnostic reads that are due (M21-7c): health periodically, history
 * pages while a read is in progress. At most one of each per poll.
 */
static void diagnostics_time(furnace_hmi_controller_client_t *c)
{
    const bool connected = (c->observation.link == FURNACE_HMI_LINK_CONNECTED);

    for (int i = 0; i < 2; i++) {
        struct furnace_wire_envelope *env;

        if (!furnace_hmi_diagnostics_due(&c->diagnostics, c->now_ms, connected, &c->out)) {
            return;
        }
        env = (c->out.type == FURNACE_WIRE_MSG_HEALTH_REQUEST)
                  ? &c->out.u.health_request.envelope
                  : &c->out.u.event_history_request.envelope;
        if (!fill_envelope(c, env)) {
            return; /* a new client session is opening; asked again once connected */
        }
        furnace_hmi_diagnostics_sent(&c->diagnostics, &c->out, env->request_id, c->now_ms);
        send_out(c);
    }
}

/* ---- time ---------------------------------------------------------------------------------- */

/*
 * A snapshot or capabilities answer that did not come is asked for again, in
 * any connected state: a pending request after FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS,
 * a synchronization without a request (the controller's own snapshot) likewise,
 * and an outdated fault list whose request was rejected or incomplete.
 */
static void on_request_timeout(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    const uint32_t now = c->now_ms;
    bool again;

    if (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) {
        again = elapsed(now, c->snapshot_requested_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS);
    } else if (o->link == FURNACE_HMI_LINK_SYNCHRONIZING) {
        again = elapsed(now, c->sync_started_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS) &&
                elapsed(now, c->snapshot_requested_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS);
    } else {
        again = o->faults_outdated &&
                elapsed(now, c->snapshot_requested_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS);
    }
    if (!again) {
        return;
    }
    c->sync_started_ms = now;
    if (!o->capabilities_valid) {
        request_capabilities(c);
    }
    request_snapshot(c);
}

static void on_time(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    const uint32_t now = c->now_ms;

    switch (o->link) {
    case FURNACE_HMI_LINK_CONNECTING:
    case FURNACE_HMI_LINK_INCOMPATIBLE:
    case FURNACE_HMI_LINK_SILENT:
        if (!c->hello_sent || elapsed(now, c->last_hello_ms, FURNACE_HMI_CLIENT_HELLO_RETRY_MS)) {
            send_hello(c);
        }
        return;
    case FURNACE_HMI_LINK_SYNCHRONIZING:
    case FURNACE_HMI_LINK_CONNECTED:
    default:
        break;
    }
    /*
     * Only a synchronized link falls silent. While synchronizing, kept data is
     * already shown as stale, and the request timeout asks again.
     */
    if (o->link == FURNACE_HMI_LINK_CONNECTED &&
        elapsed(now, c->last_status_ms, o->link_silence_ms)) {
        set_link(c, FURNACE_HMI_LINK_SILENT);
        c->reconnect_baseline_active = true;
        c->reconnect_baseline = o->status_sequence;
        /* The operator's intent ends with the link; a Stop waits for it (M21-5 design). */
        unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST);
        send_hello(c);
        return;
    }
    on_request_timeout(c);
    if (o->capabilities_valid && elapsed(now, c->last_send_ms, o->heartbeat_interval_ms)) {
        send_heartbeat(c);
    }
}

static void update_availability(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    furnace_hmi_state_availability_t availability = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;

    if (o->link == FURNACE_HMI_LINK_CONNECTED) {
        availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    } else if ((o->link == FURNACE_HMI_LINK_SILENT || o->link == FURNACE_HMI_LINK_SYNCHRONIZING) &&
               o->status_valid && o->capabilities_valid) {
        availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    }
    if (o->availability != availability) {
        o->availability = availability;
        changed(c);
    }
    o->status_age_ms = o->status_valid ? (uint32_t)(c->now_ms - c->last_status_ms)
                                       : FURNACE_HMI_CONTROLLER_AGE_NONE;
}

/* ---- interface ----------------------------------------------------------------------------- */

void furnace_hmi_controller_client_init(furnace_hmi_controller_client_t *client,
                                        const furnace_hmi_controller_client_config_t *config,
                                        uint32_t now_ms)
{
    (void)memset(client, 0, sizeof(*client));
    client->config = *config;
    client->now_ms = now_ms;
    furnace_wire_rx_init(&client->rx);
    client->observation.link = FURNACE_HMI_LINK_CONNECTING;
    client->observation.availability = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    client->observation.status_age_ms = FURNACE_HMI_CONTROLLER_AGE_NONE;
    new_client_session(client);
}

void furnace_hmi_controller_client_poll(furnace_hmi_controller_client_t *client, uint32_t now_ms)
{
    uint8_t chunk[READ_CHUNK];
    size_t total = 0U;
    struct furnace_wire_frame frame;

    client->now_ms = now_ms;
    while (total < FURNACE_HMI_CLIENT_POLL_BYTES) {
        const size_t want = ((FURNACE_HMI_CLIENT_POLL_BYTES - total) < sizeof(chunk))
                                ? (FURNACE_HMI_CLIENT_POLL_BYTES - total)
                                : sizeof(chunk);
        const size_t n = client->config.port.read(client->config.port.context, chunk, want);

        if (n == 0U) {
            break;
        }
        total += n;
        for (size_t i = 0U; i < n; i++) {
            switch (furnace_wire_rx_feed(&client->rx, chunk[i], &frame)) {
            case FURNACE_WIRE_RX_FRAME:
                on_frame(client, &frame);
                break;
            case FURNACE_WIRE_RX_DROPPED:
                saturating_increment(&client->observation.frames_dropped);
                break;
            case FURNACE_WIRE_RX_HELLO_VERSION:
                if (frame.msg_type == FURNACE_WIRE_MSG_HELLO_ACK) {
                    on_hello_ack_of_other_version(client, &frame);
                }
                break;
            case FURNACE_WIRE_RX_NONE:
            default:
                break;
            }
        }
    }
    on_time(client);
    on_command_time(client);
    library_time(client);
    diagnostics_time(client);
    update_availability(client);
}

bool furnace_hmi_controller_client_read_history(furnace_hmi_controller_client_t *client)
{
    /* Asked while the link is not current, it starts once it is (review 3). */
    furnace_hmi_diagnostics_request_history(&client->diagnostics);
    return client->observation.link == FURNACE_HMI_LINK_CONNECTED;
}

const furnace_hmi_controller_health_t *furnace_hmi_controller_client_health(
    const furnace_hmi_controller_client_t *client)
{
    return &client->diagnostics.health;
}

const furnace_hmi_controller_history_t *furnace_hmi_controller_client_history(
    const furnace_hmi_controller_client_t *client)
{
    return &client->diagnostics.history;
}

uint32_t furnace_hmi_controller_client_diagnostics_generation(
    const furnace_hmi_controller_client_t *client)
{
    return client->diagnostics.generation;
}

const furnace_hmi_controller_observation_t *furnace_hmi_controller_client_observation(
    const furnace_hmi_controller_client_t *client)
{
    return &client->observation;
}

uint32_t furnace_hmi_controller_client_generation(const furnace_hmi_controller_client_t *client)
{
    return client->generation;
}

const furnace_hmi_controller_library_t *furnace_hmi_controller_client_library(
    const furnace_hmi_controller_client_t *client)
{
    return &client->library;
}

uint32_t furnace_hmi_controller_client_library_generation(
    const furnace_hmi_controller_client_t *client)
{
    return client->library_generation;
}

furnace_hmi_value_validity_t furnace_hmi_controller_temperature_validity(
    const furnace_hmi_controller_observation_t *o)
{
    if (!o->status_valid || o->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE) {
        return FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    }
    if (o->temperature_validity == FURNACE_HMI_CONTROLLER_MEASURED_VALID) {
        return (o->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT)
                   ? FURNACE_HMI_VALUE_VALIDITY_CURRENT
                   : FURNACE_HMI_VALUE_VALIDITY_STALE;
    }
    if (o->temperature_validity == FURNACE_HMI_CONTROLLER_MEASURED_STALE) {
        return FURNACE_HMI_VALUE_VALIDITY_STALE;
    }
    return FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
}
