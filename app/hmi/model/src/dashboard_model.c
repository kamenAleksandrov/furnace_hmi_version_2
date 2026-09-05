/* SPDX-License-Identifier: Apache-2.0 */

#include <string.h>

#include <furnace_hmi/dashboard_model.h>

static bool has_terminator(const char *text, size_t capacity)
{
    return text != NULL && memchr(text, '\0', capacity) != NULL;
}

static bool valid_i32_value(const furnace_hmi_i32_value_t *value)
{
    return value != NULL &&
           value->provenance < FURNACE_HMI_VALUE_PROVENANCE_COUNT &&
           value->validity < FURNACE_HMI_VALUE_VALIDITY_COUNT;
}

static bool valid_bool_value(const furnace_hmi_bool_value_t *value)
{
    return value != NULL &&
           value->provenance < FURNACE_HMI_VALUE_PROVENANCE_COUNT &&
           value->validity < FURNACE_HMI_VALUE_VALIDITY_COUNT;
}

void furnace_hmi_dashboard_state_initialize(furnace_hmi_dashboard_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->availability = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    state->mode = FURNACE_HMI_MACHINE_MODE_IDLE;
    state->run_state = FURNACE_HMI_RUN_STATE_IDLE;
    state->current_temperature_c.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->target_temperature_c.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->target_delta_c_per_minute.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->elapsed_seconds.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->remaining_seconds.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->heater_demand_percent.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->estimated_power_kw_x100.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->fan_rpm.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->service_time_hours.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->service_due_at_hours.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->service_forecast_completion_hours.validity =
        FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->service_run_gate = FURNACE_HMI_SERVICE_RUN_GATE_UNKNOWN;
    state->door_closed.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    state->fan_running.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
}

bool furnace_hmi_dashboard_state_is_well_formed(
    const furnace_hmi_dashboard_state_t *state
)
{
    if (state == NULL ||
        state->availability >= FURNACE_HMI_STATE_AVAILABILITY_COUNT ||
        state->mode >= FURNACE_HMI_MACHINE_MODE_COUNT ||
        state->run_state >= FURNACE_HMI_RUN_STATE_COUNT ||
        state->service_run_gate >= FURNACE_HMI_SERVICE_RUN_GATE_COUNT ||
        state->graph_sample_count > FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS ||
        !valid_i32_value(&state->current_temperature_c) ||
        !valid_i32_value(&state->target_temperature_c) ||
        !valid_i32_value(&state->target_delta_c_per_minute) ||
        !valid_i32_value(&state->elapsed_seconds) ||
        !valid_i32_value(&state->remaining_seconds) ||
        !valid_i32_value(&state->heater_demand_percent) ||
        !valid_i32_value(&state->estimated_power_kw_x100) ||
        !valid_i32_value(&state->fan_rpm) ||
        !valid_i32_value(&state->service_time_hours) ||
        !valid_i32_value(&state->service_due_at_hours) ||
        !valid_i32_value(&state->service_forecast_completion_hours) ||
        !valid_bool_value(&state->door_closed) ||
        !valid_bool_value(&state->fan_running) ||
        !has_terminator(state->program_name, sizeof(state->program_name)) ||
        !has_terminator(state->stage_name, sizeof(state->stage_name))) {
        return false;
    }

    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        if (state->graph_samples[index].measured_validity >=
            FURNACE_HMI_VALUE_VALIDITY_COUNT) {
            return false;
        }
    }

    return true;
}
