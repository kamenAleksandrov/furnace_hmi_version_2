/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The reading of the controller's health and event history (Phase 21 M21-7c;
 * plan, "M21-7c, the Device log and health"; controller interface
 * "Diagnostics"). Health is asked for periodically while the link is current;
 * the history is read on request, one page at a time, newest entries of each
 * log, and merged into one window that is complete in every log.
 */

#include <stddef.h>
#include <string.h>

#include <furnace_hmi/controller_client.h>

#include "controller_diagnostics.h"

#define HISTORY_IDLE 0U
#define HISTORY_PROBE 1U
#define HISTORY_PAGE 2U

/* EVENT_HISTORY_PAGE results (controller interface). */
#define HISTORY_OK 0U
#define HISTORY_UNAVAILABLE 1U
#define HISTORY_BUSY 3U

/* Every diagnostic category the interface defines (bits 1..9). */
#define HISTORY_ALL_CATEGORIES UINT32_C(0x000003FE)
/* Pages of one log at most in one read: a bound, never reached with the newest entries. */
#define HISTORY_MAX_PAGES 32U

static bool elapsed(uint32_t now, uint32_t since, uint32_t interval)
{
    return (uint32_t)(now - since) >= interval;
}

static void saturating_increment(uint32_t *counter)
{
    if (*counter != UINT32_MAX) {
        (*counter)++;
    }
}

static void changed(furnace_hmi_controller_diagnostics_t *d)
{
    d->generation++;
}

void furnace_hmi_diagnostics_init(furnace_hmi_controller_diagnostics_t *d)
{
    (void)memset(d, 0, sizeof(*d));
}

void furnace_hmi_diagnostics_discard(furnace_hmi_controller_diagnostics_t *d)
{
    const uint32_t health_retries = d->health.retries;
    const uint32_t history_retries = d->history.retries;
    const uint32_t generation = d->generation;

    furnace_hmi_diagnostics_init(d);
    d->health.retries = health_retries;
    d->history.retries = history_retries;
    d->generation = generation + 1U;
}

void furnace_hmi_diagnostics_request_history(furnace_hmi_controller_diagnostics_t *d)
{
    if (d->history_step == HISTORY_IDLE) {
        d->history_wanted = true;
    }
}

/* ---- health ------------------------------------------------------------------------------- */

static void health_follow(furnace_hmi_controller_diagnostics_t *d, uint32_t now, bool connected)
{
    const bool current = connected && d->health.valid &&
                         !elapsed(now, d->health_received_ms,
                                  2U * FURNACE_HMI_CLIENT_HEALTH_INTERVAL_MS);

    if (!connected) {
        d->health_pending = false;
        d->health_asked = false;
    }
    if (d->health.current != current) {
        d->health.current = current;
        changed(d);
    }
}

static bool health_due(furnace_hmi_controller_diagnostics_t *d, uint32_t now)
{
    if (d->health_pending) {
        if (!elapsed(now, d->health_sent_ms, FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS)) {
            return false;
        }
        saturating_increment(&d->health.retries);
        changed(d);
        return true;
    }
    return !d->health_asked || elapsed(now, d->health_sent_ms, FURNACE_HMI_CLIENT_HEALTH_INTERVAL_MS);
}

