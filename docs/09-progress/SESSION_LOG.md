# Session log

## 2026-09-27 - Program editor rework, equal borders and operator text sweep

**Fact:** The light strip is 12 px (native: negative bottom margin cancels the row
gap; web: 12 px side padding, the nav owns its 8 px gap). The program editor was
rebuilt in both presentations around a shared stage card, with change tracking
(web: draft snapshot; native: `program_editor_dirty` set by rename, add stage or a
changed stage). About 40 explanation strings were reworded or removed via one
mapping applied to the web demo and native catalogue; the native
`REQUEST_PREVIEW_DETAIL` string was removed and dialogs hide an empty body.
Fixed native overlays stopping 24 px short of the bottom and the editor return
page being overwritten after opening a stage.

**Verification:** Browser smoke passed; native 10/10 CTest at three sizes;
desktop preset and Zephyr workflow passed; captures inspected.

## 2026-09-26 - Touch keyboard layout refinement

**Fact:** Text keyboard rows are full (hyphen key added; Delete moved beside m;
Caps/Delete 1.5 keys wide via a 20-column web grid and 2:3 LVGL flex weights).
Keyboard rows now fill the panel height in both presentations. The numeric
keypad panel is 460 px wide and centred with a dimmed backdrop. The web label
Backspace became Delete to match native. Native disabled keys override the
LVGL default theme's disabled recolor so they recede.

**Verification:** Browser smoke passed; native 10/10 CTest at the three review
sizes without warnings; captures inspected. Physical touch not tested.

## 2026-09-26 - Program details thirds, Running reading line and cooling phase

**Fact:** Program details moved Edit program from the title bar to a three-action
bottom row and aligned graph/facts to a shared thirds grid (CSS grid and LVGL
grid). Running replaced boxed readings, native top reading cards and the web
cooling card row with one label/value line. Added `run_phase`,
`cooling_elapsed_seconds`, `cooling_remaining_seconds` (estimated) and
`graph_samples[].planned_cooling` to the dashboard model, the `cooling`
simulator scenario, four catalogue strings, and a polyline dashed cooling
overlay. The running-normal fixture now reports 4 stages and curing remaining
to the 68-minute curing end.

**Verification:** Browser smoke passed; native 10/10 CTest at all three sizes
and the desktop preset passed without warnings; captures inspected. The Zephyr
qemu_x86 workflow (3/3 cases, analysis) and tooling tests passed after the
shared model/string change. `run_foundation.py --scenario` now also accepts
`running-overrun` and `cooling`.

## 2026-09-26 - Program details, cooling preview and 24-pixel light strip

**Fact:** Set the light strip to 24 pixels in both presentations. Program
previews now end with a dashed green automatic-cooling segment limited to a
quarter of the curing time; the web Running view keeps the full estimate. LVGL
draws the dashes in a draw-event overlay because its software renderer only
dashes horizontal and vertical lines. Native Duration now precedes its values.
View stages is outline-only, and Edit program is icon-only on details and
stages in both presentations.

**Verification:** Browser smoke passed. Native 10/10 CTest passed at 800x640
and 1024x600 with no compiler warnings; captures were inspected. The 800x480
link was blocked by the open UI-lab executable.

## 2026-09-26 - Light strip height and browser Manual graph scale

**Fact:** Reduced the full-width top light target from 48 to 32 pixels in the
browser demo and native lab (`FURNACE_HMI_UI_LAB_LIGHT_TARGET_HEIGHT`). The
browser Manual graph draws in its own layout pixels through a ResizeObserver,
so text and the current-temperature marker are no longer distorted. It uses
the native labels (maximum/middle/minimum C, Time (min)/middle/end min).
Renamed the native Manual page capture to `manual-setup` because the `manual`
scenario capture overwrote it.

**Verification:** Browser smoke passed, including the new viewBox-equals-layout
check. Native builds and 10/10 CTest passed at 800x640 and 1024x600, and the
fresh captures were inspected. The 800x480 preset compiled but could not link
while the UI-lab executable was open. Physical touch was not tested.

## 2026-09-26 - Running readings, keyboard and light target refinement

**Fact:** The Manual page title is Manual mode; browser and native Running
present curing elapsed and remaining side by side, with cooling time separate.
The old switch control is removed, estimate labels use Est., and the text
keyboard is one digits-plus-letters layout. The top accent remains visibly
4 pixels high while its full-width click target is 48 pixels high.

**Verification:** Browser smoke passed keyboard entry/persistence, same-line
time layout without overflow and light-target geometry. Native builds and all
10 CTest checks passed at 800x480, 800x640 and 1024x600, including unknown
remaining time and compact maximum-stage geometry. No firmware, protocol,
physical-touch or furnace-control behavior was tested.

## 2026-09-24 - Manual action and graph-scale stability

**Fact:** Manual Start, Stop, Pause and Resume now update the retained native
LVGL Manual page rather than rebuilding it. The retained browser and native
Manual graph uses the full supplied 0--180 C range, so selection changes do
not auto-zoom the temperature axis.

**Verification:** Browser smoke passed. Native MSVC builds and all 10 CTest
checks passed at 800 x 640 and 1024 x 600. Default 800 x 480 remains pending
while its UI-lab executable is open; no firmware, protocol, physical-touch or furnace-control behavior
changed.

## 2026-09-24 - Manual graph and input stability

**Fact:** Browser Manual graph/card sizing now fills its assigned visual area.
The browser simulation leaves an open touch keyboard intact during periodic
ticks. Native Manual target/rate actions update the existing trajectory and
labels instead of rebuilding the Manual page, removing the visible
minimum-size-to-final-size graph redraw. Lower-band context text is larger in
both renderers.

**Verification:** Browser smoke passed its keyboard-persistence and graph-fill
checks. Native MSVC build and all 10 CTest checks passed at 800 x 640 and 1024
x 600, including trajectory-identity coverage after a target adjustment.
Default 800 x 480 is pending because its executable is currently open. No
firmware, protocol, physical-touch or furnace-control behavior changed.

## 2026-09-24 - Manual lower-band alignment

**Fact:** Browser and native LVGL Manual now reserve the graph for the
temperature prediction. State and Curing elapsed appear above the lower-band
controls; Start/Pause/Stop are at its left and target/rate controls at its
right. Operator-facing run buttons use Start, Pause, Resume and Stop, without
demo/preview wording. The native geometry test protects that ordering.

**Verification:** Browser smoke passed. Native MSVC build and all 10 CTest
checks passed at 800 x 480, 800 x 640 and 1024 x 600. No firmware, protocol,
physical-touch or furnace-control behavior changed.

## 2026-09-24 - Manual graph and control composition refinement

