/* SPDX-License-Identifier: Apache-2.0 */

/*
 * SIMULATION ONLY (Phase 21 M21-8, WI-H07). The M3 operator workflows through
 * the HMI's own screens: an operator's taps on the shared operator screens,
 * the application's GUI pass (gui_host.c), its command mapping and controller
 * link, against the real controller, the furnace firmware's tests/sim/link_pty
 * program started by tools/controller_workflow_check.py --suite operator.
 *
 * Each step taps what an operator taps, found by what it shows, then waits for
 * what the controller reports and checks what the screens show. Pages are
 * reached through the screens' own navigation, which the navigation rail
 * calls. Faults are the controller's own, from inputs changed through its
 * scenario channel; link loss is the runner's relay holding the bytes
 * (--relay_port). Temperatures are the controller's simulation, never furnace
 * measurements. Not run by Twister: it needs the controller.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <nsi_host_trampolines.h>

#include <furnace_hmi/screen_probe.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

#include "cmdline.h"
#include "controller_link.h"
#include "gui_host.h"
#include "soc.h"

/* Controller codes (controller interface). */
#define RUN_RUNNING 2U
#define RUN_PAUSED 3U
#define RUN_FAULTED 6U
#define RUN_AWAITING_DECISION 7U
#define SESSION_PROGRAM 1U
#define SESSION_MANUAL 2U
#define MEASUREMENT_QUALIFIED 1U
#define COOLING_ACTIVE 1U
#define CLASS_PROTECTIVE_TRIP 4U

#define WAIT_MS 30000U
#define PASS_MS 10

static const char *scenario_port;
static const char *relay_port;
static int scenario_fd = -1;
static int relay_fd = -1;
/* The screens and what they show; the GUI context's state, as in the application. */
static furnace_hmi_gui_host_t host;

#define OBS (host.observation)
#define SCREENS (host.screens)

static void add_options(void)
{
    static struct args_struct_t options[] = {
        {.option = "scenario_port",
         .name = "path",
         .type = 's',
         .dest = (void *)&scenario_port,
         .descript = "The controller's scenario pseudo-terminal (fault injection)"},
        {.option = "relay_port",
         .name = "path",
         .type = 's',
         .dest = (void *)&relay_port,
         .descript = "The runner's relay control (hold and release the link)"},
        ARG_TABLE_ENDMARKER,
    };

    native_add_command_line_opts(options);
}

NATIVE_TASK(add_options, PRE_BOOT_1, 1);

/* ---- The GUI context ---- */

/* One pass of the application's GUI context, then a short sleep. */
static void pump(void)
{
    furnace_hmi_gui_host_pass(&host, k_uptime_get_32());
    (void)lv_timer_handler();
    k_msleep(PASS_MS);
}

static void pump_for(uint32_t ms)
{
    const int64_t end = k_uptime_get() + ms;

    while (k_uptime_get() < end) {
        pump();
    }
}

/* Pump until the condition holds; false at the deadline. */
static bool wait_for(bool (*holds)(void), uint32_t ms)
{
    const int64_t end = k_uptime_get() + ms;

    while (k_uptime_get() < end) {
        pump();
        if (holds()) {
            return true;
        }
    }
    return false;
}

static const char *text(furnace_hmi_ui_string_id_t id)
{
    return furnace_hmi_ui_string(id);
}

/* The first label's text under object, for a failure message. */
static const char *first_text(const lv_obj_t *object)
{
    if (object == NULL) {
        return "";
    }
    if (lv_obj_check_type(object, &lv_label_class)) {
        return lv_label_get_text(object);
    }
    for (uint32_t i = 0U; i < lv_obj_get_child_count(object); i++) {
        const char *found = first_text(lv_obj_get_child(object, (int32_t)i));

        if (found[0] != '\0') {
            return found;
        }
    }
    return "";
}

static bool inside(const lv_obj_t *object, const lv_obj_t *container)
{
    for (; object != NULL; object = lv_obj_get_parent(object)) {
        if (object == container) {
            return true;
        }
    }
    return false;
}

/*
 * An operator's tap on a shown, enabled button; the GUI pass then hands its
 * request on. A dialog covers the page: then only its own buttons can be tapped.
 */
