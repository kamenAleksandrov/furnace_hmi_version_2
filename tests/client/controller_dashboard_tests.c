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

/* A Manual session: the controller's target, rate and time left, and the run it is. */
static void manual_session(void)
{
    current("manual session values");
    o.session_kind = SESSION_MANUAL;
    o.run_state = RUN_RUNNING;
    o.execution_active = true;
    o.run_id = 7U;
    o.manual_target_decicelsius = 1805;
    o.manual_rate_decicelsius_per_min = 25;
    o.manual_remaining_s = 3000U;
    map();
    EXPECT(s.mode == FURNACE_HMI_MACHINE_MODE_MANUAL && s.run_id == 7U, "manual run 7");
    EXPECT(s.manual_target_c.value == 181 &&
               s.manual_target_c.provenance == FURNACE_HMI_VALUE_PROVENANCE_COMMANDED &&
               s.manual_target_c.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "target, whole degrees");
    EXPECT(s.manual_rate_tenths_c_per_minute.value == 25 &&
               s.manual_rate_tenths_c_per_minute.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "rate in tenths");
    EXPECT(s.manual_remaining_seconds.value == 3000 &&
               s.manual_remaining_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "time left");

    current("program run: no manual values");
    o.session_kind = SESSION_PROGRAM;
    o.run_state = RUN_RUNNING;
    o.manual_target_decicelsius = 1805;
    map();
    EXPECT(s.manual_target_c.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE &&
               s.manual_rate_tenths_c_per_minute.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE &&
               s.manual_remaining_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "unavailable");
}

