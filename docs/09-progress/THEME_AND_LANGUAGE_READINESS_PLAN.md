# Theme and language readiness plan

**Status (2026-10-05):** complete; results below.

## Outcome

Both HMI presentations (web demo and LVGL lab) are coded so that a new colour
theme or a new language is a data change plus review, not a code hunt:

1. **One theme source.** Every HMI colour is a named token defined once in
   `app/hmi/presentation/theme/themes.json`. A generator writes the C palette
   and the web CSS token block from it. No HMI code writes a colour value.
2. **Runtime theme switch.** Each presentation can switch theme at runtime
   (rebuild of the visible page in the GUI context); a test switches to a
   synthetic palette and proves nothing kept the old colours.
3. **One text source.** Every visible HMI string is an identifier in the C
   string catalogue (`app/hmi/presentation/strings`). The web demo uses the
   same catalogue, embedded by a generator, through `t(id, ...)`. Nothing picks
   behaviour by matching English text.
4. **Runtime language switch** with English fallback for missing entries, and a
   pseudo-language check in both presentations that finds hard-coded text and
   text that does not fit.
5. **Guard rails.** A checker fails on raw colours or untranslated text in HMI
   code, and on catalogue characters the HMI font cannot draw.
6. **A repository skill** (`.agents/skills/theme-and-language`) that states the
   rules for all later UI work.

**Non-goals:** designing the light theme (next step, with the product owner);
real translations; persisting the choice across restarts (needs the HMI
storage adapter, [STATE_OWNERSHIP](../01-architecture/STATE_OWNERSHIP.md));
fonts for non-Latin scripts; right-to-left layout; Fahrenheit.

## Governing rules

- REQ-FUN-013: localizable from the start, strings separated from screen
  logic. STATE_OWNERSHIP: language, theme and UI preferences are HMI-owned.
- [ADR-0005](../07-decisions/0005-lvgl-single-owner.md): theme and language
  switches touch LVGL objects, so they run only in the GUI context.
- [ADR-0008](../07-decisions/0008-memory-policy.md) (deferred): every buffer
  bounded and owned; no unbounded work in the GUI path.
- AGENTS.md web/LVGL parity and the UI-parity skill; generated files are
  produced only by their generator.
- Proposed [ADR-0012](../07-decisions/0012-theme-and-language-catalogues.md)
  records the design choices below.

## Facts (2026-10-05 survey)

- Web: about 36 colour tokens on `#soft`, used about 320 times, but about 89
  raw colours remain in HMI CSS, about 23 in graph/illustration JS, and the
  legacy base style layer is all raw colours.
- LVGL: `ui_lab_theme.h` has about 30 compile-time colour macros; the tone
  table stores colour values; hairlines, highlights and shadows assume a dark
  page ("white over whatever they sit on"); 34 `lv_color_hex` literals remain
  in the explorer; the light-strip gradients are built once and cached.
- Text: the C catalogue has about 590 English entries (one table, no language
  switch). The explorer still has about 25 visible literals plus number and
  time formats. The web has its own literals (about 280 identical to the
  catalogue, about 120 more). The web chooses button icons and navigation
  styling by matching English labels (`BUTTON_ICONS`, `NAV_LABELS`).
- Fonts: Tomorrow HMI is subset to 103 glyphs (ASCII and a few symbols).
  Upstream Tomorrow has no Cyrillic or Greek.

## Decisions (recommendations, reversible)

- Theme data in JSON with RGBA values; generator emits C and CSS (same pattern
  as icons and fonts). Shadows, hairlines and highlights are tokens with their
  own colour and alpha, so a light theme can use dark hairlines.
- LVGL resolves colours at use time through the active palette; switching
  rebuilds the page. No duplicated style objects, no per-theme assets.
- Web keeps CSS custom properties; a `data-hmi-theme` attribute selects the
  set; drawing code reads tokens instead of literals.
- The C catalogue stays the single text source; web ids are the C ids without
  the `FURNACE_HMI_UI_STRING_` prefix. Formats keep printf syntax; the web has
  a small formatter for the same subset.
- Theme and Language cards become choices automatically when more than one
  option exists; until then they stay information cards.

## Open questions

- Target languages and scripts (decides font coverage and flash budget).
- Whether theme follows ambient light/time of day or is only manual.
- Where the choice is persisted (HMI storage adapter not designed).

