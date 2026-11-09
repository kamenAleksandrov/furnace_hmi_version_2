/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * CRC-32/ISO-HDLC (module core/crc): reflected polynomial 0xEDB88320, initial
 * value and final XOR 0xFFFFFFFF; the check value of the ASCII string 123456789
 * is 0xCBF43926. Pure ISO C17, no state. One implementation for the record store
 * (ADR-0014) and the wire codec (ADR-0018). It detects corruption and
 * authenticates nothing.
 */

#ifndef FURNACE_CRC32_H_
#define FURNACE_CRC32_H_

#include <stddef.h>
#include <stdint.h>

/** @brief CRC-32 of data, continued from crc (0 to start). */
uint32_t furnace_crc32(uint32_t crc, const void *data, size_t len);

#endif /* FURNACE_CRC32_H_ */
