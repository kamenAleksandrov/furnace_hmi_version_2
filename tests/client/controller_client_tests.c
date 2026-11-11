/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Scenarios of the HMI's controller client (Phase 21 M21-3) against a scripted
 * controller on an in-memory loopback. The controller's frames are encoded by
 * the copied controller codec, so the bytes are the controller's own. One
 * portable source: a CTest program on the host and a Ztest suite on Zephyr.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <furnace/crc32.h>
#include <furnace/wire.h>
#include <furnace/wire_messages.h>
#include <furnace_hmi/controller_client.h>
#include <furnace_hmi/loopback_port.h>

#include "controller_client_tests.h"

#define FIRST_CLIENT_ID UINT32_C(0x0A0A0A01)
#define SESSION_A 0xA5U
#define SESSION_B 0x5BU
#define HELLO_OK 0U
#define HELLO_VERSION_UNSUPPORTED 1U
#define REJECT_DUPLICATE_REQUEST 4U
#define REJECT_NOT_CONNECTED 1U
#define REJECT_STALE_SESSION 2U
#define REJECT_MALFORMED 6U
#define SENT_CAPACITY 64U

typedef struct {
    furnace_hmi_loopback_t link;
    furnace_hmi_byte_port_t controller_port;
    furnace_hmi_controller_client_t client;
    struct furnace_wire_rx rx;
    struct furnace_wire_tx tx;
    struct furnace_wire_message out;
    struct furnace_wire_message in;
    struct furnace_wire_hello last_hello;
    struct furnace_wire_heartbeat last_heartbeat;
    struct furnace_wire_snapshot_request last_snapshot_request;
    uint32_t now;
    uint32_t ids_issued;
    uint16_t sent[SENT_CAPACITY];
    size_t sent_count;
    bool forbidden_sent;
    /* Command scenarios (M21-5) allow commands; the newest one the client sent. */
    bool commands_allowed;
    uint16_t last_command_type;
    struct furnace_wire_message last_command;
} fixture_t;

static fixture_t fx;
static int failures;
static const char *scenario;

static void fail(const char *check)
{
    failures++;
    controller_client_tests_report(check, scenario);
}

#define EXPECT(cond, check)                                                                        \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fail(check);                                                                           \
        }                                                                                          \
    } while (0)

static uint32_t issue_client_id(void *context)
{
    fixture_t *f = context;

    f->ids_issued++;
    return FIRST_CLIENT_ID + (f->ids_issued - 1U) * UINT32_C(0x100);
}

static const furnace_hmi_controller_observation_t *obs(void)
{
    return furnace_hmi_controller_client_observation(&fx.client);
}

/* Decode what the client sent, as the controller would receive it. */
static void drain_client(void)
{
    uint8_t bytes[256];
    size_t n;
    struct furnace_wire_frame frame;

    while ((n = fx.controller_port.read(fx.controller_port.context, bytes, sizeof(bytes))) > 0U) {
        for (size_t i = 0U; i < n; i++) {
            if (furnace_wire_rx_feed(&fx.rx, bytes[i], &frame) != FURNACE_WIRE_RX_FRAME) {
                continue;
            }
            if (fx.sent_count < SENT_CAPACITY) {
                fx.sent[fx.sent_count++] = frame.msg_type;
            }
            if (furnace_wire_payload_decode(frame.msg_type, frame.payload, frame.payload_len,
                                            &fx.in) != FURNACE_WIRE_OK) {
                fx.forbidden_sent = true;
                continue;
            }
            switch (frame.msg_type) {
            case FURNACE_WIRE_MSG_HELLO:
            case FURNACE_WIRE_MSG_HEARTBEAT:
            case FURNACE_WIRE_MSG_SNAPSHOT_REQUEST:
            case FURNACE_WIRE_MSG_CAPABILITIES_REQUEST:
                break;
            default:
                if (!fx.commands_allowed) {
                    fx.forbidden_sent = true;
                }
                fx.last_command_type = frame.msg_type;
                fx.last_command = fx.in;
                break;
            }
            if (frame.msg_type == FURNACE_WIRE_MSG_HELLO) {
                fx.last_hello = fx.in.u.hello;
            } else if (frame.msg_type == FURNACE_WIRE_MSG_HEARTBEAT) {
                fx.last_heartbeat = fx.in.u.heartbeat;
            } else if (frame.msg_type == FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) {
                fx.last_snapshot_request = fx.in.u.snapshot_request;
            }
        }
    }
}

/*
 * Advance the clock by ms in steps 50 ms apart. At each step the client polls
 * until it has read everything waiting (one poll reads a bounded amount), as
 * its link thread does, and the controller side takes what the client sent.
 */
static void advance(uint32_t ms)
{
    do {
        const uint32_t step = (ms > 50U) ? 50U : ms;

        fx.now += step;
        ms -= step;
        do {
            furnace_hmi_controller_client_poll(&fx.client, fx.now);
        } while (fx.link.b_to_a.count > 0U);
        drain_client();
    } while (ms > 0U);
}

static size_t sent_since(size_t mark, uint16_t type)
{
    size_t count = 0U;

    for (size_t i = mark; i < fx.sent_count; i++) {
        count += (fx.sent[i] == type) ? 1U : 0U;
    }
    return count;
}

static void begin(const char *name)
{
    const furnace_hmi_controller_client_config_t config = {
        .port = furnace_hmi_loopback_port_a(&fx.link),
        .new_client_session_id = issue_client_id,
        .id_context = &fx,
    };

    scenario = name;
    (void)memset(&fx, 0, sizeof(fx));
    furnace_hmi_loopback_init(&fx.link);
    fx.controller_port = furnace_hmi_loopback_port_b(&fx.link);
    furnace_wire_rx_init(&fx.rx);
    fx.now = UINT32_C(1000);
    /* config.port was taken before the memset cleared fx; it points into fx.link. */
    furnace_hmi_controller_client_init(&fx.client, &config, fx.now);
    advance(0U);
}

static void end(void)
{
    EXPECT(!fx.forbidden_sent, "only HELLO, HEARTBEAT, SNAPSHOT_REQUEST, CAPABILITIES_REQUEST");
}

/* ---- the scripted controller ------------------------------------------------------------ */

static void send_out(void)
{
    if (furnace_wire_tx_encode(&fx.tx, &fx.out) != FURNACE_WIRE_OK) {
        fail("fixture: controller message encodes");
        return;
    }
    (void)fx.controller_port.write(fx.controller_port.context, fx.tx.encoded, fx.tx.encoded_len);
}

static void start_out(uint16_t type)
{
    (void)memset(&fx.out, 0, sizeof(fx.out));
    fx.out.type = type;
}

static void send_hello_ack(uint8_t result, uint8_t session)
{
    start_out(FURNACE_WIRE_MSG_HELLO_ACK);
    fx.out.u.hello_ack.result = result;
    (void)memset(fx.out.u.hello_ack.session_id, session, FURNACE_WIRE_SESSION_ID_SIZE);
    fx.out.u.hello_ack.firmware_id = 1U;
    fx.out.u.hello_ack.boot_count = 7U;
    send_out();
}

/* A HELLO_ACK of protocol 0.<minor>: its header and result only are readable to this client. */
static void send_hello_ack_of_version(uint8_t minor, uint8_t result)
{
    uint8_t frame[FURNACE_WIRE_HEADER_SIZE + 13U + FURNACE_WIRE_CRC_SIZE] = {0};
    uint8_t encoded[sizeof(frame) + 2U];
    const size_t payload_len = 13U;
    size_t len = 0U;
    uint32_t crc;

    frame[0] = 0U;
    frame[1] = minor;
    frame[2] = (uint8_t)(FURNACE_WIRE_MSG_HELLO_ACK & 0xFFU);
    frame[3] = (uint8_t)(FURNACE_WIRE_MSG_HELLO_ACK >> 8);
    frame[4] = (uint8_t)payload_len;
    frame[FURNACE_WIRE_HEADER_SIZE] = result;
    crc = furnace_crc32(0U, frame, FURNACE_WIRE_HEADER_SIZE + payload_len);
    for (size_t i = 0U; i < 4U; i++) {
        frame[FURNACE_WIRE_HEADER_SIZE + payload_len + i] = (uint8_t)(crc >> (8U * i));
    }
    if (furnace_wire_cobs_encode(frame, sizeof(frame), encoded, sizeof(encoded) - 1U, &len) !=
        FURNACE_WIRE_OK) {
        fail("fixture: other-version HELLO_ACK encodes");
        return;
    }
    encoded[len++] = 0U;
    (void)fx.controller_port.write(fx.controller_port.context, encoded, len);
}