void furnace_hmi_diagnostics_on_health(furnace_hmi_controller_diagnostics_t *d,
                                       const struct furnace_wire_health *h, uint32_t now_ms)
{
    furnace_hmi_controller_health_t *r = &d->health;
    uint8_t count = h->subsystems_count;

    if (!d->health_pending || h->request_id != d->health_request_id) {
        return; /* superseded, or not ours */
    }
    if (count > FURNACE_HMI_CONTROLLER_HEALTH_SUBSYSTEMS) {
        count = FURNACE_HMI_CONTROLLER_HEALTH_SUBSYSTEMS;
    }
    d->health_pending = false;
    d->health_received_ms = now_ms;
    r->valid = true;
    r->current = true;
    r->uptime_ms = h->uptime_ms;
    r->boot_count = h->boot_count;
    r->overall = h->overall;
    r->reset_cause_known = h->reset_cause_known;
    r->reset_cause = h->reset_cause;
    r->subsystem_count = count;
    (void)memset(r->subsystems, 0, sizeof(r->subsystems));
    for (uint8_t i = 0U; i < count; i++) {
        r->subsystems[i].subsystem = h->subsystems[i].subsystem;
        r->subsystems[i].state = h->subsystems[i].state;
        r->subsystems[i].detail = h->subsystems[i].detail;
    }
    r->faults_active = h->faults_active;
    r->faults_latched = h->faults_latched;
    r->faults_unacknowledged = h->faults_unacknowledged;
    r->service_s = h->service_s;
    r->service_valid = h->service_valid;
    changed(d);
}

/* ---- history ------------------------------------------------------------------------------ */

/* True when a is older than b: by controller start, then monotonic time, then log and sequence. */
static bool older(const furnace_hmi_controller_history_entry_t *a,
                  const furnace_hmi_controller_history_entry_t *b)
{
    if (a->boot_id != b->boot_id) {
        return a->boot_id < b->boot_id;
    }
    if (a->mono_ms != b->mono_ms) {
        return a->mono_ms < b->mono_ms;
    }
    if (a->log != b->log) {
        return a->log > b->log;
    }
    return a->log_sequence < b->log_sequence;
}

/* True when a was recorded before b, by time alone. */
static bool earlier(const furnace_hmi_controller_history_entry_t *a,
                    const furnace_hmi_controller_history_entry_t *b)
{
    return (a->boot_id != b->boot_id) ? (a->boot_id < b->boot_id) : (a->mono_ms < b->mono_ms);
}

/*
 * The read ended: a log that held more than was read leaves out, in every log,
 * what is older than its oldest entry read, so the window is complete in each
 * log. The rest is sorted newest first and shown.
 */
static void history_finish(furnace_hmi_controller_diagnostics_t *d)
{
    furnace_hmi_controller_history_t *r = &d->reading;
    const furnace_hmi_controller_history_entry_t *cutoff = NULL;
    uint16_t kept = 0U;

    for (uint8_t log = 1U; log <= FURNACE_HMI_CONTROLLER_HISTORY_LOGS; log++) {
        const furnace_hmi_controller_history_entry_t *oldest = NULL;

        if (!r->logs[log - 1U].truncated) {
            continue;
        }
        for (uint16_t i = 0U; i < r->count; i++) {
            if (r->entries[i].log == log && (oldest == NULL || older(&r->entries[i], oldest))) {
                oldest = &r->entries[i];
            }
        }
        if (oldest != NULL && (cutoff == NULL || older(cutoff, oldest))) {
            cutoff = oldest;
        }
    }
    for (uint16_t i = 0U; i < r->count; i++) {
        const furnace_hmi_controller_history_entry_t e = r->entries[i];
        uint16_t at = kept;

        if (cutoff != NULL && earlier(&e, cutoff)) {
            continue;
        }
        /* Insertion, newest first. */
        while (at > 0U && older(&d->history.entries[at - 1U], &e)) {
            d->history.entries[at] = d->history.entries[at - 1U];
            at--;
        }
        d->history.entries[at] = e;
        kept++;
    }
    d->history.count = kept;
    (void)memcpy(d->history.logs, r->logs, sizeof(d->history.logs));
    d->history.state = FURNACE_HMI_HISTORY_READ;
    d->history.reading = false;
    d->history.interrupted = false;
    d->history_step = HISTORY_IDLE;
    changed(d);
}

static void history_next_log(furnace_hmi_controller_diagnostics_t *d)
{
    d->history_log++;
    if (d->history_log >= FURNACE_HMI_CONTROLLER_HISTORY_LOGS) {
        history_finish(d);
        return;
    }
    d->history_step = HISTORY_PROBE;
    d->history_pages = 0U;
    d->history_log_start = d->reading.count;
    d->history_wait_busy = false;
    d->history_ready = true;
}

