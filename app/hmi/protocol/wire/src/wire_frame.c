/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Frame layer of the controller client protocol (module core/wire, Phase 20b).
 * Contract: wire.h and docs/interfaces/controller-wire-format.md §2, §3.
 */

#include <string.h>

#include <furnace/crc32.h>
#include <furnace/wire.h>

#define COBS_FULL_BLOCK_CODE 0xFFU
#define MSG_HELLO            0x0001U
#define MSG_HELLO_ACK        0x8001U

enum furnace_wire_result furnace_wire_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst,
						  size_t cap, size_t *out_len)
{
	size_t code_at = 0U;
	size_t out = 1U;
	uint8_t code = 1U;

	if ((dst == NULL) || (out_len == NULL) || ((src == NULL) && (len != 0U))) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	if (cap == 0U) {
		return FURNACE_WIRE_ERR_BUFFER;
	}
	for (size_t i = 0U; i < len; i++) {
		if (out >= cap) {
			return FURNACE_WIRE_ERR_BUFFER;
		}
		if (src[i] != 0U) {
			dst[out] = src[i];
			out++;
			code++;
			if ((code != COBS_FULL_BLOCK_CODE) || ((i + 1U) == len)) {
				continue;
			}
			if (out >= cap) {
				return FURNACE_WIRE_ERR_BUFFER;
			}
		}
		dst[code_at] = code;
		code_at = out;
		out++;
		code = 1U;
	}
	dst[code_at] = code;
	*out_len = out;
	return FURNACE_WIRE_OK;
}

enum furnace_wire_result furnace_wire_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst,
						  size_t cap, size_t *out_len)
{
	size_t in = 0U;
	size_t out = 0U;

	if ((dst == NULL) || (out_len == NULL) || ((src == NULL) && (len != 0U))) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	while (in < len) {
		const uint8_t code = src[in];
		const size_t run = (size_t)code - 1U;

		in++;
		if ((code == 0U) || (run > (len - in))) {
			return FURNACE_WIRE_ERR_FRAMING;
		}
		if (run > (cap - out)) {
			return FURNACE_WIRE_ERR_BUFFER;
		}
		for (size_t i = 0U; i < run; i++) {
			if (src[in + i] == 0U) {
				return FURNACE_WIRE_ERR_FRAMING;
			}
			dst[out + i] = src[in + i];
		}
		in += run;
		out += run;
		if ((code != COBS_FULL_BLOCK_CODE) && (in < len)) {
			if (out >= cap) {
				return FURNACE_WIRE_ERR_BUFFER;
			}
			dst[out] = 0U;
			out++;
		}
	}
	*out_len = out;
	return FURNACE_WIRE_OK;
}

static void put_le16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
}

static void put_le32(uint8_t *p, uint32_t v)
{
	for (uint32_t i = 0U; i < 4U; i++) {
		p[i] = (uint8_t)(v >> (8U * i));
	}
}

static uint16_t get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

static uint32_t get_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

enum furnace_wire_result furnace_wire_tx_finish(struct furnace_wire_tx *tx, uint16_t msg_type,
						uint16_t payload_len)
{
	size_t coded = 0U;
	enum furnace_wire_result result;

	if (tx == NULL) {
		return FURNACE_WIRE_ERR_ARGUMENT;
	}
	if (payload_len > FURNACE_WIRE_MAX_PAYLOAD) {
		return FURNACE_WIRE_ERR_BUFFER;
	}
	tx->frame[0] = FURNACE_WIRE_PROTO_MAJOR;
	tx->frame[1] = FURNACE_WIRE_PROTO_MINOR;
	put_le16(&tx->frame[2], msg_type);
	put_le16(&tx->frame[4], payload_len);
	tx->frame_len = (size_t)FURNACE_WIRE_HEADER_SIZE + payload_len;
	put_le32(&tx->frame[tx->frame_len], furnace_crc32(0U, tx->frame, tx->frame_len));
	tx->frame_len += FURNACE_WIRE_CRC_SIZE;

	result = furnace_wire_cobs_encode(tx->frame, tx->frame_len, tx->encoded,
					  FURNACE_WIRE_MAX_ENCODED, &coded);
	if (result != FURNACE_WIRE_OK) {
		return result;
	}
	tx->encoded[coded] = 0U;
	tx->encoded_len = coded + 1U;
	return FURNACE_WIRE_OK;
}