static void fill_capabilities(uint32_t request_id, uint8_t session)
{
    struct furnace_wire_capabilities *c = &fx.out.u.capabilities;

    start_out(FURNACE_WIRE_MSG_CAPABILITIES);
    c->request_id = request_id;
    (void)memset(c->session_id, session, FURNACE_WIRE_SESSION_ID_SIZE);
    c->proto_major = FURNACE_WIRE_PROTO_MAJOR;
    c->proto_minor = FURNACE_WIRE_PROTO_MINOR;
    c->firmware_id = 1U;
    c->configured = true;
    c->config_state = 1U;
    (void)memcpy(c->model_id, (session == SESSION_A) ? "FX-SIM-A" : "FX-SIM-B", sizeof("FX-SIM-A"));
    c->config_revision = 4U;
    c->profile_revision = 2U;
    c->max_stages = 18U;
    c->max_payload = FURNACE_WIRE_MAX_PAYLOAD;
    c->status_interval_ms = 200U;
    c->heartbeat_interval_ms = 500U;
    c->link_silence_ms = 3000U;
}

static void send_capabilities(uint32_t request_id, uint8_t session)
{
    fill_capabilities(request_id, session);
    send_out();
}

static void send_status(uint32_t sequence, int32_t temperature, uint8_t validity, uint8_t session)
{
    struct furnace_wire_status *s = &fx.out.u.status;

    start_out(FURNACE_WIRE_MSG_STATUS);
    (void)memset(s->session_id, session, FURNACE_WIRE_SESSION_ID_SIZE);
    s->status_sequence = sequence;
    s->measured_temperature.value = temperature;
    s->measured_temperature.validity = validity;
    s->measured_temperature.age_ms = 120U;
    s->run_status.process_temperature.value = temperature;
    s->run_status.process_temperature.validity = validity;
    s->run_status.run_state = 2U;
    s->run_status.session_kind = 1U;
    s->run_status.run_id = 41U;
    s->run_status.program_id = 3U;
    s->run_status.program_revision = 2U;
    s->run_status.stage_index = 1U;
    s->run_status.stage_count = 4U;
    s->run_status.setpoint_valid = true;
    s->run_status.setpoint = 1500;
    s->run_status.heating_requested = true;
    s->run_status.measurement_qualified_s = 90U;
    s->fault_status.unacknowledged_count = 1U;
    s->fault_status.latched_class = 2U;
    s->fault_status.inhibit_ids = 0x8U;
    s->fault_status.heating_inhibited = true;
    s->controller.configured = true;
    s->controller.config_state = 1U;
    s->controller.session_fresh = true;
    s->controller.store_readiness = 1U;
    s->lifecycle.execution_active = true;
    s->lifecycle.heating_authorized = true;
    s->equipment.measurement_state = 2U;
    s->equipment.door_state = 1U;
    send_out();
}

static void send_fault_list(uint32_t request_id, uint32_t status_sequence, uint8_t total,
                            uint8_t first, uint8_t count)
{
    struct furnace_wire_fault_list *l = &fx.out.u.fault_list;

    start_out(FURNACE_WIRE_MSG_FAULT_LIST);
    l->request_id = request_id;
    l->status_sequence = status_sequence;
    l->total = total;
    l->first_index = first;
    l->entries_count = count;
    for (uint8_t i = 0U; i < count; i++) {
        l->entries[i].fault_id = (uint8_t)(first + i + 1U);
        l->entries[i].source = 1U;
        l->entries[i].class = 2U;
        l->entries[i].condition_active = (i == 0U);
        l->entries[i].latched = true;
        l->entries[i].acknowledged = false;
        l->entries[i].occurrences = 1U;
    }
    send_out();
}

static void send_snapshot_end(uint32_t request_id, uint32_t status_sequence, uint32_t fault_seq,
                              uint32_t run_seq)
{
    start_out(FURNACE_WIRE_MSG_SNAPSHOT_END);
    fx.out.u.snapshot_end.request_id = request_id;
    fx.out.u.snapshot_end.status_sequence = status_sequence;
    fx.out.u.snapshot_end.fault_sequence = fault_seq;
    fx.out.u.snapshot_end.run_event_sequence = run_seq;
    send_out();
}

static void send_run_event(uint32_t sequence)
{
    start_out(FURNACE_WIRE_MSG_RUN_EVENT);
    fx.out.u.run_event.sequence = sequence;
    fx.out.u.run_event.run_id = 41U;
    fx.out.u.run_event.event = 1U;
    send_out();
}

static void send_fault_event(uint32_t sequence)
{
    start_out(FURNACE_WIRE_MSG_FAULT_EVENT);
    fx.out.u.fault_event.sequence = sequence;
    fx.out.u.fault_event.fault_id = 3U;
    fx.out.u.fault_event.transition = 1U;
    send_out();
}

static void send_command_rejected(uint32_t request_id, uint8_t reason)
{
    start_out(FURNACE_WIRE_MSG_COMMAND_REJECTED);
    fx.out.u.command_rejected.request_id = request_id;
    fx.out.u.command_rejected.msg_type = FURNACE_WIRE_MSG_SNAPSHOT_REQUEST;
    fx.out.u.command_rejected.reason = reason;
    send_out();
}

/* HELLO answered, CAPABILITIES and a complete snapshot: run seq 5, fault seq 3, one fault. */
static void synchronize(uint8_t session, uint32_t status_sequence)
{
    send_hello_ack(HELLO_OK, session);
    send_capabilities(0U, session);
    send_status(status_sequence, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, session);
    send_fault_list(0U, status_sequence, 1U, 0U, 1U);
    send_snapshot_end(0U, status_sequence, 3U, 5U);
    advance(10U);
}

/* ---- scenarios -------------------------------------------------------------------------- */

static void hello_at_start(void)
{
    begin("hello at start");
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_HELLO) == 1U, "one HELLO at once");
    EXPECT(fx.last_hello.client_session_id == FIRST_CLIENT_ID, "HELLO names the client session");
    EXPECT(fx.last_hello.client_kind == 1U, "client kind HMI");
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTING, "connecting");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE, "unavailable");
    EXPECT(furnace_hmi_controller_temperature_validity(obs()) ==
               FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no temperature");
    end();
}

static void hello_retry(void)
{
    begin("hello retry");
    advance(FURNACE_HMI_CLIENT_HELLO_RETRY_MS - 50U);
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_HELLO) == 1U, "no second HELLO early");
    advance(100U);
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_HELLO) == 2U, "HELLO repeated after the retry time");
    end();
}

