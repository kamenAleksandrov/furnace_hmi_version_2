/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_ZEPHYR_OPERATOR_COMMANDS_H
#define FURNACE_HMI_ZEPHYR_OPERATOR_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/controller_command.h>
#include <furnace_hmi/operator_screens.h>

/**
 * The controller command for an operator screen request (Phase 21 M21-5b):
 * whole degrees become 0.1 °C, and run commands name run_id, the run the
 * controller reports now. False for requests that are not commands yet
 * (program Start and closing a run: M21-7); the host then hands nothing over.
 */
bool furnace_hmi_operator_command_for(const furnace_hmi_operator_request_t *request, uint32_t run_id,
                                      furnace_hmi_command_t *command);

#endif /* FURNACE_HMI_ZEPHYR_OPERATOR_COMMANDS_H */
