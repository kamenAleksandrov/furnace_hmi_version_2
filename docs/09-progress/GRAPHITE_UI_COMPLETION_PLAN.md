# Graphite UI: all-page completion and verification plan

Status: M0-M5 complete; software verified for operator review (2026-09-09).

Execution evidence (2026-09-09): baseline dirty-file list and patch are in
`build/graphite-review/baseline-status.txt` and `baseline.diff`; initial native
800 x 640 captures are in `baseline-800x640/`. Confirmed defects included
header/button clipping, gap-unaware columns, graph sizing before final layout,
misaligned furnace pieces, hidden Program facts, invalid door claims, and
telemetry page rebuilds. Implementation and native inspection are complete; see
[verification results](GRAPHITE_UI_VERIFICATION.md). Final host, Zephyr, tooling, documentation/index, and whitespace checks passed.
Created: 2026-09-08.
Scope: Furnace HMI V2 host-only LVGL UI laboratory.

## 1. Outcome and starting point

Finish a coherent, practical graphite instrument-panel UI across **every
existing page and dialog**, using the [running-screen reference](../images/theme/graphite-running-concept.svg)
for visual hierarchy and composition. Keep resistive-touch operation, legible
engineering values, predictable navigation, and normal Stop access ahead of
decoration. The reference is an illustration, not a firmware screenshot or a
pixel-perfect layout mandate. Its sample values/paths are not domain semantics.

**Fact:** The current working tree already contains an initial graphite pass,
three viewport presets, and a subsequent furnace-canvas correction. See the
[session log](SESSION_LOG.md). Do not redo those changes blindly or revert
uncommitted work. Inspect, retain, refine, and verify the actual implementation.
Recorded headless success does not establish that text and drawings fit.

**Accepted direction:** Dark neutral surfaces, restrained blue selection,
state accents, clear reading hierarchy, continuous navigation, no gesture-only
operations, and a proportioned furnace drawing. The canonical presentation
baseline is [UX and domain baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md).

**Recommendations delegated to the implementer:** Exact column ratios, spacing
adjustments, font sizes, and responsive card counts may change to make every
viewport readable. Preserve the information and workflows below; do not hide
required information merely to match the illustration.

**Not included:** Production controller integration, wire protocols, thermal/PID
logic, program persistence/acceptance, hardware selection, touch drivers,
firmware flashing, new reset/clear permissions, or a new framework. Device and
Settings receive complete visual treatment of their existing content, not
invented operational features. Do not edit the sibling controller repository.

## 2. Read first and preserve

Read root [AGENTS.md](../../AGENTS.md) and [PLANS.md](../../PLANS.md), then
[INDEX](../INDEX.md), [STATUS](STATUS.md), this plan, and:

- [Requirements](../00-project/REQUIREMENTS.md), particularly REQ-FUN-002/003/009/013/016/017,
  REQ-NFR-001 through 007, and ARCH-INV-001 through 009.
- [Ownership](../01-architecture/STATE_OWNERSHIP.md),
  [boundaries](../01-architecture/BOUNDARIES_AND_INVARIANTS.md), and
  [program/startup model](../03-hmi/PROGRAM_AND_STARTUP_MODEL.md).
- [ADR-0001](../07-decisions/0001-authority-boundary.md),
  [ADR-0004](../07-decisions/0004-desktop-simulator.md),
  [ADR-0005](../07-decisions/0005-lvgl-single-owner.md), and the explicit
  ownership/failure constraints of deferred [ADR-0008](../07-decisions/0008-memory-policy.md).
- [Open questions](OPEN_QUESTIONS.md), [UI-lab plan](UI_LAB_VERTICAL_SLICE_PLAN.md),
  [runbook](../05-platform/DEVELOPER_RUNBOOK.md), and applicable repository skills.