**Fact:** The browser Manual prediction graph now occupies roughly the upper
70 percent of a vertical split; browser and native LVGL use the lower 30
percent as a compact control band. Browser and native LVGL
source share label-over-control target/rate
bands with 48 x 52 minimum plus/minus targets, yellow outlines and bottom
anchoring above Start or Pause/Stop. Native Manual adds controller-observed
`Curing elapsed`; the browser shows its equivalent through its clearly
session-scoped demo simulator.

**Verification:** Browser smoke, whitespace and generated-index freshness
checks passed. Native MSVC builds and all 10 CTest checks passed at 800 x 480,
800 x 640 and 1024 x 600, including the Manual elapsed/control geometry capture
path. No firmware, protocol, physical-touch or furnace-control behavior changed.

## 2026-09-24 - Shared UI parity refinement

**Fact:** Updated both browser and native LVGL UI shells to remove the
status/current-page bezel and retain a single yellow, glowing top-line interior
light request control. Reworked Manual into a current-target-only prediction
graph with direct and +/- controls, Pause/Stop, and no separate Manual cooling
screen. The browser controller fixture now returns to Manual setup when it
ends heating at 48 hours and provides the ten-minute warning. Revised stage
cards and Program details to the agreed shared hierarchy. Native visual/smoke
tests now invoke the retained service-dialog API instead of the removed header
button.

**Verification:** Native MSVC UI-lab build and configured CTest passed 10/10;
the browser smoke passed with explicit light, Manual controls, stage-card and
Program-detail assertions. No firmware, physical touchscreen, deployment, or
real furnace/control-CPU integration was run.

## 2026-09-23 - Operator workflow first implementation slice

**Fact:** Implemented the first native LVGL/browser slice from the operator
workflow plan. Quick Launch now opens Manual instead of Choose program; Manual
supports controller-bounded target/rate setup, start/pause/stop, light-demo
control, 48-hour automatic cooling transition, and run containment. Programs
now have a browser creation/edit route, authored Heating/Hold stages only,
automatic cooling estimates, and no user stage names. Native LVGL mirrors the
new Manual, Program Editor, automatic cooling, Device log and Maintenance
routes; the obsolete Program preferences Settings category is removed. Device
now exposes a volatile event log in the browser demo. The browser simulates
asynchronous HMI requests and Control CPU acceptance for showable demos.

**Deferred:** Production protocol messages, Control CPU validation/timing,
furnace-side light and door policy, real cooling estimation, maintenance
acknowledgement records, persistence, and restart behavior remain backlog work.

**Verification:** Native MSVC build and configured CTest passed 10/10,
including geometry and semantic UI-lab smoke. The extended headless browser
smoke and `git diff --check` passed. No firmware, physical touchscreen,
website deployment, or real furnace integration was run.

## 2026-09-23 - Operator workflow plan and screen concepts

**Fact:** Inspected requirements, authority/protocol/GUI ADRs, current browser
routes and prior plans. Added [plan](OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md),
[concepts](../03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md) and
[controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md). Recorded REQ-FUN-005/017
conflicts for explicit reconciliation. Summary-row editing is recommended from
the 800 x 480 budget, not a visually tested implementation.

**Verification:** All 146 local Markdown targets across the eight planning/index
records resolve; scoped diff and new-document whitespace checks passed. The
owning codebase index generator and freshness check passed. Runtime, screenshots,
native/Zephyr, thermal and physical-panel checks were not run. No application
or firmware code changed.

## 2026-09-18 - Client web demo planning

**Fact:** Inspected both HTML copies, website sync/build ownership, existing browser behavior, governing HMI records and exact emulator key mappings 1-8. Created the [agent build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) for incremental single-file implementation. The user confirmed faithful HMI presentation with simulated controller responses. The follow-up answers confirmed session-only saved programs with premade examples, an ordinary online webpage without installation/download/offline support, and English only. The plan was updated to remove persistent storage, import/export and service-worker work. No HTML, firmware or website behavior was changed.

**Verification:** All 130 local Markdown file targets across the six planning/index records resolved; diff whitespace checks passed. The owning codebase-index generator completed successfully. Its final freshness check is recorded with the handoff result. Browser interactions, screenshots, native/Zephyr builds, physical phones and deployment were not run for this planning pass.

## 2026-09-15 - Running readings refinement

The [running readings refinement](RUNNING_READINGS_REFINEMENT_PLAN.md) adds a
tappable elapsed/remaining total, filled tiles, and live full-graph readings.
Native builds and 10/10 tests pass at all three review sizes (67 captures each);
Chrome running layout/toggle checks pass at the same sizes. Existing keyboard,
program title, star and graph refinements are preserved. Host/browser only.

## 2026-09-08 - All-page graphite execution handoff

- Prepared the [completion plan](GRAPHITE_UI_COMPLETION_PLAN.md) at the
  stakeholder's request for another agent. It starts from the existing dirty
  graphite implementation and preserves that work, rather than assuming a
  palette-only implementation or another clean baseline.
- Specified all ten routes, dialogs, shared controls/graph/furnace composition,
  editor and running containment, ownership constraints, and phased acceptance
  criteria. Actual LVGL screenshots at 800 x 480, 800 x 640, and 1024 x 600 are
  required; previous smoke success is not promoted to visual sign-off.
- Preserved the existing illustrative SVG under
  [docs/images/theme](../images/theme/graphite-running-concept.svg) so the
  reference travels with the repository rather than living only in ignored build.
- This handoff changes documentation only. No application implementation,
  controller behavior, or new UI build/test/visual result is claimed here.
- Handoff verification: all 40 Python tooling tests passed; 81 local Markdown
  link targets resolved; the preserved SVG parsed as XML; index generation,
  index --check, and git diff --check passed. Application/Zephyr builds and
  live UI review were not rerun for this documentation-only change.

## 2026-09-08 - Screenshot-driven furnace composition correction

### Fact and decision

- The stakeholder's live screenshot showed that the first graphite pass had
  applied the palette and hierarchy but had not achieved the intended concept
  composition: the Home furnace visual was narrow, clipped, and surrounded by
  unused space.
- The cause was a layout defect in the host-only lab: furnace illustration
  pieces were direct children of a column flex panel while also being aligned
  as overlays, and the visual panel had no explicit full-column width.
- Corrected the composition with an explicit full-width visual panel and a
  dedicated non-layout canvas for the furnace, roof, side, and door overlays.
  The door is now aligned to the furnace canvas, and compact widths reduce the
  illustration proportionally.

### Verification

