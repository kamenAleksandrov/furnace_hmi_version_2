/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The HMI's Zephyr application on a board with a display. The Zephyr LVGL
 * module initializes LVGL and the display before main; from then on the main
 * thread is the GUI execution context, the only one that changes LVGL
 * (ADR-0005). The one exception is the Zephyr pointer glue, whose input thread
 * reads the LVGL display and its user data, both set before main. Boards
 * without a display build main.c instead.
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/state_availability_view.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

/* The longest the GUI context sleeps between LVGL timer passes. */
#define GUI_MAX_SLEEP_MS UINT32_C(10)
/* At least one tick, so that native_sim time advances. */
#define GUI_MIN_SLEEP_MS UINT32_C(1)

int main(void)
{
    static furnace_hmi_state_availability_view_t availability_view;
    const furnace_hmi_state_availability_view_state_t initial_state = {0};
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    printk("%s (build probe %u)\n",
           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
           (unsigned int)furnace_hmi_foundation_probe_version());

    if (!device_is_ready(display) || lv_display_get_default() == NULL) {
        printk("Display not ready for LVGL.\n");
        return 2;
    }
    if (!furnace_hmi_state_availability_view_create(&availability_view, lv_screen_active(),
                                                    &initial_state)) {
        printk("State-availability view creation failed.\n");
        return 13;
    }

    /* Draw the first frame, then show it: the display starts blanked. */
    (void)lv_timer_handler();
    (void)display_blanking_off(display);

    for (;;) {
        uint32_t sleep_ms = lv_timer_handler();

        if (sleep_ms == LV_NO_TIMER_READY || sleep_ms > GUI_MAX_SLEEP_MS) {
            sleep_ms = GUI_MAX_SLEEP_MS;
        } else if (sleep_ms < GUI_MIN_SLEEP_MS) {
            sleep_ms = GUI_MIN_SLEEP_MS;
        }
        k_msleep((int32_t)sleep_ms);
    }
}