The Control CPU owns truth, validation, limits, persistence, service policy,
and safety. Local editing buffers remain necessary even though operators see
**New program**, **Program editor**, **Save**, and **Add stage**, not "draft"
or "controller accepted revision." A preview must never claim authoritative
save/start/acknowledgement success. Keep simulation identity continuously
visible without overlapping ordinary header text.

The active/paused Program navigation exception in REQ-FUN-003 remains in force:
ordinary navigation is suppressed until normal Stop. Manual remains a distinct
mode; do not turn it into a synthetic Program or extend the Program lock to it
by accident. Host scenario-switching remains available for testing.

## 3. Visual system and layout contract

Use a single set of named theme/style helpers, not slightly different colors
and padding in each page. Start from the existing tokens:

| Role | Baseline |
| --- | --- |
| Background / panel / secondary control | `#171B1E` / `#242A2E` / `#30383D` |
| Primary / supporting text | `#F1F3F2` / `#BBC3C7` |
| Selected navigation | `#285D7A` |
| Divider / graph grid | `#465159` / `#3A454C` |
| Planned / measured path | `#9BA5AB` / `#F1F3F2` |
| Running / heating | Amber; existing `#E8A04B` |
| Paused / hold | Violet; existing `#A77BD1` |
| Manual / cooling | Teal; existing `#42B6A3` |
| Fault | Red; existing `#E76562` |

Machine state and stage kind are different concepts even where they share a
color. Always retain explicit text. Keep idle, stale, and unavailable distinct;
never let a blue selected tab imply readiness or a stale value look current.
Use state accents on the persistent status strip/badge, not whole-screen
recoloring. Measured-path color stays stable when the machine is paused.

- Use an 8 px spacing rhythm, about 12 px panel padding, and 6 px corners.
  Prefer grouped readings and subtle separators over a box around every label.
- Starting typography at 800 x 640: primary temperature 40–48 px, important
  secondary values 26–32 px, page/program headings 20–24 px, ordinary labels
  16–18 px, secondary graph ticks 14–16 px. Adapt after real LVGL measurement;
  do not shrink safety/status/action text to force a dense layout.
- Frequent targets normally have at least 48 x 48 px interactive bounds;
  primary/run actions start at 56 px high. Toolbar peers have the **same
  height**: Back, Add stage, favourite, and rename must not jump in height.
  Increase width for **Add favourite / Remove favourite**; do not use +/-.
- Physical touch sizing is unverified until a panel is selected. Aim to review
  roughly 9–12 mm frequent targets on real resistive hardware; pixel counts
  alone do not prove suitability on either a 7-inch or 10-inch display.
- Frame: reserved simulation/status/header area, bounded flexible content,
  reserved footer. Content cannot extend under the footer. Editor Save/Discard
  and run controls replace navigation rather than stack on top of it.
- Account for all padding, gaps, and borders when sizing columns: 52% + 48%
  **plus a gap** is wider than the parent. Use remaining-space/flex sizing or
  explicit gap-aware dimensions. Never use 100% content height after already
  consuming some of that parent's height with a toolbar.
- Text wraps only in a deliberately budgeted region. No marquee, scroll,
  swipe, hover dependency, or drag-to-edit. Use pages or a details dialog for
  overflow. Full names must be available by a tap if a list needs ellipsis;
  action labels, engineering values, and units may not silently truncate.
- Keep bottom navigation a continuous surface with four equal target regions,
  stable active selection, and optional short 140 ms feedback. Never animate
  readings, plots, fault/stale transitions, or Stop availability. No large
  blurs, perpetual glow, or decorative effect that hides information.

## 4. All-page delivery matrix

Every row is required, including routes not normally reached by a single
happy-path walkthrough. First map the enum and action handlers to the visible
route; do not consider a page complete solely because its builder returns true.

