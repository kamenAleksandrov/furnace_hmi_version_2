# Session log

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