static void tap(lv_obj_t *button, const char *what)
{
    zassert_not_null(button, "no button for %s", what);
    zassert_true(SCREENS.modal == NULL || inside(button, SCREENS.modal),
                 "%s is under the dialog '%s'", what, first_text(SCREENS.modal));
    zassert_true(furnace_hmi_probe_enabled(button), "%s is not offered: hidden %d disabled %d modal %d",
                 what, lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN),
                 lv_obj_has_state(button, LV_STATE_DISABLED), SCREENS.modal != NULL);
    printk("workflow: tap %s\n", what);
    furnace_hmi_probe_click(button);
    pump();
}

/*
 * A Manual page widget, only while the Manual page is shown: the screens keep
 * the pointers of a page they have deleted, and they must not be touched then.
 */
static lv_obj_t *manual(lv_obj_t *widget)
{
    return SCREENS.page == FURNACE_HMI_OPERATOR_PAGE_MANUAL ? widget : NULL;
}

static lv_obj_t *page_button(furnace_hmi_ui_string_id_t id)
{
    return furnace_hmi_probe_button_with_text(SCREENS.root, text(id));
}

static lv_obj_t *modal_button(furnace_hmi_ui_string_id_t id)
{
    return SCREENS.modal == NULL ? NULL : furnace_hmi_probe_button_with_text(SCREENS.modal, text(id));
}

static bool page_shows(const char *what)
{
    return furnace_hmi_probe_tree_contains(SCREENS.root, what);
}

static bool modal_shows(const char *what)
{
    return SCREENS.modal != NULL && furnace_hmi_probe_tree_contains(SCREENS.modal, what);
}

/* An information dialog: closed with its first button. */
static void dismiss(void)
{
    if (SCREENS.modal != NULL) {
        tap(furnace_hmi_probe_first_button(SCREENS.modal), "the dialog's button");
    }
}

static void go_to(furnace_hmi_operator_page_t page)
{
    zassert_true(furnace_hmi_operator_screens_navigate(&SCREENS, page), "page %d not reachable",
                 (int)page);
    pump();
}

/* ---- The controller's scenario channel and the runner's relay ---- */

/* One line to a channel, and its "ok". */
static void channel(int fd, const char *line)
{
    char reply[16] = {0};
    size_t n = 0U;
    const int64_t end = k_uptime_get() + WAIT_MS;

    zassert_true(fd >= 0, "channel not given for '%s'", line);
    zassert_equal(nsi_host_write(fd, line, strlen(line)), (long)strlen(line));
    zassert_equal(nsi_host_write(fd, "\n", 1U), 1L);
    while (n < sizeof(reply) - 1U) {
        char c;

        /* Non-blocking: the GUI keeps running while waiting. */
        if (nsi_host_read(fd, &c, 1U) != 1L) {
            zassert_true(k_uptime_get() < end, "no answer to '%s'", line);
            pump();
            continue;
        }
        if (c == '\n') {
            break;
        }
        reply[n++] = c;
    }
    zassert_str_equal(reply, "ok", "'%s' answered '%s'", line, reply);
    printk("workflow: %s\n", line);
}

static void scenario(const char *line)
{
    channel(scenario_fd, line);
}

/* ---- What the controller reports ---- */

static uint32_t expected_run;

static bool connected_and_qualified(void)
{
    return OBS.link == FURNACE_HMI_LINK_CONNECTED &&
           OBS.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT && OBS.status_valid &&
           OBS.measurement_state == MEASUREMENT_QUALIFIED &&
           host.library.state == FURNACE_HMI_LIBRARY_CURRENT;
}

static bool not_executing(void)
{
    return !OBS.execution_active;
}

static bool nothing_latched(void)
{
    return OBS.latched_class == 0U && OBS.unacknowledged_faults == 0U;
}

static bool manual_running(void)
{
    return OBS.run_state == RUN_RUNNING && OBS.session_kind == SESSION_MANUAL &&
           OBS.execution_active && SCREENS.manual_running;
}

static bool same_manual_running(void)
{
    return manual_running() && OBS.run_id == expected_run;
}

static bool manual_start_offered(void)
{
    return furnace_hmi_probe_enabled(manual(SCREENS.manual_start_button));
}

static bool modal_open(void)
{
    return SCREENS.modal != NULL;
}

/*
 * Connected, qualified and idle, with no dialog open: where each workflow starts.
 * Each workflow answers its own faults; only the protective trip's latch stays.
 */
