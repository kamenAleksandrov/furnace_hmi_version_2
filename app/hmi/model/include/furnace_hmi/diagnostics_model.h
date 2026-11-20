/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_DIAGNOSTICS_MODEL_H
#define FURNACE_HMI_DIAGNOSTICS_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The controller's health and event history as the HMI read them (Phase 21
 * M21-7c; ADR-0020; controller interface "Diagnostics"). Plain data
 * (ADR-0003) shared by the client, which fills it, and the presentation, which
 * shows it; codes and units as received.
 */

/** Subsystems in one HEALTH report (controller interface: at most 10). */
#define FURNACE_HMI_CONTROLLER_HEALTH_SUBSYSTEMS 10U
/** The controller's persistent logs: CRITICAL 1, GENERAL 2, AUDIT 3. */
#define FURNACE_HMI_CONTROLLER_HISTORY_LOGS 3U
/** The newest entries read from each log; provisional HMI value (plan, M21-7c). */
#define FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG 40U
#define FURNACE_HMI_CONTROLLER_HISTORY_CAPACITY \
    (FURNACE_HMI_CONTROLLER_HISTORY_LOGS * FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG)
/** Typed parameters of one entry (DIAG-10). */
#define FURNACE_HMI_CONTROLLER_EVENT_PARAMS 8U

/** Health states (overall and per subsystem), as received. */
typedef enum {
    FURNACE_HMI_HEALTH_UNKNOWN = 0,
    FURNACE_HMI_HEALTH_OK = 1,
    FURNACE_HMI_HEALTH_WARNING = 2,
    FURNACE_HMI_HEALTH_FAULT = 3,
} furnace_hmi_health_state_t;

typedef struct {
    uint8_t subsystem;
    uint8_t state;
    uint32_t detail;
} furnace_hmi_controller_subsystem_t;

/** The newest HEALTH report of this controller session. */
typedef struct {
    /** A report of the current controller session was received. */
    bool valid;
    /**
     * The report is up to date: the link is current and a report came within
     * two health intervals. Otherwise it is the last one known, shown as stale
     * (ADR-0020: the controller keeps recording meanwhile).
     */
    bool current;
    uint64_t uptime_ms;
    uint32_t boot_count;
    uint8_t overall;
    bool reset_cause_known;
    uint32_t reset_cause;
    uint8_t subsystem_count;
    furnace_hmi_controller_subsystem_t subsystems[FURNACE_HMI_CONTROLLER_HEALTH_SUBSYSTEMS];
    uint8_t faults_active;
    uint8_t faults_latched;
    uint8_t faults_unacknowledged;
    uint32_t service_s;
    bool service_valid;
    /** Requests asked again after no answer; saturating. */
    uint32_t retries;
} furnace_hmi_controller_health_t;

/** One history entry, as stored by the controller. */
typedef struct {
    uint32_t log_sequence;
    uint32_t boot_id;
    uint64_t mono_ms;
    int64_t wall_ms;
    uint16_t event_id;
    /** The log it came from: 1 critical, 2 general, 3 audit. */
    uint8_t log;
    uint8_t wall_state;
    uint8_t severity;
    uint8_t category;
    uint8_t param_count;
    uint8_t param_types[FURNACE_HMI_CONTROLLER_EVENT_PARAMS];
    uint32_t param_values[FURNACE_HMI_CONTROLLER_EVENT_PARAMS];
} furnace_hmi_controller_history_entry_t;

/** How the reading of one log ended. */
typedef enum {
    FURNACE_HMI_HISTORY_LOG_NOT_READ = 0,
    /** Read: the newest entries, or every entry the log holds. */
    FURNACE_HMI_HISTORY_LOG_READ,
    /** The controller has no persistent log in this start (UNAVAILABLE). */
    FURNACE_HMI_HISTORY_LOG_UNAVAILABLE,
    /** A page failed (READ_FAILED or a refused filter): what came before it is kept. */
    FURNACE_HMI_HISTORY_LOG_FAILED,
} furnace_hmi_history_log_state_t;

typedef struct {
    uint8_t state;
    uint32_t first_retained;
    uint32_t next_sequence;
    /** Entries evicted while the log was read: they are missing. */
    uint32_t evicted;
    /** The log held more than was read; older entries of every log are left out. */
    bool truncated;
} furnace_hmi_controller_history_log_t;

/** Where the history stands. */
typedef enum {
    /** Nothing read in this controller session. */
    FURNACE_HMI_HISTORY_NONE = 0,
    /** A read ended: entries and per-log states are its result. */
    FURNACE_HMI_HISTORY_READ,
} furnace_hmi_history_state_t;

/**
 * The newest complete read of the controller's logs, merged newest first. It
 * stays until the next read ends; a new controller session discards it.
 */
typedef struct {
    uint8_t state;
    /** A read is in progress; the entries are still the previous read's. */
    bool reading;
    /** The latest read was stopped by the link; the entries are an older read's. */
    bool interrupted;
    furnace_hmi_controller_history_log_t logs[FURNACE_HMI_CONTROLLER_HISTORY_LOGS];
    uint16_t count;
    furnace_hmi_controller_history_entry_t entries[FURNACE_HMI_CONTROLLER_HISTORY_CAPACITY];
    /** Pages asked again after no answer or BUSY; saturating. */
    uint32_t retries;
} furnace_hmi_controller_history_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_DIAGNOSTICS_MODEL_H */
