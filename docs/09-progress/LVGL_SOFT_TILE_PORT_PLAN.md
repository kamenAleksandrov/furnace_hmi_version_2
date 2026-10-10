# LVGL soft-tile port plan

**Status:** complete (opened and completed 2026-09-30).

## 1. Outcome and non-goals

**Outcome:** the native LVGL UI lab presents the soft-tile UI adopted on
2026-09-29 (see [HMI web demo](HMI_WEB_DEMO.md#soft-tile-ui-adopted-2026-09-29)):
the same palette, tile depth, fading edges, square corners, typeface, icons,
page compositions and the listed behaviour changes, at 800 x 480, 800 x 640
and 1024 x 600.

**Non-goals:** no controller, protocol, firmware-transport, storage or
authority change. The browser demo's toolbar/wrapper is demo-only and is not
ported. Hover and press animation are browser affordances and are reduced to
LVGL pressed/checked/disabled states.

## 2. Governing requirements, invariants and decisions

- [ADR-0001](../07-decisions/0001-authority-boundary.md): the Control CPU owns
  furnace truth. Start, Pause, Stop and stage edits stay request previews.
- [ADR-0005](../07-decisions/0005-lvgl-single-owner.md): only the GUI context
  manipulates LVGL objects; the port adds no threads or timers outside it.
- [ADR-0008](../07-decisions/0008-memory-policy.md): generated fonts and icons
  are `const` data; no new heap allocation policy.
- AGENTS web/LVGL parity rule and the [UI-parity skill](../../.agents/skills/ui-parity/SKILL.md).
- **Decision (product owner, 2026-09-29):** the soft-tile web demo is the
  reference presentation.
- **Decision (product owner, 2026-09-30):** both presentations use the Inter
  typeface (SIL Open Font License), Regular and SemiBold. Native fonts are
  generated with `lv_font_conv`; the web demo embeds the same font.
- **Decision (product owner, 2026-09-30):** LVGL icons are 8-bit alpha images
  generated from `app/hmi/presentation/icons/svg/` by a repository generator and
  recoloured at runtime. This replaces the icon-font plan in the icon README.

## 3. Facts, assumptions, open items and risks

- **Fact:** LVGL is 9.5.0 with the software renderer. Per-stop gradient opacity
  is available; the default of two gradient stops is raised in the desktop
  `lv_conf.h` so a fading edge is one object.
- **Fact:** the shared model already carries heater demand and estimated
  power, but not the controller's instantaneous setpoint or a measured cooling
  history; the web demo synthesises those.
- **Assumption:** a 2 px edge drawn as a child object with a four-stop
  horizontal/vertical gradient is visually equivalent to the CSS edge at the
  review sizes. Checked in milestone 1 captures.
- **Open:** the LVGL `lift` drop shadow (0 2 6 px) costs software-render time
  on the target; the lab may use it, but target cost is unmeasured. Revisit
  before a target build of this UI.
- **Open:** Manual's controller setpoint needs a model field and simulator
  value. Until then the native strip shows it as unavailable (fail-closed),
  which is a recorded parity gap, not a fabricated value.
- **Risk:** the explorer is one 7 400-line file; theme helpers are introduced
  first so page changes are mostly call-site swaps, keeping diffs reviewable.
- **Kept deliberately:** the native "simulation" identity marker remains
  (see status 2026-09-27); the web wrapper supplies its own context.

## 4. Affected modules and ownership

| Area | Files | Owner boundary |
| --- | --- | --- |
| Theme helpers, pages | `simulator/ui_lab/src/ui_lab_explorer.c/.h` | Host lab presentation |
| Lab tests, captures | `simulator/ui_lab/src/main.c` | Host lab verification |
| LVGL config | `simulator/desktop/lv_conf.h` | Desktop simulator only |
| Fonts | `app/hmi/presentation/fonts/` (new, generated + OFL licence) | Portable presentation data |
| Icons | `app/hmi/presentation/icons/` + `tools/build_lvgl_icons.py` (new) | Portable presentation data |
| Web font | `docs/images/theme/hmi-web-demo.html` | Web demo |
| Strings | `app/hmi/presentation/strings/` | Shared catalogue |
| Docs | UX baseline, program/startup model, status, verification | Documentation |

## 5. Milestones

Each milestone ends with the verification in section 6 and a status entry.

1. **Theme foundation.** Palette tokens from the web `#soft` block; square
   corners; tile helper (vertical face gradient, hairline border, lift
   shadow, fading 2 px edge on a chosen side); roles (amber primary and
   Cancel, yellow Edit, red destructive, steel navigation, purple Hold, green
   OK); selected, pressed and disabled states; panel, info-tile and well
   styles; top strip fading edge and light-on glow. Apply to bottom
   navigation, Device/Settings rails and all shared button constructors.
   *Done when* every page renders with the new surfaces and no page layout
   regresses in the geometry checks.
2. **Typeface and icons.** Inter Regular/SemiBold at the sizes the pages use;
   icon generator with `--check`; icon+label button helper. Apply to navigation,
   rails, Program actions, pagers, steppers and keyboards. Web demo switches to
   embedded Inter. *Done when* the generator checks pass and native captures show
   the icons and weights of the web captures.
3. **Programs.** Shared pager component (‹ 1 / 2 ›); library cards; Program
   details with title row (Back, name, favourite), four-slot action row
   (Start program, View stages, Edit program, Delete program), graph over three
   quarters and facts in the fourth; **Start program** with confirmation;
   Load page only from Quick launch, graph full height and Start under the
   facts; stage cards with kind header, large target and detail line.
4. **Stage editor dialog.** Fixed-height dialog over the originating page;
   delete / pager with editable position (moves the stage) / add in one row;
   Heating/Hold type pair; same rows for both (Hold locks Target and shows Rate
   "None"); one-piece field steppers; delete confirms.
5. **Running and Manual.** Title with stage status; readings strip above the
   graph; segmented Graph/Split/Furnace switch; Pause/Stop tiles; Program graph
   shows only the start of cooling, with Show/Hide cooling once measured
   cooling passes that window; measured cooling blue dashed; planned cooling
   ends at its estimate. Manual readings strip (state, elapsed, to target,
   current, setpoint, heater demand, estimated power), measured line plus
   prediction, one-piece Target/Rate steppers in the four-slot control row.
6. **Home, Device, Settings, keyboards and dialogs.** Home columns follow the
   navigation grid; Quick launch shows favourites then recents trimmed to fit,
   Start manual below. Device rows with status-coloured left edge; Device and
   Settings lists show up to five 48 px rows with the pager. Keyboards: Cancel
   and Clear left, Apply right, blinking caret, centred entry text, no numeric
   sign key. Dialogs: amber (or red for destructive) top edge, equal-width
   centred buttons, dimmed backdrop.

## 6. Verification

Per milestone:

- MSVC UI-lab build with no warnings and 10/10 CTest at 800 x 480, 800 x 640
  and 1024 x 600 (`tools/review_ui_lab.py`), extending smoke/geometry checks
  for new behaviour: Start confirmation cancel/accept, stage move/add/delete
  confirm, Hold locking, cooling toggle visibility, pager bounds, five-row
  lists, keyboard key groups.
- Side-by-side captures of both presentations for the affected pages (web via
  headless Chrome in inspect mode, native via `--capture`), visually reviewed.
- `tools/hmi_web_demo_p1_smoke.cjs` whenever the web file changes (font).
- Generators: `tools/build_icon_sprite.py --check`, the new LVGL icon
  generator `--check`, and codebase index `--check`.
- Failure cases: disabled controls render dim and ignore input; unavailable
  readings show the unavailable text, never a synthesised value; cancelling a
  confirmation leaves state unchanged.
- `run_foundation.py zephyr` when shared presentation sources compiled into the
  Zephyr image change (strings, fonts, icons).

Not covered by this plan: physical touch, target frame-time measurement.

## 7. Documentation and generated artefacts

- Record Start-from-details, Load-from-Quick-launch-only and the stage-editor
  dialog in the UX baseline and program/startup model (milestone 3/4).
- Update the icon README for the image generator (milestone 2).
- Regenerate `docs/_generated/CODEBASE_INDEX.md` with its generator.
- Status, session log and Graphite verification entries per milestone.

## 8. Rollback

Presentation-only and host-lab-only apart from shared font/icon/string data.
Revert the milestone commit; no persisted state or protocol compatibility is
affected.

## Progress

- 2026-09-30: plan opened; font and icon decisions recorded above.
- 2026-09-30, milestone 1 (theme foundation) done. New
  `simulator/ui_lab/src/ui_lab_theme.c/.h` holds the palette, tones (tile,
  quiet, navigation, selected, primary, edit, danger, stage kinds, neutral,
  key, well, OK, cooling, flat) and edge sides. Faces, hairlines and the lift
  shadow are LVGL styles; the fading 2 px edge, selection glow and top
  highlight are drawn in a `DRAW_MAIN_END` callback, so no child objects are
  added and existing `lv_obj_get_child(button, 0)` label lookups still hold.
  Button constructors take a tone instead of a background colour (87 call
  sites mapped one by one). Action rows declare their edge side with
  `mark_edge_row()` (LVGL user flag). Radius is 0 everywhere; the bottom
  navigation is four separate tiles; dialogs have the raised face and
  amber/red top edge; the top strip fades at both ends and glows while the
  light is on. The nav selection glow animation was removed (the web tabs
  switch instantly). `LV_GRADIENT_MAX_STOPS` is 5 in the desktop `lv_conf.h`.
  - Verification: MSVC builds of all three UI-lab presets; no project
    warnings (78 upstream LVGL theme warnings from the forced LVGL rebuild).
    CTest 9/9 per preset excluding `tools.codebase_index`, which fails because
    the local MinGW `cc.exe` cannot compile a one-line C file (environment,
    unrelated). Captures at 800 x 480 and 1024 x 600 reviewed against the web
    captures. Codebase index regenerated; `--check` passes.
  - Carried to later milestones: page compositions, Inter, icons, stage-card
    kind edges, settings-row tiles, keyboard well/caret, segmented Running
    switch and Manual steppers.
- 2026-09-30, milestone 2 (typeface and icons) done.
  - Inter 4.1 Regular/SemiBold subsets and OFL in
    `app/hmi/presentation/fonts/`; `tools/build_fonts.py` generates 21 LVGL
    fonts (Regular 11-36, SemiBold 12-26, from the probed web type scale), the
    declarations header and the web demo's embedded WOFF2 `@font-face`, with a
    reproducible `--check`. The web demo HMI (screen and dialogs, not the demo
    toolbar) now renders in Inter.
  - `tools/build_lvgl_icons.py` renders the 33 shared SVG icons at 16/18/20/22
    px as A8 images with an enum and lookup (`furnace_hmi::icons`).
  - Native buttons take icons from a table keyed by string-catalogue IDs that
    mirrors the web `BUTTON_ICONS`; navigation and rails use the web icon maps.
    The label stays child 0 (a reversed flex row puts a leading icon first).
    Value wells never take text-keyed icons.
  - The theme sets SemiBold for important and selected tones and recolours a
    tile's label and icon on every state change, so disabled tiles dim their
    content (previously disabled Discard/Save kept coloured text).
  - Discovered: Inter is wider than Segoe UI, which also affected the web
    demo. Web fixes: Settings rail gap 8 px and right padding 4 px; Running
    view switch share 1.55 to 1.8. Web overflow probe clean afterwards.
  - Pulled forward because Inter no longer fitted the old layouts: the shared
    pager (arrows around "1 / 2"; page format string is now "%u / %u") for
    Programs, Stages and Device; the segmented Running view switch; the web
    Settings row (48 px info tile, soft amber left edge when actionable, 15/11
    px text, transparent 120 px value); Device rows at 48 px with 14/11/12 px
    text; rails and Quick launch one navigation tab wide; Quick launch names
    at 14 px wrapping to two lines; Back buttons 95 px; "More" and "Back" with
    chevron/arrow icons and value chevrons as "›", matching the web.
  - Verification: all three presets build with no project warnings; CTest
    9/9 each (codebase-index test excluded, environment issue above); web
    smoke passed after the CSS change; font, LVGL icon and web sprite
    generators all pass `--check`; captures of both presentations compared.
  - Carried forward: heading weights and sizes per page, Home/Running
    wording ("Unknown", "Closed", "1,450 rpm" on the web versus lower-case and
    "1450 RPM" natively), Running readings above the graph, five-row Settings
    pages, keyboard key groups and caret. Delete program is temporarily 172 px
    in the Program details toolbar until milestone 3 moves it.
- 2026-09-30, milestone 3 (Programs) done.
  - Program details: title row (Back, name, star icon), four-slot action row
    (Start program, View stages, Edit program, Delete program), graph over
    three quarters and a shared facts tile (web 13/24 px facts, 13/18 px
    Duration rows). New `UI_LAB_ACTION_CONFIRM_START` opens "Start <name>?"
    with stages, curing and maximum; confirming runs the existing start
    request preview, Cancel changes nothing.
  - Load program: Quick launch only; Back to Home; graph three quarters at full
    height, facts and Start program beneath them. The old Load program and
    View details buttons are gone.
  - Program stages: Back, Edit program (157 px), pager; no heading or Add
    stage. Stage cards: tinted square ordinal, upper-case kind (SemiBold 12,
    letter-spaced), ramp/hold glyph, SemiBold 26 target, "duration · rate"
    detail, kind-coloured edge. Cards (stages, editor) and library cards are
    sized after layout to fill a 3 x 3 or 2 x 3 grid like the web.
  - Program editor: compact actions 140 px at SemiBold 14, name centred at
    18 px. Web fix: the web itself wrapped "Edit stages" (124 px with its
    icon), so its compact actions are now 140 px too.
  - Fonts: SemiBold 13 (ordinal) and 22 (dialog titles) added; every dialog
    title is now SemiBold 22 as on the web.
  - Documents: UX baseline, screen concepts and program/startup model record
    Start-from-details and Load-from-Quick-launch, with the authority note
    that the confirmation is an HMI check and the Control CPU starts the run.
  - Tests: smoke checks Cancel leaves state unchanged and confirming records
    the recent program and opens the request preview; new `start-confirm`
    capture; the favourite check now expects an icon image; the stage-card
    tap uses the new detail text; `add-stage` goes through the editor's
    "+ Add stage".
  - Verification: three presets build with no project warnings; CTest 9/9
    each (codebase-index excluded as before); web smoke passed; web overflow
    probe clean on details, editor and load; captures compared.
  - Carried forward: graph internals (web caption "Temperature (C) · Time
    (min)", 0-based time axis, smaller stage nodes) belong with the Running
    and Manual graphs in milestone 5; dialog width/button layout in milestone 6.
- 2026-09-30, milestone 4 (stage editor dialog) done.
  - The stage editor page now draws the page it was opened from (program
    editor, Program details, Home, ...) and a 440 x 404 dialog over it
    (`build_stage_dialog`; `stage_editor_origin`, `stage_dialog`). The nav
    follows the origin page and is covered by the dimmed backdrop.
  - Session model (web stageSession): opening copies every stage into the
    staged arrays and snapshots them (`stage_backup` for Discard,
    `stage_baseline` for the dirty check). Add after (Heating, +20 C, 1 C/min),
    move by tapping the position (numeric keypad "Move stage to position"),
    delete with "Delete stage N?" confirmation. After every change the session
    reflows as on the web: Holds inherit the previous target; a stage whose
    start changed keeps its rate and gets a new duration (starts move with
    their stage).
  - Validation (web stageErrors): first problem shown in the note, Save
    disabled while invalid. Save is direct (no confirmation), as on the web;
    Discard asks "Discard stage changes?" and restores the whole session. The
    program editor's "+ Add stage" now adds Heating 120 C / 1 C/min / 95 min
    like the web.
  - Fields: one-piece steppers (minus, 96 px value well, plus) with a soft
    amber edge, purple while Hold is selected; a Hold greys Target and Rate
    ("None"). Theme gained `UI_LAB_TONE_FIELD_HOLD`.
  - Tests: new smoke flow covers add, keypad move, delete confirmation,
    Discard restoring the editor's cards, Save marking the draft changed, and
    the Hold lock. Existing flows (Hold/Discard, unsaved-leave prompt,
    telemetry keeping the modal, validation capture) still pass.
  - Verification: three presets build without project warnings; CTest 9/9
    each (codebase-index excluded as before). Web unchanged.
  - Follow-up: the native-only Stage detail page has no route in the UI (only
    the test harness opens it) and no web counterpart; remove it with its
    capture paths in a later slice.
- 2026-09-30, milestone 5 (Running, Manual, graphs) done.
  - Graph panels (all pages): the trajectory is itself the panel; caption
    "Temperature (C) · Time (min)" and one muted legend (Planned, Planned /
    Measured, Predicted, Measured / Predicted); 13 px labels in the web
    gutters; time axis from 0 with no trailing 1/8 margin; 5 x 7 grid in the
    web grid colour; measured line amber in every run state; 10 px stage dots
    only in previews and not under the stage dialog.
  - Running: title with a 14 px amber stage/phase; the readings tile moved
    above the visuals (14 px labels, SemiBold 16 values); Split is two equal
    halves. The program graph shows only the start of automatic cooling (web
    coolingWindowMinutes: at most a quarter of curing); measured cooling is a
    blue dashed overlay. Once measured cooling passes that window the reading
    line offers Show / Hide cooling (`running_cooling_full`); telemetry adds or
    removes the toggle as the window is crossed, and a new run resets it.
  - Manual: readings strip (state, Current / Est. to target and the idle note,
    or Elapsed, To target, Current, Setpoint, Heater demand, Est. power); the
    forecast fills the page with an amber "now" marker; while running it shows
    the measured history, then the prediction to the target, holding briefly.
    Controls tile: Start or Pause/Stop beside one-piece Target/Rate steppers
    with TARGET/RATE captions. Setpoint shows "--": the observation model has no
    controller setpoint (fail-closed; CTRL-SETPOINT in the controller backlog).
  - Tests: Manual geometry check rewritten for the new layout (strip above
    graph above controls, Setpoint present, steppers 48 x 56, trajectory kept on
    a target change); new cooling-toggle check and `running-cooling-full`
    capture; the setup check looks for the Ready strip instead of a title.
  - Verification: three presets build with no project warnings; CTest 9/9
    each (codebase-index excluded as before); Zephyr workflow 3/3; captures of
    both presentations compared. Web unchanged.
  - Known wording differences (milestone 6 with Home): native stage status
    uses the controller's stage name ("3 / 4 - Heating to 160 C") where the web
    shows the kind; door/fan read "closed" / "1450 RPM" natively and "Closed" /
    "1,450 rpm" on the web.
- 2026-09-30, milestone 6 (Home, Device, Settings, keyboards, dialogs) done;
  plan complete.
  - Dialogs: 430 px wide (screen width minus 24 px when narrower), 25 px
    padding, 14 px gaps, SemiBold 20 title, equal centred buttons 112-220 px
    wide and 48 px high; Settings dialogs share the geometry. Delete program now
    asks "Delete program?" / "<name> will be permanently deleted." with a
    "Delete program" button, then shows "Deletion request accepted" (web
    confirmAction and tell). Running dialogs still leave Stop uncovered.
  - Keyboards: SemiBold 18 title (new font size) with a live "20 / 32" count
    for text; the field is a well with an amber bottom edge, centred text, an
    amber blinking caret and the opening value shown selected. Text action row
    on the web's 20-column grid (Cancel, Clear, Space, arrows, Apply). Numeric
    keypad as the web grid: Delete spans two rows, 0 two columns, Cancel /
    Clear / Apply below; the sign key is removed (no field has a negative
    minimum) and "." is dimmed for integer fields.
  - Home: Quick launch title SemiBold 20, 12 px headings, 6 px gaps; recent
    programs hold up to six (web demoRecent 3, 1, 2, 5, 4, 7) and entries are
    dropped from the end until Start manual fits (web fitQuickLaunch), so
    800 x 480 shows four and taller screens six. Readings are an information
    tile with 14 px labels directly above the values; door/fan use 13/19 px.
  - Wording (shared strings, also used by the Zephyr dashboard view):
    "Unknown", "Closed", "Open", "rpm"; the lab formats fan speed as
    "1,450 rpm". Device subtitle uses "·"; Language / units reads
    "English / °C".
  - Device: five rows per page, "4 items" count (page shown by the pager),
    SemiBold 20 headings, rows as tiles with a status-coloured left edge and a
    warning or check-circle icon before the tag.
  - Settings: SemiBold 20 headings, 13 px subtitles, 12 px footer status; the
    Display page shows five rows (Theme moves to More); time values drop the
    chevron and value buttons use 8 px padding so "14 Sep 2026" fits.
  - Stage detail page removed (enum, builder, strings "Stage details" and
    "Edit stage"); the tests open the stage editor from the stage's graph node
    on Program details (`open_stage_editor`).
  - Fix found by the smoke test: when measured cooling crossed the graph
    window during a telemetry update, adding Show cooling rebuilt the whole
    Running page. The toggle is now built whenever a cooling window applies
    and only shown or hidden on telemetry.
  - Verification: three presets build with no project warnings; CTest 9/9
    each (codebase-index excluded as before); web smoke passed (web file not
    changed in this milestone); font, LVGL icon generators and codebase index
    `--check` pass; Zephyr workflow 3/3; captures of both presentations
    compared at all three sizes.
  - Remaining known differences, not changed: (1) the Running stage status
    shows the controller-reported stage name ("3 / 4 - Heating to 160 C")
    where the web derives the kind from its own catalogue ("3 / 4 - Heating");
    the observation model has no stage-kind field, so the native wording is
    kept (recommendation: add a stage kind to the controller observation if
    the shorter form is wanted). (2) The furnace illustration is drawn
    differently (the web's is larger and filled). (3) Device and Service
    attention detail texts come from the native device catalogue and differ
    from the web fixtures.
  - 2026-10-03: (1) resolved by an HMI-only `stage_kind` in the model; (2)
    resolved, the native furnace uses the web drawing and its 58 % panel share;
    single-button dialogs are now centred. The typeface changed to Tomorrow HMI
    in both presentations (fonts README).