static void handshake_to_current(void)
{
    const furnace_hmi_controller_observation_t *o;

    begin("handshake to current");
    synchronize(SESSION_A, 10U);
    o = obs();
    EXPECT(o->link == FURNACE_HMI_LINK_CONNECTED, "connected");
    EXPECT(o->availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT, "current");
    EXPECT(o->identity_known && o->session_id[0] == SESSION_A && o->session_id[15] == SESSION_A,
           "controller session");
    EXPECT(o->firmware_id == 1U && o->boot_count == 7U, "firmware and boot count");
    EXPECT(o->controller_proto_major == 0U && o->controller_proto_minor == 16U, "version 0.16");
    EXPECT(o->capabilities_valid && strcmp(o->model_id, "FX-SIM-A") == 0, "model");
    EXPECT(o->status_interval_ms == 200U && o->heartbeat_interval_ms == 500U &&
               o->link_silence_ms == 3000U,
           "intervals from CAPABILITIES");
    EXPECT(o->status_valid && o->status_sequence == 10U, "status");
    EXPECT(o->temperature_decicelsius == 1234 &&
               o->temperature_validity == FURNACE_HMI_CONTROLLER_MEASURED_VALID &&
               o->temperature_age_ms == 120U,
           "temperature as sent");
    EXPECT(furnace_hmi_controller_temperature_validity(o) == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "temperature current");
    EXPECT(o->measurement_state == 2U && o->measurement_qualified_s == 90U, "measurement");
    EXPECT(o->run_state == 2U && o->run_id == 41U && o->program_id == 3U &&
               o->program_revision == 2U && o->stage_index == 1U && o->stage_count == 4U,
           "run");
    EXPECT(o->setpoint_valid && o->setpoint_decicelsius == 1500 && o->heating_requested,
           "setpoint");
    EXPECT(o->heating_inhibited && o->latched_class == 2U && o->inhibit_ids == 0x8U &&
               o->unacknowledged_faults == 1U,
           "fault status");
    EXPECT(o->session_fresh && o->store_readiness == 1U && o->execution_active &&
               o->heating_authorized && o->door_state == 1U,
           "controller, lifecycle, equipment");
    EXPECT(o->faults_valid && o->fault_count == 1U && o->faults[0].fault_id == 1U &&
               o->faults[0].latched && o->faults[0].condition_active,
           "fault list");
    EXPECT(o->run_event_sequence == 5U && o->fault_event_sequence == 3U, "event sequences");
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 0U,
           "no request: the controller sent the snapshot");
    end();
}

static void incompatible_version(void)
{
    begin("incompatible version");
    send_hello_ack_of_version(15U, HELLO_VERSION_UNSUPPORTED);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_INCOMPATIBLE, "incompatible");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE, "unavailable");
    EXPECT(obs()->controller_version_known && obs()->controller_proto_minor == 15U,
           "controller version from the header");
    EXPECT(obs()->hello_result == HELLO_VERSION_UNSUPPORTED, "result");
    EXPECT(!obs()->identity_known, "no session from a refusal");
    advance(2000U);
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 0U, "nothing but HELLO");
    end();
}

static void silence_to_stale(void)
{
    size_t mark;

    begin("silence to stale");
    synchronize(SESSION_A, 10U);
    mark = fx.sent_count;
    advance(2900U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "current within link_silence_ms");
    advance(200U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent after link_silence_ms");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE, "stale");
    EXPECT(obs()->status_valid && obs()->temperature_decicelsius == 1234, "data kept");
    EXPECT(furnace_hmi_controller_temperature_validity(obs()) == FURNACE_HMI_VALUE_VALIDITY_STALE,
           "temperature stale");
    EXPECT(obs()->status_age_ms >= 3000U, "age");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) >= 1U, "HELLO again");
    end();
}

static void reconnect_same_session(void)
{
    size_t mark;

    begin("reconnect, same controller session");
    synchronize(SESSION_A, 10U);
    advance(3200U);
    mark = fx.sent_count;
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(10U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "snapshot requested");
    EXPECT(fx.last_snapshot_request.envelope.session_id[0] == SESSION_A &&
               fx.last_snapshot_request.envelope.client_session_id == FIRST_CLIENT_ID &&
               fx.last_snapshot_request.envelope.request_id == 1U,
           "envelope");
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_PENDING, "pending");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE, "stale until complete");
    send_status(30U, 1250, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(1U, 30U, 0U, 0U, 0U);
    send_snapshot_end(1U, 30U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "connected");
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_COMPLETED, "completed");
    EXPECT(obs()->temperature_decicelsius == 1250 && obs()->fault_count == 0U, "rebuilt");
    EXPECT(obs()->controller_session_changes == 0U, "no restart");
    end();
}

static void controller_restart(void)
{
    begin("controller restart");
    synchronize(SESSION_A, 10U);
    advance(3200U);
    send_hello_ack(HELLO_OK, SESSION_B);
    advance(10U);
    EXPECT(obs()->controller_session_changes == 1U, "restart seen");
    EXPECT(obs()->session_id[0] == SESSION_B, "new session");
    EXPECT(!obs()->status_valid && !obs()->capabilities_valid && !obs()->faults_valid,
           "old session's data discarded");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE, "unavailable");
    EXPECT(obs()->link == FURNACE_HMI_LINK_SYNCHRONIZING, "synchronizing");
    send_capabilities(0U, SESSION_B);
    send_status(1U, 200, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_B);
    send_fault_list(0U, 1U, 0U, 0U, 0U);
    send_snapshot_end(0U, 1U, 0U, 0U);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "connected to the new session");
    EXPECT(strcmp(obs()->model_id, "FX-SIM-B") == 0 && obs()->temperature_decicelsius == 200 &&
               obs()->status_sequence == 1U,
           "new session's data");
    end();
}

/*
 * A restarted controller that drew the same session ID (improbable, not
 * impossible): its snapshot reports a status_sequence below one the client
 * already holds, which one session never does. Nothing old may be shown as current.
 */
static void restart_with_repeated_session_id(void)
{
    begin("restart with a repeated session ID");
    synchronize(SESSION_A, 100U);
    advance(3200U);
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(1U, 205, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(0U, 1U, 0U, 0U, 0U);
    send_snapshot_end(0U, 1U, 0U, 0U);
    advance(10U);
    EXPECT(obs()->controller_session_changes == 1U, "seen as another controller session");
    EXPECT(obs()->status_regressions == 1U, "counted");
    EXPECT(obs()->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT,
           "the old session's status is never current");
    EXPECT(!obs()->status_valid || obs()->temperature_decicelsius != 1234, "old status discarded");
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_PENDING, "a new snapshot asked for");
    EXPECT(!obs()->capabilities_valid, "capabilities asked for again");
    send_capabilities(obs()->snapshot_request_id - 1U, SESSION_A);
    send_status(2U, 210, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(obs()->snapshot_request_id, 2U, 0U, 0U, 0U);
    send_snapshot_end(obs()->snapshot_request_id, 2U, 0U, 0U);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED && obs()->temperature_decicelsius == 210 &&
               obs()->status_sequence == 2U,
           "current with the new controller's data");
    end();
}

static void restart_unresolves_request(void)
{
    begin("restart leaves the request unresolved");
    synchronize(SESSION_A, 10U);
    advance(3200U);
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(10U);
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_PENDING, "pending");
    send_hello_ack(HELLO_OK, SESSION_B);
    advance(10U);
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_UNRESOLVED, "unresolved, not replayed");
    end();
}

static void older_status_ignored(void)
{
    begin("older status ignored");
    synchronize(SESSION_A, 10U);
    send_status(9U, 999, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    advance(10U);
    EXPECT(obs()->status_sequence == 10U && obs()->temperature_decicelsius == 1234, "kept newer");
    send_status(12U, 1300, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    advance(10U);
    EXPECT(obs()->status_sequence == 12U && obs()->temperature_decicelsius == 1300,
           "newer applied; a gap in status is normal");
    end();
}

static void status_of_other_session(void)
{
    size_t mark;

    begin("status of another controller session");
    synchronize(SESSION_A, 10U);
    mark = fx.sent_count;
    send_status(1U, 500, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_B);
    advance(10U);
    EXPECT(obs()->controller_session_changes == 1U, "identity change seen");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE, "unavailable");
    EXPECT(!obs()->capabilities_valid, "capabilities discarded");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U, "HELLO to resynchronize");
    end();
}

static void event_gap_requests_snapshot(void)
{
    size_t mark;

    begin("event gap requests a snapshot");
    synchronize(SESSION_A, 10U);
    mark = fx.sent_count;
    send_run_event(5U);
    send_run_event(6U);
    advance(10U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 0U, "in sequence: no request");
    EXPECT(obs()->run_event_sequence == 6U && obs()->event_gaps == 0U, "applied");
    send_run_event(8U);
    advance(10U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "gap: snapshot requested");
    EXPECT(obs()->event_gaps == 1U, "gap counted");
    end();
}

