/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>

#include <furnace_hmi/simulated_controller.h>

static furnace_hmi_i32_value_t i32_value(
    int32_t value,
    furnace_hmi_value_provenance_t provenance,
    furnace_hmi_value_validity_t validity
)
{
    furnace_hmi_i32_value_t result = {
        .value = value,
        .provenance = provenance,
        .validity = validity,
    };
    return result;
}

static furnace_hmi_bool_value_t bool_value(
    bool value,
    furnace_hmi_value_provenance_t provenance,
    furnace_hmi_value_validity_t validity
)
{
    furnace_hmi_bool_value_t result = {
        .value = value,
        .provenance = provenance,
        .validity = validity,
    };
    return result;
}

const char *furnace_hmi_sim_scenario_name(furnace_hmi_sim_scenario_t scenario)
{
    static const char *const names[FURNACE_HMI_SIM_SCENARIO_COUNT] = {
        [FURNACE_HMI_SIM_SCENARIO_DISCONNECTED] = "disconnected",
        [FURNACE_HMI_SIM_SCENARIO_IDLE] = "idle",
        [FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL] = "running-normal",
        [FURNACE_HMI_SIM_SCENARIO_MANUAL] = "manual",
        [FURNACE_HMI_SIM_SCENARIO_PAUSED] = "paused",
        [FURNACE_HMI_SIM_SCENARIO_FAULT] = "fault",
        [FURNACE_HMI_SIM_SCENARIO_STALE] = "stale",
        [FURNACE_HMI_SIM_SCENARIO_RUNNING_OVERRUN] = "running-overrun",
    };

    if (scenario >= FURNACE_HMI_SIM_SCENARIO_COUNT) {
        return NULL;
    }
    return names[scenario];
}

bool furnace_hmi_sim_scenario_from_name(
    const char *name,
    furnace_hmi_sim_scenario_t *scenario
)
{
    if (name == NULL || scenario == NULL) {
        return false;
    }

    for (furnace_hmi_sim_scenario_t candidate = 0;
         candidate < FURNACE_HMI_SIM_SCENARIO_COUNT;
         ++candidate) {
        if (strcmp(name, furnace_hmi_sim_scenario_name(candidate)) == 0) {
            *scenario = candidate;
            return true;
        }
    }
    return false;
}

static void fill_graph(
    furnace_hmi_dashboard_state_t *state,
    furnace_hmi_value_validity_t measured_validity
)
{
    static const uint32_t stage_times[] = { 0U, 8U * 60U, 20U * 60U, 44U * 60U, 59U * 60U, 64U * 60U };
    static const int32_t stage_temperatures[] = { 25, 180, 180, 850, 850, 250 };
    state->graph_sample_count = FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS;
    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        const uint32_t elapsed = (uint32_t)index * stage_times[5] /
            (uint32_t)(state->graph_sample_count - 1U);
        size_t stage_index = 0U;
        while (stage_index + 1U < sizeof(stage_times) / sizeof(stage_times[0]) &&
               elapsed > stage_times[stage_index + 1U]) {
            ++stage_index;
        }
        const uint32_t first_time = stage_times[stage_index];
        const uint32_t last_time = stage_times[stage_index + 1U];
        const int32_t first_temperature = stage_temperatures[stage_index];
        const int32_t last_temperature = stage_temperatures[stage_index + 1U];
        const int32_t planned = first_temperature + (last_temperature - first_temperature) *
            (int32_t)(elapsed - first_time) / (int32_t)(last_time - first_time);
        const int32_t measured = planned - 22 + (int32_t)((index % 4U) * 7U);
        const furnace_hmi_value_validity_t sample_validity =
            measured_validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            state->elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
            elapsed <= (uint32_t)state->elapsed_seconds.value
                ? FURNACE_HMI_VALUE_VALIDITY_CURRENT
                : (measured_validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT
                    ? FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE : measured_validity);
        state->graph_samples[index] = (furnace_hmi_graph_sample_t) {
            .elapsed_seconds = elapsed,
            .planned_temperature_c = planned,
            .measured_temperature_c = measured,
            .measured_validity = sample_validity,
        };
    }
}

