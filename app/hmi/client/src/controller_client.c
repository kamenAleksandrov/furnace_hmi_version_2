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
#define CLIENT_KIND_HMI 1U

#define READ_CHUNK 64U

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

static void fill_envelope(furnace_hmi_controller_client_t *c, struct furnace_wire_envelope *env)
{
    (void)memcpy(env->session_id, c->observation.session_id, sizeof(env->session_id));
    env->client_session_id = c->observation.client_session_id;
    env->request_id = c->next_request_id++;
}

static void request_capabilities(furnace_hmi_controller_client_t *c)
{
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_CAPABILITIES_REQUEST;
    fill_envelope(c, &c->out.u.capabilities_request.envelope);
    send_out(c);
}

/* A new request supersedes one still pending: only the newer gets SNAPSHOT_END. */
static void request_snapshot(furnace_hmi_controller_client_t *c)
{
    if (!c->observation.identity_known) {
        return;
    }
    (void)memset(&c->out, 0, sizeof(c->out));
    c->out.type = FURNACE_WIRE_MSG_SNAPSHOT_REQUEST;
    fill_envelope(c, &c->out.u.snapshot_request.envelope);
    c->observation.snapshot_request = FURNACE_HMI_REQUEST_PENDING;
    c->observation.snapshot_request_id = c->out.u.snapshot_request.envelope.request_id;
    c->observation.snapshot_reject_reason = 0U;
    saturating_increment(&c->observation.snapshots_requested);
    c->snapshot_active = true;
    c->pending_fault_count = 0U;
    send_out(c);
    changed(c);
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
    changed(c);
}

/* ---- the controller session -------------------------------------------------------------- */

static void discard_session_data(furnace_hmi_controller_client_t *c)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    o->capabilities_valid = false;
    o->status_valid = false;
    o->faults_valid = false;
    o->fault_count = 0U;
    o->run_event_sequence = 0U;
    o->fault_event_sequence = 0U;
    if (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) {
        o->snapshot_request = FURNACE_HMI_REQUEST_UNRESOLVED;
    }
    c->event_sequences_known = false;
    c->newest_fault_event_seen = 0U;
    c->pending_fault_count = 0U;
    changed(c);
}

static bool is_current_session(const furnace_hmi_controller_client_t *c, const uint8_t *session)
{
    return c->observation.identity_known &&
           memcmp(c->observation.session_id, session, FURNACE_HMI_CONTROLLER_SESSION_ID_SIZE) == 0;
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
    set_link(c, FURNACE_HMI_LINK_SYNCHRONIZING);
    c->sync_started_ms = c->now_ms;
    /* The controller sends CAPABILITIES and a snapshot to a new client or controller session. */
    c->snapshot_active = true;
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
    set_link(c, FURNACE_HMI_LINK_SYNCHRONIZING);
    c->sync_started_ms = c->now_ms;
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
    } else if (o->status_valid && s->status_sequence <= o->status_sequence) {
        return; /* older than what is shown; a gap alone is normal (coalesced) */
    }
    copy_status(o, s);
    c->last_status_ms = c->now_ms;
    changed(c);
    if (o->link == FURNACE_HMI_LINK_SILENT) {
        /* Contact again without a HELLO: rebuild from a snapshot (reconnect, step 5). */
        set_link(c, FURNACE_HMI_LINK_SYNCHRONIZING);
        c->sync_started_ms = c->now_ms;
        request_snapshot(c);
    }
}

