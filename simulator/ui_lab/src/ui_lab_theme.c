/* SPDX-License-Identifier: Apache-2.0 */

#include "ui_lab_theme.h"

#include <string.h>

#include <furnace_hmi/hmi_fonts.h>

/* Everything here runs on the LVGL owner context (ADR-0005): styling happens
 * while pages are built, drawing happens inside LVGL's own render pass. Every
 * colour is read from the active palette at that moment; nothing caches a
 * colour value except the light-strip gradients, rebuilt on palette change. */

#define EDGE_THICKNESS 2
/* The web edge fades in over 28 % of its length and out over the last 28 %. */
#define EDGE_FADE_FRAC 71U
/* Side edges on rails and rows use the web "straight" variant: 24 % fades. */
#define SIDE_EDGE_FADE_FRAC 61U
#define GLOW_REACH_FRAC 217U

/* The glow under the interior-light strip: strength of its soft shadow. */
#define LIGHT_GLOW_OPA ((lv_opa_t)110)

/* Active palette: a generated theme, or a caller-owned palette for tests. */
static furnace_hmi_theme_id_t selected_theme = (furnace_hmi_theme_id_t)0;
static const furnace_hmi_color_t *palette_override;
/* Bumped whenever the palette changes so cached gradients are rebuilt. */
static uint32_t palette_generation = 1U;

static const furnace_hmi_color_t *active_palette(void)
{
    if (palette_override != NULL) {
        return palette_override;
    }
    const furnace_hmi_theme_t *theme = furnace_hmi_theme(selected_theme);
    return theme != NULL ? theme->colors : NULL;
}

bool ui_lab_theme_select(furnace_hmi_theme_id_t id)
{
    if (furnace_hmi_theme(id) == NULL) {
        return false;
    }
    selected_theme = id;
    palette_override = NULL;
    ++palette_generation;
    return true;
}

furnace_hmi_theme_id_t ui_lab_theme_selected(void)
{
    return selected_theme;
}

void ui_lab_theme_use_palette(const furnace_hmi_color_t *colors)
{
    palette_override = colors;
    ++palette_generation;
}

lv_color_t ui_lab_color(furnace_hmi_color_role_t role)
{
    const furnace_hmi_color_t *palette = active_palette();
    if (palette == NULL || (int)role < 0 || role >= FURNACE_HMI_COLOR_COUNT) {
        return lv_color_black(); /* theme-lint: allow (invalid role fallback) */
    }
    return lv_color_hex(palette[role].rgb);
}

lv_opa_t ui_lab_opa(furnace_hmi_color_role_t role)
{
    const furnace_hmi_color_t *palette = active_palette();
    if (palette == NULL || (int)role < 0 || role >= FURNACE_HMI_COLOR_COUNT) {
        return LV_OPA_COVER;
    }
    return (lv_opa_t)palette[role].opa;
}

void ui_lab_theme_border(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector)
{
    if (object == NULL) {
        return;
    }
    lv_obj_set_style_border_color(object, ui_lab_color(role), selector);
    lv_obj_set_style_border_opa(object, ui_lab_opa(role), selector);
}

void ui_lab_theme_background(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector)
{
    if (object == NULL) {
        return;
    }
    lv_obj_set_style_bg_color(object, ui_lab_color(role), selector);
    lv_obj_set_style_bg_opa(object, ui_lab_opa(role), selector);
}

/* A tone names colour roles, never colour values, so it follows the theme.
 * NO_ROLE: the tone has no edge or no glow. */
#define NO_ROLE FURNACE_HMI_COLOR_COUNT

typedef struct {
    furnace_hmi_color_role_t edge;
    furnace_hmi_color_role_t text;
    furnace_hmi_color_role_t glow;
    /* Important and selected tiles set their label in SemiBold (web 600). */
    bool strong;
} tone_spec_t;

typedef struct {
    uint8_t tone;
    uint8_t edge;
} edge_key_t;