| Page | Required result and checks |
| --- | --- |
| Home | Quick launch on the left; up to three favourites first, then recent programs without duplicates where slots remain. Choose a program stays at the bottom of that column. Use the remaining width for a proportioned 2.5D furnace with clearly positioned temperature, fan RPM, and door readouts. No narrow cropped tower or large accidental empty canvas. Verify empty/few/full quick launch and changed recents in the simulator. |
| Program loading | Selected name/header, large planned graph, compact summary and primary Start preview with secondary Details. Both columns end above navigation, including every X-axis label. No destructive action next to Start. Selecting a different program must change displayed identity and its illustrative data coherently. |
| Programs | New program toolbar; six cards in two columns at the intended baseline where legible. Cards show name, stage count, runtime, maximum temperature; no accepted-revision banner. Center Page X of Y immediately above the card area; Previous/Next match. Test both pages, a partial last page, and empty-library presentation. |
| Program details | Header contains Back, name, explicitly worded favourite toggle, and Edit name; reserve actual widths for both toggle states. Graph fills its allocated left region. Facts in a two-column group on the right: stages, runtime, Max. temp, Est. energy, and material when present. Actions below remain fully visible. Keep delete in a clearly secondary program-management route with confirmation, separate from load/start. |
| Program stages | Ordered numbered cards: name, Heating/Hold/Cooling text and accent, target, rate or ASAP/inherited meaning, duration. Start with three columns where contents fit; paginate instead of clipping/shrinking text. Add stage matches Back height. Center page counter above the grid, not at the left of the toolbar. Include enough fixture stages to test a second page and bounded maximum. |
| Stage details | Same stage identity, ordering, color, units, and values as its card and graph node. Group type/target/rate/duration, make Edit and previous/next navigation predictable, and keep all controls within the content/footer budget. Do not omit this route because the editor is also available. |
| Stage editor / New program / Add stage entry | Heating/Hold/Cooling selector uses consistent type accents. Heating target 1 C, rate 0.1 C/min, duration 1 min steps; rate/duration update each other as local assistance. Hold shows inherited target and edits duration; Cooling supports target plus optional rate, otherwise ASAP. Keep short field labels, explicit units, validation feedback, and large controls. Save/Discard replace bottom nav and stay glued to the same bottom position for every type. Preserve the existing preview scope: New/Add are not proof of persisted creation. |
| Running | Dedicated screen with dominant current temperature; target and estimated remaining time next; estimated power and elapsed time subordinate but visible. Stage number/name/type/target and machine state remain readable in Graph, Split, and Furnace modes. Split follows the reference hierarchy, using a properly scaled furnace. Pause/Resume and normal Stop remain prominent and reachable; a segmented Graph/Split/Furnace selector is visually secondary. Bottom navigation stays absent during active/paused Program presentation. |
| Device | Apply the same grouped readings, proportioned drawing, ownership labels, and service/maintenance card style. Display temperature, demand, fan, and door with validity; invalid door is not "Open" or "Closed." Service condition/details and any available acknowledgement record belong here, not in Program authoring. Missing controller fields are unavailable/preview, not hard-coded operational claims. |
| Settings | Consistent label/value rows or grouped cards for existing theme, Celsius, English, and controller-settings placeholder content. Distinguish HMI-local preferences from controller-owned/unimplemented settings. Do not make read-only rows look like functioning controls or introduce a reset/clear operation. Paginate future overflow. |

### Dialogs and transient states

Inventory all `open_dialog` callers and action entry points, including Notices,
service warning, delete, save, discard, run details, rename if exposed, and
generic request-preview/result feedback. Use one dialog shell with a stable
title/body/action layout, clear dismissal, and touch-sized actions. Long text
must fit through concise copy or explicitly paged details, not scroll.

No background navigation click-through or lost edit state. Repeated telemetry
updates must not dismiss/recreate an open dialog. A run details/notice overlay
must not obstruct the normal Stop path. Service acknowledgement remains a
controller-owned one-time policy with later Device visibility; local dismissal
or simulated confirmation is not permission, persistence, or fault clearance.

