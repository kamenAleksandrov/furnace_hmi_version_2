/* SPDX-License-Identifier: Apache-2.0 */

/*
 * SIMULATION ONLY (Phase 21 M21-5). The HMI's controller link, its link thread
 * and command mailbox included, against the real controller: the furnace
 * firmware's tests/sim/link_pty program, started by
 * tools/controller_workflow_check.py. Each workflow submits operator commands as
 * a screen does and checks the controller's results and status; faults are the
 * controller's own, from inputs changed through its scenario channel
 * (--scenario_port, firmware PR #53). Temperatures are the controller's
 * simulation, never furnace measurements. Not run by Twister: it needs the
 * controller.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <nsi_host_trampolines.h>

#include <furnace_hmi/controller_command.h>
#include <furnace_hmi/controller_observation.h>

#include "cmdline.h"
#include "controller_link.h"
#include "soc.h"

/* Controller codes (controller interface). */
#define RUN_RUNNING 2U
#define RUN_PAUSED 3U
#define RUN_STOPPED 5U
#define RUN_FAULTED 6U
#define RUN_AWAITING_DECISION 7U
#define SESSION_MANUAL 2U
#define MEASUREMENT_QUALIFIED 1U
#define STORE_READY 3U
#define FAULT_MEASUREMENT_INVALID 1U
#define FAULT_INHIBIT 6U
#define FAULT_PROTECTIVE_TRIP 7U
#define FAULT_LAG 10U
#define CLASS_PROTECTIVE_TRIP 4U
#define REJECT_NOT_PAUSED 12U
#define REJECT_RESUME_BLOCKED 13U
#define REJECT_FAULT_LATCHED 14U
#define REJECT_NOT_AWAITING_DECISION 15U
#define RECOVERY_PROTECTIVE_RESET_EXTERNAL 4U
#define RECOVERY_CONDITION_ACTIVE 2U
#define RECOVERY_DIAGNOSTICS_INVALID 6U
#define RECOVERY_RECOVERED 7U
#define REJECT_NOT_ACTIVE 11U

#define WAIT_MS 20000U
#define POLL_MS 20

static const char *scenario_port;
static int scenario_fd = -1;
static furnace_hmi_controller_observation_t obs;
static uint32_t seen;

static void add_options(void)
{
    static struct args_struct_t options[] = {
        {.option = "scenario_port",
         .name = "path",
         .type = 's',
         .dest = (void *)&scenario_port,
         .descript = "The controller's scenario pseudo-terminal (fault injection)"},
        ARG_TABLE_ENDMARKER,
    };

    native_add_command_line_opts(options);
}

NATIVE_TASK(add_options, PRE_BOOT_1, 1);

/* Take the newest observation, if one was published. */
static void refresh(void)
{
    (void)furnace_hmi_controller_link_take(&obs, &seen);
}

/* Wait until the condition holds on a published observation. */
static bool wait_until(bool (*holds)(void))
{
    const int64_t end = k_uptime_get() + WAIT_MS;

    while (k_uptime_get() < end) {
        refresh();
        if (holds()) {
            return true;
        }
        k_msleep(POLL_MS);
    }
    return false;
}

/* One scenario command to the controller, and its answer. */
static void scenario(const char *line)
{
    char reply[16] = {0};
    size_t n = 0U;
    const int64_t end = k_uptime_get() + WAIT_MS;

    zassert_true(scenario_fd >= 0, "no --scenario_port");
    zassert_equal(nsi_host_write(scenario_fd, line, strlen(line)), (long)strlen(line));
    zassert_equal(nsi_host_write(scenario_fd, "\n", 1U), 1L);
    while (n < sizeof(reply) - 1U) {
        char c;

        /* Non-blocking: nothing yet is -1, and the clock runs on while waiting. */
        if (nsi_host_read(scenario_fd, &c, 1U) != 1L) {
            zassert_true(k_uptime_get() < end, "no answer to scenario '%s'", line);
            k_msleep(POLL_MS);
            continue;
        }
        if (c == '\n') {
            break;
        }
        reply[n++] = c;
    }
    zassert_str_equal(reply, "ok", "scenario '%s' answered '%s'", line, reply);
    printk("workflow: scenario %s\n", line);
}

