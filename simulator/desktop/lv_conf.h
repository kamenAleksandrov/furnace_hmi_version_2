/* SPDX-License-Identifier: Apache-2.0 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* Keep this configuration intentionally small; LVGL supplies defaults for
 * options not overridden here. This is a desktop integration proof only. */
#define LV_COLOR_DEPTH 32
#define LV_MEM_SIZE (4U * 1024U * 1024U)

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_SDL 0

#define LV_USE_SDL 1
#define LV_SDL_INCLUDE_PATH <SDL.h>
#define LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_PARTIAL
#define LV_SDL_BUF_COUNT 2
#define LV_SDL_ACCELERATED 0
#define LV_SDL_FULLSCREEN 0
#define LV_SDL_DIRECT_EXIT 1
#define LV_SDL_USE_EGL 0

#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1
#define LV_USE_TEXTAREA 1
#define LV_FONT_MONTSERRAT_24 1

#endif /* LV_CONF_H */