## Milestones

1. **Theme source and generator.** `themes.json`, `tools/build_theme.py`
   (`--check`), C module `furnace_hmi_theme`, generated web token block.
   Done when both generators' `--check` pass and pixel captures are unchanged.
2. **LVGL palette at runtime.** Theme macros resolve through the active
   palette; tone table holds roles; explorer literals become roles; cached
   gradients refresh; explorer API to switch theme. Done when native captures
   at three sizes are pixel-identical to the baseline and a synthetic-palette
   test passes.
3. **Web colours.** All HMI CSS and JS colours use tokens; theme switch
   function. Done when web captures match the baseline and a synthetic-theme
   check finds no element keeping a dark-theme colour.
4. **Native language.** Language API with fallback; explorer literals and
   formats moved to the catalogue; explorer API to switch language;
   pseudo-language test reports no hard-coded labels.
5. **Web language.** Generated catalogue block, `t()`, all HMI text through
   it, icon/navigation choice by id; pseudo-language check in the smoke test.
6. **Guard rails and skill.** Checker for raw colours, literals and font
   coverage; Theme/Language cards; skill, ADR, documentation, status,
   generated index.

## Verification

Pixel comparison of all native captures (95 states, three sizes) and 35 web
states at two sizes against the 2026-10-05 baseline; native CTest at three
sizes; web smoke; generator `--check`s; the new checker; synthetic-theme and
pseudo-language tests in both presentations. Not covered: physical panel,
real translations.

## Rollback

Each milestone is a separate, behaviour-neutral change and can be reverted on
its own. No stored data or protocol changes.

## Progress

- 2026-10-05: survey done; baselines captured (native deterministic; web
  deterministic except Device log timestamps and Manual running).
- 2026-10-05, milestone 1 done: `themes.json` (95 tokens, one theme, WCAG
  contrast pairs), `tools/build_theme.py` (C palette, web token block,
  `--check`, contrast check), `furnace_hmi_theme` module, host tests
  `host.theme_palette` and `tools.theme_generated`. Web captures unchanged.
- 2026-10-05, milestone 2 done: LVGL colours resolve through the active
  palette (`ui_lab_theme_select`, `ui_lab_theme_use_palette`); the tone table
  holds roles; no colour value is left in `ui_lab_theme.c` or
  `ui_lab_explorer.c`. Device status and panel style are explicit instead of
  chosen by comparing colours. `furnace_hmi_ui_lab_explorer_refresh_presentation`
  restyles the chrome and page. New smoke check: a probe palette distinct
  from every theme colour; over 1,000 visible style colours on every page,
  Settings and Device tab and a dialog must follow it (a planted literal is
  caught). Native captures differ from the baseline by at most 4 of 255
  levels: hairlines, highlights and neutral edges now use the web's pure
  white token instead of text white, and the scrim rounds to 179 instead of
  178. **Discovery:** `app/hmi/presentation/dashboard` (an older proof view
  with its own palette) is linked into the lab but never called; left as is,
  follow-up to remove or theme it.