static void ready(void)
{
    zassert_true(wait_for(connected_and_qualified, WAIT_MS),
                 "not connected and qualified: link %u availability %u measurement %u library %u",
                 OBS.link, OBS.availability, OBS.measurement_state, host.library.state);
    zassert_true(wait_for(not_executing, WAIT_MS), "a session executes");
    while (SCREENS.modal != NULL && !SCREENS.decision_open) {
        dismiss();
    }
}

/* ---- Shared steps ---- */

/* Manual through its page: readiness asked by opening it, then Start (owner: the longest session). */
static void manual_start(void)
{
    go_to(FURNACE_HMI_OPERATOR_PAGE_MANUAL);
    zassert_true(wait_for(manual_start_offered, WAIT_MS), "Manual Start is not offered");
    tap(manual(SCREENS.manual_start_button), "Manual Start");
    zassert_true(wait_for(manual_running, WAIT_MS), "no Manual session: state %u kind %u",
                 OBS.run_state, OBS.session_kind);
    expected_run = OBS.run_id;
    zassert_equal(SCREENS.page, FURNACE_HMI_OPERATOR_PAGE_MANUAL, "the session is not on Manual");
    printk("workflow: Manual session %u to %d\n", OBS.run_id, OBS.manual_target_decicelsius);
}

static bool manual_cooling_shown(void)
{
    return !OBS.execution_active && SCREENS.manual_cooling &&
           furnace_hmi_probe_enabled(manual(SCREENS.manual_exit_button));
}

/* Stop on Manual; the session cools; Exit leaves it. */
static void manual_stop_and_exit(void)
{
    tap(manual(SCREENS.manual_stop_button), "Manual Stop");
    zassert_true(wait_for(manual_cooling_shown, WAIT_MS), "the stopped session is not shown ending");
    tap(manual(SCREENS.manual_exit_button), "Exit");
    zassert_equal(SCREENS.page, FURNACE_HMI_OPERATOR_PAGE_HOME,
                  "Exit does not leave the session: page %d modal %d run %u state %u cooling %u "
                  "navigation %d",
                  (int)SCREENS.page, SCREENS.modal != NULL, OBS.run_id, OBS.run_state, OBS.cooling,
                  OBS.navigation_permitted);
}

/* The page's button whose text starts as the format does, before its number. */
static lv_obj_t *button_formatted(furnace_hmi_ui_string_id_t id)
{
    char prefix[48];
    const char *format = text(id);
    const char *percent = strchr(format, '%');
    const size_t length = percent == NULL ? strlen(format) : (size_t)(percent - format);

    zassert_true(length < sizeof(prefix));
    (void)memcpy(prefix, format, length);
    prefix[length] = '\0';
    return furnace_hmi_probe_button_with_text(SCREENS.root, prefix);
}

/* The way to the Faults dialog: Faults (n) on Running and Manual, the banner on Home. */
static lv_obj_t *faults_button(void)
{
    lv_obj_t *button = button_formatted(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BUTTON_FORMAT);

    return button != NULL ? button : button_formatted(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_BANNER_FORMAT);
}

/* The fault alert the screens raise by themselves, read and closed as an operator does. */
static void close_alert(void)
{
    pump();
    if (SCREENS.alert_open != 0U) {
        tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), "Close the alert");
    }
}

/*
 * Every fault answered from the Faults dialog, as an operator does: Acknowledge
 * each entry, then Recover each latched one. A refused recovery (the condition
 * or the measurement not back yet) is shown, and the operator taps again a
 * second later: each a new action, never an automatic resend.
 */
static void answer_faults(void)
{
    int taps = 0;

    pump();
    if (OBS.fault_count == 0U) {
        return;
    }
    close_alert();
    tap(faults_button(), "Faults");
    zassert_true(modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_FAULTS_TITLE)), "no Faults dialog");
    while (!nothing_latched() && taps++ < 40) {
        lv_obj_t *acknowledge = modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE);
        lv_obj_t *recover = modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RECOVER);

        if (furnace_hmi_probe_enabled(acknowledge)) {
            tap(acknowledge, "Acknowledge");
            pump_for(500U);
        } else if (furnace_hmi_probe_enabled(recover)) {
            tap(recover, "Recover");
            pump_for(1000U);
        } else {
            pump_for(200U);
        }
    }
    zassert_true(nothing_latched(), "faults left: latched %u unacknowledged %u", OBS.latched_class,
                 OBS.unacknowledged_faults);
    if (SCREENS.modal != NULL) {
        tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), "Close");
    }
}