/*
 * Submit as a screen does, through the link thread's mailbox, and wait for the
 * outcome: the controller's result, or the client's local refusal.
 */
static furnace_hmi_command_outcome_t run(furnace_hmi_command_t command)
{
    const bool stop = furnace_hmi_command_is_stop(command.kind);
    const int64_t end = k_uptime_get() + WAIT_MS;
    uint32_t submission;

    while ((submission = furnace_hmi_controller_link_submit(&command)) == 0U) {
        k_msleep(POLL_MS);
    }
    while (k_uptime_get() < end) {
        const furnace_hmi_command_outcome_t *o;
        const furnace_hmi_command_refusal_t *r;

        refresh();
        o = stop ? &obs.stop : &obs.command;
        r = stop ? &obs.stop_refusal : &obs.command_refusal;
        zassert_false(r->submission == submission, "refused before sending: %u", r->reason);
        if (o->submission == submission && o->state != FURNACE_HMI_COMMAND_PENDING) {
            printk("workflow: command %u -> state %u reason %u run %u attempts %u\n",
                   command.kind, o->state, o->reason, o->run_id, o->attempts);
            return *o;
        }
        k_msleep(POLL_MS);
    }
    zassert_unreachable("no outcome for command %u", command.kind);
    return (furnace_hmi_command_outcome_t){0};
}

static uint32_t awaited_stop;

static bool stop_answered(void)
{
    return obs.stop.submission == awaited_stop && obs.stop.state != FURNACE_HMI_COMMAND_PENDING;
}

static bool current_and_qualified(void)
{
    return obs.link == FURNACE_HMI_LINK_CONNECTED &&
           obs.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT && obs.status_valid &&
           obs.measurement_state == MEASUREMENT_QUALIFIED;
}

static bool store_ready(void)
{
    return obs.store_readiness == STORE_READY;
}

static uint32_t expected_run;

static bool manual_running(void)
{
    return obs.run_state == RUN_RUNNING && obs.session_kind == SESSION_MANUAL &&
           obs.run_id == expected_run && obs.execution_active;
}

static bool paused(void)
{
    return obs.run_state == RUN_PAUSED && obs.run_id == expected_run;
}

static bool stopped(void)
{
    return obs.run_state == RUN_STOPPED && !obs.execution_active;
}

static bool faulted(void)
{
    return obs.run_state == RUN_FAULTED && obs.execution_active && !obs.navigation_permitted;
}

static bool measurement_fault(void)
{
    return (obs.inhibit_ids & (1U << FAULT_MEASUREMENT_INVALID)) != 0U;
}

static bool not_executing(void)
{
    return !obs.execution_active;
}

static bool all_acknowledged(void)
{
    return obs.faults_valid && !obs.faults_outdated && obs.unacknowledged_faults == 0U;
}

static bool nothing_latched(void)
{
    return obs.latched_class == 0U;
}

/* Ready for a Start: current, qualified, the store written (Start readiness, rule 1 and 2). */
static void prepare(void)
{
    zassert_true(wait_until(current_and_qualified), "not current and qualified");
    zassert_equal(run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_START_PREPARE}).state,
                  FURNACE_HMI_COMMAND_ACCEPTED);
    zassert_true(wait_until(store_ready), "store readiness %u", obs.store_readiness);
}

/* As the screens start it: the controller's longest session (owner 2026-11-11, M21-5b). */
static uint32_t manual_start(void)
{
    zassert_true(obs.capabilities_valid && obs.manual_max_minutes > 0U, "no Manual maximum");
    const furnace_hmi_command_outcome_t o = run((furnace_hmi_command_t){
        .kind = FURNACE_HMI_COMMAND_MANUAL_START,
        .target_decicelsius = 600,
        .rate_decicelsius_per_min = 20,
        .session_minutes = obs.manual_max_minutes,
    });

    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Manual Start: reason %u", o.reason);
    zassert_not_equal(o.run_id, 0U);
    expected_run = o.run_id;
    zassert_true(wait_until(manual_running), "not running: state %u", obs.run_state);
    return o.run_id;
}

