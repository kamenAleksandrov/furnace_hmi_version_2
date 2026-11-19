/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_BYTE_PORT_H
#define FURNACE_HMI_BYTE_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A byte stream to the controller: the HMI's view of its link (PROTO-005).
 * Framing and meaning belong to the layers above; a port moves bytes only.
 *
 * Both functions return at once. ``read`` copies up to ``capacity`` bytes that
 * have already arrived and returns how many, 0 when none. ``write`` queues up
 * to ``length`` bytes and returns how many it took, fewer when the link cannot
 * take more now. A port reports no link state: silence is the only sign of a
 * lost link.
 */
typedef struct {
    size_t (*read)(void *context, uint8_t *buffer, size_t capacity);
    size_t (*write)(void *context, const uint8_t *bytes, size_t length);
    void *context;
} furnace_hmi_byte_port_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_BYTE_PORT_H */