static const furnace_hmi_controller_program_t *program_named(const char *name)
{
    for (uint8_t i = 0U; i < host.library.count; i++) {
        if (strcmp(host.library.programs[i].name, name) == 0) {
            return &host.library.programs[i];
        }
    }
    return NULL;
}

/* The program's details, opened from Programs by its place in the list shown. */
static void open_program(const char *name)
{
    go_to(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS);
    for (uint8_t i = 0U; i < host.shown_library.count; i++) {
        if (strcmp(host.shown_library.programs[i].name, name) == 0) {
            SCREENS.selected_program = i;
            go_to(FURNACE_HMI_OPERATOR_PAGE_PROGRAM_DETAIL);
            return;
        }
    }
    zassert_unreachable("%s is not listed", name);
}

static void setup_channels(void)
{
    if (scenario_port != NULL) {
        scenario_fd = nsi_host_open(scenario_port, 2 /* O_RDWR */ | 04000 /* O_NONBLOCK */);
    }
    if (relay_port != NULL) {
        relay_fd = nsi_host_open(relay_port, 2 /* O_RDWR */ | 04000 /* O_NONBLOCK */);
    }
}

static void *setup(void)
{
    setup_channels();
    zassert_true(furnace_hmi_gui_host_init(&host, lv_screen_active(), k_uptime_get_32()),
                 "the screens could not be created");
    zassert_equal(furnace_hmi_controller_link_start(), 0);
    return NULL;
}

static void before(void *fixture)
{
    ARG_UNUSED(fixture);
    ready();
    go_to(FURNACE_HMI_OPERATOR_PAGE_HOME);
}

/* The controller's inputs as they were, whatever the workflow did: a failure stays its own. */
static void after(void *fixture)
{
    ARG_UNUSED(fixture);
    if (scenario_fd >= 0) {
        scenario("offset 0");
        scenario("sensor measurement");
        scenario("interlock off");
        scenario("protection off");
    }
    if (relay_fd >= 0) {
        channel(relay_fd, "release");
    }
    /* A session a failed workflow left running is stopped, as an operator would. */
    pump_for(500U);
    if (OBS.execution_active) {
        lv_obj_t *stop = SCREENS.page == FURNACE_HMI_OPERATOR_PAGE_MANUAL
                             ? manual(SCREENS.manual_stop_button)
                             : page_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW);

        if (furnace_hmi_probe_enabled(stop)) {
            printk("workflow: cleanup Stop\n");
            furnace_hmi_probe_click(stop);
        }
        (void)wait_for(not_executing, WAIT_MS);
    }
    if (furnace_hmi_probe_enabled(manual(SCREENS.manual_exit_button))) {
        furnace_hmi_probe_click(manual(SCREENS.manual_exit_button));
    }
}

ZTEST_SUITE(operator_workflows, NULL, setup, before, after, NULL);

/* ---- W1 Program edit and save ---- */

static const char long_name[] = "1234567890123456789012345678901"; /* 31 characters */
static uint32_t wanted_revision;

static bool long_name_listed(void)
{
    const furnace_hmi_controller_program_t *p = program_named(long_name);

    return host.library.state == FURNACE_HMI_LIBRARY_CURRENT && !host.library.updating &&
           p != NULL && p->program_revision == wanted_revision;
}

static bool long_name_gone(void)
{
    return host.library.state == FURNACE_HMI_LIBRARY_CURRENT && !host.library.updating &&
           program_named(long_name) == NULL;
}

/* The answer to a save: acknowledged when the controller asks for it, then "saved". */
static void save_answered(void)
{
    zassert_true(wait_for(modal_open, WAIT_MS), "no answer to the save");
    while (SCREENS.modal != NULL &&
           modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_SENDING_TITLE))) {
        pump();
    }
    if (modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_ACKNOWLEDGE_TITLE))) {
        tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_ACKNOWLEDGE_CONFIRM), "accept the duration");
        zassert_true(wait_for(modal_open, WAIT_MS), "no answer to the acknowledged save");
        while (modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_LIBRARY_SENDING_TITLE))) {
            pump();
        }
    }
    zassert_true(modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_SAVED)),
                 "the controller did not save the program");
    dismiss();
}