static void fault_event_refreshes_list(void)
{
    size_t mark;

    begin("fault event refreshes the fault list");
    synchronize(SESSION_A, 10U);
    mark = fx.sent_count;
    send_fault_event(4U);
    advance(10U);
    EXPECT(obs()->fault_event_sequence == 4U && obs()->event_gaps == 0U, "in sequence");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "list asked again");
    end();
}

static void corrupt_and_garbage(void)
{
    static const uint8_t garbage[] = {0x13U, 0x37U, 0xFFU, 0x00U, 0x02U, 0x00U, 0x41U};

    begin("corruption and garbage");
    synchronize(SESSION_A, 10U);
    (void)fx.controller_port.write(fx.controller_port.context, garbage, sizeof(garbage));
    start_out(FURNACE_WIRE_MSG_STATUS);
    send_status(11U, 777, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    /* The same status again, one byte flipped on the wire: the CRC fails. */
    fx.tx.encoded[8] ^= 0x10U;
    (void)fx.controller_port.write(fx.controller_port.context, fx.tx.encoded, fx.tx.encoded_len);
    send_status(12U, 888, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    advance(10U);
    EXPECT(obs()->frames_dropped >= 2U, "invalid frames dropped and counted");
    EXPECT(obs()->status_sequence == 12U && obs()->temperature_decicelsius == 888,
           "valid frames still applied");
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "link unaffected");
    end();
}

static void heartbeat_when_idle(void)
{
    size_t mark;

    begin("heartbeat when idle");
    synchronize(SESSION_A, 10U);
    mark = fx.sent_count;
    for (uint32_t i = 0U; i < 4U; i++) {
        send_status(11U + i, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
        advance(200U);
    }
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HEARTBEAT) >= 1U, "HEARTBEAT within the interval");
    EXPECT(fx.last_heartbeat.client_session_id == FIRST_CLIENT_ID, "names the client session");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 0U, "no HELLO while current");
    end();
}

static void duplicate_on_first_request(void)
{
    size_t mark;

    begin("duplicate on the first request: a new client session");
    synchronize(SESSION_A, 10U);
    advance(3200U);
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(10U);
    mark = fx.sent_count;
    send_command_rejected(1U, REJECT_DUPLICATE_REQUEST);
    advance(10U);
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_REJECTED &&
               obs()->snapshot_reject_reason == REJECT_DUPLICATE_REQUEST,
           "rejected with its reason");
    EXPECT(obs()->client_session_id != FIRST_CLIENT_ID && obs()->client_session_id != 0U,
           "new client session");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U &&
               fx.last_hello.client_session_id == obs()->client_session_id,
           "HELLO with the new client session");
    end();
}

static void rejected_request(void)
{
    size_t mark;

    begin("rejected request");
    synchronize(SESSION_A, 10U);
    advance(3200U);
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(10U);
    mark = fx.sent_count;
    send_command_rejected(1U, REJECT_MALFORMED);
    advance(10U);
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_REJECTED &&
               obs()->snapshot_reject_reason == REJECT_MALFORMED,
           "rejected");
    EXPECT(obs()->client_session_id == FIRST_CLIENT_ID, "same client session");
    EXPECT(obs()->availability == FURNACE_HMI_STATE_AVAILABILITY_STALE, "still stale");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 0U, "no retry at once");
    advance(FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS + 100U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "asked again after the timeout");
    end();
}

static void sync_timeout_requests(void)
{
    size_t mark;

    begin("synchronization timeout asks for the snapshot");
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(10U);
    mark = fx.sent_count;
    advance(FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS + 100U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_CAPABILITIES_REQUEST) == 1U, "capabilities asked");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "snapshot asked");
    EXPECT(obs()->link == FURNACE_HMI_LINK_SYNCHRONIZING, "still synchronizing");
    end();
}

static void fault_list_paged(void)
{
    begin("fault list in two frames");
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(0U, 10U, 20U, 0U, 16U);
    send_fault_list(0U, 10U, 20U, 16U, 4U);
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->faults_valid && obs()->fault_count == 20U, "20 entries");
    EXPECT(obs()->faults[0].fault_id == 1U && obs()->faults[19].fault_id == 20U, "in order");
    end();
}

static void client_messages_ignored(void)
{
    begin("client-direction messages ignored");
    synchronize(SESSION_A, 10U);
    start_out(FURNACE_WIRE_MSG_HELLO);
    fx.out.u.hello.client_session_id = 99U;
    fx.out.u.hello.client_kind = 1U;
    send_out();
    advance(10U);
    EXPECT(obs()->frames_ignored == 1U, "counted");
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "no effect");
    end();
}

static void bounded_poll(void)
{
    static uint8_t noise[2000];

    begin("bounded poll");
    (void)memset(noise, 0x55, sizeof(noise));
    (void)fx.controller_port.write(fx.controller_port.context, noise, sizeof(noise));
    fx.now += 10U;
    furnace_hmi_controller_client_poll(&fx.client, fx.now);
    EXPECT(fx.link.b_to_a.count >= sizeof(noise) - FURNACE_HMI_CLIENT_POLL_BYTES,
           "one poll reads at most FURNACE_HMI_CLIENT_POLL_BYTES");
    end();
}

static void stale_temperature_from_controller(void)
{
    begin("temperature validity as the controller marks it");
    synchronize(SESSION_A, 10U);
    send_status(11U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_STALE, SESSION_A);
    advance(10U);
    EXPECT(furnace_hmi_controller_temperature_validity(obs()) == FURNACE_HMI_VALUE_VALIDITY_STALE,
           "controller STALE");
    send_status(12U, 0, FURNACE_HMI_CONTROLLER_MEASURED_UNAVAILABLE, SESSION_A);
    advance(10U);
    EXPECT(furnace_hmi_controller_temperature_validity(obs()) ==
               FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "controller UNAVAILABLE");
    end();
}

static void clock_wrap(void)
{
    const furnace_hmi_controller_client_config_t config = {
        .port = furnace_hmi_loopback_port_a(&fx.link),
        .new_client_session_id = issue_client_id,
        .id_context = &fx,
    };

    begin("clock wrap");
    fx.now = UINT32_C(0xFFFFFE00);
    furnace_hmi_controller_client_init(&fx.client, &config, fx.now);
    synchronize(SESSION_A, 10U);
    advance(1000U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "current across the wrap");
    advance(2500U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent across the wrap");
    end();
}

/* ---- scenarios from the M21-3 protocol review -------------------------------------------- */

/* A periodic STATUS newer than the snapshot arrives before its end: normal (spec: Snapshot). */
static void periodic_status_within_snapshot(void)
{
    begin("periodic status within a snapshot");
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_status(11U, 1240, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(0U, 10U, 1U, 0U, 1U);
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "connected");
    EXPECT(obs()->controller_session_changes == 0U && obs()->status_regressions == 0U,
           "no session change");
    EXPECT(obs()->status_sequence == 11U && obs()->temperature_decicelsius == 1240,
           "the newer status kept");
    end();
}

/* A refresh while connected whose end is lost: asked again; the list is marked outdated. */
static void lost_end_while_connected(void)
{
    size_t mark;

    begin("lost snapshot end while connected");
    synchronize(SESSION_A, 10U);
    send_fault_event(4U);
    advance(10U);
    EXPECT(obs()->faults_outdated, "list outdated while refreshed");
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "status still current");
    mark = fx.sent_count;
    for (uint32_t i = 0U; i < 16U; i++) {
        send_status(11U + i, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
        advance(200U);
    }
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "asked again after the timeout");
    send_status(30U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(obs()->snapshot_request_id, 30U, 2U, 0U, 2U);
    send_snapshot_end(obs()->snapshot_request_id, 30U, 4U, 5U);
    advance(10U);
    EXPECT(!obs()->faults_outdated && obs()->fault_count == 2U, "list refreshed");
    end();
}

static void lost_fault_page(void)
{
    begin("lost fault list page");
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(0U, 10U, 20U, 16U, 4U); /* page 0 lost */
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->snapshots_incomplete == 1U, "counted");
    EXPECT(!obs()->faults_valid && obs()->link == FURNACE_HMI_LINK_SYNCHRONIZING,
           "not applied, not current");
    advance(FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS + 100U);
    EXPECT(obs()->snapshot_request == FURNACE_HMI_REQUEST_PENDING, "asked again");
    end();
}

