/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The HMI's controller client (Phase 21 M21-3). The rules are the controller
 * interface's "Session, link and delivery"; the plan's M21-3 design lists how
 * this client applies them.
 */

#include <stddef.h>
#include <string.h>

#include <furnace_hmi/controller_client.h>

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

#define READ_CHUNK 64U

static void unresolve(furnace_hmi_controller_client_t *c, bool stop,
                      furnace_hmi_command_unresolved_t cause);

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
    unresolve(c, false, FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED);
    unresolve(c, true, FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED);
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

/* The pending command of this request_id, if any: the Stop slot first. */
static furnace_hmi_command_outcome_t *pending_command(furnace_hmi_controller_client_t *c,
                                                      uint32_t request_id)
{
    for (int slot = 1; slot >= 0; slot--) {
        furnace_hmi_command_outcome_t *o = outcome_of(c, slot == 1);

        if (o->state == FURNACE_HMI_COMMAND_PENDING && o->request_id == request_id) {
            return o;
        }
    }
    return NULL;
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
                                  uint32_t request_id, uint8_t reason, uint32_t blocking,
                                  uint32_t run_id)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, request_id);

    if (o != NULL) {
        o->blocking_conditions = blocking;
        o->run_id = run_id;
        complete(c, o, accepted, reason);
    }
}

static void on_fault_acknowledge_result(furnace_hmi_controller_client_t *c,
                                        const struct furnace_wire_fault_acknowledge_result *r)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, r->request_id);

    if (o != NULL) {
        complete(c, o, r->result == ACKNOWLEDGE_DONE, r->result);
    }
}

static void on_fault_recovery_result(furnace_hmi_controller_client_t *c,
                                     const struct furnace_wire_fault_recovery_result *r)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, r->request_id);

    if (o != NULL) {
        o->fault_found = r->found;
        complete(c, o, r->found && r->reply == RECOVERY_RECOVERED, r->reply);
    }
}

static void on_start_prepare_result(furnace_hmi_controller_client_t *c,
                                    const struct furnace_wire_start_prepare_result *r)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, r->request_id);

    if (o != NULL) {
        o->store_readiness = r->store_readiness;
        complete(c, o, true, 0U);
    }
}

/*
 * COMMAND_REJECTED of a pending command: refused by the service, except that a
 * duplicate and STOP_UNRESOLVED leave the outcome unknown (controller interface).
 */
static bool command_rejected(furnace_hmi_controller_client_t *c, uint32_t request_id,
                             uint8_t reason)
{
    furnace_hmi_command_outcome_t *o = pending_command(c, request_id);

    if (o == NULL) {
        return false;
    }
    o->reason = reason;
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
 * connected, until its attempts run out. Stop waits through a silent link.
 */
static void on_command_time(furnace_hmi_controller_client_t *c)
{
    for (int slot = 0; slot < 2; slot++) {
        const bool stop = (slot == 1);
        furnace_hmi_command_outcome_t *o = outcome_of(c, stop);
        const uint8_t attempts =
            stop ? FURNACE_HMI_CLIENT_STOP_ATTEMPTS : FURNACE_HMI_CLIENT_COMMAND_ATTEMPTS;

        if (o->state != FURNACE_HMI_COMMAND_PENDING ||
            c->observation.link != FURNACE_HMI_LINK_CONNECTED ||
            !elapsed(c->now_ms, delivery_of(c, stop)->last_sent_ms,
                     FURNACE_HMI_CLIENT_COMMAND_RETRY_MS)) {
            continue;
        }
        if (o->attempts >= attempts) {
            unresolve(c, stop, FURNACE_HMI_COMMAND_UNRESOLVED_NO_ANSWER);
        } else {
            send_command(c, stop);
        }
    }
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

    if (command->kind <= FURNACE_HMI_COMMAND_NONE || command->kind >= FURNACE_HMI_COMMAND_COUNT) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_INVALID;
    } else if (c->observation.link != FURNACE_HMI_LINK_CONNECTED) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT;
    } else if (o->state == FURNACE_HMI_COMMAND_PENDING) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_BUSY;
    } else if (!take_request_id(c, &request_id)) {
        refusal = FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT; /* a new client session is opening */
    }
    if (refusal != FURNACE_HMI_COMMAND_LOCAL_NONE) {
        c->observation.command_refusal.submission = submission;
        c->observation.command_refusal.kind = command->kind;
        c->observation.command_refusal.reason = refusal;
        changed(c);
        return refusal;
    }
    (void)memset(o, 0, sizeof(*o));
    o->submission = submission;
    o->kind = command->kind;
    o->state = FURNACE_HMI_COMMAND_PENDING;
    o->request_id = request_id;
    delivery_of(c, stop)->command = *command;
    send_command(c, stop);
    return FURNACE_HMI_COMMAND_LOCAL_NONE;
}

/* ---- the controller session -------------------------------------------------------------- */

static void discard_session_data(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    o->capabilities_valid = false;
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
    o->setpoint_valid = s->run_status.setpoint_valid;
    o->setpoint_decicelsius = s->run_status.setpoint;
    o->heating_requested = s->run_status.heating_requested;
    o->heating_inhibited = s->fault_status.heating_inhibited;
    o->latched_class = s->fault_status.latched_class;
    o->inhibit_ids = s->fault_status.inhibit_ids;
    o->unacknowledged_faults = s->fault_status.unacknowledged_count;
    o->session_fresh = s->controller.session_fresh;
    o->store_readiness = s->controller.store_readiness;
    o->execution_active = s->lifecycle.execution_active;
    o->heating_authorized = s->lifecycle.heating_authorized;
    o->navigation_permitted = s->lifecycle.navigation_permitted;
    o->fan_feedback = s->equipment.fan_feedback;
    o->door_state = s->equipment.door_state;
    o->emergency_active = s->equipment.emergency_active;
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
    const bool command = command_rejected(c, r->request_id, r->reason);

    if (!snapshot && !capabilities && !command) {
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
                              m->u.run_command_accepted.reason,
                              m->u.run_command_accepted.blocking_conditions,
                              m->u.run_command_accepted.run_id);
        break;
    case FURNACE_WIRE_MSG_RUN_COMMAND_REJECTED:
        on_run_command_result(c, false, m->u.run_command_rejected.request_id,
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
    default:
        break; /* SETTINGS, start proposals and library replies: later milestones */
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
    update_availability(client);
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