static void fill_current_common(furnace_hmi_dashboard_state_t *state)
{
    state->availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state->controller_session_epoch = 7U;
    state->state_revision = 42U;
    state->mode = FURNACE_HMI_MACHINE_MODE_PROGRAM;
    state->run_state = FURNACE_HMI_RUN_STATE_RUNNING;
    state->program_id = 101U;
    state->program_revision = 3U;
    (void)snprintf(
        state->program_name,
        sizeof(state->program_name),
        "%s",
        "Ceramic test program"
    );
    state->stage_index = 2U;
    state->stage_count = 5U;
    (void)snprintf(state->stage_name, sizeof(state->stage_name), "%s", "Heating to 850 C");

    state->current_temperature_c = i32_value(
        642,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->target_temperature_c = i32_value(
        850,
        FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->target_delta_c_per_minute = i32_value(
        35,
        FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->elapsed_seconds = i32_value(
        26 * 60,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->remaining_seconds = i32_value(
        38 * 60,
        FURNACE_HMI_VALUE_PROVENANCE_CALCULATED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->heater_demand_percent = i32_value(
        72,
        FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->estimated_power_kw_x100 = i32_value(
        340,
        FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->fan_rpm = i32_value(
        1450,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->service_time_hours = i32_value(
        128,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->service_due_at_hours = i32_value(
        130,
        FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->service_forecast_completion_hours = i32_value(
        132,
        FURNACE_HMI_VALUE_PROVENANCE_CALCULATED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->service_run_gate =
        FURNACE_HMI_SERVICE_RUN_GATE_ACKNOWLEDGEMENT_REQUIRED;
    state->door_closed = bool_value(
        true,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->fan_running = bool_value(
        true,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
}

void furnace_hmi_simulator_set_scenario(
    furnace_hmi_simulator_t *simulator,
    furnace_hmi_sim_scenario_t scenario
)
{
    if (simulator == NULL) {
        return;
    }
    if (scenario >= FURNACE_HMI_SIM_SCENARIO_COUNT) {
        scenario = FURNACE_HMI_SIM_SCENARIO_DISCONNECTED;
    }

    simulator->scenario = scenario;
    simulator->virtual_elapsed_seconds = 0U;
    furnace_hmi_dashboard_state_initialize(&simulator->state);

    switch (scenario) {
    case FURNACE_HMI_SIM_SCENARIO_DISCONNECTED:
        (void)snprintf(
            simulator->state.program_name,
            sizeof(simulator->state.program_name),
            "%s",
            ""
        );
        break;

    case FURNACE_HMI_SIM_SCENARIO_IDLE:
        simulator->state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
        simulator->state.controller_session_epoch = 7U;
        simulator->state.state_revision = 9U;
        simulator->state.current_temperature_c = i32_value(
            24,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.door_closed = bool_value(
            true,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.fan_running = bool_value(
            false,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.service_time_hours = i32_value(
            128,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.service_due_at_hours = i32_value(
            130,
            FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.service_forecast_completion_hours = i32_value(
            132,
            FURNACE_HMI_VALUE_PROVENANCE_CALCULATED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.service_run_gate =
            FURNACE_HMI_SERVICE_RUN_GATE_ACKNOWLEDGEMENT_REQUIRED;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE);
        break;

    case FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL:
        fill_current_common(&simulator->state);
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_MANUAL:
        fill_current_common(&simulator->state);
        simulator->state.mode = FURNACE_HMI_MACHINE_MODE_MANUAL;
        simulator->state.program_id = 0U;
        simulator->state.program_revision = 0U;
        (void)snprintf(simulator->state.program_name, sizeof(simulator->state.program_name), "%s", "Manual session");
        (void)snprintf(simulator->state.stage_name, sizeof(simulator->state.stage_name), "%s", "Manual target");
        simulator->state.stage_index = 0U;
        simulator->state.stage_count = 0U;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_PAUSED:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_PAUSED;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.fan_running.value = false;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_FAULT:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.door_closed.value = false;
        simulator->state.fan_running.value = false;
        (void)snprintf(
            simulator->state.stage_name,
            sizeof(simulator->state.stage_name),
            "%s",
            "Safety stop: door open"
        );
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_STALE:
        fill_current_common(&simulator->state);
        simulator->state.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
        simulator->state.current_temperature_c.validity = FURNACE_HMI_VALUE_VALIDITY_STALE;
        simulator->state.target_temperature_c.validity = FURNACE_HMI_VALUE_VALIDITY_STALE;
        simulator->state.heater_demand_percent.validity = FURNACE_HMI_VALUE_VALIDITY_STALE;
        simulator->state.fan_rpm.validity = FURNACE_HMI_VALUE_VALIDITY_STALE;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_STALE);
        break;

    case FURNACE_HMI_SIM_SCENARIO_RUNNING_OVERRUN:
        fill_current_common(&simulator->state);
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        simulator->state.elapsed_seconds.value = 70 * 60;
        simulator->state.remaining_seconds.value = 0;
        simulator->state.current_temperature_c.value = 1120;
        simulator->state.graph_samples[simulator->state.graph_sample_count - 1U]
            .measured_temperature_c = 1140;
        (void)snprintf(simulator->state.stage_name, sizeof(simulator->state.stage_name), "%s",
                       "Observed run beyond planned range");
        break;

    case FURNACE_HMI_SIM_SCENARIO_COUNT:
        break;
    }
}

void furnace_hmi_simulator_initialize(
    furnace_hmi_simulator_t *simulator,
    furnace_hmi_sim_scenario_t scenario
)
{
    if (simulator == NULL) {
        return;
    }
    memset(simulator, 0, sizeof(*simulator));
    furnace_hmi_simulator_set_scenario(simulator, scenario);
}

void furnace_hmi_simulator_tick(
    furnace_hmi_simulator_t *simulator,
    uint32_t elapsed_seconds
)
{
    if (simulator == NULL || elapsed_seconds == 0U) {
        return;
    }

    simulator->virtual_elapsed_seconds += elapsed_seconds;
    furnace_hmi_dashboard_state_t *state = &simulator->state;
    if (state->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        state->run_state != FURNACE_HMI_RUN_STATE_RUNNING) {
        return;
    }

    state->elapsed_seconds.value += (int32_t)elapsed_seconds;
    if (state->remaining_seconds.value > (int32_t)elapsed_seconds) {
        state->remaining_seconds.value -= (int32_t)elapsed_seconds;
    }

    const int32_t rise = (int32_t)(simulator->virtual_elapsed_seconds * 2U);
    const int32_t current = 642 + rise;
    state->current_temperature_c.value = current > state->target_temperature_c.value
        ? state->target_temperature_c.value
        : current;
    state->state_revision += 1U;

    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        if (state->graph_samples[index].elapsed_seconds > (uint32_t)state->elapsed_seconds.value) {
            state->graph_samples[index].measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
            continue;
        }
        const int32_t wobble = (int32_t)((simulator->virtual_elapsed_seconds + index) % 5U) - 2;
        state->graph_samples[index].measured_temperature_c =
            state->graph_samples[index].planned_temperature_c - 18 + wobble * 3;
        state->graph_samples[index].measured_validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT;
    }
}

const furnace_hmi_dashboard_state_t *furnace_hmi_simulator_state(
    const furnace_hmi_simulator_t *simulator
)
{
    return simulator == NULL ? NULL : &simulator->state;
}