static void missing_only_page(void)
{
    begin("missing only fault list page");
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->snapshots_incomplete == 1U && !obs()->faults_valid,
           "no list is not an empty list");
    EXPECT(obs()->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT, "not current");
    end();
}

static void superseded_snapshot(void)
{
    uint32_t first;

    begin("superseded snapshot");
    synchronize(SESSION_A, 10U);
    send_fault_event(4U);
    advance(10U);
    first = obs()->snapshot_request_id;
    for (uint32_t i = 0U; i < 16U; i++) {
        send_status(11U + i, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
        advance(200U);
    }
    EXPECT(obs()->snapshot_request_id != first, "a newer request");
    send_fault_list(first, 20U, 3U, 0U, 3U);
    send_snapshot_end(first, 20U, 4U, 5U);
    advance(10U);
    EXPECT(obs()->faults_outdated && obs()->fault_count == 1U, "the superseded end ignored");
    send_fault_list(obs()->snapshot_request_id, 27U, 2U, 0U, 2U);
    send_snapshot_end(obs()->snapshot_request_id, 27U, 4U, 5U);
    advance(10U);
    EXPECT(!obs()->faults_outdated && obs()->fault_count == 2U, "the newer end applied");
    end();
}

static void events_during_first_snapshot(void)
{
    size_t mark;

    begin("events during the first snapshot");
    mark = fx.sent_count;
    send_hello_ack(HELLO_OK, SESSION_A);
    send_capabilities(0U, SESSION_A);
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_event(4U); /* after the snapshot's instant: fault sequence 3 */
    send_run_event(5U);   /* already in the snapshot */
    send_fault_list(0U, 10U, 1U, 0U, 1U);
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "connected");
    EXPECT(obs()->run_event_sequence == 5U && obs()->event_gaps == 0U, "run event in the snapshot");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U && obs()->faults_outdated,
           "the later fault asks for the list again");
    end();
}

static void request_id_exhausted(void)
{
    size_t mark;

    begin("request ID exhausted");
    synchronize(SESSION_A, 10U);
    fx.client.next_request_id = UINT32_MAX;
    mark = fx.sent_count;
    send_fault_event(4U);
    advance(10U);
    EXPECT(obs()->client_session_id != FIRST_CLIENT_ID, "a new client session");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U, "HELLO with it");
    EXPECT(fx.last_snapshot_request.envelope.request_id != UINT32_MAX, "2^32-1 never sent");
    end();
}

static void absurd_intervals(void)
{
    begin("absurd link intervals");
    send_hello_ack(HELLO_OK, SESSION_A);
    fill_capabilities(0U, SESSION_A);
    fx.out.u.capabilities.link_silence_ms = 0U;
    send_out();
    fill_capabilities(0U, SESSION_A);
    fx.out.u.capabilities.link_silence_ms = UINT32_MAX;
    send_out();
    send_status(10U, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    send_fault_list(0U, 10U, 0U, 0U, 0U);
    send_snapshot_end(0U, 10U, 3U, 5U);
    advance(10U);
    EXPECT(obs()->capabilities_rejected == 2U, "both refused");
    EXPECT(!obs()->capabilities_valid, "no capabilities accepted");
    EXPECT(obs()->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT, "never current");
    end();
}

static void rejections_send_hello(void)
{
    size_t mark;

    begin("NOT_CONNECTED and STALE_SESSION send HELLO");
    synchronize(SESSION_A, 10U);
    send_fault_event(4U);
    advance(10U);
    mark = fx.sent_count;
    send_command_rejected(obs()->snapshot_request_id, REJECT_NOT_CONNECTED);
    advance(10U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U, "HELLO after NOT_CONNECTED");
    send_fault_event(5U);
    advance(10U);
    mark = fx.sent_count;
    send_command_rejected(obs()->snapshot_request_id, REJECT_STALE_SESSION);
    advance(10U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U, "HELLO after STALE_SESSION");
    end();
}

static void lost_hello_ack(void)
{
    begin("lost HELLO_ACK");
    advance(FURNACE_HMI_CLIENT_HELLO_RETRY_MS + 50U);
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_HELLO) == 2U, "HELLO again");
    synchronize(SESSION_A, 10U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "connected on the second answer");
    end();
}

static void duplicate_on_capabilities_request(void)
{
    size_t mark;

    begin("duplicate on the capabilities request: a new client session");
    synchronize(SESSION_A, 10U);
    send_status(1U, 500, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_B);
    advance(10U);
    send_hello_ack(HELLO_OK, SESSION_B);
    advance(10U);
    mark = fx.sent_count;
    send_command_rejected(fx.client.capabilities_request_id, REJECT_DUPLICATE_REQUEST);
    advance(10U);
    EXPECT(obs()->client_session_id != FIRST_CLIENT_ID, "new client session");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) == 1U, "HELLO with it");
    end();
}

static void older_periodic_status_recovers(void)
{
    size_t mark;

    begin("older periodic status asks for a snapshot");
    synchronize(SESSION_A, 10U);
    send_status(12U, 1300, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    advance(10U);
    mark = fx.sent_count;
    send_status(11U, 1250, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
    advance(10U);
    EXPECT(obs()->temperature_decicelsius == 1300, "the older one not shown");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_SNAPSHOT_REQUEST) == 1U, "snapshot asked (spec)");
    EXPECT(obs()->controller_session_changes == 0U, "no session change while connected");
    end();
}

/* ---- commands (M21-5) ------------------------------------------------------------------- */

#define RUN_COMMAND_PAUSE 1U
#define RUN_COMMAND_STOP 3U
#define RUN_COMMAND_MANUAL_START 6U
#define REJECT_QUEUE_FULL 5U
#define REJECT_STOP_UNRESOLVED 13U
#define RECOVERY_CONDITION_ACTIVE 2U
#define RECOVERY_RECOVERED 7U

static uint32_t status_sequence;

/* Synchronize with session A, commands allowed. */
static void begin_commands(const char *name)
{
    begin(name);
    fx.commands_allowed = true;
    status_sequence = 10U;
    synchronize(SESSION_A, status_sequence);
}

/* Advance ms with the controller's periodic STATUS, so the link stays current. */
static void keep_current(uint32_t ms)
{
    do {
        const uint32_t step = (ms > 200U) ? 200U : ms;

        status_sequence++;
        send_status(status_sequence, 1234, FURNACE_HMI_CONTROLLER_MEASURED_VALID, SESSION_A);
        advance(step);
        ms -= step;
    } while (ms > 0U);
}

/* The envelope of the newest command the client sent. */
static const struct furnace_wire_envelope *last_envelope(void)
{
    const struct furnace_wire_message *m = &fx.last_command;

    switch (fx.last_command_type) {
    case FURNACE_WIRE_MSG_START_PREPARE:
        return &m->u.start_prepare.envelope;
    case FURNACE_WIRE_MSG_MANUAL_START:
        return &m->u.manual_start.envelope;
    case FURNACE_WIRE_MSG_MANUAL_RETARGET:
        return &m->u.manual_retarget.envelope;
    case FURNACE_WIRE_MSG_MANUAL_SET_LENGTH:
        return &m->u.manual_set_length.envelope;
    case FURNACE_WIRE_MSG_RUN_PAUSE:
        return &m->u.run_pause.envelope;
    case FURNACE_WIRE_MSG_RESUME_REQUEST:
        return &m->u.resume_request.envelope;
    case FURNACE_WIRE_MSG_RUN_CONTINUE:
        return &m->u.run_continue.envelope;
    case FURNACE_WIRE_MSG_RUN_STOP:
        return &m->u.run_stop.envelope;
    case FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE:
        return &m->u.fault_acknowledge.envelope;
    case FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST:
        return &m->u.fault_recovery_request.envelope;
    default:
        return NULL;
    }
}