ZTEST(operator_workflows, test_1_program_edit_and_save)
{
    go_to(FURNACE_HMI_OPERATOR_PAGE_PROGRAMS);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_NEW_DRAFT), "New program");
    zassert_true(SCREENS.program_editor_active, "no editor");

    /* The name, typed on the screens' keyboard, at the controller's 31 characters. */
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_NEW_PROGRAM_NAME), "the name");
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_KEYBOARD_CLEAR), "Clear");
    for (size_t i = 0U; i < strlen(long_name); i++) {
        const char key[2] = {long_name[i], '\0'};

        furnace_hmi_probe_click(furnace_hmi_probe_button_with_text(SCREENS.modal, key));
    }
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_SETTINGS_APPLY), "Apply");
    zassert_str_equal(SCREENS.program_editor_name, long_name, "the name typed");

    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE_ACTION), "Add stage");
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM), "Save");
    save_answered();
    wanted_revision = 1U;
    zassert_true(wait_for(long_name_listed, WAIT_MS), "the new program is not listed by the controller");
    printk("workflow: saved, program %u revision %u\n", program_named(long_name)->program_id,
           program_named(long_name)->program_revision);

    /* Edited and saved again: the controller's revision 2. */
    open_program(long_name);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_EDIT_PROGRAM), "Edit");
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_ADD_STAGE_ACTION), "Add stage");
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_SAVE_PROGRAM), "Save");
    save_answered();
    wanted_revision = 2U;
    zassert_true(wait_for(long_name_listed, WAIT_MS), "revision 2 is not listed");
    zassert_equal(program_named(long_name)->stage_count, 2U, "the edited stages");

    /* Deleted. */
    open_program(long_name);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT), "Delete");
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DELETE_DRAFT), "confirm Delete");
    zassert_true(wait_for(long_name_gone, WAIT_MS), "the deleted program is listed");
    zassert_true(wait_for(modal_open, WAIT_MS));
    zassert_true(modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_PROGRAM_DELETED)), "no deletion shown");
    dismiss();
}

/* ---- W2 to W5: a hot start, Pause and Resume, contained navigation, Stop and cooling ---- */

static const char post_cure[] = "Sim cure and post-cure";

static bool hot_and_qualified(void)
{
    return connected_and_qualified() && OBS.temperature_decicelsius >= 1050;
}

static bool proposal_shown(void)
{
    return SCREENS.proposal_open;
}

static bool program_running(void)
{
    return OBS.run_state == RUN_RUNNING && OBS.session_kind == SESSION_PROGRAM &&
           OBS.run_id == expected_run && SCREENS.page == FURNACE_HMI_OPERATOR_PAGE_RUNNING;
}

static bool program_paused(void)
{
    return OBS.run_state == RUN_PAUSED && OBS.run_id == expected_run &&
           furnace_hmi_probe_enabled(page_button(FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW));
}

static bool program_resumed(void)
{
    return OBS.run_state == RUN_RUNNING && OBS.run_id == expected_run &&
           furnace_hmi_probe_enabled(page_button(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW));
}

static bool stopped_and_cooling(void)
{
    return !OBS.execution_active && OBS.cooling == COOLING_ACTIVE &&
           furnace_hmi_probe_enabled(page_button(FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME));
}

static bool run_started(void)
{
    return OBS.run_id != 0U && OBS.session_kind == SESSION_PROGRAM && OBS.execution_active;
}

