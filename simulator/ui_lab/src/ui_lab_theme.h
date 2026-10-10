/* SPDX-License-Identifier: Apache-2.0 */

#ifndef FURNACE_HMI_UI_LAB_THEME_H
#define FURNACE_HMI_UI_LAB_THEME_H

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
bool ui_lab_theme_select(furnace_hmi_theme_id_t id);

/** The selected theme (the default theme until one is selected). */
furnace_hmi_theme_id_t ui_lab_theme_selected(void);

/** Use a caller-owned palette of FURNACE_HMI_COLOR_COUNT colours instead, for
 * tests and theme previews. It must stay valid until another palette or theme
 * is selected. NULL returns to the selected theme. */
void ui_lab_theme_use_palette(const furnace_hmi_color_t *colors);

/** Colour of a role in the active palette (black for an invalid role). */
lv_color_t ui_lab_color(furnace_hmi_color_role_t role);

/** Opacity of a role in the active palette (cover for an invalid role). */
lv_opa_t ui_lab_opa(furnace_hmi_color_role_t role);

/** Set a border's colour and opacity from one role. */
void ui_lab_theme_border(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector);

/** Set a background's colour and opacity from one role. */
void ui_lab_theme_background(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector);

#define UI_LAB_THEME_PAGE ui_lab_color(FURNACE_HMI_COLOR_PAGE)
#define UI_LAB_THEME_PANEL ui_lab_color(FURNACE_HMI_COLOR_PANEL)
#define UI_LAB_THEME_TILE_HI ui_lab_color(FURNACE_HMI_COLOR_TILE_HI)
#define UI_LAB_THEME_TILE_LO ui_lab_color(FURNACE_HMI_COLOR_TILE_LO)
#define UI_LAB_THEME_WELL ui_lab_color(FURNACE_HMI_COLOR_WELL)
#define UI_LAB_THEME_TEXT ui_lab_color(FURNACE_HMI_COLOR_TEXT)
#define UI_LAB_THEME_MUTED ui_lab_color(FURNACE_HMI_COLOR_MUTED)
#define UI_LAB_THEME_DIM ui_lab_color(FURNACE_HMI_COLOR_DIM)
#define UI_LAB_THEME_AMBER ui_lab_color(FURNACE_HMI_COLOR_AMBER)
#define UI_LAB_THEME_AMBER_TEXT ui_lab_color(FURNACE_HMI_COLOR_AMBER_TEXT)
#define UI_LAB_THEME_RED ui_lab_color(FURNACE_HMI_COLOR_RED)
#define UI_LAB_THEME_RED_TEXT ui_lab_color(FURNACE_HMI_COLOR_RED_TEXT)
#define UI_LAB_THEME_HEAT ui_lab_color(FURNACE_HMI_COLOR_HEAT)
#define UI_LAB_THEME_HOLD ui_lab_color(FURNACE_HMI_COLOR_HOLD)
#define UI_LAB_THEME_COOL ui_lab_color(FURNACE_HMI_COLOR_COOL)
#define UI_LAB_THEME_OK ui_lab_color(FURNACE_HMI_COLOR_OK)
#define UI_LAB_THEME_BLUE ui_lab_color(FURNACE_HMI_COLOR_BLUE)

