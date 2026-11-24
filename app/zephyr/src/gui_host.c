/* SPDX-License-Identifier: Apache-2.0 */

/*
 * The GUI context's work between LVGL timer passes (gui_host.h). Every value
 * comes from the controller, the application links no simulator, and the
 * screens get no simulated answers. The program library is the controller's,
 * taken from the link as it changes (M21-7b); so are the Device log and
 * Health (M21-7c), the log read when the operator opens it. Manual, Pause,
 * Resume, Stop and the other operator actions are commands to the controller
 * (M21-5b and later); an action that cannot be mapped sends nothing.
 */

#include <string.h>

#include <furnace_hmi/controller_dashboard.h>

#include "controller_link.h"
#include "gui_host.h"
#include "operator_commands.h"
#include "operator_library.h"

/*
 * The link thread publishes at least every 200 ms. Nothing for this long means
 * it is stuck (for example in a blocked write): what is shown is then stale.
 */
#define GUI_MAILBOX_STALE_MS UINT32_C(1000)

bool furnace_hmi_gui_host_init(furnace_hmi_gui_host_t *host, lv_obj_t *parent, uint32_t now_ms)
{
    (void)memset(host, 0, sizeof(*host));
    furnace_hmi_controller_dashboard_state(&host->observation, &host->status);
    if (!furnace_hmi_operator_screens_create(&host->screens, parent, &host->status,
                                             &host->no_content)) {
        return false;
    }
    /* Until the link has read it, the controller's library is being read (LOADING). */
    (void)furnace_hmi_operator_screens_set_library(&host->screens, &host->shown_library);
    /* Nothing read and nothing reported yet. */
    (void)furnace_hmi_operator_screens_set_diagnostics(&host->screens, &host->health,
                                                        &host->history);
    host->last_taken = now_ms;
    return true;
}

void furnace_hmi_gui_host_pass(furnace_hmi_gui_host_t *host, uint32_t now_ms)
{
    furnace_hmi_operator_request_t request;
    bool refresh = false;

    if (furnace_hmi_controller_link_take(&host->observation, &host->seen)) {
        host->last_taken = now_ms;
        host->watchdog_stale = false;
        refresh = true;
    } else if (!host->watchdog_stale &&
               (uint32_t)(now_ms - host->last_taken) >= GUI_MAILBOX_STALE_MS &&
               host->observation.availability == FURNACE_HMI_STATE_AVAILABILITY_CURRENT) {
        /* The link thread is silent: show its last values as stale. */
        host->observation.availability = FURNACE_HMI_STATE_AVAILABILITY_STALE;
        host->watchdog_stale = true;
        refresh = true;
    }
    bool library_changed = furnace_hmi_controller_link_take_library(&host->library,
                                                                    &host->library_seen);

    if (refresh) {
        furnace_hmi_operator_program_limits_t new_limits;
        furnace_hmi_operator_planning_reference_t new_reference;

        furnace_hmi_controller_dashboard_state(&host->observation, &host->status);
        (void)furnace_hmi_operator_screens_update(&host->screens, &host->status);
        furnace_hmi_operator_limits_from_controller(&host->observation, &new_limits,
                                                    &new_reference);
        if (!furnace_hmi_operator_program_limits_equal(&new_limits, &host->limits) ||
            !furnace_hmi_operator_planning_reference_equal(&new_reference, &host->reference)) {
            host->limits = new_limits;
            host->reference = new_reference;
            (void)furnace_hmi_operator_screens_set_program_limits(&host->screens, &host->limits,
                                                                  &host->reference);
            /* The library's ramps and exactness follow the limits and the reference. */
            library_changed = true;
        }
    }
    if (library_changed) {
        furnace_hmi_operator_library_from_controller(&host->library, &host->limits,
                                                     &host->reference, &host->shown_library);
        (void)furnace_hmi_operator_screens_set_library(&host->screens, &host->shown_library);
    }
    {
        /* M21-7e: the controller's clock, advanced to now; the screens redraw per minute. */
        furnace_hmi_operator_controller_clock_t clock;

        furnace_hmi_operator_clock_from_controller(&host->observation, now_ms, &clock);
        (void)furnace_hmi_operator_screens_set_clock(&host->screens, &clock);
    }
    if (furnace_hmi_controller_link_take_diagnostics(&host->health, &host->history,
                                                     &host->diagnostics_seen)) {
        (void)furnace_hmi_operator_screens_set_diagnostics(&host->screens, &host->health,
                                                            &host->history);
    }
    if (furnace_hmi_operator_screens_take_history_read(&host->screens)) {
        furnace_hmi_controller_link_read_history();
    }
    /* The operator's requests go to the link thread; their outcomes come back as feedback. */
    while (furnace_hmi_operator_screens_take_request(&host->screens, &request)) {
        /* Static: a command holds a whole program (M21-7a); only the GUI context uses it. */
        static furnace_hmi_command_t command;
        const uint32_t submission = furnace_hmi_operator_command_for(&request, &command)
                                        ? furnace_hmi_controller_link_submit(&command)
                                        : 0U;

        furnace_hmi_operator_screens_request_submitted(&host->screens, request.kind, submission);
    }
}
