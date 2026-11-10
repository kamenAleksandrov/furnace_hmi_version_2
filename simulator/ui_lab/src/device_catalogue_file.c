/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The UI lab's developer-editable Device catalogue (data/device_service_catalogue.txt),
 * read from a host file before LVGL starts. Host-only: the shared operator screens
 * receive the catalogue as data (Phase 21 M21-2b).
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "device_catalogue_file.h"

#define OPERATOR_DEVICE_CATALOGUE_MAX_LINES 64U
#define OPERATOR_DEVICE_CATALOGUE_LINE_CAPACITY 1024U

static void trim_device_catalogue_field(char *text)
{
    char *start = text;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1U);
    }
    size_t length = strlen(text);
    while (length > 0U && isspace((unsigned char)text[length - 1U])) {
        text[--length] = '\0';
    }
}

static bool next_device_catalogue_field(char **cursor, char *target, size_t capacity)
{
    if (cursor == NULL || *cursor == NULL || target == NULL || capacity == 0U) {
        return false;
    }
    char *field = *cursor;
    char *separator = strchr(field, '|');
    if (separator == NULL) {
        *cursor = NULL;
    }
    else {
        *separator = '\0';
        *cursor = separator + 1;
    }
    trim_device_catalogue_field(field);
    const size_t length = strlen(field);
    if (length == 0U || length >= capacity) {
        return false;
    }
    memcpy(target, field, length + 1U);
    return true;
}

static bool parse_device_task_state(
    const char *text,
    furnace_hmi_operator_device_task_state_t *state
)
{
    if (text == NULL || state == NULL) {
        return false;
    }
    if (strcmp(text, "ok") == 0) {
        *state = FURNACE_HMI_OPERATOR_DEVICE_TASK_UP_TO_DATE;
    }
    else if (strcmp(text, "soon") == 0) {
        *state = FURNACE_HMI_OPERATOR_DEVICE_TASK_DUE_SOON;
    }
    else if (strcmp(text, "due") == 0) {
        *state = FURNACE_HMI_OPERATOR_DEVICE_TASK_DUE;
    }
    else if (strcmp(text, "condition") == 0) {
        *state = FURNACE_HMI_OPERATOR_DEVICE_TASK_CONDITION;
    }
    else if (strcmp(text, "unknown") == 0) {
        *state = FURNACE_HMI_OPERATOR_DEVICE_TASK_UNKNOWN;
    }
    else {
        return false;
    }
    return true;
}

bool furnace_hmi_operator_device_catalogue_load(
    furnace_hmi_operator_device_catalogue_t *catalogue,
    const char *path
)
{
    if (catalogue == NULL || path == NULL || path[0] == '\0') {
        return false;
    }
    memset(catalogue, 0, sizeof(*catalogue));
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    bool valid = false;
    bool revision_seen = false;
    char line[OPERATOR_DEVICE_CATALOGUE_LINE_CAPACITY];
    unsigned line_count = 0U;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (++line_count > OPERATOR_DEVICE_CATALOGUE_MAX_LINES ||
            (strchr(line, '\n') == NULL && !feof(file))) {
            goto close_file;
        }
        trim_device_catalogue_field(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        char *cursor = line;
        char record[16];
        if (!next_device_catalogue_field(&cursor, record, sizeof(record))) {
            goto close_file;
        }
        if (strcmp(record, "revision") == 0) {
            if (revision_seen ||
                !next_device_catalogue_field(&cursor, catalogue->revision,
                                             sizeof(catalogue->revision)) ||
                cursor != NULL) {
                goto close_file;
            }
            revision_seen = true;
            continue;
        }
        if (strcmp(record, "component") != 0) {
            goto close_file;
        }

        furnace_hmi_operator_device_component_t component = {0};
        furnace_hmi_operator_device_task_t task = {0};
        if (!next_device_catalogue_field(&cursor, component.id, sizeof(component.id)) ||
            !next_device_catalogue_field(&cursor, component.name, sizeof(component.name)) ||
            !next_device_catalogue_field(&cursor, component.part, sizeof(component.part)) ||
            !next_device_catalogue_field(&cursor, component.location, sizeof(component.location)) ||
            !next_device_catalogue_field(&cursor, component.usage, sizeof(component.usage)) ||
            !next_device_catalogue_field(&cursor, component.source, sizeof(component.source)) ||
            !next_device_catalogue_field(&cursor, component.instance, sizeof(component.instance)) ||
            !next_device_catalogue_field(&cursor, task.id, sizeof(task.id)) ||
            !next_device_catalogue_field(&cursor, task.name, sizeof(task.name)) ||
            !next_device_catalogue_field(&cursor, task.basis, sizeof(task.basis)) ||
            !next_device_catalogue_field(&cursor, task.interval, sizeof(task.interval)) ||
            !next_device_catalogue_field(&cursor, task.remaining, sizeof(task.remaining)) ||
            !next_device_catalogue_field(&cursor, task.state, sizeof(task.state)) ||
            !next_device_catalogue_field(&cursor, task.last, sizeof(task.last)) ||
            !next_device_catalogue_field(&cursor, task.procedure, sizeof(task.procedure)) ||
            cursor != NULL ||
            !parse_device_task_state(task.state, &task.task_state)) {
            goto close_file;
        }

        size_t component_index = catalogue->component_count;
        for (size_t index = 0U; index < catalogue->component_count; ++index) {
            if (strcmp(catalogue->components[index].id, component.id) == 0) {
                component_index = index;
                break;
            }
        }
        if (component_index == catalogue->component_count) {
            if (catalogue->component_count >= FURNACE_HMI_OPERATOR_DEVICE_MAX_COMPONENTS) {
                goto close_file;
            }
            catalogue->components[component_index] = component;
            ++catalogue->component_count;
        }
        else if (strcmp(catalogue->components[component_index].name, component.name) != 0 ||
                 strcmp(catalogue->components[component_index].instance, component.instance) != 0) {
            goto close_file;
        }
        furnace_hmi_operator_device_component_t *stored = &catalogue->components[component_index];
        if (stored->task_count >= FURNACE_HMI_OPERATOR_DEVICE_MAX_TASKS) {
            goto close_file;
        }
        stored->tasks[stored->task_count++] = task;
    }
    valid = revision_seen;

close_file:
    (void)fclose(file);
    if (!valid) {
        memset(catalogue, 0, sizeof(*catalogue));
    }
    return valid;
}