Where pending/rejected/unavailable presentation exists, style and test it. A
bounded host-only visual fixture can demonstrate missing variants, clearly
labelled as simulation. Do not implement a new production request lifecycle
merely to demonstrate a dialog.

## 5. Shared graph and furnace requirements

Implement or correct once, then use consistently in loading, details, stage
previews, and all Running modes. Inspect the shared dashboard graph too if it
is reachable in the lab; do not leave two conflicting visual languages.

- Graph root fills its allocated area, without an extra framed/nested box or
  rounded plot corners. Reserve separate measured widths for Temperature (C),
  Planned, and Measured; omit Measured in a preview with no measured series if
  that is clearer. Never put a redundant title in the same label row.
- Put time units at the beginning of the X axis and numeric ticks in dedicated
  positions. Show readable minimum/middle/maximum and useful grid subdivisions.
  Leave room for the longest formatted negative temperature/time tick.
- Planned path is muted **solid**, straight between stage boundaries. Measured
  path is contrasting solid and drawn above it; do not restore the older
  dashed-line proposal. No interpolation across unavailable sample gaps.
- Preview nodes identify the **beginning** of each stage and use its type
  accent. Taps open the corresponding stage. Use expanded hit regions where
  unambiguous; closely spaced nodes must have a stage-list alternative instead
  of overlapping invisible targets that select the wrong stage.
- Show the whole predicted trajectory immediately on start. Retain initial
  bounds with headroom while samples remain inside; expand only on observed
  time/temperature overflow. Preserve bounds across mode switches and unrelated
  refreshes, reset for a different run, and never extrapolate measured history.
- Fixture graph, stage list, summary, elapsed/remaining, and selection must be
  internally consistent for the test being demonstrated. This is fixture
  coherence, not authoritative thermal forecasting or controller validation.
- Furnace geometry belongs on a dedicated non-flex canvas inside a sized
  layout container. Scale/position the body, roof, side, door, and callouts
  together; do not independently align overlay children in a column flex flow.
  Static sensor/status text remains usable without animation.

## 6. Code areas, lifetime, and bounds

Current entry points (verify against the working tree before editing):

- `simulator/ui_lab/src/ui_lab_explorer.c/.h`: page builders, graph/visual,
  dialogs, navigation, action slots, local editor state, theme helpers.
- `simulator/ui_lab/src/main.c`: LVGL owner, scenarios, structural smoke tests.
- `simulator/ui_lab/CMakeLists.txt`, root `CMakePresets.json`: viewport builds.
- `app/hmi/presentation/strings/`: localized visible copy; do not scatter new
  English strings in builders. Verify the chosen font contains every glyph.
- `app/hmi/presentation/dashboard/`, `app/hmi/model/`: shared graph/value
  presentation contracts; preserve their authority/validity semantics.
- `simulator/semantic_controller/`: deterministic bounded fixture data only.
- `simulator/desktop/lv_conf.h`: fonts and LVGL configuration; changes can
  affect the older desktop proof as well as the UI lab.
- `tools/run_ui_lab.py`, host tests, runbook, and traceability as needed.

Prefer small shared visual helpers over unrelated per-page styling. Extract
reusable LVGL-only components into `app/hmi/presentation/` only where actually
shared/needed; host fixtures, scenario shortcuts, SDL capture, and simulation
identity stay under simulator/tools. Do not mandate a large framework rewrite
or production screen migration as a prerequisite to this pass.

Only the GUI owner may touch LVGL, including screenshot/snapshot access.
Document backing-array, callback, style, and animation lifetimes. Keep graph
buffers/action slots bounded and handle capacity exhaustion visibly; do not
introduce unbounded history. Preserve the documented LVGL allocation-failure
limits rather than claiming universal recovery. Remove owned callbacks/timers
on destruction and test navigation during animation. Update live values in
place where possible; avoid rebuilding whole pages on every telemetry tick,
resetting graph domains, losing modals, or restarting decorative animations.

## 7. Ordered milestones

