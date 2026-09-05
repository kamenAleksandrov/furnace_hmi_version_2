/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <furnace_hmi/foundation.h>
#include <furnace_hmi/simulated_controller.h>
#include <furnace_hmi/ui_strings.h>
#include <lvgl.h>
#include <SDL.h>

#include "ui_lab_explorer.h"

#if defined(__ZEPHYR__)
#error "The UI laboratory is host-only and must never be built for Zephyr."
#endif

#if !defined(FURNACE_HMI_DESKTOP_WIDTH) || !defined(FURNACE_HMI_DESKTOP_HEIGHT)
#error "UI laboratory dimensions must be supplied by the target build."
#endif

typedef struct {
    furnace_hmi_simulator_t simulator;
    furnace_hmi_ui_lab_explorer_t explorer;
    lv_obj_t *simulation_marker;
    volatile int requested_scenario;
    volatile int requested_page;
    volatile bool requested_service_dialog;
    volatile bool quit_requested;
} ui_lab_context_t;

static const char *ui_string(furnace_hmi_ui_string_id_t id)
{
    const char *text = furnace_hmi_ui_string(id);
    return text == NULL ? "" : text;
}

static void handle_timers(uint32_t iterations)
{
    for (uint32_t iteration = 0; iteration < iterations; ++iteration) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms == LV_NO_TIMER_READY || delay_ms > 20U) {
            delay_ms = 5U;
        }
        lv_delay_ms(delay_ms);
    }
}

static void clear_views(ui_lab_context_t *context)
{
    furnace_hmi_ui_lab_explorer_destroy(&context->explorer);
    if (context->simulation_marker != NULL && lv_obj_is_valid(context->simulation_marker)) {
        lv_obj_delete(context->simulation_marker);
    }
    context->simulation_marker = NULL;
}