static const tone_spec_t tone_specs[UI_LAB_TONE_COUNT] = {
    [UI_LAB_TONE_TILE] = { FURNACE_HMI_COLOR_AMBER_SOFT, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_QUIET] = { FURNACE_HMI_COLOR_NAV_EDGE, FURNACE_HMI_COLOR_MUTED, NO_ROLE, false },
    [UI_LAB_TONE_NAV] = { FURNACE_HMI_COLOR_NAV_EDGE, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_SELECTED] = { FURNACE_HMI_COLOR_AMBER, FURNACE_HMI_COLOR_TEXT, FURNACE_HMI_COLOR_AMBER_GLOW, true },
    [UI_LAB_TONE_PRIMARY] = { FURNACE_HMI_COLOR_AMBER, FURNACE_HMI_COLOR_AMBER_TEXT, NO_ROLE, true },
    [UI_LAB_TONE_EDIT] = { FURNACE_HMI_COLOR_YELLOW, FURNACE_HMI_COLOR_YELLOW_TEXT, NO_ROLE, true },
    [UI_LAB_TONE_DANGER] = { FURNACE_HMI_COLOR_RED, FURNACE_HMI_COLOR_RED_TEXT, NO_ROLE, true },
    [UI_LAB_TONE_HEAT] = { FURNACE_HMI_COLOR_HEAT, FURNACE_HMI_COLOR_HEAT, FURNACE_HMI_COLOR_HEAT_GLOW, true },
    [UI_LAB_TONE_HOLD] = { FURNACE_HMI_COLOR_HOLD, FURNACE_HMI_COLOR_HOLD_TEXT, FURNACE_HMI_COLOR_HOLD_GLOW, true },
    [UI_LAB_TONE_COOL] = { FURNACE_HMI_COLOR_COOL, FURNACE_HMI_COLOR_COOL_TEXT, FURNACE_HMI_COLOR_COOL_GLOW, true },
    [UI_LAB_TONE_CARD_HEAT] = { FURNACE_HMI_COLOR_HEAT, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_CARD_HOLD] = { FURNACE_HMI_COLOR_HOLD, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_CARD_COOL] = { FURNACE_HMI_COLOR_COOL, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_NEUTRAL] = { FURNACE_HMI_COLOR_NEUTRAL_EDGE, FURNACE_HMI_COLOR_MUTED, NO_ROLE, false },
    [UI_LAB_TONE_KEY] = { NO_ROLE, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_WELL] = { FURNACE_HMI_COLOR_AMBER_SOFT, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_OK] = { FURNACE_HMI_COLOR_OK, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_COOLING] = { FURNACE_HMI_COLOR_BLUE, FURNACE_HMI_COLOR_BLUE_TEXT, NO_ROLE, true },
    [UI_LAB_TONE_FIELD_HOLD] = { FURNACE_HMI_COLOR_HOLD_EDGE, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
    [UI_LAB_TONE_SEGMENT] = { NO_ROLE, FURNACE_HMI_COLOR_MUTED, NO_ROLE, false },
    [UI_LAB_TONE_FLAT] = { NO_ROLE, FURNACE_HMI_COLOR_TEXT, NO_ROLE, false },
};

/* Stable user-data pointers for the draw callback, one per tone and side. */
static edge_key_t edge_keys[UI_LAB_TONE_COUNT][UI_LAB_EDGE_COUNT];
static bool edge_keys_ready;
static const edge_key_t dialog_keys[2] = {
    { UI_LAB_TONE_PRIMARY, UI_LAB_EDGE_TOP },
    { UI_LAB_TONE_DANGER, UI_LAB_EDGE_TOP },
};

static const edge_key_t *edge_key(ui_lab_tone_t tone, ui_lab_edge_t edge)
{
    if (!edge_keys_ready) {
        for (uint8_t t = 0U; t < (uint8_t)UI_LAB_TONE_COUNT; ++t) {
            for (uint8_t e = 0U; e < (uint8_t)UI_LAB_EDGE_COUNT; ++e) {
                edge_keys[t][e].tone = t;
                edge_keys[t][e].edge = e;
            }
        }
        edge_keys_ready = true;
    }
    return &edge_keys[tone][edge];
}

/* A line that fades in from both ends: transparent, colour, colour, transparent. */
static void init_fading_line(lv_grad_dsc_t *grad, lv_color_t color, lv_opa_t opa,
                             bool horizontal, uint8_t fade)
{
    const lv_color_t colors[4] = { color, color, color, color };
    const lv_opa_t opas[4] = { LV_OPA_TRANSP, opa, opa, LV_OPA_TRANSP };
    const uint8_t fracs[4] = { 0U, fade, (uint8_t)(255U - fade), 255U };
    memset(grad, 0, sizeof(*grad));
    lv_grad_init_stops(grad, colors, opas, fracs, 4);
    if (horizontal) {
        lv_grad_horizontal_init(grad);
    }
    else {
        lv_grad_vertical_init(grad);
    }
}

static void draw_fill(lv_layer_t *layer, const lv_area_t *area, const lv_grad_dsc_t *grad)
{
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.radius = 0;
    rect.bg_opa = LV_OPA_COVER;
    rect.bg_grad = *grad;
    lv_draw_rect(layer, &rect, area);
}

static void draw_solid(lv_layer_t *layer, const lv_area_t *area, lv_color_t color, lv_opa_t opa)
{
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.radius = 0;
    rect.bg_color = color;
    rect.bg_opa = opa;
    lv_draw_rect(layer, &rect, area);
}

/* Glow: the tone's colour fading from the edge side into the tile. */
static void draw_glow(lv_layer_t *layer, const lv_area_t *coords, const tone_spec_t *spec,
                      ui_lab_edge_t edge)
{
    const bool from_start = edge == UI_LAB_EDGE_TOP || edge == UI_LAB_EDGE_LEFT;
    const lv_color_t glow = ui_lab_color(spec->glow);
    const lv_opa_t glow_opa = ui_lab_opa(spec->glow);
    const lv_color_t colors[2] = { glow, glow };
    lv_opa_t opas[2];
    uint8_t fracs[2];
    if (from_start) {
        opas[0] = glow_opa;
        opas[1] = LV_OPA_TRANSP;
        fracs[0] = 0U;
        fracs[1] = GLOW_REACH_FRAC;
    }
    else {
        opas[0] = LV_OPA_TRANSP;
        opas[1] = glow_opa;
        fracs[0] = (uint8_t)(255U - GLOW_REACH_FRAC);
        fracs[1] = 255U;
    }
    lv_grad_dsc_t grad;
    memset(&grad, 0, sizeof(grad));
    lv_grad_init_stops(&grad, colors, opas, fracs, 2);
    if (edge == UI_LAB_EDGE_TOP || edge == UI_LAB_EDGE_BOTTOM) {
        lv_grad_vertical_init(&grad);
    }
    else {
        lv_grad_horizontal_init(&grad);
    }
    draw_fill(layer, coords, &grad);
}

static void draw_edge(lv_layer_t *layer, const lv_area_t *coords, const tone_spec_t *spec,
                      ui_lab_edge_t edge)
{
    if (spec->edge == NO_ROLE || ui_lab_opa(spec->edge) == LV_OPA_TRANSP) {
        return;
    }
    lv_area_t area = *coords;
    const bool horizontal = edge == UI_LAB_EDGE_TOP || edge == UI_LAB_EDGE_BOTTOM;
    switch (edge) {
    case UI_LAB_EDGE_TOP:
        area.y2 = area.y1 + EDGE_THICKNESS - 1;
        break;
    case UI_LAB_EDGE_BOTTOM:
        area.y1 = area.y2 - EDGE_THICKNESS + 1;
        break;
    case UI_LAB_EDGE_RIGHT:
        area.x1 = area.x2 - EDGE_THICKNESS + 1;
        break;
    case UI_LAB_EDGE_LEFT:
    case UI_LAB_EDGE_COUNT:
    default:
        area.x2 = area.x1 + EDGE_THICKNESS - 1;
        break;
    }
    lv_grad_dsc_t grad;
    init_fading_line(&grad, ui_lab_color(spec->edge), ui_lab_opa(spec->edge), horizontal,
                     horizontal ? EDGE_FADE_FRAC : SIDE_EDGE_FADE_FRAC);
    draw_fill(layer, &area, &grad);
}

static void tile_draw_event(lv_event_t *event)
{
    lv_obj_t *object = lv_event_get_current_target_obj(event);
    const edge_key_t *key = lv_event_get_user_data(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    if (object == NULL || key == NULL || layer == NULL ||
        lv_obj_has_state(object, LV_STATE_DISABLED)) {
        return;
    }
    const tone_spec_t *spec = &tone_specs[key->tone];
    const ui_lab_edge_t edge = (ui_lab_edge_t)key->edge;
    lv_area_t coords;
    lv_obj_get_coords(object, &coords);
    if (key->tone != UI_LAB_TONE_WELL && key->tone != UI_LAB_TONE_FLAT) {
        /* The web tile's inset 1 px highlight along its top. */
        lv_area_t highlight = coords;
        highlight.x1 += 1;
        highlight.x2 -= 1;
        highlight.y1 += 1;
        highlight.y2 = highlight.y1;
        draw_solid(layer, &highlight, ui_lab_color(FURNACE_HMI_COLOR_HIGHLIGHT),
                   ui_lab_opa(FURNACE_HMI_COLOR_HIGHLIGHT));
    }
    if (spec->glow != NO_ROLE) {
        draw_glow(layer, &coords, spec, edge);
    }
    draw_edge(layer, &coords, spec, edge);
}

static void attach_edge(lv_obj_t *object, ui_lab_tone_t tone, ui_lab_edge_t edge)
{
    (void)lv_obj_remove_event_cb(object, tile_draw_event);
    lv_obj_add_event_cb(object, tile_draw_event, LV_EVENT_DRAW_MAIN_END,
                        (void *)edge_key(tone, edge));
}

/* Regular and SemiBold faces generated at the same size. */
static const lv_font_t *const weight_pairs[][2] = {
    { &furnace_hmi_font_regular_12, &furnace_hmi_font_semibold_12 },
    { &furnace_hmi_font_regular_13, &furnace_hmi_font_semibold_13 },
    { &furnace_hmi_font_regular_14, &furnace_hmi_font_semibold_14 },
    { &furnace_hmi_font_regular_15, &furnace_hmi_font_semibold_15 },
    { &furnace_hmi_font_regular_16, &furnace_hmi_font_semibold_16 },
    { &furnace_hmi_font_regular_19, &furnace_hmi_font_semibold_19 },
    { &furnace_hmi_font_regular_20, &furnace_hmi_font_semibold_20 },
    { &furnace_hmi_font_regular_22, &furnace_hmi_font_semibold_22 },
    { &furnace_hmi_font_regular_24, &furnace_hmi_font_semibold_24 },
};

const lv_font_t *ui_lab_theme_font_weight(const lv_font_t *font, bool strong)
{
    for (size_t index = 0U; index < sizeof(weight_pairs) / sizeof(weight_pairs[0]); ++index) {
        if (font == weight_pairs[index][strong ? 0 : 1]) {
            return weight_pairs[index][strong ? 1 : 0];
        }
    }
    return font;
}

static void apply_label_weight(lv_obj_t *object, bool strong)
{
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *child = lv_obj_get_child(object, (int32_t)index);
        if (lv_obj_check_type(child, &lv_label_class)) {
            const lv_font_t *font = lv_obj_get_style_text_font(child, LV_PART_MAIN);
            lv_obj_set_style_text_font(child, ui_lab_theme_font_weight(font, strong), LV_PART_MAIN);
        }
    }
}

static void apply_text_color(lv_obj_t *object, lv_color_t color)
{
    lv_obj_set_style_text_color(object, color, LV_PART_MAIN);
    const uint32_t count = lv_obj_get_child_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_obj_t *child = lv_obj_get_child(object, (int32_t)index);
        if (lv_obj_check_type(child, &lv_label_class)) {
            lv_obj_set_style_text_color(child, color, LV_PART_MAIN);
        }
        else if (lv_obj_check_type(child, &lv_image_class)) {
            lv_obj_set_style_image_recolor(child, color, LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(child, LV_OPA_COVER, LV_PART_MAIN);
        }
    }
}

static void set_hairline(lv_obj_t *object, furnace_hmi_color_role_t role, lv_style_selector_t selector)
{
    lv_obj_set_style_border_width(object, 1, selector);
    ui_lab_theme_border(object, role, selector);
    lv_obj_set_style_border_side(object, LV_BORDER_SIDE_FULL, selector);
}

static void set_lift(lv_obj_t *object, lv_style_selector_t selector)
{
    lv_obj_set_style_shadow_color(object, ui_lab_color(FURNACE_HMI_COLOR_SHADOW_LIFT), selector);
    lv_obj_set_style_shadow_opa(object, ui_lab_opa(FURNACE_HMI_COLOR_SHADOW_LIFT), selector);
    lv_obj_set_style_shadow_width(object, 6, selector);
    lv_obj_set_style_shadow_offset_x(object, 0, selector);
    lv_obj_set_style_shadow_offset_y(object, 2, selector);
    lv_obj_set_style_shadow_spread(object, 0, selector);
}

/* The tone a tile was themed with, found through its draw event. */
static const edge_key_t *tile_key(lv_obj_t *object)
{
    const uint32_t count = lv_obj_get_event_count(object);
    for (uint32_t index = 0U; index < count; ++index) {
        lv_event_dsc_t *dsc = lv_obj_get_event_dsc(object, index);
        if (dsc != NULL && lv_event_dsc_get_cb(dsc) == tile_draw_event) {
            const edge_key_t *key = lv_event_dsc_get_user_data(dsc);
            if (key >= &edge_keys[0][0] &&
                key <= &edge_keys[UI_LAB_TONE_COUNT - 1][UI_LAB_EDGE_COUNT - 1]) {
                return key;
            }
            return NULL;
        }
    }
    return NULL;
}

/* Labels and icons are children, so they never see their tile's DISABLED
 * state; colour them from the tile instead: tone colour, or dim when disabled. */
static void refresh_content_color(lv_obj_t *button)
{
    const edge_key_t *key = tile_key(button);
    if (key == NULL) {
        return;
    }
    apply_text_color(button, lv_obj_has_state(button, LV_STATE_DISABLED)
                                 ? UI_LAB_THEME_DIM : ui_lab_color(tone_specs[key->tone].text));
}

static void tile_state_event(lv_event_t *event)
{
    refresh_content_color(lv_event_get_current_target_obj(event));
}

static void attach_state_listener(lv_obj_t *button)
{
    (void)lv_obj_remove_event_cb(button, tile_state_event);
    lv_obj_add_event_cb(button, tile_state_event, LV_EVENT_STATE_CHANGED, NULL);
}

lv_color_t ui_lab_theme_text_color(ui_lab_tone_t tone)
{
    return tone < UI_LAB_TONE_COUNT ? ui_lab_color(tone_specs[tone].text) : UI_LAB_THEME_TEXT;
}

void ui_lab_theme_button(lv_obj_t *button, ui_lab_tone_t tone, ui_lab_edge_t edge)
{
    if (button == NULL || tone >= UI_LAB_TONE_COUNT || edge >= UI_LAB_EDGE_COUNT) {
        return;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    const lv_style_selector_t pressed =
        (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED;
    const lv_style_selector_t disabled =
        (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DISABLED;
    lv_obj_set_style_radius(button, 0, main);
    lv_obj_set_style_outline_width(button, 0, main);
    /* The default theme recolours pressed/disabled buttons; tiles do their own. */
    lv_obj_set_style_recolor_opa(button, LV_OPA_TRANSP, pressed);
    lv_obj_set_style_recolor_opa(button, LV_OPA_TRANSP, disabled);
    lv_obj_set_style_translate_y(button, 0, main);

    if (tone == UI_LAB_TONE_FLAT || tone == UI_LAB_TONE_SEGMENT) {
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, main);
        lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, pressed);
        lv_obj_set_style_border_width(button, 0, main);
        lv_obj_set_style_shadow_width(button, 0, main);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, main);
        attach_edge(button, tone, edge);
        attach_state_listener(button);
        apply_label_weight(button, tone_specs[tone].strong);
        refresh_content_color(button);
        return;
    }

    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, main);
    if (tone == UI_LAB_TONE_WELL) {
        lv_obj_set_style_bg_color(button, UI_LAB_THEME_WELL, main);
        lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_NONE, main);
        lv_obj_set_style_shadow_width(button, 0, main);
        lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, main);
    }
    else {
        lv_obj_set_style_bg_color(button, UI_LAB_THEME_TILE_HI, main);
        lv_obj_set_style_bg_grad_color(button, UI_LAB_THEME_TILE_LO, main);
        lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, main);
        set_lift(button, main);
    }
    set_hairline(button, FURNACE_HMI_COLOR_HAIRLINE, main);

    /* Pressed: slightly brighter face, nudged down one pixel. */
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, pressed);
    lv_obj_set_style_bg_color(button, ui_lab_color(FURNACE_HMI_COLOR_TILE_HI_PRESSED), pressed);
    lv_obj_set_style_bg_grad_color(button, ui_lab_color(FURNACE_HMI_COLOR_TILE_LO_PRESSED), pressed);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, pressed);
    lv_obj_set_style_border_color(button, ui_lab_color(FURNACE_HMI_COLOR_HAIRLINE), pressed);
    lv_obj_set_style_translate_y(button, 1, pressed);

    /* Disabled: flat, darker face, dim text, no edge (the draw event skips it). */
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, disabled);
    lv_obj_set_style_bg_color(button, ui_lab_color(FURNACE_HMI_COLOR_TILE_HI_DISABLED), disabled);
    lv_obj_set_style_bg_grad_color(button, ui_lab_color(FURNACE_HMI_COLOR_TILE_LO_DISABLED), disabled);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, disabled);
    lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, disabled);
    lv_obj_set_style_text_color(button, UI_LAB_THEME_DIM, disabled);

    attach_edge(button, tone, edge);
    attach_state_listener(button);
    apply_label_weight(button, tone_specs[tone].strong);
    refresh_content_color(button);
}

