/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The controller link on Zephyr (Phase 21 M21-3). A thread of its own polls the
 * UART and the portable client, and publishes the observation to a single-slot
 * mailbox: the latest value, never a queue that could fall behind. Only the GUI
 * context reads the mailbox, without waiting, and only it touches LVGL
 * (ADR-0005). Nothing here waits on the controller.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>

#include <furnace_hmi/controller_client.h>

#include "controller_link.h"

#define LINK_POLL_MS 5
/* Below the GUI thread (main, priority 0): the link never preempts drawing. */
#define LINK_PRIORITY K_PRIO_PREEMPT(2)
#define LINK_STACK_SIZE 4096
/* The age of what is shown stays within this, even when nothing else changes. */
#define PUBLISH_INTERVAL_MS 200U
/* The console trace: every change of link or availability, and at most once a second else. */
#define TRACE_INTERVAL_MS 1000U

static const struct device *const link_uart = DEVICE_DT_GET(DT_NODELABEL(controller_link));

static furnace_hmi_controller_client_t client;

static K_MUTEX_DEFINE(mailbox_lock);
static furnace_hmi_controller_observation_t mailbox;
static uint32_t mailbox_sequence;

/* Commands from the GUI context to the link thread: one slot each for ordinary and Stop. */
struct command_slot {
    bool full;
    furnace_hmi_command_t command;
    uint32_t submission;
};
static struct k_spinlock command_lock;
static struct command_slot command_slots[2];
static uint32_t last_submission;

K_THREAD_STACK_DEFINE(link_stack, LINK_STACK_SIZE);
static struct k_thread link_thread;

static const char *const link_names[FURNACE_HMI_LINK_COUNT] = {
    "CONNECTING", "INCOMPATIBLE", "SYNCHRONIZING", "CONNECTED", "SILENT",
};
static const char *const availability_names[] = {"UNAVAILABLE", "STALE", "CURRENT"};

static size_t uart_read(void *context, uint8_t *buffer, size_t capacity)
{
    size_t n = 0U;

    (void)context;
    while (n < capacity && uart_poll_in(link_uart, &buffer[n]) == 0) {
        n++;
    }
    return n;
}

static size_t uart_write(void *context, const uint8_t *bytes, size_t length)
{
    (void)context;
    for (size_t i = 0U; i < length; i++) {
        uart_poll_out(link_uart, bytes[i]);
    }
    return length;
}

static uint32_t new_client_session_id(void *context)
{
    uint32_t id;

    (void)context;
    do {
        id = sys_rand32_get();
    } while (id == 0U);
    return id;
}

static void publish(const furnace_hmi_controller_observation_t *observation)
{
    (void)k_mutex_lock(&mailbox_lock, K_FOREVER);
    mailbox = *observation;
    mailbox_sequence++;
    (void)k_mutex_unlock(&mailbox_lock);
}

/* Submit what the GUI context handed over; the client reports each outcome. */
static void take_commands(void)
{
    struct command_slot taken[2];
    k_spinlock_key_t key = k_spin_lock(&command_lock);

    for (size_t i = 0U; i < 2U; i++) {
        taken[i] = command_slots[i];
        command_slots[i].full = false;
    }
    k_spin_unlock(&command_lock, key);
    if (taken[1].full) {
        (void)furnace_hmi_controller_client_submit(&client, &taken[1].command,
                                                   taken[1].submission);
        /*
         * The operator pressed Stop after this command, before it was sent: it is
         * dropped, never sent after the Stop (review F2).
         */
        if (taken[0].full) {
            furnace_hmi_controller_client_refuse(&client, &taken[0].command, taken[0].submission,
                                                 FURNACE_HMI_COMMAND_LOCAL_SUPERSEDED_BY_STOP);
        }
    } else if (taken[0].full) {
        (void)furnace_hmi_controller_client_submit(&client, &taken[0].command,
                                                   taken[0].submission);
    }
}