ZTEST(operator_workflows, test_2_hot_start_pause_resume_stop)
{
    char skipped[48];

    /* The controller's reading about 110 C: above the program's first Heating target, 100 C. */
    scenario("offset 900");
    zassert_true(wait_for(hot_and_qualified, WAIT_MS), "reading %d", OBS.temperature_decicelsius);

    open_program(post_cure);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM), "Start");
    if (SCREENS.modal != NULL && !SCREENS.proposal_open) {
        tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_PROGRAM), "confirm Start");
    }
    zassert_true(wait_for(proposal_shown, WAIT_MS), "no hot-start proposal (modal %p)",
                 (void *)SCREENS.modal);
    (void)snprintf(skipped, sizeof(skipped), "Start %s?", post_cure);
    zassert_true(modal_shows(skipped), "the proposal does not name the program");
    zassert_true(modal_shows("stage 1, Heating to 100.0"), "the skipped stage is not shown");
    zassert_true(OBS.start_proposal.skipped_count > 0U, "the controller skips nothing");
    printk("workflow: proposal stage %u, %u skipped\n", OBS.start_proposal.proposed_stage,
           OBS.start_proposal.skipped_count);
    const uint8_t proposed = OBS.start_proposal.proposed_stage;

    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_START_CONFIRM), "Confirm");
    zassert_true(wait_for(run_started, WAIT_MS), "the confirmed program does not run");
    expected_run = OBS.run_id;
    zassert_true(wait_for(program_running, WAIT_MS), "Running does not show the run");
    zassert_str_equal(OBS.program_name, post_cure);
    zassert_true(OBS.stage_index >= proposed, "the run starts at stage %u, proposed %u",
                 OBS.stage_index, proposed);

    /* W5: the library cannot be reached while the run executes (contained navigation). */
    zassert_false(furnace_hmi_operator_screens_navigate(&SCREENS, FURNACE_HMI_OPERATOR_PAGE_PROGRAMS),
                  "Programs is reachable during a run");

    /* W3: Pause and Resume, each the controller's answer. */
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_PAUSE_PREVIEW), "Pause");
    zassert_true(wait_for(program_paused, WAIT_MS), "not paused: state %u", OBS.run_state);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_RESUME_PREVIEW), "Resume");
    zassert_true(wait_for(program_resumed, WAIT_MS), "not resumed: state %u", OBS.run_state);

    /* W4: Stop; the run cools on Running; Return Home leaves it cooling. */
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_STOP_PREVIEW), "Stop");
    zassert_true(wait_for(stopped_and_cooling, WAIT_MS), "not stopped and cooling: cooling %u",
                 OBS.cooling);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_RETURN_HOME), "Return Home");
    pump_for(500U);
    zassert_equal(SCREENS.page, FURNACE_HMI_OPERATOR_PAGE_HOME, "Return Home does not show Home");
    zassert_equal(OBS.cooling, COOLING_ACTIVE, "Return Home ended the cooling");
    scenario("offset 0");
}

/* ---- W9 Manual ---- */

static int32_t wanted_target;

static bool target_followed(void)
{
    return OBS.manual_target_decicelsius == wanted_target && SCREENS.manual_target_c * 10 == wanted_target;
}

static bool manual_paused(void)
{
    return OBS.run_state == RUN_PAUSED && OBS.run_id == expected_run && SCREENS.manual_paused;
}

ZTEST(operator_workflows, test_3_manual)
{
    manual_start();
    wanted_target = OBS.manual_target_decicelsius + 10;
    tap(furnace_hmi_probe_button_with_text(manual(SCREENS.manual_steppers),
                                           text(FURNACE_HMI_UI_STRING_UI_LAB_INCREASE)),
        "target +1");
    zassert_true(wait_for(target_followed, WAIT_MS), "the retarget is not the controller's: %d",
                 OBS.manual_target_decicelsius);
    tap(manual(SCREENS.manual_pause_button), "Pause");
    zassert_true(wait_for(manual_paused, WAIT_MS), "not paused: state %u", OBS.run_state);
    tap(manual(SCREENS.manual_pause_button), "Resume");
    zassert_true(wait_for(same_manual_running, WAIT_MS), "not resumed: state %u", OBS.run_state);
    manual_stop_and_exit();
}

/* ---- W6 Quality decision and Continue ---- */

static bool decision_shown(void)
{
    return OBS.run_state == RUN_AWAITING_DECISION && SCREENS.decision_open &&
           furnace_hmi_probe_enabled(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE));
}

static bool continued(void)
{
    return same_manual_running() && !SCREENS.decision_open;
}

ZTEST(operator_workflows, test_4_quality_decision_and_continue)
{
    manual_start();
    /* A reading 30 C below the ramp: LAG, continuable in the link simulation's fixture policy. */
    scenario("offset -300");
    zassert_true(wait_for(decision_shown, 2U * WAIT_MS), "no decision panel: state %u",
                 OBS.run_state);
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_DECISION_CONTINUE), "Continue");
    zassert_true(wait_for(continued, WAIT_MS), "the session does not continue: state %u",
                 OBS.run_state);
    scenario("offset 0");
    manual_stop_and_exit();
    answer_faults();
}

/* ---- W7 Faults ---- */

