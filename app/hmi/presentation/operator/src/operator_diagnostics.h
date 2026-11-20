/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Internal to the operator screens: the texts of the controller's history
 * entries and health (Phase 21 M21-7c). No LVGL; codes as the furnace
 * firmware defines them (core/diag, core/run, core/fault, app/runtime).
 */

#ifndef FURNACE_HMI_OPERATOR_DIAGNOSTICS_H
#define FURNACE_HMI_OPERATOR_DIAGNOSTICS_H

#include <stddef.h>
#include <stdint.h>

#include <furnace_hmi/diagnostics_model.h>

/* Device log filters an entry belongs to besides All (plan, M21-7c). */
typedef enum {
    OPERATOR_HISTORY_PROGRAMS = 1,
    OPERATOR_HISTORY_ALARMS,
    OPERATOR_HISTORY_SYSTEM,
} operator_history_group_t;

/* The controller's diagnostic categories and severities (furnace core/diag). */
#define OPERATOR_DIAG_CATEGORY_SYSTEM 1U
#define OPERATOR_DIAG_CATEGORY_DIAG 2U
#define OPERATOR_DIAG_CATEGORY_RUN 3U
#define OPERATOR_DIAG_CATEGORY_FAULT 4U
#define OPERATOR_DIAG_CATEGORY_SENSOR 5U
#define OPERATOR_DIAG_CATEGORY_ACTUATOR 6U
#define OPERATOR_DIAG_CATEGORY_COMM 7U
#define OPERATOR_DIAG_CATEGORY_CONFIG 8U
#define OPERATOR_DIAG_SEVERITY_WARNING 1U
#define OPERATOR_DIAG_SEVERITY_ERROR 2U
#define OPERATOR_DIAG_SEVERITY_COUNT 4U

operator_history_group_t operator_history_group(const furnace_hmi_controller_history_entry_t *entry);

/* The operator's text of an entry; an unknown event is named by its number. */
void operator_history_text(const furnace_hmi_controller_history_entry_t *entry, char *buffer,
                           size_t capacity);

/* Time since the controller start, "+h:mm:ss". */
void operator_history_since_start(uint64_t mono_ms, char *buffer, size_t capacity);

/* The raw parameters, "type:value" each, for the details. */
void operator_history_parameters(const furnace_hmi_controller_history_entry_t *entry, char *buffer,
                                 size_t capacity);

/* A controller fault's name: the catalogue's text for its ID, or its number. */
const char *operator_fault_title(uint8_t fault_id, char *buffer, size_t capacity);

/* A health subsystem's name, and the summary of its detail. */
const char *operator_subsystem_name(uint8_t subsystem, char *buffer, size_t capacity);
void operator_subsystem_detail(const furnace_hmi_controller_subsystem_t *subsystem, char *buffer,
                               size_t capacity);

#endif /* FURNACE_HMI_OPERATOR_DIAGNOSTICS_H */