void ui_lab_theme_panel(lv_obj_t *panel)
{
    if (panel == NULL) {
        return;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    lv_obj_set_style_bg_color(panel, UI_LAB_THEME_PANEL, main);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, main);
    lv_obj_set_style_bg_grad_dir(panel, LV_GRAD_DIR_NONE, main);
    set_hairline(panel, FURNACE_HMI_COLOR_HAIRLINE, main);
    lv_obj_set_style_radius(panel, 0, main);
    lv_obj_set_style_shadow_width(panel, 0, main);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_TRANSP, main);
}

void ui_lab_theme_info_tile(lv_obj_t *tile)
{
    if (tile == NULL) {
        return;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    lv_obj_set_style_bg_color(tile, UI_LAB_THEME_TILE_HI, main);
    lv_obj_set_style_bg_grad_color(tile, UI_LAB_THEME_TILE_LO, main);
    lv_obj_set_style_bg_grad_dir(tile, LV_GRAD_DIR_VER, main);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, main);
    set_hairline(tile, FURNACE_HMI_COLOR_HAIRLINE, main);
    lv_obj_set_style_radius(tile, 0, main);
    set_lift(tile, main);
}

void ui_lab_theme_edge(lv_obj_t *object, ui_lab_tone_t tone, ui_lab_edge_t edge)
{
    if (object == NULL || tone >= UI_LAB_TONE_COUNT || edge >= UI_LAB_EDGE_COUNT) {
        return;
    }
    attach_edge(object, tone, edge);
}

