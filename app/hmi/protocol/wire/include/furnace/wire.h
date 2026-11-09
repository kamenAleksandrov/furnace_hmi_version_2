/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Frame layer of the controller client protocol (module core/wire, Phase 20b).
 *
 * Pure ISO C17: no Zephyr kernel services or drivers, no heap, no global state.
 * A frame is a six-byte header, a payload and a CRC-32, COBS-encoded and ended
 * by one zero byte (docs/interfaces/controller-wire-format.md §2). The receiver
 * applies steps 1-5 of §3: it collects bytes up to a delimiter, decodes them and
 * checks the length, the CRC and the version, and it counts what it drops. It
 * knows nothing about message meaning, and a passing CRC carries no authority
 * (ADR-0018). Requirements: COMM-04, API-02.
 */

#ifndef FURNACE_WIRE_H_
#define FURNACE_WIRE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FURNACE_WIRE_PROTO_MAJOR 0U
#define FURNACE_WIRE_PROTO_MINOR 16U

/* The controller session identity (§4.6). */
#define FURNACE_WIRE_SESSION_ID_SIZE 16U

/* A fixture until the link is characterized (HW-BLOCK-006, ADR-0018). */
#define FURNACE_WIRE_MAX_PAYLOAD 1024U
#define FURNACE_WIRE_HEADER_SIZE 6U
#define FURNACE_WIRE_CRC_SIZE    4U
#define FURNACE_WIRE_MIN_FRAME   (FURNACE_WIRE_HEADER_SIZE + FURNACE_WIRE_CRC_SIZE)
#define FURNACE_WIRE_MAX_FRAME   (FURNACE_WIRE_MIN_FRAME + FURNACE_WIRE_MAX_PAYLOAD)
/* COBS adds one code byte per started block of 254 bytes: 5 for the largest frame. */
#define FURNACE_WIRE_MAX_ENCODED 1039U
/* The largest frame on the wire, delimiter included. */
#define FURNACE_WIRE_MAX_ON_WIRE (FURNACE_WIRE_MAX_ENCODED + 1U)

#define FURNACE_WIRE_CLIENT_MESSAGE_LIMIT 0x8000U

enum furnace_wire_result {
	FURNACE_WIRE_OK = 0,
	FURNACE_WIRE_ERR_ARGUMENT = 1,     /* a NULL argument: a caller error */
	FURNACE_WIRE_ERR_BUFFER = 2,       /* the output does not fit */
	FURNACE_WIRE_ERR_FRAMING = 3,      /* invalid COBS */
	FURNACE_WIRE_ERR_UNKNOWN_TYPE = 4, /* no layout for this message type */
	FURNACE_WIRE_ERR_MALFORMED = 5,    /* the payload does not fit the layout */
};

/* What the receiver found at a delimiter. */
enum furnace_wire_rx_status {
	FURNACE_WIRE_RX_NONE = 0,    /* no complete frame yet */
	FURNACE_WIRE_RX_FRAME = 1,   /* a valid frame of this protocol version */
	FURNACE_WIRE_RX_DROPPED = 2, /* an invalid frame: counted, never answered */
	/*
	 * A valid HELLO or HELLO_ACK of another version. HELLO keeps its layout in every
	 * version, and result stays the first byte of HELLO_ACK: the controller answers
	 * VERSION_UNSUPPORTED, and a refused client learns the controller's version from the
	 * header. Nothing else of it is read (wire format §3).
	 */
	FURNACE_WIRE_RX_HELLO_VERSION = 3,
};

/* Dropped frames by kind (wire format §3); saturating. Link status of WI-I08. */
struct furnace_wire_counters {
	uint32_t overlong;
	uint32_t framing;
	uint32_t length;
	uint32_t crc;
	uint32_t version;
};

/* A received frame; payload points into the receiver until its next byte. */
struct furnace_wire_frame {
	uint8_t proto_major;
	uint8_t proto_minor;
	uint16_t msg_type;
	const uint8_t *payload;
	uint16_t payload_len;
};

/* One link's receiver; a caller-owned, statically allocated object. */
struct furnace_wire_rx {
	uint8_t encoded[FURNACE_WIRE_MAX_ENCODED];
	uint8_t decoded[FURNACE_WIRE_MAX_ENCODED];
	uint16_t len;
	bool discarding; /* overlong: everything up to the next delimiter is dropped */
	struct furnace_wire_counters counters;
};

/* One link's transmit buffers; a caller-owned, statically allocated object. */
struct furnace_wire_tx {
	uint8_t frame[FURNACE_WIRE_MAX_FRAME];
	uint8_t encoded[FURNACE_WIRE_MAX_ON_WIRE];
	size_t frame_len;
	size_t encoded_len;
};

/** @brief True for a message sent by a client (bit 15 clear, wire format §6). */
static inline bool furnace_wire_is_client_message(uint16_t msg_type)
{
	return msg_type < FURNACE_WIRE_CLIENT_MESSAGE_LIMIT;
}

/**
 * @brief COBS-encode bytes, without the delimiter.
 *
 * A full last block of 254 non-zero bytes adds no further code byte.
 *
 * @retval FURNACE_WIRE_OK Written; dst holds no zero byte.
 * @retval FURNACE_WIRE_ERR_ARGUMENT A NULL argument.
 * @retval FURNACE_WIRE_ERR_BUFFER The output does not fit.
 */
enum furnace_wire_result furnace_wire_cobs_encode(const uint8_t *src, size_t len, uint8_t *dst,
						  size_t cap, size_t *out_len);

/**
 * @brief Decode COBS, without the delimiter.
 *
 * @retval FURNACE_WIRE_OK Decoded.
 * @retval FURNACE_WIRE_ERR_ARGUMENT A NULL argument.
 * @retval FURNACE_WIRE_ERR_BUFFER The output does not fit.
 * @retval FURNACE_WIRE_ERR_FRAMING A zero byte, or a code that runs past the end.
 */
enum furnace_wire_result furnace_wire_cobs_decode(const uint8_t *src, size_t len, uint8_t *dst,
						  size_t cap, size_t *out_len);

/**
 * @brief Complete a frame whose payload is already at frame[FURNACE_WIRE_HEADER_SIZE].
 *
 * Writes the header and the CRC, then the encoded frame with its delimiter.
 *
 * @retval FURNACE_WIRE_OK tx->encoded is ready for the transport.
 * @retval FURNACE_WIRE_ERR_ARGUMENT tx is NULL.
 * @retval FURNACE_WIRE_ERR_BUFFER payload_len exceeds FURNACE_WIRE_MAX_PAYLOAD.
 */
enum furnace_wire_result furnace_wire_tx_finish(struct furnace_wire_tx *tx, uint16_t msg_type,
						uint16_t payload_len);

/** @brief Prepare a receiver: no bytes held, every counter zero. */
void furnace_wire_rx_init(struct furnace_wire_rx *rx);

/**
 * @brief Give the receiver one byte (wire format §3 steps 1-5).
 *
 * @param rx    Receiver.
 * @param byte  The next byte of the stream.
 * @param frame Set when the result is RX_FRAME or RX_HELLO_VERSION.
 * @return What the byte completed; RX_NONE for every byte but a delimiter, and for
 *         the delimiter of an empty frame, which is neither a frame nor an error.
 */
enum furnace_wire_rx_status furnace_wire_rx_feed(struct furnace_wire_rx *rx, uint8_t byte,
						 struct furnace_wire_frame *frame);

#endif /* FURNACE_WIRE_H_ */