static void stop(uint32_t run_id)
{
    const furnace_hmi_command_outcome_t o =
        run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_STOP, .run_id = run_id});

    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Stop: reason %u", o.reason);
    zassert_true(wait_until(not_executing), "still executing after Stop");
}

/* Acknowledge every unacknowledged entry, as the operator does from the list. */
static void acknowledge_all(void)
{
    refresh();
    for (uint8_t i = 0U; i < obs.fault_count; i++) {
        const furnace_hmi_controller_fault_t f = obs.faults[i];

        if (!f.acknowledged) {
            zassert_equal(run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_FAULT_ACKNOWLEDGE,
                                                      .fault_id = f.fault_id,
                                                      .source = f.source})
                              .state,
                          FURNACE_HMI_COMMAND_ACCEPTED, "acknowledge fault %u", f.fault_id);
        }
    }
    zassert_true(wait_until(all_acknowledged), "unacknowledged %u", obs.unacknowledged_faults);
}

/*
 * Recover every latched entry, as the operator does from the list. Recovery
 * needs the measurement qualified again, so a refusal is tried again (each try
 * is a new operator action, never an automatic resend).
 */
static void recover_all(void)
{
    refresh();
    for (uint8_t i = 0U; i < obs.fault_count; i++) {
        const furnace_hmi_controller_fault_t f = obs.faults[i];
        furnace_hmi_command_outcome_t o;
        int tries = 0;

        if (!f.latched) {
            continue;
        }
        do {
            k_msleep(1000);
            o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_FAULT_RECOVERY,
                                            .fault_id = f.fault_id,
                                            .source = f.source});
            /* Only "not yet" is tried again: the condition, or a measurement not qualified. */
            zassert_true(o.state == FURNACE_HMI_COMMAND_ACCEPTED ||
                             (o.state == FURNACE_HMI_COMMAND_REJECTED &&
                              (o.reason == RECOVERY_CONDITION_ACTIVE ||
                               o.reason == RECOVERY_DIAGNOSTICS_INVALID)),
                         "recovery of %u: state %u reply %u", f.fault_id, o.state, o.reason);
        } while (o.state != FURNACE_HMI_COMMAND_ACCEPTED && ++tries < 15);
        zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "recovery of %u: found %u reply %u",
                      f.fault_id, o.fault_found, o.reason);
        zassert_equal(o.reason, RECOVERY_RECOVERED);
    }
    zassert_true(wait_until(nothing_latched), "still latched: class %u", obs.latched_class);
}

static void *setup(void)
{
    if (scenario_port != NULL) {
        scenario_fd = nsi_host_open(scenario_port, 2 /* O_RDWR */ | 04000 /* O_NONBLOCK */);
    }
    zassert_equal(furnace_hmi_controller_link_start(), 0);
    return NULL;
}

ZTEST_SUITE(controller_workflows, NULL, setup, NULL, NULL, NULL);

/* Manual: Start, retarget, length, Pause, Resume, Stop; a command on a stopped run is rejected. */
ZTEST(controller_workflows, test_a_manual_session)
{
    furnace_hmi_command_outcome_t o;
    uint32_t r;

    prepare();
    r = manual_start();
    o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_MANUAL_RETARGET,
                                    .run_id = r,
                                    .target_decicelsius = 500,
                                    .rate_decicelsius_per_min = 30});
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "retarget: reason %u", o.reason);
    o = run((furnace_hmi_command_t){
        .kind = FURNACE_HMI_COMMAND_MANUAL_SET_LENGTH, .run_id = r, .session_minutes = 90U});
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "length: reason %u", o.reason);
    o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_PAUSE, .run_id = r});
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Pause: reason %u", o.reason);
    zassert_true(wait_until(paused), "not paused: state %u", obs.run_state);
    o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_RESUME, .run_id = r});
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Resume: reason %u", o.reason);
    zassert_true(wait_until(manual_running), "not running after Resume");
    stop(r);
    zassert_true(wait_until(stopped), "not stopped: state %u", obs.run_state);
    o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_RESUME, .run_id = r});
    zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.reason == REJECT_NOT_PAUSED,
                 "Resume after Stop: state %u reason %u", o.state, o.reason);
}

