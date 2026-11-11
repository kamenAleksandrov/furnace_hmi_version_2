/* SPDX-License-Identifier: Apache-2.0 */

/* Operator screen requests as controller commands (Phase 21 M21-5b). */

#include "operator_commands.h"

bool furnace_hmi_operator_command_for(const furnace_hmi_operator_request_t *request,
                                      furnace_hmi_command_t *command)
{
    *command = (furnace_hmi_command_t){.run_id = request->run_id};
    switch (request->kind) {
    case FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE:
        command->kind = FURNACE_HMI_COMMAND_START_PREPARE;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START:
        if (request->session_minutes <= 0) {
            return false;
        }
        command->kind = FURNACE_HMI_COMMAND_MANUAL_START;
        command->session_minutes = (uint32_t)request->session_minutes;
        break;
    case FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET:
        command->kind = FURNACE_HMI_COMMAND_MANUAL_RETARGET;
        break;
    case FURNACE_HMI_OPERATOR_REQUEST_PAUSE:
        command->kind = FURNACE_HMI_COMMAND_PAUSE;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_RESUME:
        command->kind = FURNACE_HMI_COMMAND_RESUME;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_STOP:
        command->kind = FURNACE_HMI_COMMAND_STOP;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_CONTINUE:
        command->kind = FURNACE_HMI_COMMAND_CONTINUE;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_FAULT_ACKNOWLEDGE:
        command->kind = FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE;
        command->fault_id = request->fault_id;
        command->source = request->source;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_FAULT_RECOVERY:
        command->kind = FURNACE_HMI_COMMAND_FAULT_RECOVERY;
        command->fault_id = request->fault_id;
        command->source = request->source;
        return true;
    default:
        return false;
    }
    command->target_decicelsius = request->target_c * 10;
    command->rate_decicelsius_per_min = request->rate_tenths_c_per_minute;
    return true;
}
