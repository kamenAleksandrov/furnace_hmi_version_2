/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Conformance of the HMI's copy of the controller's wire codec (ADR-0007,
 * ADR-0009) against the controller's golden vectors, copied with it. One
 * portable source: a CTest program on the host (MSVC and GCC) and a Ztest suite
 * on Zephyr. It checks what the HMI depends on; the controller's own suite
 * covers the codec in depth.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <furnace/crc32.h>
#include <furnace/wire.h>
#include <furnace/wire_messages.h>

#include "vector_table.h"
#include "wire_conformance.h"

static struct furnace_wire_message decoded;
static struct furnace_wire_tx tx;
static struct furnace_wire_rx rx;
static struct furnace_wire_frame frame;
static uint8_t payload[FURNACE_WIRE_MAX_PAYLOAD];
static uint8_t cobs[FURNACE_WIRE_MAX_ON_WIRE];

static int failures;

static void fail(const char *check, const char *name)
{
    failures++;
    wire_conformance_report(check, name);
}

#define EXPECT(cond, check, name)                                                                  \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fail((check), (name));                                                                 \
        }                                                                                          \
    } while (0)

/* The limits of the wire document (§2), checked when compiling. */
_Static_assert(FURNACE_WIRE_MAX_FRAME == 1034U, "largest frame");
_Static_assert(FURNACE_WIRE_MAX_ON_WIRE == 1040U, "largest frame on the wire");

static void crc_and_limits(void)
{
    EXPECT(furnace_crc32(0U, "123456789", 9U) == 0xCBF43926U, "crc check value", "crc");
    EXPECT(wire_positive_count >= 52U * 3U, "every message has min, typical and max", "limits");
}

/* Every message's exact payload, frame and COBS bytes; then decoded back to the same bytes. */
static void positive_vectors(void)
{
    for (size_t i = 0U; i < wire_positive_count; i++) {
        const struct wire_positive_vector *v = &wire_positive_vectors[i];
        size_t len = 0U;

        EXPECT(v->proto_major == FURNACE_WIRE_PROTO_MAJOR &&
                   v->proto_minor == FURNACE_WIRE_PROTO_MINOR,
               "vector for this version", v->name);
        EXPECT(furnace_wire_payload_encode(v->message, payload, sizeof(payload), &len) ==
                       FURNACE_WIRE_OK &&
                   len == v->payload_len && memcmp(payload, v->payload, len) == 0,
               "payload bytes", v->name);
        EXPECT(furnace_wire_tx_encode(&tx, v->message) == FURNACE_WIRE_OK &&
                   tx.frame_len == v->frame_len &&
                   memcmp(tx.frame, v->frame, v->frame_len) == 0 &&
                   tx.encoded_len == v->cobs_len + 1U &&
                   memcmp(tx.encoded, v->cobs, v->cobs_len) == 0 &&
                   tx.encoded[v->cobs_len] == 0U,
               "frame and COBS bytes", v->name);
        EXPECT(furnace_wire_payload_decode(v->message->type, v->payload, v->payload_len,
                                           &decoded) == FURNACE_WIRE_OK &&
                   furnace_wire_payload_encode(&decoded, payload, sizeof(payload), &len) ==
                       FURNACE_WIRE_OK &&
                   len == v->payload_len && memcmp(payload, v->payload, len) == 0,
               "decode to the same values", v->name);
    }
}

/* The receiver delivers each frame at its delimiter, and not before. */
static void receiver(void)
{
    for (size_t i = 0U; i < wire_positive_count; i++) {
        const struct wire_positive_vector *v = &wire_positive_vectors[i];
        enum furnace_wire_rx_status last = FURNACE_WIRE_RX_NONE;

        furnace_wire_rx_init(&rx);
        for (size_t k = 0U; k < v->cobs_len; k++) {
            const enum furnace_wire_rx_status s = furnace_wire_rx_feed(&rx, v->cobs[k], &frame);

            last = (s != FURNACE_WIRE_RX_NONE) ? s : last;
        }
        EXPECT(last == FURNACE_WIRE_RX_NONE, "no frame before the delimiter", v->name);
        last = furnace_wire_rx_feed(&rx, 0U, &frame);
        EXPECT(last == FURNACE_WIRE_RX_FRAME && frame.msg_type == v->message->type &&
                   frame.payload_len == v->payload_len &&
                   memcmp(frame.payload, v->payload, v->payload_len) == 0,
               "frame delivered", v->name);
    }
}

