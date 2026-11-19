/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_LOOPBACK_PORT_H
#define FURNACE_HMI_LOOPBACK_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <furnace_hmi/byte_port.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bytes one direction of a loopback holds before writes are refused. */
#define FURNACE_HMI_LOOPBACK_CAPACITY 4096U

/** One direction: a bounded ring of bytes. */
typedef struct {
    uint8_t bytes[FURNACE_HMI_LOOPBACK_CAPACITY];
    size_t head;
    size_t count;
} furnace_hmi_loopback_pipe_t;

/**
 * An in-memory link between two ports, for tests and simulation. ``a`` and
 * ``b`` are the two ends: what one writes, the other reads. Single-threaded:
 * both ends are used from one execution context.
 */
typedef struct {
    furnace_hmi_loopback_pipe_t a_to_b;
    furnace_hmi_loopback_pipe_t b_to_a;
    /** While false, writes at either end are accepted and discarded: a cut cable. */
    bool connected;
} furnace_hmi_loopback_t;

/** Empty both directions and connect them. */
void furnace_hmi_loopback_init(furnace_hmi_loopback_t *loopback);

/** The port of end A (reads what B wrote) or end B. */
furnace_hmi_byte_port_t furnace_hmi_loopback_port_a(furnace_hmi_loopback_t *loopback);
furnace_hmi_byte_port_t furnace_hmi_loopback_port_b(furnace_hmi_loopback_t *loopback);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_LOOPBACK_PORT_H */
