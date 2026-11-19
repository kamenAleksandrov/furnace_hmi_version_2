/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The controller observation mapped to the operator screens' status model
 * (Phase 21 M21-2b): only what the controller reported, nothing made up.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <furnace_hmi/controller_dashboard.h>

#include "controller_client_tests.h"

/* Controller codes (interface and wire format §7.5). */
enum {
    RUN_IDLE = 1, RUN_RUNNING = 2, RUN_PAUSED = 3, RUN_COMPLETED = 4, RUN_STOPPED = 5,
    RUN_FAULTED = 6, RUN_AWAITING_DECISION = 7,
};
#define SESSION_PROGRAM 1U
#define SESSION_MANUAL 2U
#define COOLING_ACTIVE 1U
#define STAGE_HEATING 1U
#define STAGE_HOLD 2U
#define DOOR_CLOSED 1U
#define DOOR_OPEN 2U
#define FAN_SPINNING 2U

static int failures;
static const char *scenario;
static furnace_hmi_controller_observation_t o;
static furnace_hmi_dashboard_state_t s;

#define EXPECT(cond, check)                                                                        \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            failures++;                                                                            \
            controller_client_tests_report((check), scenario);                                     \
        }                                                                                          \
    } while (0)

static void current(const char *name)
{
    scenario = name;
    (void)memset(&o, 0, sizeof(o));
    o.link = FURNACE_HMI_LINK_CONNECTED;
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_CURRENT;
    o.status_valid = true;
    o.status_sequence = 9U;
    o.temperature_decicelsius = 1234;
    o.temperature_validity = FURNACE_HMI_CONTROLLER_MEASURED_VALID;
    o.run_state = RUN_IDLE;
    o.navigation_permitted = true;
}

static void map(void)
{
    furnace_hmi_controller_dashboard_state(&o, &s);
    EXPECT(furnace_hmi_dashboard_state_is_well_formed(&s), "well formed");
}

static void unavailable(void)
{
    scenario = "no controller state";
    (void)memset(&o, 0, sizeof(o));
    map();
    EXPECT(s.availability == FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE, "unavailable");
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_IDLE && s.run_state == FURNACE_HMI_RUN_STATE_IDLE,
           "idle default");
    EXPECT(s.navigation_permitted.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no navigation permission without the controller");
    EXPECT(s.current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no temperature, not an assumed one");
    EXPECT(s.door_closed.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE &&
               s.fan_running.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no equipment state");
    EXPECT(s.graph_sample_count == 0U, "no history made up");
}

static void idle_current(void)
{
    current("idle, current");
    o.door_state = DOOR_CLOSED;
    map();
    EXPECT(s.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT, "current");
    EXPECT(s.current_temperature_c.value == 123 &&
               s.current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT &&
               s.current_temperature_c.provenance == FURNACE_HMI_VALUE_PROVENANCE_MEASURED,
           "123.4 C measured, current");
    EXPECT(s.door_closed.value && s.door_closed.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "door closed from feedback");
    EXPECT(s.fan_running.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "fan feedback not available: unavailable");
    EXPECT(s.target_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no setpoint");
    EXPECT(s.fault_kind == FURNACE_HMI_FAULT_KIND_NONE, "no fault category made up");
    EXPECT(s.state_revision == 9U, "revision from the status sequence");
}

static void rounding(void)
{
    current("rounding to whole degrees");
    o.temperature_decicelsius = 1235;
    map();
    EXPECT(s.current_temperature_c.value == 124, "123.5 -> 124");
    o.temperature_decicelsius = -15;
    map();
    EXPECT(s.current_temperature_c.value == -2, "-1.5 -> -2");
}

static void stale(void)
{
    current("stale link");
    o.link = FURNACE_HMI_LINK_SILENT;
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    o.door_state = DOOR_OPEN;
    o.fan_feedback = FAN_SPINNING;
    map();
    EXPECT(s.availability == FURNACE_HMI_STATE_AVAILABILITY_STALE, "stale");
    EXPECT(s.current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_STALE, "temperature stale");
    EXPECT(!s.door_closed.value && s.door_closed.validity == FURNACE_HMI_VALUE_VALIDITY_STALE,
           "door open, stale");
    EXPECT(s.fan_running.value && s.fan_running.validity == FURNACE_HMI_VALUE_VALIDITY_STALE,
           "fan spinning, stale");
}

static void controller_stale_temperature(void)
{
    current("temperature the controller marks stale");
    o.temperature_validity = FURNACE_HMI_CONTROLLER_MEASURED_STALE;
    map();
    EXPECT(s.current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_STALE, "stale");
    o.temperature_validity = FURNACE_HMI_CONTROLLER_MEASURED_UNAVAILABLE;
    map();
    EXPECT(s.current_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE, "unavailable");
}

static void running_program(void)
{
    current("running program");
    o.run_state = RUN_RUNNING;
    o.session_kind = SESSION_PROGRAM;
    o.execution_active = true;
    o.navigation_permitted = false;
    o.program_id = 3U;
    o.program_revision = 2U;
    (void)memcpy(o.program_name, "Cure 120", sizeof("Cure 120"));
    o.stage_index = 1U;
    o.stage_count = 4U;
    o.stage_kind = STAGE_HOLD;
    o.setpoint_valid = true;
    o.setpoint_decicelsius = 1200;
    o.run_elapsed_s = 3600U;
    map();
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && s.run_state == FURNACE_HMI_RUN_STATE_RUNNING &&
               s.run_phase == FURNACE_HMI_RUN_PHASE_CURING,
           "program running, curing");
    EXPECT(s.program_id == 3U && s.program_revision == 2U && strcmp(s.program_name, "Cure 120") == 0,
           "program identity");
    EXPECT(s.stage_index == 1U && s.stage_count == 4U && s.stage_kind == FURNACE_HMI_STAGE_KIND_HOLD,
           "stage");
    EXPECT(s.target_temperature_c.value == 120 &&
               s.target_temperature_c.provenance == FURNACE_HMI_VALUE_PROVENANCE_COMMANDED &&
               s.target_temperature_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "setpoint commanded");
    EXPECT(s.elapsed_seconds.value == 3600 &&
               s.elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "elapsed");
    EXPECT(!s.run_stopped, "not stopped");
}

static void stopped_cooling(void)
{
    current("stopped, background cooling");
    o.run_state = RUN_STOPPED;
    o.session_kind = SESSION_PROGRAM;
    o.cooling = COOLING_ACTIVE;
    o.cooling_s = 600U;
    o.run_elapsed_s = 4200U;
    map();
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_PROGRAM && s.run_state == FURNACE_HMI_RUN_STATE_COMPLETE &&
               s.run_stopped,
           "finished, stopped by the operator");
    EXPECT(s.run_phase == FURNACE_HMI_RUN_PHASE_COOLING, "cooling");
    EXPECT(s.cooling_elapsed_seconds.value == 600 &&
               s.cooling_elapsed_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "cooling time");
}

static void other_states(void)
{
    current("completed, faulted, awaiting a decision, manual");
    o.session_kind = SESSION_PROGRAM;
    o.run_state = RUN_COMPLETED;
    map();
    EXPECT(s.run_state == FURNACE_HMI_RUN_STATE_COMPLETE && !s.run_stopped &&
               s.run_phase == FURNACE_HMI_RUN_PHASE_NONE,
           "completed");
    o.run_state = RUN_FAULTED;
    o.navigation_permitted = false;
    map();
    EXPECT(s.run_state == FURNACE_HMI_RUN_STATE_FAULT, "faulted");
    EXPECT(!s.navigation_permitted.value &&
               s.navigation_permitted.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "faulted: navigation held by the controller");
    o.run_state = RUN_AWAITING_DECISION;
    o.execution_active = true;
    map();
    EXPECT(s.run_state == FURNACE_HMI_RUN_STATE_PAUSED && s.run_phase == FURNACE_HMI_RUN_PHASE_CURING &&
               s.awaiting_decision,
           "awaiting a decision: paused and named");
    o.session_kind = SESSION_MANUAL;
    o.run_state = RUN_RUNNING;
    map();
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_MANUAL, "manual");
}

