/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/sys/printk.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/ui_strings.h>

int main(void)
{
    printk("%s (build probe %u)\n",
           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
           (unsigned int)furnace_hmi_foundation_probe_version());
    return 0;
}
