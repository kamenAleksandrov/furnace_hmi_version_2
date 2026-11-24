/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The HMI's Zephyr application on a board with a display. The Zephyr LVGL
 * module initializes LVGL and the display before main; from then on the main
 * thread is the GUI execution context, the only one that changes LVGL
 * (ADR-0005). The one exception is the Zephyr pointer glue, whose input thread
 * reads the LVGL display and its user data, both set before main. Boards
 * without a display build main.c instead.
 *
 * It shows the shared operator screens (Phase 21 M21-2b). The controller link
 * runs in its own thread (controller_link.c); between LVGL timer passes this
 * context takes the link's news into the screens and hands the screens'
 * requests to the link without waiting (gui_host.c, which the end-to-end
 * operator workflows also run, M21-8). Every value comes from the controller,
 * the application links no simulator, and the screens get no simulated
 * answers. Maintenance and service records stay unavailable until the
 * controller reports them.
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <furnace_hmi/development_marker.h>
#include <furnace_hmi/foundation.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

#include "controller_link.h"
#include "gui_host.h"

/* The longest the GUI context sleeps between LVGL timer passes. */
#define GUI_MAX_SLEEP_MS UINT32_C(10)
/* At least one tick, so that native_sim time advances. */
#define GUI_MIN_SLEEP_MS UINT32_C(1)

/* The screens and what they show (gui_host.h); static, as it holds whole libraries. */
static furnace_hmi_gui_host_t host;

int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    printk("%s (build probe %u)\n",
           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
           (unsigned int)furnace_hmi_foundation_probe_version());

    if (!device_is_ready(display) || lv_display_get_default() == NULL) {
        printk("Display not ready for LVGL.\n");
        return 2;
    }
    if (!furnace_hmi_gui_host_init(&host, lv_screen_active(), k_uptime_get_32()) ||
        furnace_hmi_development_tag_create(FURNACE_HMI_UI_STRING_SIMULATED_LINK_DEVELOPMENT_LABEL) ==
            NULL) {
        printk("Operator screens could not be created.\n");
        return 13;
    }
    if (furnace_hmi_controller_link_start() != 0) {
        printk("Controller link UART not ready.\n");
        return 3;
    }

    /* Draw the first frame, then show it: the display starts blanked. */
    (void)lv_timer_handler();
    (void)display_blanking_off(display);
    host.last_taken = k_uptime_get_32();

    for (;;) {
        uint32_t sleep_ms;

        furnace_hmi_gui_host_pass(&host, k_uptime_get_32());
        sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY || sleep_ms > GUI_MAX_SLEEP_MS) {
            sleep_ms = GUI_MAX_SLEEP_MS;
        } else if (sleep_ms < GUI_MIN_SLEEP_MS) {
            sleep_ms = GUI_MIN_SLEEP_MS;
        }
        k_msleep((int32_t)sleep_ms);
    }
}
