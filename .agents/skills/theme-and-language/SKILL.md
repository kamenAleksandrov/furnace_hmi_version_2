---
name: theme-and-language
description: Keep every HMI colour a theme token and every visible text a catalogue string, in both the web demo and the LVGL lab, so themes and languages can be switched and added as data. Use for any change that adds or edits a colour, a visible text, a number/date/time format, a keyboard layout, or a theme or language.
---

# Theme and language

The HMI must be switchable between themes and languages at runtime, and a new
theme or language must be a data change plus review. Both presentations read
the same two sources:

- **Colours:** `app/hmi/presentation/theme/themes.json` (tokens, one value per
  token for every theme, contrast pairs). `tools/build_theme.py` generates the
  C palette (`furnace_hmi_theme`) and the web CSS token block.
- **Text:** `app/hmi/presentation/strings/catalogue/<tag>.json` (`en.json`
  defines every id; other languages translate a subset and fall back to
  English). `tools/build_strings.py` generates the C ids and tables and the
  web copy (`HMI_STRINGS`).

Design record: [ADR-0012](../../../docs/07-decisions/0012-theme-and-language-catalogues.md).

## Rules

1. **No colour values in UI code.** LVGL: `ui_lab_color(FURNACE_HMI_COLOR_x)`,
   `ui_lab_opa()`, `ui_lab_theme_border()`, `ui_lab_theme_background()` or the
   `UI_LAB_THEME_*` macros. Web: `var(--token)` in CSS, `themeColor('token')`
   in drawing code. Need a new colour? Add a token with a `doc` to
   `themes.json` for **every** theme, then regenerate.
2. **Name the role, not the look.** A token says what it is for
   (`status-due`, `graph-grid`), so a light theme can change it alone. Do not
   borrow another role's token because the value happens to match.
3. **Never decide behaviour by colour or by text.** Use an explicit state or
   id (status enums, `NAV_IDS`, `BUTTON_ICONS` keyed by string id).
4. **No visible text in UI code.** LVGL: `ui_string(FURNACE_HMI_UI_STRING_x)`.
   Web: `t('ID', ...args)`, `tp('ID', {field})`, or pass an id to `button()`,
   `tell()`, `confirmAction()`, `chooseOption()` options. Data shown as it is
   (program names, controller and device-catalogue text, language names) is
   the only exception.
5. **Whole sentences, not fragments.** Put the whole phrase in one string
   with conversions (`"Delete %s?"`), never `"Delete " + name + "?"`, never
   change case in code (`toupper`, `toLowerCase`): write the text in the case
   to display, or add an in-sentence variant.
6. **Formats are catalogue strings.** Decimal and thousands separators live
   in the format strings (`UI_LAB_DECIMAL_FORMAT`, `..._THOUSANDS_FORMAT`);
   the keypad uses `UI_LAB_DECIMAL_SEPARATOR`. Date and time order come from
   `_PATTERN` strings with `{field}`s. Keyboard rows (and their Shift rows)
   are catalogue strings.
7. **Translations keep the shape.** printf conversions in the same order
   (C cannot reorder arguments; rephrase instead), the same `{fields}` in any
   order, characters the HMI font can draw. The generator enforces all three.
8. **Switching is a rebuild in the GUI context.** LVGL:
   `ui_lab_theme_select()` / `furnace_hmi_ui_set_language()` then
   `furnace_hmi_ui_lab_explorer_refresh_presentation()`. Web: candidate
   preference, `render()` applies it. Never restyle from another thread.
9. **Both presentations in the same slice** (UI-parity skill).

## Adding a theme or a language

- **Theme:** add an entry to `themes.json` with every token, a `name_string`
  id for its display name, then `python tools/build_theme.py`. The contrast
  check must pass. The Theme card becomes a choice automatically.
- **Language:** add `catalogue/<tag>.json` with `language.tag`/`name` (the
  autonym) and the translated strings, then `python tools/build_strings.py`.
  Glyphs outside the font fail the check: extend the font subset
  (`tools/build_fonts.py`) or choose a fallback font first (flash cost; open
  question for non-Latin scripts). The Language card becomes a choice
  automatically.

## Verify

- `python tools/build_theme.py --check`, `python tools/build_strings.py --check`
- `python tools/check_theme_language.py` (colour literals in HMI code, visible
  text literals in the LVGL explorer)
- Native CTest (all sizes): the smoke test switches to a probe palette and to
  a pseudo language (`[text~~~]`, 30 % longer) and fails on any visible style
  colour or label that did not follow.
- `node tools/hmi_web_demo_p1_smoke.cjs`: the same probe theme and pseudo
  language checks on the web, plus Theme/Language choices with an injected
  second option.
- Review layout at all sizes with the pseudo language when adding long text.