/* The examples written by hand from the wire document, in both directions. */
static void hand_vectors(void)
{
    EXPECT(wire_hand_count >= 9U, "hand-checked examples present", "hand");
    for (size_t i = 0U; i < wire_hand_count; i++) {
        const struct wire_hand_vector *v = &wire_hand_vectors[i];
        size_t len = 0U;

        EXPECT(furnace_wire_payload_encode(v->message, payload, sizeof(payload), &len) ==
                       FURNACE_WIRE_OK &&
                   len == v->payload_len && memcmp(payload, v->payload, len) == 0,
               "hand bytes", v->name);
        EXPECT(furnace_wire_payload_decode(v->message->type, v->payload, v->payload_len,
                                           &decoded) == FURNACE_WIRE_OK,
               "hand decode", v->name);
    }
}

/* Malformed input gives the documented result; accepted variants re-encode as documented. */
static void negative_vectors(void)
{
    for (size_t i = 0U; i < wire_negative_count; i++) {
        const struct wire_negative_vector *v = &wire_negative_vectors[i];
        size_t len = 0U;
        const enum furnace_wire_result r =
            furnace_wire_payload_decode(v->type, v->payload, v->payload_len, &decoded);

        EXPECT(r == v->expect, "malformed result", v->name);
        if ((r == FURNACE_WIRE_OK) && (v->expect == FURNACE_WIRE_OK)) {
            EXPECT(furnace_wire_payload_encode(&decoded, payload, sizeof(payload), &len) ==
                           FURNACE_WIRE_OK &&
                       len == v->payload_len - v->trim &&
                       memcmp(payload, v->payload, len) == 0,
                   "accepted variant re-encodes", v->name);
        }
    }
}

static void cobs_vectors(void)
{
    for (size_t i = 0U; i < wire_cobs_count; i++) {
        const struct wire_cobs_vector *v = &wire_cobs_vectors[i];
        size_t len = 0U;

        EXPECT(furnace_wire_cobs_encode(v->raw, v->raw_len, cobs, sizeof(cobs), &len) ==
                       FURNACE_WIRE_OK &&
                   len == v->cobs_len && memcmp(cobs, v->cobs, len) == 0,
               "COBS encode", v->name);
        EXPECT(furnace_wire_cobs_decode(v->cobs, v->cobs_len, payload, sizeof(payload), &len) ==
                       FURNACE_WIRE_OK &&
                   len == v->raw_len && memcmp(payload, v->raw, len) == 0,
               "COBS decode", v->name);
    }
}

/* Streams with noise, corruption and other versions: results and counters as documented. */
static void stream_vectors(void)
{
    for (size_t i = 0U; i < wire_stream_count; i++) {
        const struct wire_stream_vector *v = &wire_stream_vectors[i];
        size_t found = 0U;
        int order_ok = 1;

        furnace_wire_rx_init(&rx);
        for (size_t k = 0U; k < v->len; k++) {
            const enum furnace_wire_rx_status s = furnace_wire_rx_feed(&rx, v->bytes[k], &frame);

            if (s == FURNACE_WIRE_RX_NONE) {
                continue;
            }
            if ((found >= v->result_count) || (s != v->results[found])) {
                order_ok = 0;
            }
            found++;
        }
        EXPECT(order_ok && found == v->result_count, "stream results", v->name);
        EXPECT(rx.counters.overlong == v->counters.overlong &&
                   rx.counters.framing == v->counters.framing &&
                   rx.counters.length == v->counters.length &&
                   rx.counters.crc == v->counters.crc &&
                   rx.counters.version == v->counters.version,
               "stream counters", v->name);
    }
}

int wire_conformance_run(void)
{
    failures = 0;
    crc_and_limits();
    positive_vectors();
    receiver();
    hand_vectors();
    negative_vectors();
    cobs_vectors();
    stream_vectors();
    return failures;
}