/*
 * Start, then Stop before the link thread took either (the test thread runs
 * above it): the Start is dropped and reported, the Stop sent; nothing runs
 * (review F2: a Stop is never overtaken by an earlier command).
 */
ZTEST(controller_workflows, test_ab_stop_supersedes_an_unsent_start)
{
    const furnace_hmi_command_t start = {
        .kind = FURNACE_HMI_COMMAND_MANUAL_START,
        .target_decicelsius = 600,
        .rate_decicelsius_per_min = 20,
        .session_minutes = 60U,
    };
    const furnace_hmi_command_t stop_command = {.kind = FURNACE_HMI_COMMAND_STOP};
    uint32_t start_submission;
    uint32_t stop_submission;

    prepare();
    start_submission = furnace_hmi_controller_link_submit(&start);
    stop_submission = furnace_hmi_controller_link_submit(&stop_command);
    zassert_true(start_submission != 0U && stop_submission != 0U);
    awaited_stop = stop_submission;
    zassert_true(wait_until(stop_answered), "no Stop outcome");
    zassert_true(obs.stop.state == FURNACE_HMI_COMMAND_REJECTED &&
                     obs.stop.reason == REJECT_NOT_ACTIVE,
                 "Stop with nothing running: state %u reason %u", obs.stop.state, obs.stop.reason);
    zassert_true(obs.command_refusal.submission == start_submission &&
                     obs.command_refusal.reason == FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP,
                 "the Start was not dropped: refusal %u", obs.command_refusal.reason);
    k_msleep(1000);
    refresh();
    zassert_false(obs.execution_active, "something runs");
    zassert_false(obs.command.submission == start_submission, "the Start was sent");
}

/*
 * A sensor fault during a Manual session: the controller's fault reaches the
 * HMI; after Stop the operator acknowledges, and recovers once the measurement
 * is qualified again.
 */
ZTEST(controller_workflows, test_b_sensor_fault_and_recovery)
{
    furnace_hmi_command_outcome_t o;
    uint32_t r;

    prepare();
    r = manual_start();
    scenario("sensor not_finite");
    zassert_true(wait_until(measurement_fault), "no measurement fault: inhibit_ids %x",
                 obs.inhibit_ids);
    zassert_true(wait_until(faulted), "not faulted: state %u", obs.run_state);
    o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_PAUSE, .run_id = r});
    zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.reason == REJECT_FAULT_LATCHED,
                 "Pause on a faulted run: state %u reason %u", o.state, o.reason);
    scenario("sensor measurement");
    stop(r);
    acknowledge_all();
    recover_all();
}

static bool inhibit_fault(void)
{
    return (obs.inhibit_ids & (1U << FAULT_INHIBIT)) != 0U;
}

static bool interlock_pause(void)
{
    return inhibit_fault() && paused() && !obs.navigation_permitted && obs.latched_class == 0U;
}

static bool protective_trip(void)
{
    return (obs.inhibit_ids & (1U << FAULT_PROTECTIVE_TRIP)) != 0U &&
           obs.latched_class == CLASS_PROTECTIVE_TRIP && faulted();
}

static furnace_hmi_command_outcome_t run_command(furnace_hmi_command_kind_t kind, uint32_t run_id)
{
    return run((furnace_hmi_command_t){.kind = kind, .run_id = run_id});
}

/*
 * An interlock during a Manual session: the controller pauses the run with a
 * recoverable INHIBIT fault, nothing latched. Resume is blocked while it is
 * active and accepted once it clears; Continue belongs to a decision, which an
 * interlock does not ask for.
 */