- Rebuilt the default, 800 x 640, and 1024 x 600 UI-lab variants successfully.
- Full CTest passed 9/9 for each UI-lab variant after the correction.
- Launched the freshly rebuilt 800 x 640 UI-lab executable for stakeholder
  inspection. Manual visual sign-off and physical touch-target measurement
  remain pending.

## 2026-09-08 - Graphite instrument-panel UI pass

### Scope

- Applied the graphite instrument-panel visual system across the complete
  host-only explorer: shared charcoal palette, restrained blue navigation,
  deliberate state accents, common 6 px corners, 8 px spacing, grouped metric
  bands, and touch-sized controls.
- Rebalanced Home and Running hierarchy so current temperature dominates target
  and remaining time; estimated power and elapsed time are subordinate.
- Rebuilt the furnace visual layout to be width-aware, kept planned/measured
  graph language stable through paused presentation, added a persistent state
  strip, and moved Delete into Program management away from Start.
- Added explicit 800 x 640 and 1024 x 600 UI-lab presets while retaining 800 x
  480 and the compact 640 x 360 smoke floor.

### Facts and decision

The implementation changes only the host-only presentation lab and its desktop
LVGL font configuration. The Control CPU remains authoritative; no production
protocol, controller behavior, persistence, safety logic, or embedded display
wiring was added. The graphite visual system is accepted as a reversible
presentation direction pending manual operator review.

### Verification

- Default `host-msvc-ui-lab-debug`: build passed; full CTest passed 9/9.
- `host-msvc-ui-lab-800x640-debug`: configure/build passed; full CTest passed
  9/9.
- `host-msvc-ui-lab-1024x600-debug`: configure/build passed; full CTest passed
  9/9.
- Canonical `host-debug` and `host-msvc-desktop-debug`: builds passed; CTest
  passed 10/10 for each.
- `git diff --check` passed. The compact smoke path exercises 640 x 360 and
  verifies the state strip, grouped Running reading band, graph objects, and
  all current routes/scenarios.
- Manual visual review and physical touch-target measurement were not run in
  this session. Existing Zephyr evidence remains applicable because no Zephyr
  target or production source changed.

## 2026-09-04 - Favourite-state clarity and Program-loading containment

### Scope

- Replaced the ambiguous favourite `+`/`-` labels with **Add favourite** and
  **Remove favourite**. The favourite action is widened without changing its
  height; the wider **Add stage** action likewise remains the same height as
  its compact Back neighbor. These remain HMI-local quick-launch preferences,
  not controller program changes.
- Corrected the Program-loading content row to the actual remaining height
  after its toolbar, rather than a percentage that included that toolbar. The
  trajectory matches that visible row, so its lower axis/labels cannot extend
  under bottom navigation; the facts/actions column shares the same bound.

### Verification

- Rebuilt `host-msvc-ui-lab-debug` and passed all 9 CTest tests. Host Debug
  also passed 10/10 CTest tests.
- The UI-lab smoke now fails if the Program-loading graph or its time-axis
  label enters bottom navigation at the 640 x 360 structural viewport.
- Regenerated the deterministic codebase index; `--check` and `git diff
  --check` passed. The rebuilt 800 x 480 visual review is open.

## 2026-09-04 - Graph-label clearance and continuous navigation feedback

### Scope

- Removed the duplicate trajectory title that collided with the temperature
  axis. The axis, planned legend, and measured legend now each have dedicated
  top-row space; the X-axis text and time labels retain a reserved bottom band.
- Replaced four separated bottom-navigation buttons with one neutral continuous
  tab surface. The current destination is the only raised segment, with a
  140 ms deep-blue glow after a destination change and an immediate pressed
  treatment.
- Constrained this motion to cosmetic LVGL-owner feedback. Controller values,
  graph data, faults, alarms, Stop availability, and stale/unavailable status
  remain immediate rather than animated. A reduced-motion policy is recorded
  as open.

### Verification

- Rebuilt `host-msvc-ui-lab-debug` and passed all 9 CTest tests. Host Debug
  also passed 10/10 CTest tests.
- Regenerated the deterministic codebase index; `--check` and `git diff
  --check` passed.
- Reopened the exact rebuilt idle scenario for 800 x 480 operator review.

## 2026-09-04 - Program authority wording and compact editor layout

### Scope

- Applied the operator-facing terminology decision: the library offers **New
  program**, stages offer **Add stage**, and the edit screen is **Program
  editor**. Program cards no longer present a controller-revision badge. This
  does not create HMI program ownership: the transient editing buffer remains
  internal presentation state and the controller must validate any library
  change.
- Restructured Program detail facts into two compact columns and shortened the
  labels; moved the favourite/name controls to fitting top-row actions.
- Removed the nested graph panel. The trajectory now uses its parent’s available
  space, retains the time label at the X-axis start, separates planned/measured
  legend text, and anchors color-coded stage nodes at stage beginnings.
- Reworked Program stages into compact three-column cards with an Add stage
  action. Program-editor Save/Discard now occupy the bottom-navigation position
  and ordinary navigation is hidden for that edit surface.

### Verification

- Rebuilt `host-msvc-ui-lab-debug`; all 9 CTest tests passed. The smoke now
  also fails if ordinary bottom navigation remains visible in the Program
  editor.
- Reopened the interactive idle scenario for visual review.

### Open boundary

The UI does not decide whether a stage is validated individually or as part of
an atomic Program save. That controller-interface lifecycle is recorded in
[open questions](OPEN_QUESTIONS.md) and is not simulated as furnace behavior.

## 2026-09-04 - Program, Stage, and trajectory visual refinement

### Scope

- Recorded the decision that the planned trajectory is a muted **solid** path,
  while the contrasting solid measured path is drawn above it. The active-run
  graph starts with the full accepted-plan domain and retains it until a live
  observation escapes its time or temperature headroom; an HMI graph expansion
  is only a display response, never a fault decision.
- Tightened Program detail and Stage-list layouts for the fixed host viewport:
  top-row favourite/name actions, compact colored facts, centered page labels,
  smaller ordered Stage cards, and Heating/Hold/Cooling color accents. Preview
  trajectory nodes now use those same stage colors.
- Refined HEATING draft assistance: target steps are 1 °C, rate steps are
  0.1 °C/min, and duration steps are one minute. Editing rate recalculates the
  local duration estimate; editing duration recalculates the local rate
  estimate. This is keyboard-ready HMI assistance, not a controller formula or
  validation rule.
- Updated the host semantic fixture to supply a complete planned run from the
  outset and to reveal measured samples only as simulated time reaches them.

### Verification

- Rebuilt `host-msvc-ui-lab-debug`; all 9 CTest tests passed. Its structural
  smoke covers every scenario, page, Running view mode, and stage-editor kind.
