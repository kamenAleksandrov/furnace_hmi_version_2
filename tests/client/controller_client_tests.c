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
            switch (frame.msg_type) {
            case FURNACE_WIRE_MSG_HELLO:
            case FURNACE_WIRE_MSG_HEARTBEAT:
            case FURNACE_WIRE_MSG_SNAPSHOT_REQUEST:
            case FURNACE_WIRE_MSG_CAPABILITIES_REQUEST:
                break;
            default:
                fx.forbidden_sent = true;
                break;
            }
            if (fx.sent_count < SENT_CAPACITY) {
                fx.sent[fx.sent_count++] = frame.msg_type;
            }
            if (furnace_wire_payload_decode(frame.msg_type, frame.payload, frame.payload_len,
                                            &fx.in) != FURNACE_WIRE_OK) {
                fx.forbidden_sent = true;
                continue;
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

int controller_client_tests_run(void)
{
    failures = 0;
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