static void on_fault_list(furnace_hmi_controller_client_t *c,
                          const struct furnace_wire_fault_list *list)
{
    if (!c->snapshot_active) {
        return;
    }
    if (list->first_index == 0U) {
        c->pending_fault_count = 0U;
    }
    for (uint8_t i = 0U; i < list->entries_count; i++) {
        const struct furnace_wire_fault_entry *e = &list->entries[i];
        furnace_hmi_controller_fault_t *f;

        if (c->pending_fault_count >= FURNACE_HMI_CONTROLLER_MAX_FAULTS) {
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

static void on_snapshot_end(furnace_hmi_controller_client_t *c,
                            const struct furnace_wire_snapshot_end *end)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    const bool ours = (o->snapshot_request == FURNACE_HMI_REQUEST_PENDING) &&
                      (end->request_id == o->snapshot_request_id);

    if (!c->snapshot_active || (end->request_id != 0U && !ours)) {
        return; /* superseded, or not expected */
    }
    if (o->status_valid && end->status_sequence < o->status_sequence) {
        /*
         * A snapshot is captured at one instant, never before a STATUS its session
         * already published: this is another controller session that drew the same
         * ID (a restart; improbable, not impossible). Its data replaces everything.
         */
        saturating_increment(&o->status_regressions);
        saturating_increment(&o->controller_session_changes);
        discard_session_data(c);
        set_link(c, FURNACE_HMI_LINK_SYNCHRONIZING);
        c->sync_started_ms = c->now_ms;
        request_capabilities(c);
        request_snapshot(c);
        return;
    }
    if (ours) {
        o->snapshot_request = FURNACE_HMI_REQUEST_COMPLETED;
    }
    (void)memcpy(o->faults, c->pending_faults, sizeof(o->faults));
    o->fault_count = c->pending_fault_count;
    o->faults_valid = true;
    o->run_event_sequence = end->run_event_sequence;
    o->fault_event_sequence = end->fault_sequence;
    c->event_sequences_known = true;
    c->snapshot_active = false;
    changed(c);
    if (o->capabilities_valid && o->status_valid) {
        set_link(c, FURNACE_HMI_LINK_CONNECTED);
    }
    if (c->newest_fault_event_seen > end->fault_sequence) {
        request_snapshot(c); /* a fault changed after the snapshot's instant */
    }
}

static void on_event(furnace_hmi_controller_client_t *c, uint32_t sequence, bool fault)
{
    furnace_hmi_controller_observation_t *o = &c->observation;
    uint32_t *last = fault ? &o->fault_event_sequence : &o->run_event_sequence;

    if (fault && sequence > c->newest_fault_event_seen) {
        c->newest_fault_event_seen = sequence;
    }
    if (!c->event_sequences_known || sequence <= *last) {
        return; /* before the snapshot's base, or already in it */
    }
    if (sequence != *last + 1U) {
        saturating_increment(&o->event_gaps);
        request_snapshot(c);
    } else if (fault) {
        request_snapshot(c); /* the fault list is taken from a snapshot, never inferred */
    }
    *last = sequence;
    changed(c);
}

static void on_command_rejected(furnace_hmi_controller_client_t *c,
                                const struct furnace_wire_command_rejected *r)
{
    furnace_hmi_controller_observation_t *o = &c->observation;

    if (o->snapshot_request != FURNACE_HMI_REQUEST_PENDING ||
        r->request_id != o->snapshot_request_id) {
        return;
    }
    o->snapshot_request = FURNACE_HMI_REQUEST_REJECTED;
    o->snapshot_reject_reason = r->reason;
    c->snapshot_active = false;
    changed(c);
    if (r->reason == REJECT_DUPLICATE_REQUEST && r->request_id == 1U) {
        /* The first request of a sequence was a duplicate: this client session ID was used. */
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
    default:
        break; /* SETTINGS, proposals and results: later milestones */
    }
}

/* ---- time ---------------------------------------------------------------------------------- */

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
     * already shown as stale, and the synchronization timeout asks again.
     */
    if (o->link == FURNACE_HMI_LINK_CONNECTED &&
        elapsed(now, c->last_status_ms, o->link_silence_ms)) {
        set_link(c, FURNACE_HMI_LINK_SILENT);
        send_hello(c);
        return;
    }
    if (o->link == FURNACE_HMI_LINK_SYNCHRONIZING &&
        elapsed(now, c->sync_started_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS)) {
        c->sync_started_ms = now;
        if (!o->capabilities_valid) {
            request_capabilities(c);
        }
        request_snapshot(c);
    }
    if (o->capabilities_valid && o->heartbeat_interval_ms > 0U &&
        elapsed(now, c->last_send_ms, o->heartbeat_interval_ms)) {
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