static uint32_t last_request_id(void)
{
    const struct furnace_wire_envelope *e = last_envelope();

    return (e != NULL) ? e->request_id : 0U;
}

static furnace_hmi_command_local_t submit(furnace_hmi_command_kind_t kind, uint32_t submission)
{
    const furnace_hmi_command_t command = {
        .kind = kind,
        .run_id = 77U,
        .target_decicelsius = 600,
        .rate_decicelsius_per_min = 20,
        .session_minutes = 60U,
        .fault_id = 3U,
        .source = 1U,
    };
    const furnace_hmi_command_local_t refusal =
        furnace_hmi_controller_client_submit(&fx.client, &command, submission);

    advance(10U);
    return refusal;
}

static void send_run_result(bool accepted, uint32_t request_id, uint8_t command, uint8_t reason,
                            uint32_t run_id)
{
    start_out(accepted ? FURNACE_WIRE_MSG_RUN_COMMAND_ACCEPTED
                       : FURNACE_WIRE_MSG_RUN_COMMAND_REJECTED);
    if (accepted) {
        fx.out.u.run_command_accepted.request_id = request_id;
        fx.out.u.run_command_accepted.command = command;
        fx.out.u.run_command_accepted.reason = reason;
        fx.out.u.run_command_accepted.run_id = run_id;
    } else {
        fx.out.u.run_command_rejected.request_id = request_id;
        fx.out.u.run_command_rejected.command = command;
        fx.out.u.run_command_rejected.reason = reason;
        fx.out.u.run_command_rejected.blocking_conditions = 0x24U;
        fx.out.u.run_command_rejected.run_id = run_id;
    }
    send_out();
    advance(10U);
}

static void send_rejected_command(uint32_t request_id, uint16_t msg_type, uint8_t reason)
{
    start_out(FURNACE_WIRE_MSG_COMMAND_REJECTED);
    fx.out.u.command_rejected.request_id = request_id;
    fx.out.u.command_rejected.msg_type = msg_type;
    fx.out.u.command_rejected.reason = reason;
    send_out();
    advance(10U);
}

static void command_not_sent_unless_current(void)
{
    size_t mark;

    begin("command not sent unless current");
    fx.commands_allowed = true;
    mark = fx.sent_count;
    EXPECT(submit(FURNACE_HMI_COMMAND_MANUAL_START, 5U) == FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT,
           "refused while connecting");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_MANUAL_START) == 0U, "nothing sent");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_IDLE, "the slot is unchanged");
    EXPECT(obs()->command_refusal.submission == 5U &&
               obs()->command_refusal.kind == FURNACE_HMI_COMMAND_MANUAL_START &&
               obs()->command_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT,
           "the refusal is reported");
    synchronize(SESSION_A, 10U);
    advance(3100U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent");
    EXPECT(submit(FURNACE_HMI_COMMAND_PAUSE, 6U) == FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT,
           "refused while silent");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RUN_PAUSE) == 0U, "nothing sent while silent");
    end();
}

static void manual_start_accepted(void)
{
    const struct furnace_wire_manual_start *m = &fx.last_command.u.manual_start;
    const furnace_hmi_command_outcome_t *c;
    uint32_t id;

    begin_commands("manual start accepted");
    EXPECT(submit(FURNACE_HMI_COMMAND_MANUAL_START, 7U) == FURNACE_HMI_COMMAND_LOCAL_NONE, "sent");
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_MANUAL_START, "MANUAL_START on the wire");
    EXPECT(m->target == 600 && m->rate == 20 && m->session_minutes == 60U, "its fields");
    EXPECT(m->envelope.session_id[0] == SESSION_A && m->envelope.session_id[15] == SESSION_A &&
               m->envelope.client_session_id == FIRST_CLIENT_ID,
           "the envelope names both sessions");
    id = m->envelope.request_id;
    c = &obs()->command;
    EXPECT(c->state == FURNACE_HMI_COMMAND_PENDING && c->submission == 7U &&
               c->kind == FURNACE_HMI_COMMAND_MANUAL_START && c->request_id == id &&
               c->attempts == 1U,
           "pending, never accepted on sending");
    send_run_result(true, id, RUN_COMMAND_MANUAL_START, 0U, 91U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_ACCEPTED && c->run_id == 91U, "accepted with its run");
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_IDLE, "the Stop slot is untouched");
    end();
}

static void run_command_rejected(void)
{
    const furnace_hmi_command_outcome_t *c = &obs()->command;

    begin_commands("run command rejected");
    (void)submit(FURNACE_HMI_COMMAND_PAUSE, 1U);
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_RUN_PAUSE &&
               fx.last_command.u.run_pause.run_id == 77U,
           "RUN_PAUSE names the run");
    send_run_result(false, last_request_id(), RUN_COMMAND_PAUSE, 9U, 77U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_REJECTED && c->reason == 9U &&
               c->blocking_conditions == 0x24U && c->run_id == 77U,
           "rejected with the controller's reason and conditions");
    end();
}

static void result_of_another_request_ignored(void)
{
    begin_commands("result of another request ignored");
    (void)submit(FURNACE_HMI_COMMAND_RESUME, 1U);
    send_run_result(true, last_request_id() + 5U, 2U, 0U, 77U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_PENDING, "still pending");
    end();
}

static void busy_slot_refused_without_losing_the_pending(void)
{
    uint32_t id;

    begin_commands("busy slot refused");
    (void)submit(FURNACE_HMI_COMMAND_MANUAL_START, 1U);
    id = last_request_id();
    EXPECT(submit(FURNACE_HMI_COMMAND_PAUSE, 2U) == FURNACE_HMI_COMMAND_LOCAL_BUSY,
           "a second ordinary command is refused");
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_MANUAL_START, "nothing else sent");
    EXPECT(obs()->command.submission == 1U && obs()->command.state == FURNACE_HMI_COMMAND_PENDING,
           "the pending command keeps its outcome");
    EXPECT(obs()->command_refusal.submission == 2U &&
               obs()->command_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_BUSY,
           "the refusal is reported apart");
    EXPECT(submit(FURNACE_HMI_COMMAND_STOP, 3U) == FURNACE_HMI_COMMAND_LOCAL_NONE,
           "Stop is never blocked by an ordinary command");
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_RUN_STOP, "RUN_STOP sent");
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_PENDING && obs()->stop.submission == 3U,
           "Stop pending in its slot");
    EXPECT(submit(FURNACE_HMI_COMMAND_STOP, 4U) == FURNACE_HMI_COMMAND_LOCAL_BUSY,
           "a second Stop while one is pending");
    send_run_result(true, id, RUN_COMMAND_MANUAL_START, 0U, 91U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_ACCEPTED, "the first still completes");
    end();
}

static void invalid_kind_refused(void)
{
    begin_commands("invalid kind refused");
    EXPECT(submit(FURNACE_HMI_COMMAND_NONE, 1U) == FURNACE_HMI_COMMAND_LOCAL_INVALID, "NONE");
    EXPECT(submit(FURNACE_HMI_COMMAND_COUNT, 2U) == FURNACE_HMI_COMMAND_LOCAL_INVALID, "COUNT");
    EXPECT(fx.last_command_type == 0U, "nothing sent");
    EXPECT(obs()->command_refusal.submission == 2U &&
               obs()->command_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_INVALID,
           "reported as invalid");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_IDLE &&
               obs()->stop.state == FURNACE_HMI_COMMAND_IDLE,
           "both slots unchanged");
    end();
}

static void retried_under_the_same_request_id(void)
{
    uint32_t id;
    size_t mark;

    begin_commands("retried under the same request id");
    mark = fx.sent_count;
    (void)submit(FURNACE_HMI_COMMAND_RESUME, 1U);
    id = last_request_id();
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS - 100U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RESUME_REQUEST) == 1U, "not again too early");
    keep_current(200U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RESUME_REQUEST) == 2U, "sent again");
    EXPECT(last_request_id() == id, "under the same request_id");
    EXPECT(obs()->command.attempts == 2U, "two attempts");
    send_run_result(true, id, 2U, 0U, 77U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_ACCEPTED, "the retry's answer");
    end();
}