- Opened the updated interactive UI-lab window using `running-normal` for
  operator visual review. The visible window, not a test, remains the next
  source of layout feedback.

### Boundary

The full plan and stage data remain deterministic host fixtures. Production
must obtain the accepted forecast/revision and live observations from the
Control CPU through the yet-to-be-designed interface; it must not copy the
simulator's local calculator or infer a furnace fault from display bounds.

## 2026-09-03 - UI-lab program workflow refinement

### Scope

- Recorded the stakeholder decision to call the rising-temperature stage
  **HEATING** everywhere in the HMI. Its local draft takes target and °C/min as
  inputs and shows calculated duration; HOLD uses inherited/saved-ambient
  preview context, and COOL supports either optional controlled rate or the
  controller's fastest permitted cooling request.
- Reworked Home to put quick launch on the left, retain up to three local
  favourites, and maintain a bounded synthetic recently-run list after Start
  preview. Added a complete two-page, six-card-per-page program library.
- Reworked Program detail into graph and facts/actions columns, expanded stage
  cards with type/target/rate/duration, enlarged the furnace visual, and made
  preview-stage graph nodes locally clickable. The graph now uses more grid
  divisions, middle-range labels, and a dashed planned path beneath measured
  samples.
- Recorded the current explicit product exception to general run-time
  navigation: while the Program-running view is active, ordinary bottom
  navigation is hidden and rejected until normal Stop. This is presentation
  containment, not a controller permission, stop action, or safety behavior.

### Verification

- `host-msvc-ui-lab-debug` built and passed 9/9 CTest tests. Its smoke now
  checks every scenario/route/view mode, stage-editor kind, service dialog,
  hidden active-run navigation, and rejected attempted route departure.
- `host-debug` configured, built, and passed 10/10 CTest tests.
- The project-local Zephyr environment check passed; the already-configured
  `qemu_x86` build accepted the shared catalog update (`ninja: no work to do`).
  The existing Twister 3/3 evidence remains current for unchanged tests.

### Remaining review

Open the visible UI-lab window at 800 x 480 and provide operator visual
feedback. The lab remains non-authoritative: it does not persist a program,
validate a furnace run, execute Stop/Pause/Resume, or provide production
controller/protocol behavior.

## 2026-09-02 - clarified Home, Programs, and dedicated Running UI-lab pass

### Scope

- Recorded the user-approved service policy boundary: the Control CPU emits one
  acknowledgement opportunity per configured service period, records an
  accepted acknowledgement for later Device/maintenance inspection, and remains
  the only authority for allowing operation.
- Recorded local ProgramDraft authoring with controller-provided future
  capability assistance. HEATING target/rate and calculated duration remain an explicit
  cross-field/controller validation problem; HOLD inherits its target; COOL
  exposes fastest-allowed versus controlled-rate presentation.
- Reworked the host-only UI laboratory: Home now has a 2.5D furnace view with
  observed temperature/fan/door bubbles and favourite/recent quick launch;
  Program loading/library/overview/stages/draft pages are distinct; active
  programs use a dedicated Running page with graph, split, and visual modes.
- Replaced the generic chart with a rectangular trajectory view: time and
  temperature labels, grid, muted dashed planned path, solid state-accented
  measured segments above it, invalid-data gaps, and headroom/overrun scaling.
  The `running-overrun` scenario makes the expansion visible without device
  data.

### Verification

- Host Debug configured, built, and passed 10/10 CTest tests.
- The canonical MSVC UI-lab preset configured, built, and passed 9/9 CTest
  tests. Its smoke renders every scenario, page, three Running view modes, and
  three StageDraft kinds at the 640 x 360 structural viewport.
- The pinned Zephyr qemu_x86 build completed; Twister passed 3/3 test cases and
  GCC analysis produced no repository-owned warning. Known upstream Zephyr libc
  analyzer warnings remain.
- The codebase index was regenerated and its fail-closed `--check` passed.

### Remaining review

The host laboratory is ready for operator visual feedback at its 800 x 480
development viewport. It still does not validate a real Control CPU, persist a
program, enact service policy, or authorize a furnace operation.

## 2026-09-02 - expanded host-only UI explorer and service-warning baseline

### Scope

- Expanded the UI laboratory into touch-first Home, Programs, Program detail,
  Stage detail/draft editor, Device status, Settings, notices, and confirmation
  views. The layout uses fixed pages and explicit navigation rather than scroll
  or drag gestures.
- Replaced the light-blue direction with a darker neutral palette, deeper-blue
  ordinary actions, and persistent state accents: amber running, violet paused,
  teal Manual, red fault, and cautious stale/unavailable treatments.
- Added a bounded controller-observation service gate with synthetic service
  counter/due/forecast fields. The lab warning dialog can collect only an
  acknowledgement preview; it cannot issue a request, authorize a run, or
  bypass controller maintenance policy.
- Recorded `REQ-FUN-016` and `PROTO-010`, plus ownership, startup, protocol,
  UX, traceability, and open-question updates for controller-owned
  service-period preflight.

### Boundary and non-goals

The richer explorer remains entirely under `simulator/ui_lab/` and is guarded
out of Zephyr. Program names/stages and confirmations are synthetic visual data
or local draft state. The shared model only represents controller observations;
it does not decide service policy. No wire schema, controller behavior, program
persistence, real editing, or device UI integration was added.

### Verification in progress

- Host Debug CTest passed 10/10 and an independent MinGW UI-lab build rendered
  every scenario, every page, and the service dialog in its headless smoke test.
- The target-facing model/string change was built on `qemu_x86`; Twister passed
  3/3 and GCC analysis remained free of repository-owned warnings (the known
  two upstream Zephyr libc warnings remain).
- The canonical MSVC UI-lab artifact requires its pre-existing visible window
  to close before the updated executable can be linked and opened for visual
  review. No process was terminated by this session.

## 2026-09-02 — Host-only semantic UI laboratory and dashboard

### Scope

- Recorded the stakeholder's visual feedback: the fourth navigation item remains
  intentionally undecided, Settings is likely a bottom-navigation destination,
  graphs are planned/measured with distinct color/style, controls are
  click-to-edit, and the furnace visual is an optional 2.5D enhancement.
- Recorded the intended program-estimation rule: a saved ambient temperature is
  an estimation input only; each run starts and updates from current measured
  controller temperature.
- Added a bounded shared dashboard observation model with explicit session,
  revision, provenance, validity, and graph sample fields.
- Added the host-only `furnace_hmi_ui_lab` composition, deterministic semantic
  controller scenarios, a read-only running dashboard, scenario selection by
  command line, and keys 1–7 while the window is open.

### Boundary and non-goals

