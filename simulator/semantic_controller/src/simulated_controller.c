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
        [FURNACE_HMI_SIM_SCENARIO_COOLING] = "cooling",
        [FURNACE_HMI_SIM_SCENARIO_OVER_TEMPERATURE] = "overtemperature",
        [FURNACE_HMI_SIM_SCENARIO_HEATING_SLOW] = "heating-slow",
        [FURNACE_HMI_SIM_SCENARIO_SENSOR_FAULT] = "sensor-fault",
        [FURNACE_HMI_SIM_SCENARIO_POWER_LOSS] = "power-loss",
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

#define SIM_CURING_END_SECONDS (68U * 60U)

/* The simulated program: corner times and planned temperatures. */
static const uint32_t stage_times[] = { 0U, 3U * 60U, 15U * 60U, 57U * 60U, 68U * 60U, 103U * 60U };
static const int32_t stage_temperatures[] = { 25, 34, 34, 160, 160, 47 };

static int32_t sim_planned_at(uint32_t elapsed)
{
    size_t stage = 0U;
    while (stage + 2U < sizeof(stage_times) / sizeof(stage_times[0]) &&
           elapsed > stage_times[stage + 1U]) {
        ++stage;
    }
    if (elapsed >= stage_times[stage + 1U]) {
        return stage_temperatures[stage + 1U];
    }
    return stage_temperatures[stage] +
        (stage_temperatures[stage + 1U] - stage_temperatures[stage]) *
        (int32_t)(elapsed - stage_times[stage]) /
        (int32_t)(stage_times[stage + 1U] - stage_times[stage]);
}

/* Simulated reading at `elapsed` of a run observed until `now`. A healthy
 * furnace's controller holds the plan within 0.5 C, which whole degrees show
 * as the plan itself. A fault scenario's current reading departs from the plan;
 * the trace leaves the plan over the last quarter of the run to meet it. */
static int32_t sim_measured_at(uint32_t elapsed, uint32_t now, int32_t current)
{
    const int32_t planned = sim_planned_at(elapsed);
    const uint32_t departure = now - now / 4U;
    if (now == 0U || elapsed <= departure) {
        return planned;
    }
    const int64_t progress = (int64_t)(elapsed > now ? now : elapsed) - departure;
    const int64_t span = (int64_t)(now - departure);
    return planned + (int32_t)((int64_t)(current - sim_planned_at(now)) * progress * progress /
                               (span * span));
}

static void fill_graph(
    furnace_hmi_dashboard_state_t *state,
    furnace_hmi_value_validity_t measured_validity
)
{
    state->graph_sample_count = FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS;
    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        const size_t section = index / 6U < 4U ? index / 6U : 4U;
        const uint32_t steps = section == 4U ? 7U : 6U;
        const uint32_t elapsed = stage_times[section] +
            (stage_times[section + 1U] - stage_times[section]) * (uint32_t)(index - section * 6U) / steps;
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
        const int32_t measured = state->elapsed_seconds.value > 0
            ? sim_measured_at(elapsed, (uint32_t)state->elapsed_seconds.value,
                              state->current_temperature_c.value)
            : state->current_temperature_c.value;
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
            .planned_cooling = elapsed > SIM_CURING_END_SECONDS,
        };
    }
}

/* Measured trace follows the plan closely up to the observed run time. */
static void track_planned_measurements(furnace_hmi_dashboard_state_t *state, uint32_t until_seconds)
{
    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        furnace_hmi_graph_sample_t *sample = &state->graph_samples[index];
        sample->measured_temperature_c = sample->planned_temperature_c -
            (sample->elapsed_seconds > SIM_CURING_END_SECONDS ? 3 : 0);
        sample->measured_validity = sample->elapsed_seconds <= until_seconds
            ? FURNACE_HMI_VALUE_VALIDITY_CURRENT
            : FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
    }
}