/* Start readiness and the Manual maximum: what a Manual Start needs (owner 2026-11-11). */
static void start_inputs(void)
{
    current("start inputs");
    o.capabilities_valid = true;
    o.manual_max_minutes = 2880U;
    o.store_readiness = 3U; /* READY */
    map();
    EXPECT(s.manual_max_minutes.value == 2880 &&
               s.manual_max_minutes.provenance == FURNACE_HMI_VALUE_PROVENANCE_CONFIGURED &&
               s.manual_max_minutes.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "manual maximum");
    EXPECT(s.start_ready.value && s.start_ready.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT, "ready");

    o.store_readiness = 2U; /* PREPARING */
    map();
    EXPECT(!s.start_ready.value && s.start_ready.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT, "not ready");

    o.capabilities_valid = false;
    map();
    EXPECT(s.manual_max_minutes.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no maximum without capabilities");

    o.store_readiness = 3U;
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
    map();
    EXPECT(s.start_ready.validity == FURNACE_HMI_VALUE_VALIDITY_STALE, "stale readiness is stale");
}

/* The outcome of the operator's commands, the newest of result and local refusal. */
static void command_feedback(void)
{
    current("command feedback");
    map();
    EXPECT(s.command_feedback.state == FURNACE_HMI_FEEDBACK_NONE &&
               s.stop_feedback.state == FURNACE_HMI_FEEDBACK_NONE,
           "none");

    o.command = (furnace_hmi_command_outcome_t){
        .submission = 4U, .kind = FURNACE_HMI_COMMAND_MANUAL_START, .state = FURNACE_HMI_COMMAND_PENDING};
    map();
    EXPECT(s.command_feedback.submission == 4U && s.command_feedback.state == FURNACE_HMI_FEEDBACK_PENDING,
           "pending");
    o.command.state = FURNACE_HMI_COMMAND_REJECTED;
    o.command.reason = 19U;
    map();
    EXPECT(s.command_feedback.state == FURNACE_HMI_FEEDBACK_REJECTED && s.command_feedback.reason == 19U,
           "rejected with the controller's reason");
    o.command.state = FURNACE_HMI_COMMAND_REFUSED;
    o.command.reason = 10U;
    map();
    EXPECT(s.command_feedback.state == FURNACE_HMI_FEEDBACK_REFUSED && s.command_feedback.reason == 10U,
           "refused by the service");
    o.command.state = FURNACE_HMI_COMMAND_UNRESOLVED;
    o.command.unresolved_cause = FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST;
    map();
    EXPECT(s.command_feedback.state == FURNACE_HMI_FEEDBACK_UNRESOLVED &&
               s.command_feedback.reason == FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST,
           "unresolved with its cause");
    o.command.state = FURNACE_HMI_COMMAND_ACCEPTED;
    map();
    EXPECT(s.command_feedback.state == FURNACE_HMI_FEEDBACK_ACCEPTED, "accepted");

    o.command_refusal = (furnace_hmi_command_refusal_t){
        .submission = 5U, .kind = FURNACE_HMI_COMMAND_PAUSE, .reason = FURNACE_HMI_COMMAND_LOCAL_BUSY};
    map();
    EXPECT(s.command_feedback.submission == 5U && s.command_feedback.state == FURNACE_HMI_FEEDBACK_NOT_SENT &&
               s.command_feedback.reason == FURNACE_HMI_NOT_SENT_BUSY,
           "a newer local refusal: not sent");
    o.command_refusal.reason = FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP;
    map();
    EXPECT(s.command_feedback.reason == FURNACE_HMI_NOT_SENT_SUPERSEDED_BY_STOP, "superseded by Stop");
    o.command_refusal.reason = FURNACE_HMI_COMMAND_LOCAL_NOT_CURRENT;
    map();
    EXPECT(s.command_feedback.reason == FURNACE_HMI_NOT_SENT_LINK, "link not current");
    o.command_refusal.submission = 3U;
    map();
    EXPECT(s.command_feedback.submission == 4U && s.command_feedback.state == FURNACE_HMI_FEEDBACK_ACCEPTED,
           "an older refusal does not hide the outcome");

    o.stop = (furnace_hmi_command_outcome_t){
        .submission = 6U, .kind = FURNACE_HMI_COMMAND_STOP, .state = FURNACE_HMI_COMMAND_UNRESOLVED,
        .unresolved_cause = FURNACE_HMI_COMMAND_UNRESOLVED_STOP};
    map();
    EXPECT(s.stop_feedback.submission == 6U && s.stop_feedback.state == FURNACE_HMI_FEEDBACK_UNRESOLVED &&
               s.stop_feedback.reason == FURNACE_HMI_COMMAND_UNRESOLVED_STOP,
           "stop slot apart");
    EXPECT(s.command_feedback.submission == 4U, "ordinary slot unchanged");

    current("feedback while the link is lost");
    o.availability = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    o.status_valid = false;
    o.command = (furnace_hmi_command_outcome_t){
        .submission = 8U, .kind = FURNACE_HMI_COMMAND_PAUSE, .state = FURNACE_HMI_COMMAND_UNRESOLVED,
        .unresolved_cause = FURNACE_HMI_COMMAND_UNRESOLVED_LINK_LOST};
    map();
    EXPECT(s.command_feedback.submission == 8U && s.command_feedback.state == FURNACE_HMI_FEEDBACK_UNRESOLVED,
           "still reported");
}

/* The controller's fault entries as the screens show them (ADR-0018). */
static void fault_entries(void)
{
    static const uint8_t recoveries[] = {1U, 2U, 3U, 5U, 6U, 4U, 99U};
    static const furnace_hmi_fault_recovery_t mapped[] = {
        FURNACE_HMI_FAULT_RECOVERY_NOT_REQUIRED, FURNACE_HMI_FAULT_RECOVERY_AWAIT_CLEAR,
        FURNACE_HMI_FAULT_RECOVERY_RESUME, FURNACE_HMI_FAULT_RECOVERY_PROTECTIVE_EXTERNAL,
        FURNACE_HMI_FAULT_RECOVERY_OPERATOR, FURNACE_HMI_FAULT_RECOVERY_UNKNOWN,
        FURNACE_HMI_FAULT_RECOVERY_UNKNOWN,
    };

    current("fault entries");
    map();
    EXPECT(!s.faults_known && s.fault_count == 0U, "unknown without a list");
    o.faults_valid = true;
    o.fault_count = (uint8_t)sizeof(recoveries);
    for (uint8_t i = 0U; i < o.fault_count; i++) {
        o.faults[i] = (furnace_hmi_controller_fault_t){
            .fault_id = (uint8_t)(i + 1U), .source = 2U, .fault_class = 3U,
            .condition_active = (i == 0U), .latched = (i == 1U), .acknowledged = (i == 2U),
            .recovery = recoveries[i], .continuable = (i == 3U)};
    }
    map();
    EXPECT(s.faults_known && s.fault_count == o.fault_count && s.fault_total == o.fault_count,
           "every entry");
    for (uint8_t i = 0U; i < s.fault_count; i++) {
        EXPECT(s.faults[i].fault_id == i + 1U && s.faults[i].source == 2U &&
                   s.faults[i].recovery == mapped[i],
               "identity and recovery as received");
    }
    EXPECT(s.faults[0].active && !s.faults[1].active && s.faults[1].latched &&
               s.faults[2].acknowledged && s.faults[3].continuable && !s.faults[0].continuable,
           "flags");

    o.fault_count = FURNACE_HMI_CONTROLLER_MAX_FAULTS;
    map();
    EXPECT(s.fault_count == FURNACE_HMI_DASHBOARD_MAX_FAULTS &&
               s.fault_total == FURNACE_HMI_CONTROLLER_MAX_FAULTS,
           "bounded, with the total kept");

    o.faults_outdated = true;
    map();
    EXPECT(s.faults_known && s.faults_outdated, "an outdated list is still shown until the new one");
    o.faults_valid = false;
    map();
    EXPECT(!s.faults_known && s.fault_count == 0U, "no list, no entries");
}

/* The decision: its time left and the fault it is about (ADR-0019). */
static void decision(void)
{
    current("awaiting a decision");
    o.session_kind = SESSION_MANUAL;
    o.run_state = RUN_AWAITING_DECISION;
    o.execution_active = true;
    o.decision_remaining_s = 95U;
    o.continuable_ids = (1U << 10) | (1U << 11);
    map();
    EXPECT(s.awaiting_decision && s.decision_remaining_seconds.value == 95 &&
               s.decision_remaining_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_CURRENT,
           "time left");
    EXPECT(s.decision_fault_id == 10U, "the lowest continuable fault");

    o.run_state = RUN_RUNNING;
    map();
    EXPECT(!s.awaiting_decision && s.decision_fault_id == 0U &&
               s.decision_remaining_seconds.validity == FURNACE_HMI_VALUE_VALIDITY_UNAVAILABLE,
           "no decision while running");
}

/* HMI-FAULT-CATEGORY: the screens' alert kinds from the controller's inhibiting faults. */
static void fault_kinds(void)
{
    static const struct {
        uint8_t id;
        furnace_hmi_fault_kind_t kind;
    } cases[] = {
        {16U, FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE}, {1U, FURNACE_HMI_FAULT_KIND_SENSOR},
        {2U, FURNACE_HMI_FAULT_KIND_SENSOR},           {3U, FURNACE_HMI_FAULT_KIND_SENSOR},
        {11U, FURNACE_HMI_FAULT_KIND_TARGET_NOT_REACHED}, {8U, FURNACE_HMI_FAULT_KIND_NONE},
        {10U, FURNACE_HMI_FAULT_KIND_NONE},             {18U, FURNACE_HMI_FAULT_KIND_NONE},
        {7U, FURNACE_HMI_FAULT_KIND_NONE},              {6U, FURNACE_HMI_FAULT_KIND_NONE},
    };

    current("fault kinds");
    o.session_kind = SESSION_PROGRAM;
    o.run_state = RUN_FAULTED;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); i++) {
        o.inhibit_ids = 1U << cases[i].id;
        map();
        EXPECT(s.fault_kind == cases[i].kind && s.fault_id == cases[i].id, "kind and fault");
    }
    o.inhibit_ids = (1U << 16) | (1U << 3);
    map();
    EXPECT(s.fault_kind == FURNACE_HMI_FAULT_KIND_OVER_TEMPERATURE && s.fault_id == 16U,
           "over-temperature before a sensor fault");
    o.inhibit_ids = (1U << 7) | (1U << 6) | (1U << 2);
    map();
    EXPECT(s.fault_id == 7U, "the protective trip leads");
    o.inhibit_ids = (1U << 11) | (1U << 6);
    map();
    EXPECT(s.fault_id == 6U, "otherwise the lowest fault ID");
    o.run_state = RUN_RUNNING;
    o.inhibit_ids = 1U << 16;
    map();
    EXPECT(s.fault_kind == FURNACE_HMI_FAULT_KIND_NONE && s.fault_id == 0U, "no kind while running");
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
    manual_session();
    start_inputs();
    command_feedback();
    fault_entries();
    decision();
    fault_kinds();
    return failures;
}