Simulator sources are below `simulator/` and are enabled only by the
`host-msvc-ui-lab-debug` preset. The UI lab carries a permanent simulation
marker and a compile-time `__ZEPHYR__` guard. It does not implement the wire
protocol, controller safety/control, program persistence, editing, or accepted
HEATING/HOLD/COOL semantics. Synthetic stage and graph data are illustrative.

### Verification

- `host-msvc-ui-lab-debug`: configure, build, and 9/9 CTest tests passed.
- The lab smoke test rendered all seven scenarios at the 640 x 360 structural
  viewport; the normal development viewport remains 800 x 480.
- Host Release, MSVC desktop, and portable-core suites passed (9/9, 9/9, and
  7/7 respectively). Zephyr qemu_x86 built, Twister executed all three cases,
  and the GCC SCA build passed; only two upstream Zephyr libc analyzer warnings
  remain. The deterministic index was regenerated and its consistency check
  passed after both canonical File API inputs were refreshed.

## 2026-08-25 — repository foundation execution

### Scope and result

Executed the approved engineering-foundation pass while excluding furnace
business logic, real protocol semantics, controller commands/state machines,
and production furnace screens. The implementation exists in the local
worktree and has not been committed by this session.

### Repository and documentation

- Normalized the existing Obsidian vault to `docs/`, preserved shared settings,
  excluded user workspace state, and removed the default welcome document.
- Added root navigation/governance, line-ending/editor policy, ignore policy,
  requirements and invariant registers, V1 lessons, nine ADRs, platform and
  testing strategies, traceability, progress records, and generated-file
  policy.
- Added three narrow repository skills and six read-focused reviewer
  configurations. Added a concise policy that reserves broad adversarial review
  for consequential gates.
- Kept unresolved furnace semantics, physical transport details, serialization,
  memory policy, and protocol sharing decisions explicitly open.

### Build, simulator, tests, and tooling

- Pinned Zephyr 4.4.0 to commit
  `684c9e8f32e4373a21098559f748f06915f950c9` and LVGL to the exact revision
  selected by that release.
- Added a portable C17 compile/link probe, foundation-only Zephyr application,
  Ztest suite, host tests, and project-owned LVGL/SDL2 desktop proof.
- Verified a native desktop window and nonzero native handle. The headless smoke
  path pushes real SDL pointer and text events and asserts their delivery into
  LVGL widgets.
- Added project-local VS Code settings, compilation-database configurations,
  build/test/static-analysis tasks, a real Zephyr QEMU debug launch, and an
  MSVC/PDB desktop `cppvsdbg` launch.
- Added an isolated west/bootstrap verifier and documented exact tool versions,
  paths, package hashes, CLI workflows, and the fresh-clone preparation order.
- Added deterministic Indexer V1 using Git, compiler databases, and CMake File
  API metadata; it emits schema-validated JSON plus generated Markdown without
  timestamps or absolute paths.

### Verification evidence

- Zephyr environment check passed: west 1.5.0, exact Zephyr checkout, SDK 1.0.1,
  x86 GCC 14.3.0, GDB 16.2, SDK QEMU 10.0.2, DTC 1.7.2, and `pip check` clean.
- Fresh Host Debug and Release configure/builds passed with warnings treated as
  errors; both CTest presets passed 5/5.
- Portable MSVC Debug configure/build passed; CTest 4/4 passed.
- The complete MSVC LVGL/SDL desktop preset configured and built against the
  checksum-verified official SDL2 VC 2.32.10 package; CTest 5/5 passed.
- Zephyr `qemu_x86` built 135 steps and emitted a C17 compilation database.
- Twister ran one QEMU configuration and both Ztest cases passed.
- Zephyr GCC analyzer build passed. Its two warnings were confined to upstream
  Zephyr libc `malloc.c`; repository-owned sources emitted none.
- SDK GDB attached to the generated QEMU debug server and stopped at the real
  application `main` source breakpoint.
- VS Code `cppvsdbg` stopped at `simulator/desktop/src/main.c:242` in `main`
  and displayed `argc`, `argv`, `display`, `smoke_test`, and `view`. A separate
  Visual Studio native-engine attach reproduced the active line and locals.
- Indexer tests passed 15/15; schema validation, fail-closed required-input
  behavior, deterministic generation, and `--check` passed.
- Bootstrap reproducibility and error-path regression tests passed 5/5.

Exact reproduction commands and the tested-version table live in the
[development-environment record](../05-platform/DEVELOPMENT_ENVIRONMENT.md).

### Review findings addressed

- Architecture review corrected ambiguous HMI authority wording, separated all
  visible proof strings from screen logic, removed an invented ABI implication,
  restored standalone CMake composition, and aligned traceability.
- Embedded review corrected west task working directories, explicit SDK/QEMU/DTC
  environment propagation, Twister execution, debugger configuration, and
  missing environment documentation.
- Indexer adversarial review removed dependence on arbitrary transient build
  trees, made the File API query reproducible, hardened stale/error reply
  handling and schema constraints, and added deterministic fallback coverage.
- Dependency review made the LVGL checkout pin reject tracked, modified, or
  untracked drift, extended the same clean-checkout invariant to the bootstrap
  for both Zephyr and LVGL, and recorded the tested SDL2 floor.
- Final adversarial findings made Indexer V1 fail closed on missing or invalid
  canonical build metadata, classified tool tests correctly, translated
  bootstrap I/O failures into actionable errors, added host-prerequisite
  preflight coverage, and closed the desktop source-debug requirement.
- The final software-critic, error-auditor, and embedded-workflow audits found
  no remaining blocker. Their two low hardening observations were also closed:
  the manual Twister command now clobbers stale output, and bootstrap rejects
  an altered LVGL manifest pin before it mutates the workspace.

### Machine and user-level changes

- Created ignored project-local `.venv/`, `.deps/`, and `build/` content.
- Downloaded and checksum-verified the official SDL2 VC 2.32.10 development
  archive under `.deps/downloads/` and extracted it under `.deps/tools/` for
  the MSVC/PDB desktop preset.
- Installed no ESP-IDF runtime dependency and made no change to global MSYS2.
- Zephyr SDK setup registered the project-local SDK in the current user's CMake
  package registry; this is the only lasting user-level configuration change.
- A redundant temporary standalone QEMU download was uninstalled and removed
  after the SDK-supplied QEMU was verified; it is recoverable by downloading it
  again and no QEMU process remains.

### Known limitations and decisions

- The global MinGW GDB cannot start because its expected Python runtime DLL is
  absent. It is outside the verified workflows: Zephyr uses SDK GDB and the
  desktop proof uses MSVC PDBs with `cppvsdbg`.