static void no_answer_is_unresolved(void)
{
    size_t mark;

    begin_commands("no answer is unresolved");
    mark = fx.sent_count;
    (void)submit(FURNACE_HMI_COMMAND_MANUAL_RETARGET, 1U);
    EXPECT(fx.last_command.u.manual_retarget.run_id == 77U &&
               fx.last_command.u.manual_retarget.target == 600 &&
               fx.last_command.u.manual_retarget.rate == 20,
           "MANUAL_RETARGET fields");
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS * (FURNACE_HMI_CLIENT_COMMAND_ATTEMPTS + 1U));
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_MANUAL_RETARGET) == FURNACE_HMI_CLIENT_COMMAND_ATTEMPTS,
           "every attempt and no more");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_NO_ANSWER,
           "unresolved, never assumed");
    EXPECT(obs()->link == FURNACE_HMI_LINK_CONNECTED, "the link itself is fine");
    end();
}

static void duplicate_on_retry_is_unresolved(void)
{
    uint32_t id;

    begin_commands("duplicate on retry is unresolved");
    (void)submit(FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH, 1U);
    EXPECT(fx.last_command.u.manual_set_length.run_id == 77U &&
               fx.last_command.u.manual_set_length.session_minutes == 60U,
           "MANUAL_SET_LENGTH fields");
    id = last_request_id();
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS + 100U);
    send_rejected_command(id, FURNACE_WIRE_MSG_MANUAL_SET_LENGTH, REJECT_DUPLICATE_REQUEST);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_DUPLICATE,
           "the outcome is unresolved");
    end();
}

static void service_refusal(void)
{
    begin_commands("service refusal");
    (void)submit(FURNACE_HMI_COMMAND_CONTINUE, 1U);
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_RUN_CONTINUE &&
               fx.last_command.u.run_continue.run_id == 77U,
           "RUN_CONTINUE names the run");
    send_rejected_command(last_request_id(), FURNACE_WIRE_MSG_RUN_CONTINUE, REJECT_QUEUE_FULL);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_REFUSED &&
               obs()->command.reason == REJECT_QUEUE_FULL,
           "refused by the service, with its reason");
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS * 2U);
    EXPECT(sent_since(0U, FURNACE_WIRE_MSG_RUN_CONTINUE) == 1U, "never sent again by itself");
    end();
}

static void link_lost_ends_an_ordinary_command(void)
{
    size_t mark;

    begin_commands("link lost ends an ordinary command");
    advance(2000U); /* no STATUS from here: silence comes long before the attempts run out */
    (void)submit(FURNACE_HMI_COMMAND_MANUAL_START, 1U);
    advance(1100U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
           "unresolved: the operator's intent ends with the link");
    mark = fx.sent_count;
    status_sequence = 40U;
    synchronize(SESSION_A, status_sequence);
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS * 2U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_MANUAL_START) == 0U,
           "a Start is never sent again after a reconnect");
    end();
}

static void stop_survives_a_reconnect(void)
{
    uint32_t id;
    size_t mark;

    begin_commands("stop survives a reconnect");
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    EXPECT(fx.last_command.u.run_stop.run_id == 77U, "RUN_STOP names the run");
    id = last_request_id();
    advance(3100U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent");
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_PENDING, "the Stop stays pending");
    mark = fx.sent_count;
    advance(2000U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RUN_STOP) == 0U, "not sent while silent");
    status_sequence = 40U;
    synchronize(SESSION_A, status_sequence);
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS + 100U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RUN_STOP) >= 1U, "sent again after the reconnect");
    EXPECT(last_request_id() == id, "under the same request_id");
    send_run_result(true, id, RUN_COMMAND_STOP, 0U, 77U);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_ACCEPTED, "its result");
    end();
}

static void session_change_unresolves_both(void)
{
    begin_commands("session change unresolves both");
    (void)submit(FURNACE_HMI_COMMAND_PAUSE, 1U);
    (void)submit(FURNACE_HMI_COMMAND_STOP, 2U);
    synchronize(SESSION_B, 3U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED,
           "ordinary unresolved");
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->stop.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_SESSION_CHANGED,
           "Stop unresolved: the restart ended the run");
    end();
}

static void stop_unresolved_by_the_controller(void)
{
    begin_commands("stop unresolved by the controller");
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    send_rejected_command(last_request_id(), FURNACE_WIRE_MSG_RUN_STOP, REJECT_STOP_UNRESOLVED);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->stop.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_STOP,
           "STOP_UNRESOLVED is unresolved, not refused");
    end();
}

static void stop_gives_up_after_its_attempts(void)
{
    size_t mark;

    begin_commands("stop gives up after its attempts");
    mark = fx.sent_count;
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS * (FURNACE_HMI_CLIENT_STOP_ATTEMPTS + 1U));
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RUN_STOP) == FURNACE_HMI_CLIENT_STOP_ATTEMPTS,
           "every Stop attempt");
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->stop.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_NO_ANSWER,
           "then unresolved");
    end();
}

static void fault_acknowledgement(void)
{
    const furnace_hmi_command_outcome_t *c = &obs()->command;

    begin_commands("fault acknowledgement");
    (void)submit(FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE, 1U);
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE &&
               fx.last_command.u.fault_acknowledge.fault_id == 3U &&
               fx.last_command.u.fault_acknowledge.source == 1U,
           "FAULT_ACKNOWLEDGE names the entry");
    start_out(FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT);
    fx.out.u.fault_acknowledge_result.request_id = last_request_id();
    fx.out.u.fault_acknowledge_result.fault_id = 3U;
    fx.out.u.fault_acknowledge_result.source = 1U;
    fx.out.u.fault_acknowledge_result.result = 1U; /* NOT_FOUND */
    send_out();
    advance(10U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_REJECTED && c->reason == 1U, "NOT_FOUND rejected");

    (void)submit(FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE, 2U);
    start_out(FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT);
    fx.out.u.fault_acknowledge_result.request_id = last_request_id();
    fx.out.u.fault_acknowledge_result.fault_id = 3U;
    fx.out.u.fault_acknowledge_result.source = 1U;
    fx.out.u.fault_acknowledge_result.result = 0U; /* DONE */
    send_out();
    advance(10U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_ACCEPTED && c->submission == 2U, "DONE accepted");
    end();
}

static void fault_recovery(void)
{
    const furnace_hmi_command_outcome_t *c = &obs()->command;

    begin_commands("fault recovery");
    (void)submit(FURNACE_HMI_COMMAND_FAULT_RECOVERY, 1U);
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_FAULT_RECOVERY_REQUEST &&
               fx.last_command.u.fault_recovery_request.fault_id == 3U,
           "FAULT_RECOVERY_REQUEST names the entry");
    start_out(FURNACE_WIRE_MSG_FAULT_RECOVERY_RESULT);
    fx.out.u.fault_recovery_result.request_id = last_request_id();
    fx.out.u.fault_recovery_result.fault_id = 3U;
    fx.out.u.fault_recovery_result.source = 1U;
    fx.out.u.fault_recovery_result.found = true;
    fx.out.u.fault_recovery_result.reply = RECOVERY_CONDITION_ACTIVE;
    send_out();
    advance(10U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_REJECTED && c->fault_found &&
               c->reason == RECOVERY_CONDITION_ACTIVE,
           "a refused recovery keeps its reply");

    (void)submit(FURNACE_HMI_COMMAND_FAULT_RECOVERY, 2U);
    start_out(FURNACE_WIRE_MSG_FAULT_RECOVERY_RESULT);
    fx.out.u.fault_recovery_result.request_id = last_request_id();
    fx.out.u.fault_recovery_result.fault_id = 3U;
    fx.out.u.fault_recovery_result.source = 1U;
    fx.out.u.fault_recovery_result.found = true;
    fx.out.u.fault_recovery_result.reply = RECOVERY_RECOVERED;
    send_out();
    advance(10U);
    EXPECT(c->state == FURNACE_HMI_COMMAND_ACCEPTED && c->reason == RECOVERY_RECOVERED,
           "RECOVERED accepted");
    end();
}