static void history_start(furnace_hmi_controller_diagnostics_t *d)
{
    d->history_wanted = false;
    d->reading.count = 0U;
    (void)memset(d->reading.logs, 0, sizeof(d->reading.logs));
    d->history_log = 0U;
    d->history_step = HISTORY_PROBE;
    d->history_pages = 0U;
    d->history_log_start = 0U;
    d->history_wait_busy = false;
    d->history_ready = true;
    d->history.reading = true;
    changed(d);
}

static void history_stop(furnace_hmi_controller_diagnostics_t *d)
{
    d->history_wanted = false;
    if (d->history_step == HISTORY_IDLE) {
        return;
    }
    d->history_step = HISTORY_IDLE;
    d->history.reading = false;
    d->history.interrupted = true;
    changed(d);
}

static bool history_due(furnace_hmi_controller_diagnostics_t *d, uint32_t now)
{
    if (d->history_step == HISTORY_IDLE) {
        if (!d->history_wanted) {
            return false;
        }
        history_start(d);
        return true;
    }
    if (d->history_ready) {
        return true;
    }
    if (!elapsed(now, d->history_sent_ms,
                 d->history_wait_busy ? FURNACE_HMI_CLIENT_HISTORY_BUSY_MS
                                      : FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS)) {
        return false;
    }
    saturating_increment(&d->history.retries);
    changed(d);
    return true;
}

static void history_request(const furnace_hmi_controller_diagnostics_t *d,
                            struct furnace_wire_message *out)
{
    struct furnace_wire_event_history_request *q = &out->u.event_history_request;

    out->type = FURNACE_WIRE_MSG_EVENT_HISTORY_REQUEST;
    q->log = (uint8_t)(d->history_log + 1U);
    /* The first request asks for one entry from the start: it gives the log's extent. */
    q->from_sequence = (d->history_step == HISTORY_PROBE) ? 0U : d->history_cursor;
    q->max_entries = (d->history_step == HISTORY_PROBE) ? 1U : FURNACE_HMI_CONTROLLER_HISTORY_PAGE;
    q->categories = HISTORY_ALL_CATEGORIES;
}

/* Keep an entry of the log being read; at the log's capacity its oldest goes. */
static void history_keep(furnace_hmi_controller_diagnostics_t *d,
                         const struct furnace_wire_history_entry *w, uint8_t log)
{
    furnace_hmi_controller_history_t *r = &d->reading;
    furnace_hmi_controller_history_entry_t *e;
    uint8_t params = w->params_count;

    if ((uint16_t)(r->count - d->history_log_start) >= FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG) {
        (void)memmove(&r->entries[d->history_log_start], &r->entries[d->history_log_start + 1U],
                      (size_t)(r->count - d->history_log_start - 1U) * sizeof(r->entries[0]));
        r->count--;
        r->logs[log - 1U].truncated = true;
    }
    e = &r->entries[r->count++];
    (void)memset(e, 0, sizeof(*e));
    if (params > FURNACE_HMI_CONTROLLER_EVENT_PARAMS) {
        params = FURNACE_HMI_CONTROLLER_EVENT_PARAMS;
    }
    e->log = log;
    e->log_sequence = w->log_sequence;
    e->boot_id = w->boot_id;
    e->mono_ms = w->mono_ms;
    e->wall_state = w->wall_state;
    e->wall_ms = w->wall_ms;
    e->event_id = w->event_id;
    e->severity = w->severity;
    e->category = w->category;
    e->param_count = params;
    for (uint8_t i = 0U; i < params; i++) {
        e->param_types[i] = w->params[i].type;
        e->param_values[i] = w->params[i].value;
    }
}

