/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_LAB_OPERATOR_FIXTURES_H
#define FURNACE_HMI_UI_LAB_OPERATOR_FIXTURES_H

#include <furnace_hmi/operator_screens.h>

/** SIMULATION ONLY: the lab's controller-owned content, all of it known. */
void furnace_hmi_operator_fixture_content(
    furnace_hmi_operator_content_t *content,
    const furnace_hmi_operator_device_catalogue_t *device_catalogue
);

#endif /* FURNACE_HMI_UI_LAB_OPERATOR_FIXTURES_H */