void ui_lab_theme_set_edge_side(lv_obj_t *object, ui_lab_edge_t edge)
{
    if (object == NULL || edge >= UI_LAB_EDGE_COUNT) {
        return;
    }
    const edge_key_t *key = tile_key(object);
    if (key != NULL) {
        attach_edge(object, (ui_lab_tone_t)key->tone, edge);
    }
}

void ui_lab_theme_row_edges(lv_obj_t *row, ui_lab_edge_t edge)
{
    if (row == NULL) {
        return;
    }
    const uint32_t count = lv_obj_get_child_count(row);
    for (uint32_t index = 0U; index < count; ++index) {
        ui_lab_theme_set_edge_side(lv_obj_get_child(row, (int32_t)index), edge);
    }
}

void ui_lab_theme_segment(lv_obj_t *button, bool selected, bool first)
{
    if (button == NULL) {
        return;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    ui_lab_theme_button(button, selected ? UI_LAB_TONE_SELECTED : UI_LAB_TONE_SEGMENT, UI_LAB_EDGE_TOP);
    /* The segmented tile supplies the face; segments are transparent. */
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, main);
    lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, main);
    lv_obj_set_style_border_width(button, first ? 0 : 1, main);
    lv_obj_set_style_border_side(button, LV_BORDER_SIDE_LEFT, main);
    ui_lab_theme_border(button, FURNACE_HMI_COLOR_HAIRLINE_STRONG, main);
}