ZTEST(controller_workflows, test_c_interlock)
{
    furnace_hmi_command_outcome_t o;
    uint32_t r;

    prepare();
    r = manual_start();
    scenario("interlock on");
    zassert_true(wait_until(interlock_pause), "state %u inhibit %x latched %u", obs.run_state,
                 obs.inhibit_ids, obs.latched_class);
    o = run_command(FURNACE_HMI_COMMAND_RESUME, r);
    zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.reason == REJECT_RESUME_BLOCKED,
                 "Resume during the interlock: state %u reason %u", o.state, o.reason);
    o = run_command(FURNACE_HMI_COMMAND_CONTINUE, r);
    zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED &&
                     o.reason == REJECT_NOT_AWAITING_DECISION,
                 "Continue without a decision: state %u reason %u", o.state, o.reason);
    scenario("interlock off");
    /* The operator resumes once the condition is clear (a new action each time). */
    for (int tries = 0; tries < 10; tries++) {
        k_msleep(500);
        o = run_command(FURNACE_HMI_COMMAND_RESUME, r);
        if (o.state == FURNACE_HMI_COMMAND_ACCEPTED) {
            break;
        }
        /* Only a blocked Resume is tried again, as a new operator action. */
        zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.reason == REJECT_RESUME_BLOCKED,
                     "Resume after the interlock: state %u reason %u", o.state, o.reason);
    }
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Resume after the interlock: %u",
                  o.reason);
    zassert_true(wait_until(manual_running), "not running after Resume");
    stop(r);
    acknowledge_all();
    recover_all();
}

static bool awaiting_decision(void)
{
    return obs.run_state == RUN_AWAITING_DECISION && obs.decision_remaining_s > 0U;
}

static bool continued(void)
{
    return obs.run_state == RUN_RUNNING;
}

/*
 * A quality decision (M21-5c): the reading 30 °C below a Manual ramp raises LAG,
 * which the simulated controller's fixture policy makes continuable (firmware
 * scenario `offset`, HMI-DECISION-TRIGGER). The controller awaits a decision with
 * a countdown; Continue is accepted and the session runs again.
 */
ZTEST(controller_workflows, test_cc_quality_decision)
{
    furnace_hmi_command_outcome_t o;
    uint32_t r;

    prepare();
    r = manual_start();
    scenario("offset -300");
    zassert_true(wait_until(awaiting_decision), "state %u continuable %x", obs.run_state,
                 obs.continuable_ids);
    zassert_true((obs.continuable_ids & (1U << FAULT_LAG)) != 0U, "LAG not continuable: %x",
                 obs.continuable_ids);
    o = run_command(FURNACE_HMI_COMMAND_CONTINUE, r);
    zassert_equal(o.state, FURNACE_HMI_COMMAND_ACCEPTED, "Continue: state %u reason %u", o.state,
                  o.reason);
    zassert_true(wait_until(continued), "not running after Continue: state %u", obs.run_state);
    scenario("offset 0");
    stop(r);
    acknowledge_all();
    recover_all();
}

/*
 * A protective trip during a Manual session: the run is faulted with a latched
 * protective trip; Pause is refused, Stop accepted. Recovery is refused as
 * PROTECTIVE_RESET_EXTERNAL during the trip and after it: a protective trip is
 * reset outside software (controller interface). Last, because the latch stays
 * for this controller session.
 */
ZTEST(controller_workflows, test_d_protective_trip)
{
    furnace_hmi_command_outcome_t o;
    uint32_t r;

    prepare();
    r = manual_start();
    scenario("protection on");
    zassert_true(wait_until(protective_trip), "state %u inhibit %x latched %u", obs.run_state,
                 obs.inhibit_ids, obs.latched_class);
    o = run_command(FURNACE_HMI_COMMAND_PAUSE, r);
    zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.reason == REJECT_FAULT_LATCHED,
                 "Pause during the trip: state %u reason %u", o.state, o.reason);
    stop(r);
    acknowledge_all();
    for (int phase = 0; phase < 2; phase++) {
        if (phase == 1) {
            scenario("protection off");
            k_msleep(1000);
        }
        o = run((furnace_hmi_command_t){.kind = FURNACE_HMI_COMMAND_FAULT_RECOVERY,
                                        .fault_id = FAULT_PROTECTIVE_TRIP,
                                        .source = 0U});
        zassert_true(o.state == FURNACE_HMI_COMMAND_REJECTED && o.fault_found &&
                         o.reason == RECOVERY_PROTECTIVE_RESET_EXTERNAL,
                     "recovery %s the trip: state %u reply %u", phase ? "after" : "during",
                     o.state, o.reason);
    }
    refresh();
    zassert_equal(obs.latched_class, CLASS_PROTECTIVE_TRIP, "the trip's latch stays");
}