All items start unchecked intentionally: existing code is a starting point,
not fresh visual evidence. Keep this section current with actual results.

- [x] **M0 — Baseline audit.** Record dirty files without changing them, build
  fresh artifacts, inventory all routes/dialogs, and capture current screens.
  List gaps versus sections 3–5, separating confirmed defects from preferences.
- [x] **M1 — Shared frame/components.** Finish tokens, typography, buttons,
  status strip, continuous nav, bounded content/footer, dialog shell, graph,
  and furnace canvas. Add regression checks for the confirmed layout defects.
- [x] **M2 — Representative screens.** Finish Home, Program details/loading,
  and Running in all three modes. Capture and inspect all three supported
  viewport sizes before propagating composition choices to the remaining pages.
- [x] **M3 — Library and editor.** Finish Programs, Program stages, Stage
  details, all editor types, favourite labels, pagination, and fixed editor
  footer. Exercise second/last pages and unsaved-change dismissal paths.
- [x] **M4 — Remaining surfaces.** Finish Device, Settings, every dialog,
  Manual and availability/state variants within the existing preview scope.
  Check that every enum route and every visible action appears in the audit.
- [x] **M5 — Verification and handoff.** Complete the evidence matrix below,
  fix detected defects, rerun affected tests, update canonical docs and index,
  and provide exact launch commands plus representative screenshots.

Proceed between milestones without waiting for aesthetic approval on each
page. Ask only when an actual requirement/authority conflict or essential new
product decision blocks work. Record unresolved preference questions for the
final operator review rather than leaving ordinary pages unfinished.

## 8. Verification: completion requires rendered evidence

### Required coverage

1. Build and test 800 x 480, **800 x 640 primary review target**, and 1024 x 600
   with the existing presets. Keep 640 x 360 structural smoke coverage; it is
   not a promise of physical usability or a selected production display.
2. Capture **actual LVGL output**, not an HTML/SVG recreation, for every page
   at each of the three review sizes. Include every Running mode, every editor
   type, first/last library and stage pages, both favourite labels, and every
   dialog. Use deterministic scenario time when comparing images.
3. Inspect the images at native resolution. Check text extents, units, graph
   legends/ticks, bottom/right edges, toolbar height consistency, furnace
   proportions, hierarchy, and unused accidental space. Attach evidence in an
   ignored build report plus a concise durable Markdown results record; commit
   only a small useful set of review screenshots if appropriate.
4. Exercise disconnected, idle, running, Manual, paused, fault, stale, and
   running-overrun. Check zero/invalid values, long names/labels, large/negative
   readings, missing graph samples, short/long programs, empty/partial lists,
   maximum fixture capacity, and repeated navigation/dialog open-close cycles.
5. Add automated containment checks against **all clipping ancestors**, not
   just screen bounds: graph labels, cards, dialog content, and action targets
   must fit within their allocated viewport and above the footer. Check actual
   label metrics after layout; object existence alone cannot prove readable text.
6. Test touch interactions and selection: no gesture dependence, no ambiguous
   node targets, editor nav suppression including shortcuts, Program lock,
   Stop reachability, no duplicate request intent on repeated taps, and static
   selection after animations. Do not infer controller acceptance from these tests.
7. Test graph fixed-domain retention, out-of-bounds expansion, new-run reset,
   planned/measured ordering and gaps; verify session/validity behavior stays
   fail-closed. Ensure invalid door/fan/service data never becomes a valid claim.
8. Review object ownership, allocation/capacity checks, timer/callback cleanup,
   and work per telemetry update. If shared sources/fonts/configuration change,
   run their host/desktop suites and applicable Zephyr checks from the runbook.

If capture automation is missing, add a small host-only capture mechanism or
use native window capture. Do not capture the desktop instead of the intended
app, send ineffective synthetic clicks and assume navigation happened, or
inspect an old executable. Record scenario, route, viewport, and source/build
identity beside each capture. If GUI/capture execution is genuinely blocked,
report visual verification as blocked rather than mark this plan complete.