static void start_prepare(void)
{
    begin_commands("start prepare");
    (void)submit(FURNACE_HMI_COMMAND_START_PREPARE, 1U);
    EXPECT(fx.last_command_type == FURNACE_WIRE_MSG_START_PREPARE, "START_PREPARE sent");
    start_out(FURNACE_WIRE_MSG_START_PREPARE_RESULT);
    fx.out.u.start_prepare_result.request_id = last_request_id();
    fx.out.u.start_prepare_result.store_readiness = 3U;
    send_out();
    advance(10U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_ACCEPTED &&
               obs()->command.store_readiness == 3U,
           "accepted with the readiness");
    end();
}

/* Review F1: a reconnect the controller saw ends an ordinary command, even before HMI silence. */
static void hello_ack_ends_an_ordinary_command(void)
{
    size_t mark;

    begin_commands("hello ack ends an ordinary command");
    (void)submit(FURNACE_HMI_COMMAND_MANUAL_START, 1U);
    send_hello_ack(HELLO_OK, SESSION_A); /* same session: the controller had dropped the client */
    advance(10U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
           "unresolved at once");
    mark = fx.sent_count;
    status_sequence = 40U;
    synchronize(SESSION_A, status_sequence);
    keep_current(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS * 3U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_MANUAL_START) == 0U, "the Start is never sent again");
    end();
}

/*
 * Review F3: a Stop the service refused as not connected is kept and sent again
 * after HELLO; review F1: the HELLO ends a pending ordinary command.
 */
static void stop_kept_through_not_connected(void)
{
    uint32_t id;
    size_t mark;

    begin_commands("stop kept through not connected");
    (void)submit(FURNACE_HMI_COMMAND_RESUME, 2U);
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    id = last_request_id();
    mark = fx.sent_count;
    send_rejected_command(id, FURNACE_WIRE_MSG_RUN_STOP, REJECT_NOT_CONNECTED);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_PENDING, "the Stop stays pending");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->command.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
           "the Resume ends with the reconnect");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_HELLO) >= 1U, "HELLO sent");
    send_hello_ack(HELLO_OK, SESSION_A);
    advance(FURNACE_HMI_CLIENT_COMMAND_RETRY_MS + 100U);
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_RUN_STOP) >= 1U,
           "sent again while synchronizing (review F4)");
    EXPECT(last_request_id() == id, "under the same request_id");
    send_run_result(true, id, RUN_COMMAND_STOP, 0U, 77U);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_ACCEPTED, "its result");
    end();
}

/* Review F4: a Stop that cannot be sent again is unresolved after its patience. */
static void stop_patience_while_silent(void)
{
    begin_commands("stop patience while silent");
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    advance(3100U);
    EXPECT(obs()->link == FURNACE_HMI_LINK_SILENT, "silent");
    advance(FURNACE_HMI_CLIENT_STOP_PATIENCE_MS - 2000U);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_PENDING, "pending within its patience");
    advance(2100U);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->stop.unresolved_cause == FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
           "unresolved after it");
    end();
}

/* Review F5: each slot has its own refusal. */
static void refusals_per_slot(void)
{
    begin("refusals per slot");
    fx.commands_allowed = true;
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    (void)submit(FURNACE_HMI_COMMAND_PAUSE, 2U);
    EXPECT(obs()->stop_refusal.submission == 1U &&
               obs()->stop_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT,
           "the Stop's refusal is kept");
    EXPECT(obs()->command_refusal.submission == 2U, "the ordinary refusal apart");
    end();
}

/* Review F2: a dropped command is reported, and the slot keeps its outcome. */
static void refused_by_the_submitter(void)
{
    const furnace_hmi_command_t start = {.kind = FURNACE_HMI_COMMAND_MANUAL_START};
    size_t mark;

    begin_commands("refused by the submitter");
    mark = fx.sent_count;
    furnace_hmi_controller_client_refuse(&fx.client, &start, 4U,
                                         FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP);
    advance(10U);
    EXPECT(obs()->command_refusal.submission == 4U &&
               obs()->command_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP,
           "reported as superseded");
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_IDLE, "the slot is unchanged");
    EXPECT(sent_since(mark, FURNACE_WIRE_MSG_MANUAL_START) == 0U, "nothing sent");
    end();
}

/* Review F6: a new client session is its own cause. */
static void client_session_change_cause(void)
{
    begin_commands("client session change cause");
    (void)submit(FURNACE_HMI_COMMAND_STOP, 1U);
    send_hello_ack(2U /* INVALID_CLIENT_SESSION */, SESSION_A);
    advance(10U);
    EXPECT(obs()->stop.state == FURNACE_HMI_COMMAND_UNRESOLVED &&
               obs()->stop.unresolved_cause ==
                   FURNACE_HMI_COMMAND_UNRESOLVED_CLIENT_SESSION_CHANGED,
           "client session changed, not a controller restart");
    end();
}

/* Review F7: a result for another kind of command does not complete this one. */
static void result_of_another_kind_ignored(void)
{
    begin_commands("result of another kind ignored");
    (void)submit(FURNACE_HMI_COMMAND_PAUSE, 1U);
    send_run_result(true, last_request_id(), RUN_COMMAND_STOP, 0U, 77U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_PENDING, "a STOP result is not a Pause's");
    start_out(FURNACE_WIRE_MSG_FAULT_ACKNOWLEDGE_RESULT);
    fx.out.u.fault_acknowledge_result.request_id = last_request_id();
    send_out();
    advance(10U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_PENDING, "nor an acknowledgement's");
    send_run_result(true, last_request_id(), RUN_COMMAND_PAUSE, 0U, 77U);
    EXPECT(obs()->command.state == FURNACE_HMI_COMMAND_ACCEPTED, "its own result");
    end();
}

int controller_client_tests_run(void)
{
    failures = 0;
    hello_ack_ends_an_ordinary_command();
    stop_kept_through_not_connected();
    stop_patience_while_silent();
    refusals_per_slot();
    refused_by_the_submitter();
    client_session_change_cause();
    result_of_another_kind_ignored();
    command_not_sent_unless_current();
    manual_start_accepted();
    run_command_rejected();
    result_of_another_request_ignored();
    busy_slot_refused_without_losing_the_pending();
    invalid_kind_refused();
    retried_under_the_same_request_id();
    no_answer_is_unresolved();
    duplicate_on_retry_is_unresolved();
    service_refusal();
    link_lost_ends_an_ordinary_command();
    stop_survives_a_reconnect();
    session_change_unresolves_both();
    stop_unresolved_by_the_controller();
    stop_gives_up_after_its_attempts();
    fault_acknowledgement();
    fault_recovery();
    start_prepare();
    periodic_status_within_snapshot();
    lost_end_while_connected();
    lost_fault_page();
    missing_only_page();
    superseded_snapshot();
    events_during_first_snapshot();
    request_id_exhausted();
    absurd_intervals();
    rejections_send_hello();
    lost_hello_ack();
    duplicate_on_capabilities_request();
    older_periodic_status_recovers();
    hello_at_start();
    hello_retry();
    handshake_to_current();
    incompatible_version();
    silence_to_stale();
    reconnect_same_session();
    controller_restart();
    restart_with_repeated_session_id();
    restart_unresolves_request();
    older_status_ignored();
    status_of_other_session();
    event_gap_requests_snapshot();
    fault_event_refreshes_list();
    corrupt_and_garbage();
    heartbeat_when_idle();
    duplicate_on_first_request();
    rejected_request();
    sync_timeout_requests();
    fault_list_paged();
    client_messages_ignored();
    bounded_poll();
    stale_temperature_from_controller();
    clock_wrap();
    return failures;
}
