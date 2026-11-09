/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * CRC-32/ISO-HDLC (module core/crc). Contract: crc32.h.
 */

#include <furnace/crc32.h>

uint32_t furnace_crc32(uint32_t crc, const void *data, size_t len)
{
	const uint8_t *p = data;

	crc = ~crc;
	for (size_t i = 0U; i < len; i++) {
		crc ^= p[i];
		for (uint32_t k = 0U; k < 8U; k++) {
			crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
		}
	}
	return ~crc;
}