- Python 3.13.3 is a verified local deviation from Zephyr's Windows-specific
  recommendation to use 3.12.
- No consequential architecture decision beyond the user-approved baseline was
  introduced, so `decision-challenger` was not invoked to reopen those mandates.
- No furnace-domain or wire-format choice was inferred from a proof or example.

## 2026-08-27 — first UI vertical slice

### Scope and result

- Added a canonical [developer runbook](../05-platform/DEVELOPER_RUNBOOK.md)
  for setup, visible UI, debugging, host/Zephyr tests, analysis, and index
  maintenance.
- Planned and completed the first reusable presentation slice without adding
  furnace-domain values, commands, protocol behavior, or target drivers.
- The desktop executable now opens a read-only state-availability screen by
  default. The original SDL pointer/text proof remains available only through
  its diagnostic mode.

### Authority and lifecycle behavior

- `UNAVAILABLE` is the zero/default state. NULL and invalid inputs also fail
  closed, withholding furnace values and controls.
- Presentation copy distinguishes observed-state availability from transport
  connectivity. All visible text remains in the English string catalog.
- Desktop-only identity is owned by simulator wiring, while the reusable LVGL
  component contains no SDL or Zephyr APIs.
- The view owns a fixed object tree, rejects recreation until destroy, clears
  its stable handle on parent/root deletion, and cannot delete an unrelated
  object after allocator address reuse.
- Direct constructor NULL results clean partial construction. LVGL-internal
  allocation/style OOM remains a documented fatal assertion policy pending
  the production memory ADR and target budgets.

### Verification and review

- Host Debug, Release, and MSVC desktop CTest passed 6/6; portable MSVC passed
  4/4. The screen was visually captured from the exact MSVC build.
- Clean Zephyr `qemu_x86` build passed 135 steps, Twister passed 2/2 QEMU Ztest
  cases, and GCC analysis emitted no repository-owned warning.
- Table-driven presentation checks cover all valid/fallback states,
  non-interactive structure, lifecycle, and the 640 x 360 minimum viewport.
- Architecture, software, and error reviewers identified and drove fixes for
  fail-open initialization, platform leakage, state/link wording, incomplete
  state coverage, viewport overclaim, allocation contract, and ABA lifetime
  risk. Final architecture recheck found no remaining blocker.
- Progress records, traceability, command policy, and the deterministic codebase
  index were refreshed from the final source/build snapshot.

### Deferred decisions

- Product hardware, production viewport, physical transport, protocol/wire
  format, reconnect/session rules, furnace fields, modes, alarms, navigation,
  and control workflows remain open.
- These questions are intentionally answered just in time. The next UI plan
  needs only a first operator workflow, any available V1 evidence, and a choice
  to retain or replace the temporary 800 x 480 development viewport.

## 2026-09-01 — one-command desktop UI workflow

### Scope and result

- Added `tools/run_desktop_ui.py` so a developer can run one command from any
  working directory to configure, build, test, and open the Windows MSVC
  desktop UI.
- The default `run` action always verifies before launch. The `check` action
  performs the same configure/build/CTest sequence without opening a window.
- No stale-artifact-only launch mode is exposed. The wrapper performs no
  downloads and does not claim to run Zephyr, embedded LVGL, hardware, or
  furnace behavior.

### Failure and process behavior

- Commands use argv lists without a shell, run from the discovered repository
  root, stop at the first failure, and preserve the failing child exit code.
- Interactive launch forces SDL's visible Windows driver, uses the executable
  directory for runtime DLL discovery, waits in the foreground, and terminates
  and reaps the owned process on Ctrl+C.
- Missing tools, missing artifacts, access/spawn errors, non-Windows hosts, and
  nonzero UI exits produce concise actionable errors.

### Verification

- Ten focused wrapper tests cover command order, check/run separation, exact
  exit-code propagation, paths with spaces and non-ASCII text, visible SDL
  environment, generic spawn errors, missing output, non-Windows rejection,
  and interruption ownership.
- The wrapper's real `check` action configured and built the MSVC desktop target
  and passed its complete 7/7 CTest preset.
- Host Debug and Release also passed 7/7, portable MSVC passed 5/5, the full
  Python tooling suite passed 30/30, and deterministic index generation/check
  passed after the final documentation refresh.
- Zephyr was not rerun because no target-facing source, target configuration,
  or accepted Zephyr dependency changed; the previously verified 2/2 QEMU
  Ztest evidence remains current for that scope.

## 2026-09-09 - Graphite all-page completion

**Fact:** Preserved the starting dirty tree and stored its status/patch under
`build/graphite-review/`. Completed shared frame, typography, graph/furnace
geometry, every route/dialog, bounded fixture consistency, and owner-context
observation/modal lifetime fixes. Native SDL capture and all-ancestor geometry
checks now accompany real-action smoke tests. Inspected 55 frames at each of
800 x 480, 800 x 640, and 1024 x 600, including final corrected stage identity,
Manual overlay, and overrun copy. UI CTests pass 10/10 at each size; shared host
suites pass 10/10, 10/10, 10/10, and 8/8. Zephyr qemu_x86 build and Twister 3/3 passed; GCC analysis completed with two
known upstream libc warnings and none in repository-owned sources. Python
tooling passed 40/40; canonical index generation/check and diff whitespace
validation passed. Detailed logs
and hardware/product limits are in [the verification record](GRAPHITE_UI_VERIFICATION.md).
No sibling repository, architecture authority, or production protocol changed.

## 2026-09-09 - local program-edit UI follow-up

**Fact:** Refined the host-only UI-lab header, quick launch, program details,
stage grid, and stage editor. A bounded HMI-local edit session persists while
cycling stages and is retained only by local **Save all changes**; **Discard
changes** clears it. No controller, protocol, safety, or persistent-program
behavior changed. Inspected fresh 800 x 480 native captures and passed the
MSVC UI-lab build plus 10/10 CTest preset with zero capture geometry errors.
The star uses an asterisk fallback because this LVGL symbol set has no star;
stage-name keyboard input remains deferred to the next batch.
## 2026-09-09 - stage-card hierarchy refinement

**Fact:** Stage pages now fit up to nine direct-edit cards in a 3 x 3 grid at
800 x 480. Cards retain their name, use a type-coloured outlined stage-number
badge, show type plus duration, and make inherited temperatures explicit.
Fresh native capture produced zero geometry errors and the MSVC UI-lab preset
passed 10/10. This remains a host-only presentation refinement.
## 2026-09-10 - Running-view presentation refinement