/* One line, key=value, for people and for tools/controller_link_demo.py. */
static void trace(const furnace_hmi_controller_observation_t *o)
{
    printk("hmi-link: link=%s availability=%s client=%08x session=%02x%02x%02x%02x "
           "changes=%u fw=%u boot=%u proto=%u.%u hello=%u status=%u temp=%d validity=%u "
           "age=%u measurement=%u qualified=%u run_state=%u run=%u inhibited=%u latched=%u "
           "unack=%u faults=%u outdated=%u fresh=%u ready=%u requests=%u request_state=%u "
           "gaps=%u regressions=%u incomplete=%u caps_rejected=%u dropped=%u "
           "command=%u/%u/%u/%u stop=%u/%u/%u\n",
           link_names[o->link], availability_names[o->availability],
           (unsigned int)o->client_session_id, o->session_id[0], o->session_id[1],
           o->session_id[2], o->session_id[3], (unsigned int)o->controller_session_changes,
           (unsigned int)o->firmware_id, (unsigned int)o->boot_count, o->controller_proto_major,
           o->controller_proto_minor, o->hello_result, (unsigned int)o->status_sequence,
           (int)o->temperature_decicelsius, o->temperature_validity,
           (unsigned int)o->temperature_age_ms, o->measurement_state,
           (unsigned int)o->measurement_qualified_s, o->run_state, (unsigned int)o->run_id,
           o->heating_inhibited ? 1U : 0U, o->latched_class, o->unacknowledged_faults,
           o->faults_valid ? (unsigned int)o->fault_count : 0U, o->faults_outdated ? 1U : 0U,
           o->session_fresh ? 1U : 0U, o->store_readiness, (unsigned int)o->snapshots_requested,
           o->snapshot_request, (unsigned int)o->event_gaps,
           (unsigned int)o->status_regressions, (unsigned int)o->snapshots_incomplete,
           (unsigned int)o->capabilities_rejected, (unsigned int)o->frames_dropped,
           o->command.kind, o->command.state, o->command.reason, o->command.unresolved_cause,
           o->stop.state, o->stop.reason, o->stop.unresolved_cause);
}

static void link_main(void *p1, void *p2, void *p3)
{
    const furnace_hmi_controller_client_config_t config = {
        .port = {.read = uart_read, .write = uart_write, .context = NULL},
        .new_client_session_id = new_client_session_id,
        .id_context = NULL,
    };
    uint32_t published_generation;
    uint32_t last_publish;
    uint32_t last_trace;
    furnace_hmi_link_state_t traced_link = FURNACE_HMI_LINK_COUNT;
    furnace_hmi_state_availability_t traced_availability = FURNACE_HMI_STATE_AVAILABILITY_COUNT;

    (void)p1;
    (void)p2;
    (void)p3;
    furnace_hmi_controller_client_init(&client, &config, k_uptime_get_32());
    published_generation = furnace_hmi_controller_client_generation(&client) - 1U;
    last_publish = k_uptime_get_32();
    last_trace = last_publish;

    for (;;) {
        const uint32_t now = k_uptime_get_32();
        const furnace_hmi_controller_observation_t *o;

        furnace_hmi_controller_client_poll(&client, now);
        take_commands();
        o = furnace_hmi_controller_client_observation(&client);
        if (furnace_hmi_controller_client_generation(&client) != published_generation ||
            (uint32_t)(now - last_publish) >= PUBLISH_INTERVAL_MS) {
            publish(o);
            published_generation = furnace_hmi_controller_client_generation(&client);
            last_publish = now;
        }
        if (o->link != traced_link || o->availability != traced_availability ||
            (uint32_t)(now - last_trace) >= TRACE_INTERVAL_MS) {
            trace(o);
            traced_link = o->link;
            traced_availability = o->availability;
            last_trace = now;
        }
        k_msleep(LINK_POLL_MS);
    }
}

int furnace_hmi_controller_link_start(void)
{
    if (!device_is_ready(link_uart)) {
        return -ENODEV;
    }
    (void)k_thread_create(&link_thread, link_stack, K_THREAD_STACK_SIZEOF(link_stack), link_main,
                          NULL, NULL, NULL, LINK_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&link_thread, "controller_link");
    return 0;
}

bool furnace_hmi_controller_link_take(furnace_hmi_controller_observation_t *out, uint32_t *seen)
{
    bool taken = false;

    if (k_mutex_lock(&mailbox_lock, K_NO_WAIT) != 0) {
        return false;
    }
    if (mailbox_sequence != *seen) {
        *out = mailbox;
        *seen = mailbox_sequence;
        taken = true;
    }
    (void)k_mutex_unlock(&mailbox_lock);
    return taken;
}

uint32_t furnace_hmi_controller_link_submit(const furnace_hmi_command_t *command)
{
    struct command_slot *slot = &command_slots[furnace_hmi_command_is_stop(command->kind) ? 1 : 0];
    uint32_t submission = 0U;
    k_spinlock_key_t key = k_spin_lock(&command_lock);

    if (!slot->full) {
        last_submission = (last_submission == UINT32_MAX) ? 1U : last_submission + 1U;
        submission = last_submission;
        slot->full = true;
        slot->command = *command;
        slot->submission = submission;
    }
    k_spin_unlock(&command_lock, key);
    return submission;
}