static bool manual_faulted(void)
{
    return OBS.run_state == RUN_FAULTED && OBS.run_id == expected_run &&
           furnace_hmi_probe_enabled(manual(SCREENS.manual_stop_button)) &&
           !furnace_hmi_probe_enabled(manual(SCREENS.manual_pause_button));
}

ZTEST(operator_workflows, test_5_sensor_fault)
{
    manual_start();
    scenario("sensor not_finite");
    zassert_true(wait_for(manual_faulted, WAIT_MS), "not faulted with Stop only: state %u",
                 OBS.run_state);
    close_alert();
    tap(faults_button(), "Faults");
    zassert_true(modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_1)), "the sensor fault is not listed");
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), "Close");
    manual_stop_and_exit();
    scenario("sensor measurement");
    answer_faults();
}

/* ---- W10 Link loss and reconnection during a session ---- */

/* The link-lost alert (UX baseline: the alert dialog), and no command offered. */
static bool link_lost_shown(void)
{
    return OBS.availability != FURNACE_HMI_STATE_AVAILABILITY_CURRENT &&
           modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_LOST_TITLE)) &&
           !furnace_hmi_probe_enabled(manual(SCREENS.manual_stop_button)) &&
           !furnace_hmi_probe_enabled(manual(SCREENS.manual_pause_button));
}

static bool link_back(void)
{
    return OBS.link == FURNACE_HMI_LINK_CONNECTED &&
           OBS.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT && same_manual_running() &&
           modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_ALERT_LINK_RESTORED_TITLE)) &&
           furnace_hmi_probe_enabled(manual(SCREENS.manual_stop_button));
}

ZTEST(operator_workflows, test_6_link_loss_and_reconnection)
{
    uint32_t sessions;

    manual_start();
    sessions = OBS.controller_session_changes;
    channel(relay_fd, "hold");
    zassert_true(wait_for(link_lost_shown, WAIT_MS),
                 "the lost link is not shown: availability %u", OBS.availability);
    pump_for(2000U);
    channel(relay_fd, "release");
    zassert_true(wait_for(link_back, WAIT_MS),
                 "the session is not shown again: link %u availability %u run %u (was %u)",
                 OBS.link, OBS.availability, OBS.run_id, expected_run);
    printk("workflow: reconnected to run %u, controller session changes %u -> %u\n", OBS.run_id,
           sessions, OBS.controller_session_changes);
    /* The alert turned "Connection restored"; the operator closes it to reach Stop. */
    close_alert();
    manual_stop_and_exit();
}

/* ---- W8 Protective trip: last, because its latch stays for this controller session ---- */

static bool tripped(void)
{
    return OBS.run_state == RUN_FAULTED && OBS.latched_class == CLASS_PROTECTIVE_TRIP;
}

ZTEST(operator_workflows, test_7_protective_trip)
{
    manual_start();
    scenario("protection on");
    zassert_true(wait_for(tripped, WAIT_MS), "no protective trip: state %u class %u",
                 OBS.run_state, OBS.latched_class);
    close_alert();
    manual_stop_and_exit();
    scenario("protection off");
    pump_for(1000U);
    close_alert();
    tap(faults_button(), "Faults");
    zassert_true(modal_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_RESET_EXTERNAL)),
                 "the external reset is not shown");
    for (int i = 0; i < 4 && furnace_hmi_probe_enabled(modal_button(
                                 FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE));
         i++) {
        tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_FAULT_ACKNOWLEDGE), "Acknowledge");
        pump_for(500U);
    }
    zassert_equal(OBS.latched_class, CLASS_PROTECTIVE_TRIP, "the trip is no longer latched");
    tap(modal_button(FURNACE_HMI_UI_STRING_UI_LAB_CLOSE), "Close");
}

/* ---- W11 Device log ---- */

static bool history_read(void)
{
    return host.history.state == FURNACE_HMI_HISTORY_READ && !host.history.reading &&
           host.history.count > 0U;
}

ZTEST(operator_workflows, test_8_device_log)
{
    go_to(FURNACE_HMI_OPERATOR_PAGE_DEVICE);
    tap(page_button(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_LOG), "Device log");
    zassert_true(wait_for(history_read, WAIT_MS), "the controller's log is not read");
    pump_for(200U);
    zassert_false(page_shows(text(FURNACE_HMI_UI_STRING_UI_LAB_DEVICE_UNAVAILABLE)),
                  "the Device log is shown unavailable");
    printk("workflow: Device log, %u entries\n", host.history.count);
}