/** What a tile means. Only the edge, glow and text colour differ by tone. */
typedef enum {
    UI_LAB_TONE_TILE = 0,     /**< Ordinary tile: soft amber edge. */
    UI_LAB_TONE_QUIET,        /**< Unselected tab/rail/selector: steel edge, muted text. */
    UI_LAB_TONE_NAV,          /**< Navigation action (Back, pager, More): steel edge. */
    UI_LAB_TONE_SELECTED,     /**< Current tab/choice: amber edge and glow. */
    UI_LAB_TONE_PRIMARY,      /**< Primary action and Cancel: amber edge and text. */
    UI_LAB_TONE_EDIT,         /**< Edit: yellow edge and text. */
    UI_LAB_TONE_DANGER,       /**< Delete, Stop, Clear, Discard: red edge and text. */
    UI_LAB_TONE_HEAT,         /**< Selected Heating type. */
    UI_LAB_TONE_HOLD,         /**< Selected Hold type. */
    UI_LAB_TONE_COOL,         /**< Selected Cooling type. */
    UI_LAB_TONE_CARD_HEAT,    /**< Heating stage card: kind-coloured edge, normal text. */
    UI_LAB_TONE_CARD_HOLD,
    UI_LAB_TONE_CARD_COOL,
    UI_LAB_TONE_NEUTRAL,      /**< Neutral key or unselected type: white edge, muted text. */
    UI_LAB_TONE_KEY,          /**< Keyboard character key: no edge. */
    UI_LAB_TONE_WELL,         /**< Recessed value field: well face, soft amber edge. */
    UI_LAB_TONE_OK,           /**< Healthy status row: green edge. */
    UI_LAB_TONE_COOLING,      /**< Cooling toggle: blue edge and text. */
    UI_LAB_TONE_FIELD_HOLD,   /**< Stage field while Hold is selected: purple edge. */
    UI_LAB_TONE_SEGMENT,      /**< Unselected part of a segmented tile: flat, muted text. */
    UI_LAB_TONE_FLAT,         /**< No tile at all; the caller styles it. */
    UI_LAB_TONE_COUNT,
} ui_lab_tone_t;

/** The side carrying the edge: the side facing away from where the tile sits. */
typedef enum {
    UI_LAB_EDGE_BOTTOM = 0,   /**< Top toolbars and free-standing tiles. */
    UI_LAB_EDGE_TOP,          /**< Bottom bars and action rows. */
    UI_LAB_EDGE_RIGHT,        /**< Left rails and Quick launch. */
    UI_LAB_EDGE_LEFT,         /**< Right-hand rows and field buttons. */
    UI_LAB_EDGE_COUNT,
} ui_lab_edge_t;

/** Style an interactive tile. Safe to call again to change tone or side. */
void ui_lab_theme_button(lv_obj_t *button, ui_lab_tone_t tone, ui_lab_edge_t edge);

/** Move an already themed tile's edge to another side, keeping its tone. */
void ui_lab_theme_set_edge_side(lv_obj_t *object, ui_lab_edge_t edge);

/** Apply ui_lab_theme_set_edge_side() to every direct child of a row. */
void ui_lab_theme_row_edges(lv_obj_t *row, ui_lab_edge_t edge);

/** The same-size Regular or SemiBold face of an HMI font; others unchanged. */
const lv_font_t *ui_lab_theme_font_weight(const lv_font_t *font, bool strong);

/** Text colour a tone gives its label (also used for icons). */
lv_color_t ui_lab_theme_text_color(ui_lab_tone_t tone);

/** Flat container one level above the page. */
void ui_lab_theme_panel(lv_obj_t *panel);

/** Non-interactive information tile (readings, facts, rows). */
void ui_lab_theme_info_tile(lv_obj_t *tile);

/** Decorate any object with a tone's edge on one side, without restyling it. */
void ui_lab_theme_edge(lv_obj_t *object, ui_lab_tone_t tone, ui_lab_edge_t edge);

/** Segment of a segmented tile: flat face; selected segments keep the amber
 * edge and glow. `first` omits the divider on the left. */
void ui_lab_theme_segment(lv_obj_t *button, bool selected, bool first);

/** Dialog card: raised face, strong hairline, amber (or red) top edge. */
void ui_lab_theme_dialog(lv_obj_t *card, bool destructive);

/** Top strip: fading amber line, warm glow while the interior light is on. */
void ui_lab_theme_light_strip(lv_obj_t *line, lv_obj_t *glow, bool light_on);

#ifdef __cplusplus
}
#endif

#endif /* FURNACE_HMI_UI_LAB_THEME_H */