void furnace_wire_rx_init(struct furnace_wire_rx *rx)
{
	if (rx != NULL) {
		memset(rx, 0, sizeof(*rx));
	}
}

static void count(uint32_t *counter)
{
	if (*counter != UINT32_MAX) {
		(*counter)++;
	}
}

/* Steps 2-5 of wire format §3, for the bytes collected before a delimiter. */
static enum furnace_wire_rx_status rx_complete(struct furnace_wire_rx *rx,
					       struct furnace_wire_frame *frame)
{
	size_t len = 0U;
	uint16_t payload_len;
	uint16_t msg_type;

	if (furnace_wire_cobs_decode(rx->encoded, rx->len, rx->decoded, sizeof(rx->decoded),
				     &len) != FURNACE_WIRE_OK) {
		count(&rx->counters.framing);
		return FURNACE_WIRE_RX_DROPPED;
	}
	if (len < FURNACE_WIRE_MIN_FRAME) {
		count(&rx->counters.length);
		return FURNACE_WIRE_RX_DROPPED;
	}
	payload_len = get_le16(&rx->decoded[4]);
	if ((payload_len > FURNACE_WIRE_MAX_PAYLOAD) ||
	    (((size_t)payload_len + FURNACE_WIRE_MIN_FRAME) != len)) {
		count(&rx->counters.length);
		return FURNACE_WIRE_RX_DROPPED;
	}
	if (furnace_crc32(0U, rx->decoded, len - FURNACE_WIRE_CRC_SIZE) !=
	    get_le32(&rx->decoded[len - FURNACE_WIRE_CRC_SIZE])) {
		count(&rx->counters.crc);
		return FURNACE_WIRE_RX_DROPPED;
	}
	msg_type = get_le16(&rx->decoded[2]);
	frame->proto_major = rx->decoded[0];
	frame->proto_minor = rx->decoded[1];
	frame->msg_type = msg_type;
	frame->payload = &rx->decoded[FURNACE_WIRE_HEADER_SIZE];
	frame->payload_len = payload_len;
	if ((frame->proto_major != FURNACE_WIRE_PROTO_MAJOR) ||
	    (frame->proto_minor != FURNACE_WIRE_PROTO_MINOR)) {
		if ((msg_type == MSG_HELLO) || (msg_type == MSG_HELLO_ACK)) {
			return FURNACE_WIRE_RX_HELLO_VERSION;
		}
		count(&rx->counters.version);
		return FURNACE_WIRE_RX_DROPPED;
	}
	return FURNACE_WIRE_RX_FRAME;
}

enum furnace_wire_rx_status furnace_wire_rx_feed(struct furnace_wire_rx *rx, uint8_t byte,
						 struct furnace_wire_frame *frame)
{
	enum furnace_wire_rx_status status;

	if ((rx == NULL) || (frame == NULL)) {
		return FURNACE_WIRE_RX_NONE;
	}
	if (byte != 0U) {
		if (rx->discarding) {
			return FURNACE_WIRE_RX_NONE;
		}
		if (rx->len >= FURNACE_WIRE_MAX_ENCODED) {
			rx->discarding = true;
			count(&rx->counters.overlong);
			return FURNACE_WIRE_RX_NONE;
		}
		rx->encoded[rx->len] = byte;
		rx->len++;
		return FURNACE_WIRE_RX_NONE;
	}
	if (rx->discarding) {
		status = FURNACE_WIRE_RX_DROPPED;
	} else if (rx->len == 0U) {
		status = FURNACE_WIRE_RX_NONE; /* an empty frame, wire format §2 */
	} else {
		status = rx_complete(rx, frame);
	}
	rx->len = 0U;
	rx->discarding = false;
	return status;
}