**Fact:** Updated the host-only Running Graph, Split, and Furnace views with
straight-separated selectors, full-graph text statistics, improved graph-axis
clearance, and an amber measured path. Fresh 800 x 480 native captures had no
geometry errors; the MSVC UI-lab CTest preset passed 10/10. No controller,
protocol, safety, or persistent behavior changed.

## 2026-09-14 - Settings design proposal

**Fact:** Reviewed current settings source, graphite design baseline, ownership,
requirements, and ADR-0001/0005. Added a settings proposal, standalone clickable
browser concept, and staged implementation plan for operator review. Preference
limits do not override controller configuration. Policy/defaults, clock and
connectivity decisions remain open; no runtime or production behavior changed.
Verification results are recorded in the [plan](SETTINGS_IMPLEMENTATION_PLAN.md).


## 2026-09-14 - Settings Maintenance extension

**Fact:** Operator approved the Settings UI direction and requested reset settings,
delete programs, restart display, and furnace restart assessment/confirmation
for OK, busy, and no-response outcomes. Updated the concept, proposal, plan,
and open questions; drafted ADR-0010. An attempt always remains subject to final
controller validation. No force reset or real deletion/restart is implemented.

**Verification:** Link, JavaScript syntax, generated metadata, and whitespace
checks passed. Corrected mocked-DOM harness passed the 12 restart branch
combinations and scoped maintenance-dialog checks. Browser/native render and
real device operations were not tested.

## 2026-09-14 - Native Settings presentation

**Fact:** Translated `docs/images/theme/hmi-web-demo.html` into the
host-only LVGL explorer. The implementation adds the five-category Settings
rail, Program preferences, Display page 1/page 2, Date & time, Connections
availability states, local temporary preference editing, numeric plus/minus
editors, dirty navigation handling, and independent Maintenance confirmation
previews. All new operator text is catalog-backed.

**Boundary:** The implementation is presentation-only. It sends no controller
requests, does not persist settings, does not change controller configuration,
and does not perform real deletion or restart operations.

**Verification:** Host MSVC build passed; the full CTest preset passed 10/10;
Settings smoke interactions passed; native 800 x 480 captures for the default,
Display page 2, Date & time, Connections, and Maintenance routes reported zero
geometry errors. Browser inspection and physical-panel testing remain pending.


## 2026-09-14 - Device inspection design

**Fact:** Extended the same Settings browser concept with Device navigation,
runtime overview, generic configured component/task lists, notifications,
read-only service history, and device information. Added current, no-attention,
fault, stale, unavailable, and empty-catalogue fixtures. Added the Device
proposal/plan and proposed ADR-0011. All sample service intervals, parts and
records are synthetic, not manufacturer recommendations. Worker controls do
not change counters, intervals, faults or service completion. This was the
browser-design stage; existing native Settings work was preserved. Native
Device implementation is recorded below.
Verification evidence is in [the Device plan](DEVICE_IMPLEMENTATION_PLAN.md).

**Verification:** Actual Chrome rendering/interaction passed 90 Device views
at three sizes plus component details, catalogue extension, pagination,
Settings dirty navigation and 12 restart regressions. Inspected browser captures;
corrected separator encoding and an illustrative interval/baseline mismatch.
A final capture rerun initially connected before Chrome was ready; rerunning
after startup passed. Link, syntax, generated metadata and whitespace checks
passed. Native and hardware verification were deferred to the implementation
stage below.

## 2026-09-14 - Native Device presentation

**Fact:** Translated the Device browser concept into the host-only native LVGL
explorer. The implementation adds Overview, Components, Notifications, Service
history, and Device info categories; generic component/task rows and details;
All/Attention and All/Service/Errors filters; three-row pagination; read-only
history/info dialogs; stale/unavailable states; and a dirty-Settings navigation
guard. All new visible text is string-catalog backed.

**Catalogue decision for this host slice:** Service intervals are not embedded
in the renderer. `simulator/ui_lab/data/device_service_catalogue.txt` is an
editable bounded text catalogue read at startup before `lv_init()`. Developers
can add component/task records or change the interval field; CMake copies the
same source file beside the built UI-lab executable. The sample values remain
synthetic and are not manufacturer policy.

**Boundary:** This is presentation-only. The HMI does not accumulate counters,
evaluate authoritative due policy, persist service history, send controller
commands, or permit interval editing, fault clearing, reset, service
completion, calibration, or actuator tests. ADR-0011 remains Proposed, and
controller catalogue/counter/service-history integration is still pending.

**Verification:** Native MSVC builds and the complete UI-lab CTest preset passed
10/10 at 800 x 480, 800 x 640, and 1024 x 600. Direct smoke passed startup
catalogue loading, interval rendering, all Device categories, filters,
pagination, details, notifications, history, info, stale presentation,
unavailable state, and the dirty-Settings guard. Device and full-suite native
capture geometry reported zero errors. Firmware, manufacturer-source, physical
touchscreen, and controller integration tests were not run.


## 2026-09-14 - Programs presentation refinement

**Fact:** Inspected current native Programs, Program details and Program stages
captures and their fixture/action code. Added the same routes to the shared
browser concept with neutral library summaries, aligned detail facts/actions,
clear inherited-target treatment and consolidated toolbar pagination. Existing
twelve program fixtures and sixty stages were copied from the current source;
editor/Load actions remain explicit preview boundaries. Native code, program
semantics, protocol, and independent Settings/Device changes were preserved.
Evidence and the later native styling scope are in [the plan](PROGRAMS_STYLE_PLAN.md).


**Programs browser verification:** Passed 87 program views across three sizes,
nine-stage grid stress and navigation/action-preview regressions. Device/Settings
regressions passed 90 views and twelve restart branches. Inspected actual
captures, corrected nonuniform graph-label/marker scaling, then reran the
Programs suite and inspected the corrected tall capture. An automatic approval
usage-limit rejection interrupted verification; the user resumed work and the
final rerun passed. No native source or firmware behavior changed.

## 2026-09-15 - Native Programs styling

**Fact:** Translated the approved Programs style proposal into the existing
native LVGL library, detail and stages builders. The pass keeps the two-column
six-card library, graph/facts detail, three-column nine-stage layout, full-card
targets, graph-node/editor routing, Load-before-Start boundary and all fixture
values. It adds neutral aligned library summaries, toolbar page indicators,
aligned detail facts, restrained Delete styling, outlined stage ordinal badges,
and separate inherited-target captions.

**Boundary:** This is host-only presentation work. No controller, protocol,
firmware, persistence, safety or physical-panel behavior changed; stage and
program values remain illustrative local previews.

**Verification:** Native MSVC builds and complete CTest presets passed 10/10 at
800 x 480, 800 x 640 and 1024 x 600. Native capture geometry reported zero
errors at all three sizes. The local image viewer could not open the generated
BMP captures, so automated geometry/workflow evidence passed while manual
native image inspection and physical touchscreen review remain pending.