void ui_lab_theme_dialog(lv_obj_t *card, bool destructive)
{
    if (card == NULL) {
        return;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    lv_obj_set_style_bg_color(card, ui_lab_color(FURNACE_HMI_COLOR_DIALOG_HI), main);
    lv_obj_set_style_bg_grad_color(card, ui_lab_color(FURNACE_HMI_COLOR_DIALOG_LO), main);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, main);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, main);
    set_hairline(card, FURNACE_HMI_COLOR_HAIRLINE_STRONG, main);
    lv_obj_set_style_radius(card, 0, main);
    lv_obj_set_style_shadow_color(card, ui_lab_color(FURNACE_HMI_COLOR_SHADOW_DIALOG), main);
    lv_obj_set_style_shadow_opa(card, ui_lab_opa(FURNACE_HMI_COLOR_SHADOW_DIALOG), main);
    lv_obj_set_style_shadow_width(card, 50, main);
    lv_obj_set_style_shadow_offset_y(card, 20, main);
    (void)lv_obj_remove_event_cb(card, tile_draw_event);
    lv_obj_add_event_cb(card, tile_draw_event, LV_EVENT_DRAW_MAIN_END,
                        (void *)&dialog_keys[destructive ? 1 : 0]);
}

void ui_lab_theme_light_strip(lv_obj_t *line, lv_obj_t *glow, bool light_on)
{
    /* Descriptors are referenced by the style, so they must outlive it. */
    static lv_grad_dsc_t off_grad;
    static lv_grad_dsc_t on_grad;
    static uint32_t built_for;
    if (built_for != palette_generation) {
        init_fading_line(&off_grad, UI_LAB_THEME_AMBER, LV_OPA_COVER, true, EDGE_FADE_FRAC);
        const lv_color_t light = ui_lab_color(FURNACE_HMI_COLOR_LIGHT);
        const lv_color_t colors[5] = {
            light, light, ui_lab_color(FURNACE_HMI_COLOR_LIGHT_CORE), light, light,
        };
        const lv_opa_t opas[5] = {
            LV_OPA_TRANSP, LV_OPA_COVER, LV_OPA_COVER, LV_OPA_COVER, LV_OPA_TRANSP,
        };
        const uint8_t fracs[5] = { 0U, 56U, 128U, 199U, 255U };
        memset(&on_grad, 0, sizeof(on_grad));
        lv_grad_init_stops(&on_grad, colors, opas, fracs, 5);
        lv_grad_horizontal_init(&on_grad);
        built_for = palette_generation;
    }
    const lv_style_selector_t main = (lv_style_selector_t)LV_PART_MAIN;
    if (line != NULL) {
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, main);
        lv_obj_set_style_bg_grad(line, light_on ? &on_grad : &off_grad, main);
        lv_obj_set_style_shadow_width(line, 0, main);
        lv_obj_set_style_shadow_opa(line, LV_OPA_TRANSP, main);
    }
    if (glow != NULL) {
        /* The web spills a warm elliptical glow downwards; a soft shadow under a
         * centred, invisible bar gives the same falloff on the software renderer. */
        lv_obj_set_style_bg_opa(glow, LV_OPA_TRANSP, main);
        lv_obj_set_style_border_width(glow, 0, main);
        lv_obj_set_style_shadow_color(glow, ui_lab_color(FURNACE_HMI_COLOR_LIGHT), main);
        lv_obj_set_style_shadow_width(glow, 36, main);
        lv_obj_set_style_shadow_spread(glow, 0, main);
        lv_obj_set_style_shadow_opa(glow, light_on ? LIGHT_GLOW_OPA : LV_OPA_TRANSP, main);
    }
}
