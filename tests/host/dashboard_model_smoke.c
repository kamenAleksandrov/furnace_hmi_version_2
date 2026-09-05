/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <string.h>

#include <furnace_hmi/dashboard_model.h>

int main(void)
{
    furnace_hmi_dashboard_state_t state;
    furnace_hmi_dashboard_state_initialize(&state);
    if (state.availability != FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE ||
        !furnace_hmi_dashboard_state_is_well_formed(&state)) {
        (void)fprintf(stderr, "model safe initialization failed.\n");
        return 1;
    }

    (void)snprintf(state.program_name, sizeof(state.program_name), "%s", "Example");
    (void)snprintf(state.stage_name, sizeof(state.stage_name), "%s", "Heating");
    state.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    state.graph_sample_count = FURNACE_HMI_DASHBOARD_MAX_GRAPH_POINTS;
    for (size_t index = 0; index < state.graph_sample_count; ++index) {
        state.graph_samples[index].measured_validity = FURNACE_HMI_VALUE_VALIDITY_CURRENT;
    }
    if (!furnace_hmi_dashboard_state_is_well_formed(&state)) {
        (void)fprintf(stderr, "well-formed model was rejected.\n");
        return 2;
    }

    state.graph_sample_count += 1U;
    if (furnace_hmi_dashboard_state_is_well_formed(&state)) {
        (void)fprintf(stderr, "oversized graph was accepted.\n");
        return 3;
    }

    furnace_hmi_dashboard_state_initialize(&state);
    state.service_run_gate = FURNACE_HMI_SERVICE_RUN_GATE_COUNT;
    if (furnace_hmi_dashboard_state_is_well_formed(&state)) {
        (void)fprintf(stderr, "invalid service run gate was accepted.\n");
        return 4;
    }

    furnace_hmi_dashboard_state_initialize(&state);
    memset(state.program_name, 'x', sizeof(state.program_name));
    if (furnace_hmi_dashboard_state_is_well_formed(&state)) {
        (void)fprintf(stderr, "unterminated program name was accepted.\n");
        return 5;
    }
    return 0;
}
