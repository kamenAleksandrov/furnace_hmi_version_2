/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_LAB_DEVICE_CATALOGUE_FILE_H
#define FURNACE_HMI_UI_LAB_DEVICE_CATALOGUE_FILE_H

#include <stdbool.h>

#include <furnace_hmi/operator_screens.h>

/** Load the bounded developer-editable catalogue before GUI ownership starts. */
bool furnace_hmi_operator_device_catalogue_load(
    furnace_hmi_operator_device_catalogue_t *catalogue,
    const char *path
);

#endif /* FURNACE_HMI_UI_LAB_DEVICE_CATALOGUE_FILE_H */
