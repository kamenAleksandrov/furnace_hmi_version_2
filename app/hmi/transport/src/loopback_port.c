/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include <furnace_hmi/loopback_port.h>

static size_t pipe_read(furnace_hmi_loopback_pipe_t *pipe, uint8_t *buffer, size_t capacity)
{
    size_t n = 0U;

    while (n < capacity && pipe->count > 0U) {
        buffer[n++] = pipe->bytes[pipe->head];
        pipe->head = (pipe->head + 1U) % FURNACE_HMI_LOOPBACK_CAPACITY;
        pipe->count--;
    }
    return n;
}

static size_t pipe_write(furnace_hmi_loopback_pipe_t *pipe, const uint8_t *bytes, size_t length)
{
    size_t n = 0U;

    while (n < length && pipe->count < FURNACE_HMI_LOOPBACK_CAPACITY) {
        pipe->bytes[(pipe->head + pipe->count) % FURNACE_HMI_LOOPBACK_CAPACITY] = bytes[n++];
        pipe->count++;
    }
    return n;
}

static size_t read_a(void *context, uint8_t *buffer, size_t capacity)
{
    furnace_hmi_loopback_t *loopback = context;

    return pipe_read(&loopback->b_to_a, buffer, capacity);
}

static size_t write_a(void *context, const uint8_t *bytes, size_t length)
{
    furnace_hmi_loopback_t *loopback = context;

    return loopback->connected ? pipe_write(&loopback->a_to_b, bytes, length) : length;
}

static size_t read_b(void *context, uint8_t *buffer, size_t capacity)
{
    furnace_hmi_loopback_t *loopback = context;

    return pipe_read(&loopback->a_to_b, buffer, capacity);
}

static size_t write_b(void *context, const uint8_t *bytes, size_t length)
{
    furnace_hmi_loopback_t *loopback = context;

    return loopback->connected ? pipe_write(&loopback->b_to_a, bytes, length) : length;
}

void furnace_hmi_loopback_init(furnace_hmi_loopback_t *loopback)
{
    (void)memset(loopback, 0, sizeof(*loopback));
    loopback->connected = true;
}

furnace_hmi_byte_port_t furnace_hmi_loopback_port_a(furnace_hmi_loopback_t *loopback)
{
    return (furnace_hmi_byte_port_t){.read = read_a, .write = write_a, .context = loopback};
}

furnace_hmi_byte_port_t furnace_hmi_loopback_port_b(furnace_hmi_loopback_t *loopback)
{
    return (furnace_hmi_byte_port_t){.read = read_b, .write = write_b, .context = loopback};
}
