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
 * runs in its own thread (controller_link.c); this context takes its newest
 * observation without waiting and maps it to the screens' status model. Every
 * value comes from the controller, the application links no simulator, and
 * the screens get no controller-owned content and no simulated answers: until
 * the controller provides them (M21-7), library, log and clock are shown as
 * unavailable. Manual, Pause, Resume and Stop are commands to the controller
 * (M21-5b); other actions that would change controller state send nothing.
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <furnace_hmi/controller_dashboard.h>
#include <furnace_hmi/development_marker.h>
#include <furnace_hmi/foundation.h>
#include <furnace_hmi/operator_screens.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>

#include "controller_link.h"

/* The longest the GUI context sleeps between LVGL timer passes. */
#define GUI_MAX_SLEEP_MS UINT32_C(10)
/* At least one tick, so that native_sim time advances. */
#define GUI_MIN_SLEEP_MS UINT32_C(1)
/*
 * The link thread publishes at least every 200 ms. Nothing for this long means
 * it is stuck (for example in a blocked write): what is shown is then stale.
 */
#define GUI_MAILBOX_STALE_MS UINT32_C(1000)

static furnace_hmi_operator_screens_t screens;
static furnace_hmi_operator_content_t no_content;
static furnace_hmi_controller_observation_t observation;
static furnace_hmi_dashboard_state_t status;

/*
 * The controller command for a screen request (M21-5b). Run commands name the
 * run the controller reports now. False for requests that are not commands
 * yet (program Start: M21-7).
 */
static bool command_for(const furnace_hmi_operator_request_t *request, furnace_hmi_command_t *command)
{
    *command = (furnace_hmi_command_t){.run_id = observation.run_id};
    switch (request->kind) {
    case FURNACE_HMI_OPERATOR_REQUEST_START_PREPARE:
        command->kind = FURNACE_HMI_COMMAND_START_PREPARE;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_MANUAL_START:
        command->kind = FURNACE_HMI_COMMAND_MANUAL_START;
        command->session_minutes = (uint32_t)request->session_minutes;
        break;
    case FURNACE_HMI_OPERATOR_REQUEST_MANUAL_RETARGET:
        command->kind = FURNACE_HMI_COMMAND_MANUAL_RETARGET;
        break;
    case FURNACE_HMI_OPERATOR_REQUEST_PAUSE:
        command->kind = FURNACE_HMI_COMMAND_PAUSE;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_RESUME:
        command->kind = FURNACE_HMI_COMMAND_RESUME;
        return true;
    case FURNACE_HMI_OPERATOR_REQUEST_STOP:
        command->kind = FURNACE_HMI_COMMAND_STOP;
        return true;
    default:
        return false;
    }
    command->target_decicelsius = request->target_c * 10;
    command->rate_decicelsius_per_min = request->rate_tenths_c_per_minute;
    return true;
}

int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    uint32_t seen = 0U;
    uint32_t last_taken;
    bool watchdog_stale = false;

    printk("%s (build probe %u)\n",
           furnace_hmi_ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE),
           (unsigned int)furnace_hmi_foundation_probe_version());

    if (!device_is_ready(display) || lv_display_get_default() == NULL) {
        printk("Display not ready for LVGL.\n");
        return 2;
    }
    furnace_hmi_controller_dashboard_state(&observation, &status);
    if (!furnace_hmi_operator_screens_create(&screens, lv_screen_active(), &status, &no_content) ||
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
    last_taken = k_uptime_get_32();

    for (;;) {
        const uint32_t now = k_uptime_get_32();
        furnace_hmi_operator_request_t request;
        bool refresh = false;
        uint32_t sleep_ms;

        if (furnace_hmi_controller_link_take(&observation, &seen)) {
            last_taken = now;
            watchdog_stale = false;
            refresh = true;
        } else if (!watchdog_stale && (uint32_t)(now - last_taken) >= GUI_MAILBOX_STALE_MS &&
                   observation.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT) {
            /* The link thread is silent: show its last values as stale. */
            observation.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
            watchdog_stale = true;
            refresh = true;
        }
        if (refresh) {
            furnace_hmi_controller_dashboard_state(&observation, &status);
            (void)furnace_hmi_operator_screens_update(&screens, &status);
        }
        /* The operator's requests go to the link thread; their outcomes come back as feedback. */
        while (furnace_hmi_operator_screens_take_request(&screens, &request)) {
            furnace_hmi_command_t command;
            const uint32_t submission =
                command_for(&request, &command) ? furnace_hmi_controller_link_submit(&command) : 0U;

            furnace_hmi_operator_screens_request_submitted(&screens, request.kind, submission);
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
