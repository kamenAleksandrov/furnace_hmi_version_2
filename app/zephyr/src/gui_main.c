/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The HMI's Zephyr application on a board with a display. The Zephyr LVGL
 * module initializes LVGL and the display before main; from then on the main
 * thread is the GUI execution context, the only one that changes LVGL
 * (ADR-0005). The one exception is the Zephyr pointer glue, whose input thread
 * reads the LVGL display and its user data, both set before main. Boards
 * without a display build main.c instead.
 *
 * The controller link runs in its own thread (controller_link.c); this context
 * takes its newest observation without waiting. Every value shown comes from
 * the controller: the application links no simulator.
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <furnace_hmi/development_marker.h>
#include <furnace_hmi/foundation.h>
#include <furnace_hmi/state_availability_view.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

#include "controller_link.h"

/* The longest the GUI context sleeps between LVGL timer passes. */
#define GUI_MAX_SLEEP_MS UINT32_C(10)
/* At least one tick, so that native_sim time advances. */
#define GUI_MIN_SLEEP_MS UINT32_C(1)

static furnace_hmi_state_availability_view_t availability_view;
static furnace_hmi_controller_observation_t observation;

/* The availability screen for the link's state, marked as a development build. */
static bool show_availability(furnace_hmi_state_availability_t availability)
{
    const furnace_hmi_state_availability_view_state_t state = {.availability = availability};

    furnace_hmi_state_availability_view_destroy(&availability_view);
    return furnace_hmi_state_availability_view_create(&availability_view, lv_screen_active(),
                                                      &state) &&
           furnace_hmi_development_marker_create(
               availability_view.root, FURNACE_HMI_UI_STRING_SIMULATED_LINK_DEVELOPMENT_LABEL);
}

int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    furnace_hmi_state_availability_t shown = FURNACE_HMI_STATE_AVAILABILITY_UNAVAILABLE;
    uint32_t seen = 0U;

    printk("%s (build probe %u)\n",
           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
           (unsigned int)furnace_hmi_foundation_probe_version());

    if (!device_is_ready(display) || lv_display_get_default() == NULL) {
        printk("Display not ready for LVGL.\n");
        return 2;
    }
    if (!show_availability(shown)) {
        printk("State-availability view creation failed.\n");
        return 13;
    }
    if (furnace_hmi_controller_link_start() != 0) {
        printk("Controller link UART not ready.\n");
        return 3;
    }

    /* Draw the first frame, then show it: the display starts blanked. */
    (void)lv_timer_handler();
    (void)display_blanking_off(display);

    for (;;) {
        uint32_t sleep_ms;

        if (furnace_hmi_controller_link_take(&observation, &seen) &&
            observation.availability != shown) {
            shown = observation.availability;
            if (!show_availability(shown)) {
                printk("State-availability view creation failed.\n");
                return 13;
            }
        }
        sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY || sleep_ms > GUI_MAX_SLEEP_MS) {
            sleep_ms = GUI_MAX_SLEEP_MS;
        } else if (sleep_ms < GUI_MIN_SLEEP_MS) {
            sleep_ms = GUI_MIN_SLEEP_MS;
        }
        k_msleep((int32_t)sleep_ms);
    }
}
