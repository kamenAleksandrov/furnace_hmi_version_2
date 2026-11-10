/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_OPERATOR_THEME_H
#define FURNACE_HMI_OPERATOR_THEME_H

#include <lvgl.h>

#include <furnace_hmi/hmi_theme.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Colours come from the active theme (app/hmi/presentation/theme/themes.json,
 * the same tokens as the web demo). Never write a colour value in UI code:
 * name a role. Depth: page < panel < tile; value fields are recessed wells.
 * Everything here runs in the LVGL owner context (ADR-0005). */

/** Use a generated theme for everything styled or drawn from now on. Pages
 * already built keep their colours until rebuilt. Returns false for an
 * invalid id (the theme is then unchanged). */
bool operator_theme_select(furnace_hmi_theme_id_t id);

/** The selected theme (the default theme until one is selected). */
furnace_hmi_theme_id_t operator_theme_selected(void);

/** Use a caller-owned palette of FURNACE_HMI_COLOR_COUNT colours instead, for
 * tests and theme previews. It must stay valid until another palette or theme
 * is selected. NULL returns to the selected theme. */
void operator_theme_use_palette(const furnace_hmi_color_t *colors);

/** Colour of a role in the active palette (black for an invalid role). */
lv_color_t operator_color(furnace_hmi_color_role_t role);

/** Opacity of a role in the active palette (cover for an invalid role). */
lv_opa_t operator_opa(furnace_hmi_color_role_t role);

/** Set a border's colour and opacity from one role. */
void operator_theme_border(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector);

/** Set a background's colour and opacity from one role. */
void operator_theme_background(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector);

#define OPERATOR_THEME_PAGE operator_color(FURNACE_HMI_COLOR_PAGE)
#define OPERATOR_THEME_PANEL operator_color(FURNACE_HMI_COLOR_PANEL)
#define OPERATOR_THEME_TILE_HI operator_color(FURNACE_HMI_COLOR_TILE_HI)
#define OPERATOR_THEME_TILE_LO operator_color(FURNACE_HMI_COLOR_TILE_LO)
#define OPERATOR_THEME_WELL operator_color(FURNACE_HMI_COLOR_WELL)
#define OPERATOR_THEME_TEXT operator_color(FURNACE_HMI_COLOR_TEXT)
#define OPERATOR_THEME_MUTED operator_color(FURNACE_HMI_COLOR_MUTED)
#define OPERATOR_THEME_DIM operator_color(FURNACE_HMI_COLOR_DIM)
#define OPERATOR_THEME_AMBER operator_color(FURNACE_HMI_COLOR_AMBER)
#define OPERATOR_THEME_AMBER_TEXT operator_color(FURNACE_HMI_COLOR_AMBER_TEXT)
#define OPERATOR_THEME_RED operator_color(FURNACE_HMI_COLOR_RED)
#define OPERATOR_THEME_RED_TEXT operator_color(FURNACE_HMI_COLOR_RED_TEXT)
#define OPERATOR_THEME_HEAT operator_color(FURNACE_HMI_COLOR_HEAT)
#define OPERATOR_THEME_HOLD operator_color(FURNACE_HMI_COLOR_HOLD)
#define OPERATOR_THEME_COOL operator_color(FURNACE_HMI_COLOR_COOL)
#define OPERATOR_THEME_OK operator_color(FURNACE_HMI_COLOR_OK)
#define OPERATOR_THEME_BLUE operator_color(FURNACE_HMI_COLOR_BLUE)