static void fill_current_common(furnace_hmi_dashboard_state_t *state)
{
    state->availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state->controller_session_epoch = 7U;
    state->state_revision = 42U;
    state->mode = FURNACE_HMI_MACHINE_MODE_PROGRAM;
    state->run_state = FURNACE_HMI_RUN_STATE_RUNNING;
    state->run_phase = FURNACE_HMI_RUN_PHASE_CURING;
    state->program_id = 101U;
    state->program_revision = 3U;
    (void)snprintf(
        state->program_name,
        sizeof(state->program_name),
        "%s",
        "Ceramic test program"
    );
    state->stage_index = 2U;
    state->stage_count = 4U;
    (void)snprintf(state->stage_name, sizeof(state->stage_name), "%s", "Heating to 160 \xC2\xB0" "C");
    state->stage_kind = FURNACE_HMI_STAGE_KIND_HEATING;

    state->current_temperature_c = i32_value(
        sim_planned_at(42U * 60U),
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->target_temperature_c = i32_value(
        160,
        FURNACE_HMI_VALUE_PROVENANCE_COMMANDED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->target_delta_c_per_minute = i32_value(
        2,
        FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->elapsed_seconds = i32_value(
        42 * 60,
        FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
        FURNACE_HMI_VALUE_VALIDITY_CURRENT
    );
    state->remaining_seconds = i32_value(
        26 * 60,
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
    simulator->program_run = false;
    simulator->run_seconds = 0U;
    simulator->cooling_seconds = 0U;
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
        simulator->state.estimated_power_kw_x100.value = 0;
        simulator->state.fan_running.value = false;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_FAULT:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.fault_kind = FURNACE_HMI_FAULT_KIND_DOOR_OPEN;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.estimated_power_kw_x100.value = 0;
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
        simulator->state.elapsed_seconds.value = 70 * 60;
        simulator->state.remaining_seconds.value = 0;
        simulator->state.current_temperature_c.value = 190;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        simulator->state.graph_samples[simulator->state.graph_sample_count - 1U]
            .measured_temperature_c = 195;
        (void)snprintf(simulator->state.stage_name, sizeof(simulator->state.stage_name), "%s",
                       "Heating / observed overrun");
        break;

    case FURNACE_HMI_SIM_SCENARIO_COOLING:
        fill_current_common(&simulator->state);
        simulator->state.run_phase = FURNACE_HMI_RUN_PHASE_COOLING;
        simulator->state.stage_index = simulator->state.stage_count;
        (void)snprintf(simulator->state.stage_name, sizeof(simulator->state.stage_name), "%s",
                       "Automatic cooling");
        simulator->state.elapsed_seconds.value = (int32_t)SIM_CURING_END_SECONDS;
        simulator->state.remaining_seconds.value = 0;
        simulator->state.cooling_elapsed_seconds = i32_value(
            17 * 60,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.cooling_remaining_seconds = i32_value(
            25 * 60,
            FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.current_temperature_c.value = 102;
        simulator->state.target_temperature_c.value = 30;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.estimated_power_kw_x100.value = 0;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        track_planned_measurements(&simulator->state, SIM_CURING_END_SECONDS + 17U * 60U);
        break;

    /* Controller-reported faults that ended a Program run: heating is off and
     * the graph keeps the readings up to the fault. */
    case FURNACE_HMI_SIM_SCENARIO_OVER_TEMPERATURE:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.fault_kind = FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE;
        simulator->state.current_temperature_c.value = 196;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.estimated_power_kw_x100.value = 0;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_HEATING_SLOW:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.fault_kind = FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED;
        simulator->state.current_temperature_c.value = 84;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.estimated_power_kw_x100.value = 0;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        break;

    case FURNACE_HMI_SIM_SCENARIO_SENSOR_FAULT:
        fill_current_common(&simulator->state);
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.fault_kind = FURNACE_HMI_FAULT_KIND_SENSOR;
        simulator->state.heater_demand_percent.value = 0;
        simulator->state.estimated_power_kw_x100.value = 0;
        fill_graph(&simulator->state, FURNACE_HMI_VALUE_VALIDITY_CURRENT);
        simulator->state.current_temperature_c.validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
        break;

    /* After a power interruption the controller reports the run as ended. */
    case FURNACE_HMI_SIM_SCENARIO_POWER_LOSS:
        simulator->state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
        simulator->state.controller_session_epoch = 8U;
        simulator->state.state_revision = 1U;
        simulator->state.run_state = FURNACE_HMI_RUN_STATE_FAULT;
        simulator->state.fault_kind = FURNACE_HMI_FAULT_KIND_POWER_INTERRUPTED;
        simulator->state.current_temperature_c = i32_value(
            118,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        simulator->state.door_closed = bool_value(
            true,
            FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
            FURNACE_HMI_VALUE_VALIDITY_CURRENT
        );
        break;

    case FURNACE_HMI_SIM_SCENARIO_COUNT:
        break;
    }
}

/* ---- Programs started on request ------------------------------------------
 * The controller follows the plan exactly (a healthy furnace holds its
 * setpoint within half a degree, which whole degrees show as the plan), then
 * cools automatically toward 30 C and reports the run complete. */

#define SIM_START_TEMPERATURE_C 25
#define SIM_COOLING_END_C 30
#define SIM_MAX_PROGRAM_TEMPERATURE_C 180
#define SIM_MAX_RATE_TENTHS 30

static uint32_t program_curing_seconds(const furnace_hmi_sim_program_t *program)
{
    uint32_t seconds = 0U;
    for (uint8_t index = 0U; index < program->stage_count; ++index) {
        seconds += program->stages[index].minutes * 60U;
    }
    return seconds;
}

static int32_t program_stage_target(const furnace_hmi_sim_program_t *program, uint8_t index)
{
    int32_t target = SIM_START_TEMPERATURE_C;
    for (uint8_t stage = 0U; stage <= index && stage < program->stage_count; ++stage) {
        if (!program->stages[stage].hold) {
            target = program->stages[stage].target_c;
        }
    }
    return target;
}

static int32_t program_peak(const furnace_hmi_sim_program_t *program)
{
    int32_t peak = SIM_COOLING_END_C;
    for (uint8_t index = 0U; index < program->stage_count; ++index) {
        const int32_t target = program_stage_target(program, index);
        peak = target > peak ? target : peak;
    }
    return peak;
}

/* Explorer program_cooling_minutes: about 1.2 C per minute, at least 20 min. */
static uint32_t program_cooling_seconds(const furnace_hmi_sim_program_t *program)
{
    const int32_t delta = program_peak(program) - SIM_COOLING_END_C;
    const int32_t minutes = delta > 0 ? (delta * 5 + 5) / 6 : 0;
    return (uint32_t)(minutes < 20 ? 20 : minutes) * 60U;
}

/* Planned temperature at `seconds` into the run, cooling included. */
static int32_t program_planned_at(const furnace_hmi_sim_program_t *program, uint32_t seconds)
{
    uint32_t start = 0U;
    int32_t from = SIM_START_TEMPERATURE_C;
    for (uint8_t index = 0U; index < program->stage_count; ++index) {
        const uint32_t length = program->stages[index].minutes * 60U;
        const int32_t to = program_stage_target(program, index);
        if (seconds <= start + length && length > 0U) {
            return from + (int32_t)((int64_t)(to - from) * (int64_t)(seconds - start) / (int64_t)length);
        }
        start += length;
        from = to;
    }
    const uint32_t cooling = program_cooling_seconds(program);
    const uint32_t into = seconds - start < cooling ? seconds - start : cooling;
    return from + (int32_t)((int64_t)(SIM_COOLING_END_C - from) * (int64_t)into / (int64_t)cooling);
}

/* Graph samples: every stage corner and the cooling end, the rest spread
 * evenly over the run, in time order. */
static void program_fill_graph(furnace_hmi_simulator_t *simulator)
{
    const furnace_hmi_sim_program_t *program = &simulator->program;
    furnace_hmi_dashboard_state_t *state = &simulator->state;
    const uint32_t curing = program_curing_seconds(program);
    const uint32_t total = curing + program_cooling_seconds(program);
    const uint32_t observed = simulator->run_seconds + simulator->cooling_seconds;
    uint32_t times[FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS];
    size_t count = 0U;
    uint32_t corner = 0U;
    times[count++] = 0U;
    for (uint8_t index = 0U; index < program->stage_count; ++index) {
        corner += program->stages[index].minutes * 60U;
        times[count++] = corner;
    }
    times[count++] = total;
    const size_t corners = count;
    const size_t spread = FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS - corners;
    for (size_t step = 1U; step <= spread && count < FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS; ++step) {
        times[count++] = (uint32_t)((uint64_t)total * step / (spread + 1U));
    }
    for (size_t i = 1U; i < count; ++i) {
        const uint32_t value = times[i];
        size_t j = i;
        while (j > 0U && times[j - 1U] > value) {
            times[j] = times[j - 1U];
            --j;
        }
        times[j] = value;
    }
    size_t unique = 0U;
    for (size_t i = 0U; i < count; ++i) {
        if (unique == 0U || times[i] != times[unique - 1U]) {
            times[unique++] = times[i];
        }
    }
    state->graph_sample_count = unique;
    for (size_t index = 0U; index < unique; ++index) {
        const int32_t planned = program_planned_at(program, times[index]);
        state->graph_samples[index] = (furnace_hmi_graph_sample_t) {
            .elapsed_seconds = times[index],
            .planned_temperature_c = planned,
            .measured_temperature_c = planned,
            .measured_validity = times[index] <= observed
                ? FURNACE_HMI_VALUE_VALIDITY_CURRENT : FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
            .planned_cooling = times[index] > curing,
        };
    }
}

/* Observation of the program run at its current time. */
static void program_refresh(furnace_hmi_simulator_t *simulator, furnace_hmi_run_state_t run_state)
{
    const furnace_hmi_sim_program_t *program = &simulator->program;
    furnace_hmi_dashboard_state_t *state = &simulator->state;
    const uint32_t curing = program_curing_seconds(program);
    const uint32_t cooling = program_cooling_seconds(program);
    const bool cooling_phase = simulator->run_seconds >= curing;
    const bool complete = cooling_phase && simulator->cooling_seconds >= cooling;
    const furnace_hmi_value_provenance_t measured = FURNACE_HMI_VALUE_PROVENANCE_MEASURED;
    const furnace_hmi_value_validity_t current = FURNACE_HMI_VALUE_VALIDITY_CURRENT;

    state->mode = FURNACE_HMI_MACHINE_MODE_PROGRAM;
    state->run_state = complete ? FURNACE_HMI_RUN_STATE_COMPLETE : run_state;
    state->run_phase = cooling_phase ? FURNACE_HMI_RUN_PHASE_COOLING : FURNACE_HMI_RUN_PHASE_CURING;
    state->stage_count = program->stage_count;
    uint8_t stage = 0U;
    uint32_t stage_end = 0U;
    for (stage = 0U; stage < program->stage_count; ++stage) {
        stage_end += program->stages[stage].minutes * 60U;
        if (simulator->run_seconds < stage_end) {
            break;
        }
    }
    const bool running = state->run_state == FURNACE_HMI_RUN_STATE_RUNNING;
    if (cooling_phase) {
        state->stage_index = program->stage_count;
        state->stage_kind = FURNACE_HMI_STAGE_KIND_UNKNOWN;
        (void)snprintf(state->stage_name, sizeof(state->stage_name), "%s",
                       complete ? "Cooling complete" : "Automatic cooling");
        state->target_temperature_c = i32_value(SIM_COOLING_END_C, FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, current);
        state->target_delta_c_per_minute = i32_value(0, FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED, current);
        state->heater_demand_percent = i32_value(0, FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, current);
        state->estimated_power_kw_x100 = i32_value(0, FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED, current);
    }
    else {
        const furnace_hmi_sim_stage_t *active = &program->stages[stage];
        const int32_t target = program_stage_target(program, stage);
        state->stage_index = stage;
        state->stage_kind = active->hold ? FURNACE_HMI_STAGE_KIND_HOLD : FURNACE_HMI_STAGE_KIND_HEATING;
        (void)snprintf(state->stage_name, sizeof(state->stage_name),
                       active->hold ? "Hold at %ld \xC2\xB0" "C" : "Heating to %ld \xC2\xB0" "C", (long)target);
        state->target_temperature_c = i32_value(target, FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, current);
        state->target_delta_c_per_minute = i32_value(active->hold ? 0 : active->rate_tenths_c_per_minute / 10,
                                                     FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED, current);
        state->heater_demand_percent = i32_value(running ? (active->hold ? 38 : 72) : 0,
                                                 FURNACE_HMI_VALUE_PROVENANCE_COMMANDED, current);
        state->estimated_power_kw_x100 = i32_value(running ? (active->hold ? 180 : 340) : 0,
                                                   FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED, current);
    }
    const uint32_t curing_elapsed = cooling_phase ? curing : simulator->run_seconds;
    state->elapsed_seconds = i32_value((int32_t)curing_elapsed, measured, current);
    state->remaining_seconds = i32_value((int32_t)(curing - curing_elapsed),
                                         FURNACE_HMI_VALUE_PROVENANCE_CALCULATED, current);
    if (cooling_phase) {
        const uint32_t cooled = simulator->cooling_seconds < cooling ? simulator->cooling_seconds : cooling;
        state->cooling_elapsed_seconds = i32_value((int32_t)cooled, measured, current);
        state->cooling_remaining_seconds = i32_value((int32_t)(cooling - cooled),
                                                     FURNACE_HMI_VALUE_PROVENANCE_ESTIMATED, current);
    }
    state->current_temperature_c = i32_value(
        program_planned_at(program, curing_elapsed + (cooling_phase ? simulator->cooling_seconds : 0U)),
        measured, current);
    state->fan_rpm = i32_value(complete ? 0 : 1450, measured, current);
    state->fan_running = bool_value(!complete, measured, current);
    state->door_closed = bool_value(true, measured, current);
    state->state_revision += 1U;
    program_fill_graph(simulator);
}

static bool program_is_valid(const furnace_hmi_sim_program_t *program)
{
    if (program == NULL || program->stage_count == 0U || program->stage_count > FURNACE_HMI_SIM_MAX_STAGES) {
        return false;
    }
    int32_t start = SIM_START_TEMPERATURE_C;
    for (uint8_t index = 0U; index < program->stage_count; ++index) {
        const furnace_hmi_sim_stage_t *stage = &program->stages[index];
        if (stage->minutes == 0U ||
            (!stage->hold && (stage->target_c <= start || stage->target_c > SIM_MAX_PROGRAM_TEMPERATURE_C ||
                              stage->rate_tenths_c_per_minute < 1 ||
                              stage->rate_tenths_c_per_minute > SIM_MAX_RATE_TENTHS))) {
            return false;
        }
        start = program_stage_target(program, index);
    }
    return true;
}

bool furnace_hmi_simulator_start_program(
    furnace_hmi_simulator_t *simulator,
    const furnace_hmi_sim_program_t *program
)
{
    /* A new start is accepted when idle, after a run, or while one cools. */
    if (simulator == NULL || !program_is_valid(program) ||
        simulator->state.availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        ((simulator->state.run_state == FURNACE_HMI_RUN_STATE_RUNNING ||
          simulator->state.run_state == FURNACE_HMI_RUN_STATE_PAUSED) &&
         simulator->state.run_phase != FURNACE_HMI_RUN_PHASE_COOLING)) {
        return false;
    }
    const furnace_hmi_dashboard_state_t previous = simulator->state;
    furnace_hmi_dashboard_state_initialize(&simulator->state);
    furnace_hmi_dashboard_state_t *state = &simulator->state;
    state->availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state->controller_session_epoch = previous.controller_session_epoch;
    state->state_revision = previous.state_revision;
    state->service_time_hours = previous.service_time_hours;
    state->service_due_at_hours = previous.service_due_at_hours;
    state->service_forecast_completion_hours = previous.service_forecast_completion_hours;
    state->service_run_gate = previous.service_run_gate;
    simulator->next_program_id = simulator->next_program_id < 200U ? 200U : simulator->next_program_id + 1U;
    state->program_id = simulator->next_program_id;
    state->program_revision = 1U;
    (void)snprintf(state->program_name, sizeof(state->program_name), "%s", program->name);
    simulator->program = *program;
    simulator->program_run = true;
    simulator->run_seconds = 0U;
    simulator->cooling_seconds = 0U;
    program_refresh(simulator, FURNACE_HMI_RUN_STATE_RUNNING);
    return true;
}

bool furnace_hmi_simulator_set_paused(furnace_hmi_simulator_t *simulator, bool paused)
{
    if (simulator == NULL || simulator->state.availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        (simulator->state.run_state != FURNACE_HMI_RUN_STATE_RUNNING &&
         simulator->state.run_state != FURNACE_HMI_RUN_STATE_PAUSED)) {
        return false;
    }
    const furnace_hmi_run_state_t run_state = paused ? FURNACE_HMI_RUN_STATE_PAUSED : FURNACE_HMI_RUN_STATE_RUNNING;
    if (simulator->program_run) {
        program_refresh(simulator, run_state);
    }
    else {
        simulator->state.run_state = run_state;
        simulator->state.heater_demand_percent.value = paused ? 0 : 72;
        simulator->state.estimated_power_kw_x100.value = paused ? 0 : 340;
        simulator->state.state_revision += 1U;
    }
    return true;
}

/* The plan up to `minutes`: a stage running at that moment ends there (a ramp
 * at the temperature it had reached). */
static void program_cut(furnace_hmi_sim_program_t *program, uint32_t minutes)
{
    uint32_t elapsed = 0U;
    int32_t start = SIM_START_TEMPERATURE_C;
    uint8_t count = 0U;
    for (uint8_t index = 0U; index < program->stage_count && elapsed < minutes; ++index) {
        furnace_hmi_sim_stage_t stage = program->stages[index];
        const int32_t reached = stage.hold ? start : stage.target_c;
        const uint32_t take = stage.minutes < minutes - elapsed ? stage.minutes : minutes - elapsed;
        if (take < stage.minutes && !stage.hold) {
            stage.target_c = start + (int32_t)((int64_t)(stage.target_c - start) * take / stage.minutes);
        }
        stage.minutes = take;
        program->stages[count++] = stage;
        elapsed += take;
        start = reached;
    }
    program->stage_count = count;
}

bool furnace_hmi_simulator_stop(furnace_hmi_simulator_t *simulator)
{
    if (simulator == NULL || simulator->state.availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        simulator->state.mode == FURNACE_HMI_MACHINE_MODE_IDLE) {
        return false;
    }
    if (simulator->program_run) {
        if (simulator->state.run_phase == FURNACE_HMI_RUN_PHASE_CURING) {
            const uint32_t minutes = simulator->run_seconds / 60U > 0U ? simulator->run_seconds / 60U : 1U;
            program_cut(&simulator->program, minutes);
            simulator->run_seconds = program_curing_seconds(&simulator->program);
            simulator->cooling_seconds = 0U;
            program_refresh(simulator, FURNACE_HMI_RUN_STATE_RUNNING);
        }
        return true;
    }
    return furnace_hmi_simulator_close_run(simulator);
}

bool furnace_hmi_simulator_close_run(furnace_hmi_simulator_t *simulator)
{
    if (simulator == NULL || simulator->state.availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        simulator->state.mode == FURNACE_HMI_MACHINE_MODE_IDLE) {
        return false;
    }
    const uint32_t revision = simulator->state.state_revision;
    const uint32_t next_program_id = simulator->next_program_id;
    furnace_hmi_simulator_set_scenario(simulator, FURNACE_HMI_SIM_SCENARIO_IDLE);
    simulator->next_program_id = next_program_id;
    simulator->state.state_revision = revision + 1U;
    return true;
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
    if (simulator->program_run) {
        if (state->run_state != FURNACE_HMI_RUN_STATE_RUNNING) {
            return;
        }
        const uint32_t curing = program_curing_seconds(&simulator->program);
        uint32_t left = elapsed_seconds;
        if (simulator->run_seconds < curing) {
            const uint32_t step = curing - simulator->run_seconds < left ? curing - simulator->run_seconds : left;
            simulator->run_seconds += step;
            left -= step;
        }
        simulator->cooling_seconds += simulator->run_seconds >= curing ? left : 0U;
        program_refresh(simulator, FURNACE_HMI_RUN_STATE_RUNNING);
        return;
    }
    if (state->availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT ||
        state->run_state != FURNACE_HMI_RUN_STATE_RUNNING) {
        return;
    }

    if (state->run_phase == FURNACE_HMI_RUN_PHASE_COOLING) {
        /* Curing time is frozen; cooling advances and the estimate counts down. */
        state->cooling_elapsed_seconds.value += (int32_t)elapsed_seconds;
        state->cooling_remaining_seconds.value =
            state->cooling_remaining_seconds.value > (int32_t)elapsed_seconds
                ? state->cooling_remaining_seconds.value - (int32_t)elapsed_seconds : 0;
        const int32_t drop = (int32_t)(elapsed_seconds / 30U);
        state->current_temperature_c.value =
            state->current_temperature_c.value - drop > state->target_temperature_c.value
                ? state->current_temperature_c.value - drop : state->target_temperature_c.value;
        track_planned_measurements(state, (uint32_t)state->elapsed_seconds.value +
                                              (uint32_t)state->cooling_elapsed_seconds.value);
        state->state_revision += 1U;
        return;
    }

    state->elapsed_seconds.value += (int32_t)elapsed_seconds;
    if (state->remaining_seconds.value > (int32_t)elapsed_seconds) {
        state->remaining_seconds.value -= (int32_t)elapsed_seconds;
    }

    state->current_temperature_c.value = sim_planned_at((uint32_t)state->elapsed_seconds.value);
    state->state_revision += 1U;

    for (size_t index = 0; index < state->graph_sample_count; ++index) {
        if (state->graph_samples[index].elapsed_seconds > (uint32_t)state->elapsed_seconds.value) {
            state->graph_samples[index].measured_validity = FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE;
            continue;
        }
        state->graph_samples[index].measured_temperature_c =
            state->graph_samples[index].planned_temperature_c;
        state->graph_samples[index].measured_validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT;
    }
}

const furnace_hmi_dashboard_state_t *furnace_hmi_simulator_state(
    const furnace_hmi_simulator_t *simulator
)
{
    return simulator == NULL ? NULL : &simulator->state;
}