static bool create_simulation_marker(ui_lab_context_t *context)
{
    context->simulation_marker = lv_label_create(lv_layer_top());
    if (context->simulation_marker == NULL) {
        return false;
    }
    lv_label_set_text_static(
        context->simulation_marker,
        ui_string(FURNACE_HMI_UI_STRING_UI_LAB_SIMULATION_BANNER)
    );
    lv_obj_set_style_text_font(context->simulation_marker, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(context->simulation_marker, lv_color_hex(0xF2B84B), LV_PART_MAIN);
    lv_obj_align(context->simulation_marker, LV_ALIGN_TOP_RIGHT, -10, 4);
    lv_obj_remove_flag(
        context->simulation_marker,
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE
    );
    return true;
}

static bool render_scenario(ui_lab_context_t *context)
{
    const furnace_hmi_dashboard_state_t *state =
        furnace_hmi_simulator_state(&context->simulator);

    clear_views(context);
    if (state == NULL) {
        return false;
    }
    if (!furnace_hmi_ui_lab_explorer_create(&context->explorer, lv_screen_active(), state)) {
        return false;
    }

    return create_simulation_marker(context);
}

static int event_watch(void *userdata, SDL_Event *event)
{
    ui_lab_context_t *context = userdata;
    if (event->type == SDL_QUIT) {
        context->quit_requested = true;
        return 0;
    }
    if (event->type != SDL_KEYDOWN || event->key.repeat != 0) {
        return 0;
    }

    furnace_hmi_sim_scenario_t scenario;
    switch (event->key.keysym.sym) {
    case SDLK_1:
        scenario = FURNACE_HMI_SIM_SCENARIO_DISCONNECTED;
        break;
    case SDLK_2:
        scenario = FURNACE_HMI_SIM_SCENARIO_IDLE;
        break;
    case SDLK_3:
        scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL;
        break;
    case SDLK_4:
        scenario = FURNACE_HMI_SIM_SCENARIO_MANUAL;
        break;
    case SDLK_5:
        scenario = FURNACE_HMI_SIM_SCENARIO_PAUSED;
        break;
    case SDLK_6:
        scenario = FURNACE_HMI_SIM_SCENARIO_FAULT;
        break;
    case SDLK_7:
        scenario = FURNACE_HMI_SIM_SCENARIO_STALE;
        break;
    case SDLK_8:
        scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_OVERRUN;
        break;
    case SDLK_h:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_HOME;
        return 0;
    case SDLK_p:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_PROGRAMS;
        return 0;
    case SDLK_d:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_DEVICE;
        return 0;
    case SDLK_s:
        context->requested_page = FURNACE_HMI_UI_LAB_PAGE_SETTINGS;
        return 0;
    case SDLK_i:
        context->requested_service_dialog = true;
        return 0;
    default:
        return 0;
    }
    context->requested_scenario = (int)scenario;
    return 0;
}

static bool parse_arguments(
    int argc,
    char **argv,
    furnace_hmi_sim_scenario_t *scenario,
    bool *smoke_test,
    bool *list_scenarios
)
{
    *scenario = FURNACE_HMI_SIM_SCENARIO_RUNNING_NORMAL;
    *smoke_test = false;
    *list_scenarios = false;
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--smoke-test") == 0) {
            *smoke_test = true;
        }
        else if (strcmp(argv[index], "--list-scenarios") == 0) {
            *list_scenarios = true;
        }
        else if (strcmp(argv[index], "--scenario") == 0 && index + 1 < argc) {
            ++index;
            if (!furnace_hmi_sim_scenario_from_name(argv[index], scenario)) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}

static void print_usage(const char *executable)
{
    (void)fprintf(
        stderr,
        "Usage: %s [--scenario NAME] [--smoke-test] [--list-scenarios]\n",
        executable
    );
    (void)fprintf(stderr, "Scenarios: disconnected, idle, running-normal, manual, paused, fault, stale, running-overrun\n");
    (void)fprintf(stderr, "Keyboard: 1-8 scenarios; H home; P programs; D device; S settings; I service dialog\n");
}

static int run_smoke_test(lv_display_t *display, ui_lab_context_t *context)
{
    lv_display_set_resolution(display, 640, 360);
    for (furnace_hmi_sim_scenario_t scenario = 0;
         scenario < FURNACE_HMI_SIM_SCENARIO_COUNT;
         ++scenario) {
        furnace_hmi_simulator_set_scenario(&context->simulator, scenario);
        if (!furnace_hmi_dashboard_state_is_well_formed(
                furnace_hmi_simulator_state(&context->simulator)
            ) || !render_scenario(context)) {
            (void)fprintf(stderr, "UI laboratory scenario failed: %s\n",
                          furnace_hmi_sim_scenario_name(scenario));
            clear_views(context);
            return 3;
        }
        for (furnace_hmi_ui_lab_page_t page = FURNACE_HMI_UI_LAB_PAGE_HOME;
             page < FURNACE_HMI_UI_LAB_PAGE_COUNT;
             ++page) {
            if (page == FURNACE_HMI_UI_LAB_PAGE_RUNNING) {
                continue;
            }
            if (!furnace_hmi_ui_lab_explorer_navigate(&context->explorer, page)) {
                (void)fprintf(stderr, "UI laboratory page failed: %d\n", (int)page);
                clear_views(context);
                return 3;
            }
            handle_timers(1U);
            if (page == FURNACE_HMI_UI_LAB_PAGE_PROGRAM_LOADING) {
                lv_area_t trajectory_area;
                lv_area_t time_label_area;
                lv_area_t navigation_area;
                lv_obj_get_coords(context->explorer.trajectory, &trajectory_area);
                lv_obj_get_coords(context->explorer.trajectory_time_axis_label, &time_label_area);
                lv_obj_get_coords(context->explorer.nav, &navigation_area);
                if (trajectory_area.y2 >= navigation_area.y1 ||
                    time_label_area.y2 >= navigation_area.y1) {
                    (void)fprintf(stderr,
                                  "UI laboratory Program-loading graph entered bottom navigation.\n");
                    clear_views(context);
                    return 3;
                }
            }
        }
        for (uint8_t mode = 0U; mode < 3U; ++mode) {
            context->explorer.running_view_mode = mode;
            if (!furnace_hmi_ui_lab_explorer_navigate(
                    &context->explorer, FURNACE_HMI_UI_LAB_PAGE_RUNNING
                )) {
                (void)fprintf(stderr, "UI laboratory running view failed: %u\n", (unsigned int)mode);
                clear_views(context);
                return 3;
            }
            if (!lv_obj_has_flag(context->explorer.nav, LV_OBJ_FLAG_HIDDEN)) {
                (void)fprintf(stderr, "UI laboratory active-run navigation remained visible.\n");
                clear_views(context);
                return 3;
            }
            if (furnace_hmi_ui_lab_explorer_navigate(
                    &context->explorer, FURNACE_HMI_UI_LAB_PAGE_HOME
                )) {
                (void)fprintf(stderr, "UI laboratory active-run navigation was not contained.\n");
                clear_views(context);
                return 3;
            }
            handle_timers(1U);
        }
        if (!render_scenario(context)) {
            (void)fprintf(stderr, "UI laboratory reset after active-run view failed.\n");
            clear_views(context);
            return 3;
        }
        for (uint8_t kind = 0U; kind < 3U; ++kind) {
            context->explorer.draft_stage_kind = kind;
            if (!furnace_hmi_ui_lab_explorer_navigate(
                    &context->explorer, FURNACE_HMI_UI_LAB_PAGE_STAGE_EDITOR
                )) {
                (void)fprintf(stderr, "UI laboratory stage-editor view failed: %u\n",
                              (unsigned int)kind);
                clear_views(context);
                return 3;
            }
            if (!lv_obj_has_flag(context->explorer.nav, LV_OBJ_FLAG_HIDDEN)) {
                (void)fprintf(stderr, "UI laboratory Program-editor navigation remained visible.\n");
                clear_views(context);
                return 3;
            }
            handle_timers(1U);
        }
        if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context->explorer)) {
            (void)fprintf(stderr, "UI laboratory service dialog failed.\n");
            clear_views(context);
            return 3;
        }
        handle_timers(2U);
        lv_obj_t *programs_button = lv_obj_get_child(context->explorer.nav, 1);
        if (programs_button == NULL ||
            lv_obj_send_event(programs_button, LV_EVENT_CLICKED, NULL) != LV_RESULT_OK) {
            (void)fprintf(stderr, "UI laboratory navigation callback failed.\n");
            clear_views(context);
            return 3;
        }
        handle_timers(2U);
        if (context->explorer.page != FURNACE_HMI_UI_LAB_PAGE_PROGRAMS) {
            (void)fprintf(stderr, "UI laboratory deferred navigation failed.\n");
            clear_views(context);
            return 3;
        }
    }
    clear_views(context);
    return 0;
}