/** What a tile means. Only the edge, glow and text colour differ by tone. */
typedef enum {
    OPERATOR_TONE_TILE = 0,     /**< Ordinary tile: soft amber edge. */
    OPERATOR_TONE_QUIET,        /**< Unselected tab/rail/selector: steel edge, muted text. */
    OPERATOR_TONE_NAV,          /**< Navigation action (Back, pager, More): steel edge. */
    OPERATOR_TONE_SELECTED,     /**< Current tab/choice: amber edge and glow. */
    OPERATOR_TONE_PRIMARY,      /**< Primary action and Cancel: amber edge and text. */
    OPERATOR_TONE_EDIT,         /**< Edit: yellow edge and text. */
    OPERATOR_TONE_DANGER,       /**< Delete, Stop, Clear, Discard: red edge and text. */
    OPERATOR_TONE_HEAT,         /**< Selected Heating type. */
    OPERATOR_TONE_HOLD,         /**< Selected Hold type. */
    OPERATOR_TONE_COOL,         /**< Selected Cooling type. */
    OPERATOR_TONE_CARD_HEAT,    /**< Heating stage card: kind-coloured edge, normal text. */
    OPERATOR_TONE_CARD_HOLD,
    OPERATOR_TONE_CARD_COOL,
    OPERATOR_TONE_NEUTRAL,      /**< Neutral key or unselected type: white edge, muted text. */
    OPERATOR_TONE_KEY,          /**< Keyboard character key: no edge. */
    OPERATOR_TONE_WELL,         /**< Recessed value field: well face, soft amber edge. */
    OPERATOR_TONE_OK,           /**< Healthy status row: green edge. */
    OPERATOR_TONE_COOLING,      /**< Cooling toggle: blue edge and text. */
    OPERATOR_TONE_FIELD_HOLD,   /**< Stage field while Hold is selected: purple edge. */
    OPERATOR_TONE_SEGMENT,      /**< Unselected part of a segmented tile: flat, muted text. */
    OPERATOR_TONE_FLAT,         /**< No tile at all; the caller styles it. */
    OPERATOR_TONE_COUNT,
} operator_tone_t;

/** The side carrying the edge: the side facing away from where the tile sits. */
typedef enum {
    OPERATOR_EDGE_BOTTOM = 0,   /**< Top toolbars and free-standing tiles. */
    OPERATOR_EDGE_TOP,          /**< Bottom bars and action rows. */
    OPERATOR_EDGE_RIGHT,        /**< Left rails and Quick launch. */
    OPERATOR_EDGE_LEFT,         /**< Right-hand rows and field buttons. */
    OPERATOR_EDGE_COUNT,
} operator_edge_t;

/** Style an interactive tile. Safe to call again to change tone or side. */
void operator_theme_button(lv_obj_t *button, operator_tone_t tone, operator_edge_t edge);

/** Move an already themed tile's edge to another side, keeping its tone. */
void operator_theme_set_edge_side(lv_obj_t *object, operator_edge_t edge);

/** Apply operator_theme_set_edge_side() to every direct child of a row. */
void operator_theme_row_edges(lv_obj_t *row, operator_edge_t edge);

/** The same-size Regular or SemiBold face of an HMI font; others unchanged. */
const lv_font_t *operator_theme_font_weight(const lv_font_t *font, bool strong);

/** Text colour a tone gives its label (also used for icons). */
lv_color_t operator_theme_text_color(operator_tone_t tone);

/** Flat container one level above the page. */
void operator_theme_panel(lv_obj_t *panel);

/** Non-interactive information tile (readings, facts, rows). */
void operator_theme_info_tile(lv_obj_t *tile);

/** Decorate any object with a tone's edge on one side, without restyling it. */
void operator_theme_edge(lv_obj_t *object, operator_tone_t tone, operator_edge_t edge);

/** Segment of a segmented tile: flat face; selected segments keep the amber
 * edge and glow. `first` omits the divider on the left. */
void operator_theme_segment(lv_obj_t *button, bool selected, bool first);

/** Dialog card: raised face, strong hairline, amber (or red) top edge. */
void operator_theme_dialog(lv_obj_t *card, bool destructive);

/** Top strip: fading amber line, warm glow while the interior light is on. */
void operator_theme_light_strip(lv_obj_t *line, lv_obj_t *glow, bool light_on);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_OPERATOR_THEME_H */