void furnace_hmi_diagnostics_on_history_page(furnace_hmi_controller_diagnostics_t *d,
                                             const struct furnace_wire_event_history_page *p,
                                             uint32_t now_ms)
{
    furnace_hmi_controller_history_log_t *l;
    const uint8_t log = (uint8_t)(d->history_log + 1U);

    (void)now_ms;
    if (d->history_step == HISTORY_IDLE || p->request_id != d->history_request_id ||
        d->history_wait_busy || p->log != log) {
        return; /* superseded, or not ours */
    }
    l = &d->reading.logs[d->history_log];
    if (p->result == HISTORY_BUSY) {
        d->history_wait_busy = true; /* asked again after FURNACE_HMI_CLIENT_HISTORY_BUSY_MS */
        return;
    }
    if (p->result == HISTORY_UNAVAILABLE) {
        l->state = FURNACE_HMI_HISTORY_LOG_UNAVAILABLE;
        history_next_log(d);
        return;
    }
    if (p->result != HISTORY_OK) {
        l->state = FURNACE_HMI_HISTORY_LOG_FAILED; /* READ_FAILED, or a filter refused */
        history_next_log(d);
        return;
    }
    if (d->history_step == HISTORY_PROBE) {
        const uint32_t held = p->next_sequence - p->first_retained;

        l->first_retained = p->first_retained;
        l->next_sequence = p->next_sequence;
        if (p->next_sequence <= p->first_retained) {
            l->state = FURNACE_HMI_HISTORY_LOG_READ; /* empty */
            history_next_log(d);
            return;
        }
        l->truncated = held > FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG;
        d->history_cursor = l->truncated
                                ? p->next_sequence - FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG
                                : p->first_retained;
        d->history_step = HISTORY_PAGE;
        d->history_ready = true;
        return;
    }
    l->evicted += p->evicted;
    for (uint8_t i = 0U; i < p->entries_count && i < FURNACE_HMI_CONTROLLER_HISTORY_PAGE; i++) {
        /* In increasing sequence, from the cursor: anything else was served before. */
        if (p->entries[i].log_sequence >= d->history_cursor) {
            history_keep(d, &p->entries[i], log);
            d->history_cursor = p->entries[i].log_sequence + 1U;
        }
    }
    d->history_pages++;
    if (p->more && p->scanned_to >= d->history_cursor && d->history_pages < HISTORY_MAX_PAGES) {
        d->history_cursor = p->scanned_to;
        d->history_ready = true;
        return; /* the next page is due at once */
    }
    if (p->next_sequence > l->next_sequence) {
        l->next_sequence = p->next_sequence;
    }
    l->state = FURNACE_HMI_HISTORY_LOG_READ;
    history_next_log(d);
}

/* ---- the client's calls ------------------------------------------------------------------- */

bool furnace_hmi_diagnostics_due(furnace_hmi_controller_diagnostics_t *d, uint32_t now_ms,
                                 bool connected, struct furnace_wire_message *out)
{
    health_follow(d, now_ms, connected);
    if (!connected) {
        history_stop(d);
        return false;
    }
    (void)memset(out, 0, sizeof(*out));
    if (health_due(d, now_ms)) {
        out->type = FURNACE_WIRE_MSG_HEALTH_REQUEST;
        return true;
    }
    if (history_due(d, now_ms)) {
        history_request(d, out);
        return true;
    }
    return false;
}

void furnace_hmi_diagnostics_sent(furnace_hmi_controller_diagnostics_t *d,
                                  const struct furnace_wire_message *out, uint32_t request_id,
                                  uint32_t now_ms)
{
    if (out->type == FURNACE_WIRE_MSG_HEALTH_REQUEST) {
        d->health_request_id = request_id;
        d->health_sent_ms = now_ms;
        d->health_pending = true;
        d->health_asked = true;
    } else {
        d->history_request_id = request_id;
        d->history_sent_ms = now_ms;
        d->history_wait_busy = false;
        d->history_ready = false;
    }
}

bool furnace_hmi_diagnostics_on_rejected(furnace_hmi_controller_diagnostics_t *d,
                                         uint32_t request_id)
{
    if (request_id == 0U) {
        return false;
    }
    return (d->health_pending && request_id == d->health_request_id) ||
           (d->history_step != HISTORY_IDLE && request_id == d->history_request_id);
}
