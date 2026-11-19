/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * The golden vectors as C tables. The build generates the definitions from
 * tests/vectors/wire/ with scripts/wire_vectors.py --emit-c.
 */

#ifndef WIRE_VECTOR_TABLE_H_
#define WIRE_VECTOR_TABLE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <furnace/wire.h>
#include <furnace/wire_messages.h>

#define WIRE_STREAM_MAX_RESULTS 4U

struct wire_positive_vector {
	const char *name;
	uint8_t proto_major;
	uint8_t proto_minor;
	const struct furnace_wire_message *message;
	const uint8_t *payload;
	size_t payload_len;
	const uint8_t *frame;
	size_t frame_len;
	const uint8_t *cobs;
	size_t cobs_len;
};

struct wire_negative_vector {
	const char *name;
	uint16_t type;
	const uint8_t *payload;
	size_t payload_len;
	enum furnace_wire_result expect;
	/* expect OK: the re-encoded payload is this many bytes shorter (newer minor). */
	size_t trim;
};

struct wire_cobs_vector {
	const char *name;
	const uint8_t *raw;
	size_t raw_len;
	const uint8_t *cobs;
	size_t cobs_len;
};

struct wire_stream_vector {
	const char *name;
	const uint8_t *bytes;
	size_t len;
	enum furnace_wire_rx_status results[WIRE_STREAM_MAX_RESULTS];
	size_t result_count;
	struct furnace_wire_counters counters;
};

struct wire_unknown_vector {
	const char *name;
	uint16_t type;
	const uint8_t *payload;
	size_t payload_len;
};

/* Written by hand from the wire document: tests/vectors/wire/hand/. */
struct wire_hand_vector {
	const char *name;
	const struct furnace_wire_message *message;
	const uint8_t *payload;
	size_t payload_len;
};

extern const struct wire_hand_vector wire_hand_vectors[];
extern const size_t wire_hand_count;
extern const struct wire_positive_vector wire_positive_vectors[];
extern const size_t wire_positive_count;
extern const struct wire_negative_vector wire_negative_vectors[];
extern const size_t wire_negative_count;
extern const struct wire_cobs_vector wire_cobs_vectors[];
extern const size_t wire_cobs_count;
extern const struct wire_stream_vector wire_stream_vectors[];
extern const size_t wire_stream_count;
extern const struct wire_unknown_vector wire_unknown_vectors[];
extern const size_t wire_unknown_count;

#endif /* WIRE_VECTOR_TABLE_H_ */