- 2026-10-05, milestone 3 done: every web HMI colour (soft layer, legacy base
  layer, graph and furnace drawing) is a token; drawings read tokens through
  `themeColor()`; `setHmiTheme(id)` switches theme. The unused `?nav=silver`
  review variant was removed and the demo-page backdrop moved to the demo
  section. Web captures differ by at most 2 of 255 levels: live legacy rules
  that used #bbc3c7 now use the theme's muted grey #b9c1c5 (as LVGL does).
  New `warning-text` token keeps the web due/warning tags at #efc18b;
  **parity gap:** LVGL draws them in `status-due` (#e8a04b). New smoke check:
  a probe theme, every rendered colour on 23 screens and dialogs must follow
  it (a planted dialog literal is caught).
- 2026-10-05, milestone 4 done: the string catalogue is data
  (`strings/catalogue/en.json`, migrated byte-identical, 638 strings) and
  `tools/build_strings.py` generates the C ids and tables and the web copy,
  checking printf conversions, `{field}` patterns, the decimal separator and
  font glyph coverage. The strings module gained language selection with
  English fallback and a test override. The explorer no longer writes visible
  text: program durations, editor labels, stage-card kinds, the device log,
  number, time and date formats (date and time order now come from
  `_PATTERN` strings), the keypad decimal key and parser, and the text
  keyboard rows (catalogue rows with explicit Shift rows instead of an
  ASCII `toupper`) all come from the catalogue. New checks: host language
  API test, `tools.strings_generated`, and a pseudo-language smoke check
  ("[text~~~]", about 30 % longer) that finds no LVGL label outside the
  catalogue on any page, tab, scenario or dialog and confirms that a refresh
  relabels the navigation. Native captures pixel-identical.
  **Discovery:** the device service catalogue file carries English display
  text (task names, remaining, intervals); production needs the HMI to build
  that text from numbers and dates, or per-language catalogue text
  (ADR-0011 follow-up).
- 2026-10-05, milestone 5 done: the web demo takes all HMI text from the
  generated catalogue copy: `t()` (C printf semantics, including
  `%<PRIi32>`), `tp()` for `{field}` patterns, ids passed to `button()`,
  `tell()`, `confirmAction()` and choice lists; `setHmiLanguage(tag)`. Icons
  and navigation styling follow string ids (`BUTTON_ICONS`, `NAV_IDS`)
  instead of English labels. Controller alerts, service records, the session
  log, number formats (decimal, thousands, power, percent), dates and times,
  the keypad decimal key and the keyboard rows use the catalogue. The
  unreachable Program preferences tab and a hidden "design concept" caption
  were removed; the static markup holds no text. New smoke check: a pseudo
  language over 25 screens and dialogs (text, aria-label, title) with a
  planted literal caught.
- 2026-10-05, milestone 6 done: Theme and Language are display preferences
  in both presentations (preview while edited, Save, Discard, Reset); their
  cards become choices when a second theme or language exists (web smoke
  injects one of each). `tools/check_theme_language.py` (CTest
  `tools.theme_language_lint`) rejects colour literals in HMI code and
  visible text literals in the LVGL explorer. Skill
  `.agents/skills/theme-and-language`, ADR-0012 (proposed), module READMEs,
  AGENTS.md pointer, open questions.

## Results (2026-10-05)

All six milestones are done. 98 theme tokens (one theme, Dark) and 749
strings (one language, English) are shared by both presentations.

**Deviations and visible changes (all deliberate, small):**
- Native: hairlines, highlights and neutral edges use the web's pure white
  token (was text white); the modal scrim rounds to 179 (was 178). At most
  4 of 255 levels.
- Web: live legacy rules that used #bbc3c7 grey use the theme's muted grey
  #b9c1c5 (as LVGL does); at most 2 levels.
- Shared wording now identical in both: reset dialog titles use the
  in-sentence scope ("Reset display preferences?"); the delete-programs
  dialog uses the native title and detail on the web; "Server setup" /
  "Bluetooth setup" dialog titles on native; "·" separators on native stale
  banner, fault row and item counts; the healthy-fixture task text
  "No scheduled task currently due"; web session-log times follow the HMI
  time format instead of the browser locale.
- Web due/warning tags keep their own token (`warning-text`, #efc18b);
  **parity gap:** LVGL draws them in `status-due` (#e8a04b).

**Verification:** native MSVC builds without warnings and CTest passed
13/13 at 800x480, 800x640 and 1024x600 (`-E codebase_index`), including
`host.theme_palette`, `host.ui_strings_catalog` (language API),
`tools.theme_generated`, `tools.strings_generated`,
`tools.theme_language_lint` and the smoke probe-theme and pseudo-language
checks; native captures compared with the 2026-10-05 baseline (95/92/92
states) show only the listed changes. `node tools/hmi_web_demo_p1_smoke.cjs`
passed, including probe theme, pseudo language and Theme/Language choices;
70 web captures differ only as listed (plus the live Device log). Generator
`--check`s pass for themes, strings, icon sprite, LVGL icons and fonts.
Zephyr qemu_x86 build without project warnings and Twister foundation 3/3
passed (a GCC-only `-Wcomment` error in the new `ui_strings.c` header was
found and fixed). Not run: physical panel, real translations.

**Follow-ups:** design the light theme (next); decide target languages and
the font for non-Latin scripts; persist theme and language (storage
adapter); device-catalogue text localisation (ADR-0011); the
`warning-text` / `status-due` parity gap; the unused
`app/hmi/presentation/dashboard` view; ADR-0012 review.
