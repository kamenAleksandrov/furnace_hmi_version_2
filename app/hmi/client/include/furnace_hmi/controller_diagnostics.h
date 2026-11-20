/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_CONTROLLER_DIAGNOSTICS_H
#define FURNACE_HMI_CONTROLLER_DIAGNOSTICS_H

#include <stdbool.h>
#include <stdint.h>

#include <furnace_hmi/diagnostics_model.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The client's reading of the controller's diagnostics (Phase 21 M21-7c). */

/** Health is asked for this often while the link is current; provisional HMI value. */
#define FURNACE_HMI_CLIENT_HEALTH_INTERVAL_MS 5000U
/** A history page refused as BUSY is asked again after this long; provisional HMI value. */
#define FURNACE_HMI_CLIENT_HISTORY_BUSY_MS 1000U
/** Entries the controller puts in one EVENT_HISTORY_PAGE. */
#define FURNACE_HMI_CONTROLLER_HISTORY_PAGE 12U

/* The reading state the client keeps (internal; see controller_diagnostics.c). */
typedef struct {
    furnace_hmi_controller_health_t health;
    uint32_t health_request_id;
    uint32_t health_sent_ms;
    uint32_t health_received_ms;
    bool health_pending;
    bool health_asked;

    furnace_hmi_controller_history_t history;
    furnace_hmi_controller_history_t reading;
    uint8_t history_step;
    uint8_t history_log;
    uint8_t history_pages;
    uint16_t history_log_start;
    uint32_t history_cursor;
    uint32_t history_request_id;
    uint32_t history_sent_ms;
    bool history_wait_busy;
    /** The next page is due at once (not after a timeout). */
    bool history_ready;
    bool history_wanted;

    uint32_t generation;
} furnace_hmi_controller_diagnostics_t;

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_CONTROLLER_DIAGNOSTICS_H */