static void unknown_codes(void)
{
    current("unknown codes");
    o.run_state = 99U;
    o.session_kind = 7U;
    o.stage_kind = 9U;
    o.door_state = 5U;
    o.fan_feedback = 5U;
    map();
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_IDLE && s.run_state == FURNACE_HMI_RUN_STATE_IDLE,
           "idle");
    EXPECT(s.stage_kind == FURNACE_HMI_STAGE_KIND_UNKNOWN, "stage unknown");
    EXPECT(s.door_closed.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE &&
               s.fan_running.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "equipment unavailable");
}

/* Whenever the controller executes, Running stays authoritative. */
static void execution_keeps_running(void)
{
    static const uint8_t executing[] = {RUN_RUNNING, RUN_PAUSED, RUN_AWAITING_DECISION};

    current("execution keeps Running authoritative");
    o.session_kind = SESSION_PROGRAM;
    o.execution_active = true;
    o.navigation_permitted = false;
    for (size_t i = 0U; i < sizeof(executing); i++) {
        o.run_state = executing[i];
        map();
        EXPECT(s.run_state == FURNACE_HMI_RUN_STATE_RUNNING || s.run_state == FURNACE_HMI_RUN_STATE_PAUSED,
               "running or paused");
        EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_PROGRAM, "program");
    }
}

static void long_name(void)
{
    current("a long UTF-8 name is cut at a character boundary");
    o.run_state = RUN_RUNNING;
    o.session_kind = SESSION_PROGRAM;
    for (size_t i = 0U; i + 2U <= 120U; i += 2U) {
        (void)memcpy(&o.program_name[i], "\xC3\xA9", 2U); /* "é" in UTF-8 */
    }
    map();
    const size_t length = strlen(s.program_name);
    EXPECT(length > 0U && length < FURNACE_HMI_PROGRAM_NAME_CAPACITY, "truncated");
    EXPECT(length % 2U == 0U, "no half character");
}

int controller_dashboard_tests_run(void)
{
    failures = 0;
    unavailable();
    idle_current();
    rounding();
    stale();
    controller_stale_temperature();
    running_program();
    stopped_cooling();
    other_states();
    unknown_codes();
    execution_keeps_running();
    long_name();
    return failures;
}