int main(int argc, char **argv)
{
    furnace_hmi_sim_scenario_t initial_scenario;
    bool smoke_test;
    bool list_scenarios;
    if (!parse_arguments(argc, argv, &initial_scenario, &smoke_test, &list_scenarios)) {
        print_usage(argv[0]);
        return 64;
    }
    if (list_scenarios) {
        for (furnace_hmi_sim_scenario_t scenario = 0;
             scenario < FURNACE_HMI_SIM_SCENARIO_COUNT;
             ++scenario) {
            (void)printf("%s\n", furnace_hmi_sim_scenario_name(scenario));
        }
        return 0;
    }
    if (furnace_hmi_foundation_probe_version() != FURNACE_HMI_FOUNDATION_PROBE_VERSION) {
        (void)fprintf(stderr, "Portable foundation build probe mismatch.\n");
        return 1;
    }

    ui_lab_context_t context = {0};
    context.requested_scenario = -1;
    context.requested_page = -1;
    context.requested_service_dialog = false;
    context.quit_requested = false;
    furnace_hmi_simulator_initialize(&context.simulator, initial_scenario);

    lv_init();
    lv_display_t *display = lv_sdl_window_create(
        FURNACE_HMI_DESKTOP_WIDTH,
        FURNACE_HMI_DESKTOP_HEIGHT
    );
    if (display == NULL) {
        (void)fprintf(stderr, "SDL display creation failed.\n");
        return 2;
    }
    lv_sdl_window_set_title(display, ui_string(FURNACE_HMI_UI_STRING_APPLICATION_TITLE));
    lv_sdl_window_set_resizeable(display, false);
    (void)lv_sdl_mouse_create();
    SDL_AddEventWatch(event_watch, &context);

    int result;
    if (smoke_test) {
        result = run_smoke_test(display, &context);
        SDL_DelEventWatch(event_watch, &context);
        return result;
    }
    if (!render_scenario(&context)) {
        (void)fprintf(stderr, "Initial UI laboratory scenario could not be rendered.\n");
        SDL_DelEventWatch(event_watch, &context);
        return 3;
    }

    uint32_t last_tick = lv_tick_get();
    while (!context.quit_requested) {
        const int requested = context.requested_scenario;
        context.requested_scenario = -1;
        if (requested >= 0 && requested < FURNACE_HMI_SIM_SCENARIO_COUNT) {
            furnace_hmi_simulator_set_scenario(
                &context.simulator,
                (furnace_hmi_sim_scenario_t)requested
            );
            if (!render_scenario(&context)) {
                (void)fprintf(stderr, "Requested UI laboratory scenario could not be rendered.\n");
                result = 4;
                goto cleanup;
            }
        }
        const int requested_page = context.requested_page;
        context.requested_page = -1;
        if (requested_page >= 0 && requested_page < FURNACE_HMI_UI_LAB_PAGE_COUNT &&
            !(context.explorer.page == FURNACE_HMI_UI_LAB_PAGE_RUNNING &&
              requested_page != FURNACE_HMI_UI_LAB_PAGE_RUNNING) &&
            !furnace_hmi_ui_lab_explorer_navigate(
                &context.explorer,
                (furnace_hmi_ui_lab_page_t)requested_page
            )) {
            (void)fprintf(stderr, "Requested UI laboratory page could not be rendered.\n");
            result = 4;
            goto cleanup;
        }
        if (context.requested_service_dialog) {
            context.requested_service_dialog = false;
            if (!furnace_hmi_ui_lab_explorer_show_service_warning(&context.explorer)) {
                (void)fprintf(stderr, "UI laboratory service dialog could not be rendered.\n");
                result = 4;
                goto cleanup;
            }
        }

        const uint32_t now = lv_tick_get();
        if (now - last_tick >= 1000U) {
            furnace_hmi_simulator_tick(&context.simulator, (now - last_tick) / 1000U);
            last_tick = now;
            if (context.explorer.root != NULL &&
                !furnace_hmi_ui_lab_explorer_update(
                    &context.explorer, furnace_hmi_simulator_state(&context.simulator))) {
                (void)fprintf(stderr, "UI laboratory explorer update failed.\n");
                result = 5;
                goto cleanup;
            }
        }
        handle_timers(1U);
    }
    result = 0;

cleanup:
    clear_views(&context);
    SDL_DelEventWatch(event_watch, &context);
    return result;
}
