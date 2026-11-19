/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_ZEPHYR_CONTROLLER_LINK_H
#define FURNACE_HMI_ZEPHYR_CONTROLLER_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/controller_observation.h>

/**
 * Start the link thread: the controller client on the controller-link UART.
 * Returns 0, or a negative error when the UART is not ready.
 */
int furnace_hmi_controller_link_start(void);

/**
 * Copy the newest published observation into out when it is newer than
 * *seen, and update *seen. Never waits (ADR-0005): it returns false when
 * nothing new was published or the link thread is publishing at that moment.
 */
bool furnace_hmi_controller_link_take(furnace_hmi_controller_observation_t *out, uint32_t *seen);

/**
 * Hand an operator command to the link thread, which submits it at its next
 * poll (M21-5). Two slots, ordinary and Stop, so a waiting command never holds
 * up a Stop. Never waits: returns the submission number (never 0) that the
 * observation's command outcome or refusal will carry, or 0 when the slot still
 * holds a command the link thread has not taken.
 */
uint32_t furnace_hmi_controller_link_submit(const furnace_hmi_command_t *command);

#endif /* FURNACE_HMI_ZEPHYR_CONTROLLER_LINK_H */
