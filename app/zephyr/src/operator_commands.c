/* SPDX-License-Identifier: Apache-2.0 */

/* Operator screen requests as controller commands (Phase 21 M21-5b). */

#include <string.h>

#include "operator_commands.h"

_Static_assert(FURNACE_HMI_OPERATOR_PROGRAM_NAME_CAPACITY <= FURNACE_HMI_CONTROLLER_PROGRAM_NAME_CAPACITY + 1U,
               "every name the screens hold fits the controller's");

/* A save in the controller's form (M21-7b): Heating has a target and rate, Hold a duration. */
static bool program_save(const furnace_hmi_operator_request_t *request, furnace_hmi_command_t *command)
{
    const furnace_hmi_operator_program_t *program = &request->program;

    if (program->stage_count == 0U || program->stage_count > FURNACE_HMI_CONTROLLER_MAX_STAGES ||
        request->program_name[0] == '\0') {
        return false;
    }
    command->kind = FURNACE_HMI_COMMAND_PROGRAM_SAVE;
    command->program_id = request->program_id;
    command->program_revision = request->program_id == 0U ? 0U : request->program_revision;
    command->acknowledged_duration_s = request->acknowledged_duration_s;
    (void)memcpy(command->name, request->program_name, sizeof(request->program_name));
    command->name[sizeof(command->name) - 1U] = '\0';
    command->stage_count = program->stage_count;
    for (uint8_t i = 0U; i < program->stage_count; i++) {
        const furnace_hmi_operator_program_stage_t *s = &program->stages[i];
        furnace_hmi_controller_stage_t *t = &command->stages[i];

        if (s->kind == FURNACE_HMI_OPERATOR_STAGE_HOLD) {
            if (s->minutes < 1) {
                return false;
            }
            t->kind = FURNACE_HMI_CONTROLLER_STAGE_HOLD;
            t->duration_minutes = (uint32_t)s->minutes;
        } else if (s->kind == FURNACE_HMI_OPERATOR_STAGE_HEATING) {
            t->kind = FURNACE_HMI_CONTROLLER_STAGE_HEATING;
            t->target_decicelsius = s->target_c * 10;
            t->rate_decicelsius_per_min = s->rate_tenths_c_per_minute;
        } else {
            return false;
        }
    }
    return true;
}

bool furnace_hmi_operator_command_for(const furnace_hmi_operator_request_t *request,
                                      furnace_hmi_command_t *command)
{
    /* In place, not a compound literal: no second program-sized temporary on the stack. */
    (void)memset(command, 0, sizeof(*command));
    command->run_id = request->run_id;
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
    case FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_SAVE:
        return program_save(request, command);
    case FURNACE_HMI_OPERATOR_REQUEST_PROGRAM_DELETE:
        if (request->program_id == 0U) {
            return false;
        }
        command->kind = FURNACE_HMI_COMMAND_PROGRAM_DELETE;
        command->program_id = request->program_id;
        command->program_revision = request->program_revision;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_START_PROGRAM:
        /* The controller's program and the revision the operator saw (M21-7b). */
        if (request->program_id == 0U) {
            return false;
        }
        command->kind = FURNACE_HMI_COMMAND_PROGRAM_START;
        command->program_id = request->program_id;
        command->program_revision = request->program_revision;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_START_CONFIRM:
        if (request->transaction_id == 0U) {
            return false;
        }
        command->kind = FURNACE_HMI_COMMAND_START_CONFIRM;
        command->transaction_id = request->transaction_id;
        command->proposal_revision = request->proposal_revision;
        command->program_id = request->program_id;
        command->program_revision = request->program_revision;
        command->proposed_stage = request->proposed_stage;
        command->accepted_duration_s = request->accepted_duration_s;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_START_CANCEL:
        if (request->transaction_id == 0U) {
            return false;
        }
        command->kind = FURNACE_HMI_COMMAND_START_CANCEL;
        command->transaction_id = request->transaction_id;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_PLANNING_REFERENCE:
        /* SETTING_CHANGE of the planning reference (M21-7d); the controller checks its range. */
        command->kind = FURNACE_HMI_COMMAND_SETTING_CHANGE;
        command->setting = 1U;
        command->setting_value = request->planning_reference_decicelsius;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_CLOCK_SET:
        /* CLOCK_SET (M21-7e); the controller checks the range and execution. */
        command->kind = FURNACE_HMI_COMMAND_CLOCK_SET;
        command->clock_wall_ms = request->clock_wall_ms;
        return true;
    default:
        return false;
    }
    command->target_decicelsius = request->target_c * 10;
    command->rate_decicelsius_per_min = request->rate_tenths_c_per_minute;
    return true;
}