## 2026-09-15 - Home, Running and interactive stage demo

Added Home/quick launch, Load/Start demo, three Running views, pause/resume/Stop,
external bounded time advancement and local all-stage Save/Discard editing to
the shared HTML. Retained active Program navigation containment, independent
run copies, unknown cooling duration and explicit simulation identity. Browser
checks passed 39 views at three sizes plus field/transaction/navigation cases;
actual captures were inspected. Native source and website deployment unchanged.
See [the demo plan](HOME_RUNNING_DEMO_PLAN.md) for evidence and limitations.


## 2026-09-15 - Touch keyboard layouts

Added full QWERTY/number/symbol entry and a numeric/sign keypad to the shared
HTML. Program/stage names and Settings/stage values use bounded temporary
buffers with Apply/Cancel and field-specific validation. Background UI is inert
while entry is open; active runs cannot open the keyboards. Browser checks
passed 15 layouts at three sizes plus selection, Backspace, validation,
transaction and integration cases; actual captures were inspected. Improved
selection contrast and verified ten-key maximum text rows. Native firmware
and website deployment are unchanged. See [the plan](TOUCH_KEYBOARD_PLAN.md).

## 2026-09-15 - Programs visual and keyboard refinement

**Fact:** Refined the native LVGL and shared HTML presentation to match the
approved concept: both keyboard layouts display Delete; the native
numbers/symbols cycle is closed within those pages; Program detail and Load
titles are larger, left-aligned and vertically centered; the native favourite
control is a drawn five-point star; View stages is opaque with white text; and
graph annotations have additional bottom clearance. Full-graph Running
statistics now use centered background cards without vertical separators.

**Boundary:** This remains host-only presentation work. No controller,
protocol, firmware, persistence, safety or physical-touch behavior changed.
The star, keyboard state and graph values remain local UI-lab behavior.

**Verification:** Native MSVC build, complete CTest suite and capture harness
passed at 800 x 480, 800 x 640 and 1024 x 600. All capture geometry checks
reported zero errors. Native smoke explicitly checked Delete, Shift state,
closed number/symbol cycling and the drawn star. Firmware, embedded LVGL and
physical touchscreen review were not run.

## 2026-09-15 - Running time control outline

**Fact:** Updated the native LVGL and shared HTML Running time control to keep
its elapsed/remaining toggle while removing the trailing swap glyph and
background fill. The control now has a transparent interior with the blue
accent retained as its border; the associated running readings and estimate
caption remain unchanged.

**Boundary:** This is host/browser presentation work only. The elapsed and
remaining values, total calculation, local toggle state and controller
authority boundary are unchanged. No controller, protocol, firmware,
persistence, safety or physical-touch behavior changed.

**Verification:** Native MSVC builds, complete CTest suites and capture
geometry checks passed at 800 x 480, 800 x 640 and 1024 x 600. The standalone
HTML script syntax and targeted running-time style assertions also passed.
Browser interaction/capture, physical touchscreen and embedded LVGL verification
were not rerun for this presentation-only edit.

## 2026-09-18 - Web demo P1 faithful viewport

**Fact:** Implemented the first packet from
[WEB_DEMO_CLIENT_BUILD_PLAN.md](WEB_DEMO_CLIENT_BUILD_PLAN.md) in the
canonical shared HTML. Added the outer toolbar, advanced scenario disclosure,
logical-size viewport fitting, inspect mode, portrait hint, fullscreen fallback,
explicit dialog placement and responsive wrapper constraints. Added the durable
[P1 browser smoke driver](../../tools/hmi_web_demo_p1_smoke.cjs).

**Boundary:** The HMI remains a browser-side remote-interface demonstration.
No controller authority, protocol, firmware, native LVGL or persistent storage
behavior changed. Website adaptation and publication were not touched.

**Verification:** Ran
& 'C:\Program Files\nodejs\node.exe' tools\hmi_web_demo_p1_smoke.cjs.
It passed the three logical sizes, toolbar state, keyboard, dialog bounds,
390 x 800 portrait fit/no horizontal overflow, rotate hint, fullscreen fallback
and zero runtime exceptions. Real phones, other browsers, visual comparison,
website build/deployment and native/firmware checks were not run.

## 2026-09-15 - HMI web demo publishing

**Fact:** Renamed the shared browser source to `hmi-web-demo.html` and added
a repeatable client adaptation to the Telamorph website build. Build and
headless browser interaction checks passed. See the
[web demo record](HMI_WEB_DEMO.md) for workflow, deployment result and limits.

**Deployment:** Attempted; blocked by expired Firebase login. Local preview
is available; live publishing requires `firebase.cmd login --reauth`.

## 2026-09-16 - Running and program control styling

**Fact:** Removed the separate Estimate caption from the native LVGL and
shared web-demo power reading. Running Graph/Split/Furnace selectors now use
transparent surfaces with text and a colored bottom border for the active
mode. Home quick launch is sized to one quarter of the content width, matching
one of the four bottom-navigation buttons. Program detail View stages and
Delete actions are fully filled, and the web-demo stage type and numeric
plus/minus controls now use the native palette.

**Boundary:** This is host/browser presentation work only. It changes no
controller values, action lifecycle, protocol, persistence, safety behavior or
physical-touch implementation.

**Verification:** Native MSVC builds and complete CTest suites passed 10/10 at
800 x 480, 800 x 640 and 1024 x 600; native capture geometry checks reported
zero errors. Standalone HTML syntax and targeted style assertions also passed.
Browser interaction/capture, physical touchscreen and embedded LVGL verification
were not rerun for this presentation-only edit.

## 2026-09-18 - Advanced scenarios and lower-temperature program fixtures

**Decision:** Keep the 200 C / 3.0 C-per-minute values as reversible local
preview/editor fixtures only. They do not redefine controller-owned program
validation, thermal limits, safety behavior, or protocol semantics.

**Fact:** Grouped the HTML advanced scenarios into Actions, Data fixtures, and
Review and recovery. Letter and numeric keyboard previews are explicit
buttons. Started demo programs now advance once per wall-clock second at the
default 60x setting while retaining manual Advance, Pause, Freeze, speed and
Next stage controls. Rescaled all twelve browser programs and the native
LVGL/semantic fixtures, including the graph scale and overrun scenario.

**Verification:** The extended browser smoke passed. The native UI-lab build
and CTest run passed all 10/10 tests, including smoke, geometry, and the
codebase-index check. Website sync, embedded builds, physical touch, and
production controller validation remain not run.