### Commands from the repository root

Use these existing presets; stop immediately on a failed configure/build so
CTest cannot accidentally validate an old binary:

```powershell
$uiPresets = @(
    "host-msvc-ui-lab-debug",
    "host-msvc-ui-lab-800x640-debug",
    "host-msvc-ui-lab-1024x600-debug"
)
foreach ($uiPreset in $uiPresets) {
    cmake --preset $uiPreset
    if ($LASTEXITCODE -ne 0) { throw "Configure failed: $uiPreset" }
    cmake --build --preset $uiPreset
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $uiPreset" }
    ctest --preset $uiPreset --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed: $uiPreset" }
}
python tools\run_ui_lab.py --scenario idle
```

The default runner opens the default viewport. After building successfully,
launch either larger variant with its exact executable:

```powershell
.\build\host-msvc-ui-lab-800x640-debug\simulator\ui_lab\Debug\furnace_hmi_ui_lab.exe --scenario idle
.\build\host-msvc-ui-lab-1024x600-debug\simulator\ui_lab\Debug\furnace_hmi_ui_lab.exe --scenario idle
```

Open one at a time for an unambiguous review. Do not assume
that resizing a desktop window tests another native LVGL layout. Existing
keys 1–8 switch lab scenarios; the runner also supports check-only mode:

```powershell
python tools\run_ui_lab.py check --scenario running-normal
.\.venv\Scripts\python.exe -m unittest discover -s tools\tests -v
.\.venv\Scripts\python.exe tools\codebase_index.py
.\.venv\Scripts\python.exe tools\codebase_index.py --check
git diff --check
```

Run each verification command with exit-code checks. Follow the
[generated-document policy](../_generated/README.md) if canonical CMake
metadata needs preparation/reconfiguration; do not weaken validation with a
degraded report. Index generation is not visual validation. Report actual
counts and command outcomes, not previous session counts as new evidence.

## 9. Documentation, decisions, and final deliverables

Update this checklist and the canonical UX baseline only for actual agreed
presentation changes. Update [STATUS](STATUS.md), [ROADMAP](ROADMAP.md),
[SESSION_LOG](SESSION_LOG.md), [traceability](../06-testing/TRACEABILITY.md),
and the runbook as affected. Regenerate generated indexes with their owning
tool; never manually edit generated metadata.

No storage migration is expected. Preserve unrelated dirty edits; keep changes
separable so the theme/capture pass can be reverted without deleting local
program data or changing a controller. Do not use destructive Git resets.

Final handoff must include:

- Per-page completion/evidence table, including dialogs and viewport coverage.
- Representative actual screenshots and paths to the full capture report.
- Build/test/geometry results, observed defects fixed, and checks not run.
- A one-command default launch and exact larger-viewport launch instructions.
- Remaining product/hardware questions explicitly separate from software defects.

Final panel size, physical touch testing, reduced-motion preference, production
service semantics, controller validation granularity, and final Device/Settings
fields remain open. They do **not** block the bounded visual completion work.
Software work can be handed over as verified for operator review; do not claim
the operator approved the visuals or hardware usability on their behalf.

## 10. Copy/paste task for the implementing agent

> Read AGENTS.md and docs/09-progress/GRAPHITE_UI_COMPLETION_PLAN.md and execute
> its milestones against the current working tree. Preserve existing changes.
> Finish the graphite UI across every current route/dialog, using the linked
> SVG as a visual reference, not a screenshot or domain model. Fix layout and
> clipping defects, inspect actual LVGL captures at all three specified sizes,
> run the relevant tests, and update the plan/status/docs/index with evidence.
> Keep simulation host-only and preserve Control CPU authority. Do not stop at
> a palette-only change or passing smoke tests; report any genuinely blocked
> checks and leave operator/hardware sign-off explicitly pending.
