# Client web demo: agent build plan

Date: 2026-09-18. Status: P1 faithful viewport implemented and locally verified;
P2-P7 remain not started.

## Read this first

User mandate: extend the existing standalone HTML into a client/expo demo.
The HMI must remain visually and interactively 1:1 with the current HMI,
except that a browser simulator supplies the furnace CPU's responses and
permissions. Make programs and stages genuinely editable, savable and
deletable; run programs; expose emulator scenarios and faults; support phones,
PCs, fullscreen and physical device rotation. No real furnace is connected.

This file is the implementation handoff for Luna/Terra. Work in milestone order,
one milestone per task. Do not rebuild the application, introduce a framework,
or split the application into modules merely for style. Keep one HTML containing
CSS, JavaScript and SVG. Organize that file with named sections and small functions.
Separate test files are appropriate and do not change the single-file deliverable.

Implementation is authorized packet-by-packet. The current implementation task
completed P1 only; this document still does not authorize publishing a website.

## Facts, decisions and open choices

### Confirmed user decisions

- Preserve the actual HMI layout, colors, routes, controls and touch keyboards.
  There is no separate redesigned mobile HMI or replacement mobile editor.
- Simulate controller responses; remove placeholder-only behavior and dependence
  on actual hardware permissions. Normal demonstration actions succeed after
  basic valid input; rejection/failure is selected deliberately as a scenario.
- Add professional, simple client controls outside the HMI rectangle.
- Preserve proportions when fitting a phone or desktop viewport.
- Produce a plan and ordered instructions before implementation.

### Verified facts from source inspection

- Canonical source: [hmi-web-demo.html](../images/theme/hmi-web-demo.html),
  86,691 bytes at inspection. This is the source owner, not just a disposable
  test copy, under the existing [publishing record](HMI_WEB_DEMO.md).
- Website copy: `C:/dev/projects/Telamorph_website/hmi-web-demo.html`, 85,767
  bytes at inspection, dated earlier than the HMI copy. Do not overwrite the
  newer source with that older copy.
- Website `scripts/sync_hmi_demo.py` adapts the canonical source. Its current
  regex expects the existing engineering-footer paragraph and inserts wording
  saying changes reset on reload. The transformation needs attention when wrapper
  changes land; session-only reset wording remains appropriate. It also adds noindex metadata and the Telamorph title.
- Website `scripts/build_site.py` includes the demo as a standalone page in
  `dist/`; no website framework is needed.
- Existing logical sizes: 800 x 480 (default), 800 x 640, 1024 x 600.
- Existing UI includes Home, Programs/detail/stages, Load, Running with
  Graph/Split/Furnace, Settings, Device, confirmations, text/numeric keyboards.
- Stage editing already supports Heating/Hold/Cooling, inherited Hold target,
  rate/duration calculations, Save all/Discard, and a copied active program.
- New program, Add stage, program deletion and maintenance deletion are still
  preview messages. Rename/favourites work in memory. Reload resets changes.
- Runs advance through an external button, not a clock. Fastest cooling gives
  an unknown duration and currently prevents Start.
- Program IDs currently equal array positions. `selectedProgram`, favourites,
  recent items and edit sessions depend on this; deletion requires stable IDs.
- `render()` closes the touch keyboard and rebuilds content. It cannot be called
  on every timer tick without losing interaction state.
- `#message` is a body-level dialog outside `.screen`; fullscreening only
  `.screen` would exclude this element from the fullscreen subtree.

These are source-inspection facts, not a new visual or browser verification.
Use current native source/captures to resolve visual differences, not invented
design changes. Existing records report native/browser evidence but were not
rerun during planning.

### Confirmed delivery decisions (user follow-up)

- Ordinary online webpage opened by URL. No installation, download flow,
  offline reopening, service worker or PWA is requested.
- English only.
- Saved programs are session-only demonstration data. Keep the existing premade
  examples. Save commits to the current page's in-memory library; reload/new page
  restores the examples. No localStorage, sessionStorage, IndexedDB, cookies,
  backend, sharing, JSON import or export is required.
- Session interpretation: preserve state while the loaded page remains alive,
  including rotation, fullscreen and temporary tab switching. A page reload or
  explicit Reset demo session starts fresh. Browser back/forward restoration
  of the same page may preserve it; do not promise reliable close detection.
  If the phone browser discards the page, its next load starts fresh.
- All previously asked scope questions are answered. No further decision is
  required before the planned implementation can begin in a future task.

## Governing evidence and scope boundary

Read root [AGENTS.md](../../AGENTS.md), [PLANS.md](../../PLANS.md),
[index](../INDEX.md), [status](STATUS.md) and the
[documentation skill](../../.agents/skills/documentation-maintenance/SKILL.md).
Preserve existing unrelated work; this worktree contains substantial prior edits.

Production requirements remain unchanged:

- [Requirements](../00-project/REQUIREMENTS.md): REQ-FUN-002/003/004/006/007/009/
  010/013/016/017; REQ-NFR-001/002/006/007; ARCH-INV-001/003/004/007/009/010.
- [ADR-0001](../07-decisions/0001-authority-boundary.md): controller authority.
- [ADR-0004](../07-decisions/0004-desktop-simulator.md) and
  [ADR-0005](../07-decisions/0005-lvgl-single-owner.md): native simulation and
  LVGL ownership remain untouched.
- [Program/startup model](../03-hmi/PROGRAM_AND_STARTUP_MODEL.md),
  [Home/Running plan](HOME_RUNNING_DEMO_PLAN.md),
  [keyboard plan](TOUCH_KEYBOARD_PLAN.md),
  [running refinement](RUNNING_READINGS_REFINEMENT_PLAN.md).
- [Open questions](OPEN_QUESTIONS.md): actual thermal behavior, fault recovery,
  Manual expiry and controller acceptance semantics remain unresolved.

The browser supplies synthetic values and request outcomes, not production
protocol or control logic. Use a small browser-local simulator, not a UART,
wire codec, network server, authentication system, PID or physical kiln model.
No production ADR change is required for this isolated demo extension. If a
later task changes production behavior, follow its separate ADR/review process.

Preserve the normal active Program screen's navigation containment for fidelity.
An external demo control may allow library exploration during a run and return
to Running; this is explicitly a demo affordance. Library changes must never
mutate the copied active run. Manual remains its own mode, not a fake program.

## Presentation and phone contract

1. Keep the logical HMI rectangle intact. Default to 800 x 480; retain the other
   two existing sizes under advanced demo controls for regression review.
2. Add a viewport wrapper and a small outer toolbar: Fullscreen/Exit, Scenarios,
   Speed, Freeze simulation, Next stage, Return to run, Reset demo session.
   Hide advanced fixtures behind an expandable panel. Keep a concise simulation
   label. Use the existing palette and typography; no new visual theme.
3. Fit with `scale = min(availableWidth / logicalWidth,
   availableHeight / logicalHeight)`. Measure actual usable viewport dimensions,
   safe-area insets and wrapper controls; preserve the aspect ratio. The outer
   wrapper must occupy the scaled dimensions, not the original oversized box.
   Never crop, stretch, CSS-rotate the entire page, or require horizontal page scroll.
4. Portrait shows the whole proportional HMI with an unobtrusive rotate hint;
   landscape is the preferred faithful interactive experience. Proportional fit
   cannot retain 48 CSS-pixel touch targets on a narrow portrait screen. Offer
   an explicit zoom/pan inspection mode and preserve browser pinch zoom; returning
   to Fit restores the whole screen. Do not claim equally large targets in all
   orientations or invent a second editor. Validate this compromise on real phones.
5. Fullscreen the common container containing HMI, dialog layer and minimal
   demo controls. Handle native dialog/top-layer geometry explicitly: do not
   assume a modal scales correctly merely because its parent uses a transform.
   Test confirmations and keyboards at every scale/fullscreen state.
6. Fullscreen starts from a user gesture. Detect support and catch rejection.
   An expanded in-page fallback remains fully usable. Follow physical rotation
   and resize events without rebuilding application state. Landscape lock, if
   offered, is best-effort and reversible; never assume it works on all phones.
7. Keep HMI touch keyboards, Apply/Cancel, Shift and symbols. Avoid accidental
   duplicate OS keyboards on touch devices while retaining physical-keyboard
   entry on PCs. Resize must not lose the typed value, selection or draft.
8. Keep all controls reachable around notches/browser bars. External touch
   controls should be at least 48 CSS px. HMI geometry remains faithful; small
   targets caused by scaling must be assessed honestly, not hidden in reports.

Platform references: [Fullscreen standard](https://fullscreen.spec.whatwg.org/),
[Screen Orientation](https://www.w3.org/TR/screen-orientation/).

## Small internal design, inside the same HTML

Use clearly labelled sections: constants/fixtures, library, drafts, simulator,
scenario definitions, rendering, keyboards/dialogs, viewport, bootstrap.
Keep existing helpers and renderers where possible. Do not undertake a wholesale
formatting/refactoring pass before delivering behavior.

Recommended minimum state boundaries:

| State | Contents and rules |
| --- | --- |
| Library | Stable program and stage IDs; revisions; saved recipe data; derived summaries |
| Draft | Detached copy, base program ID/revision, selected stage ID, dirty status |
| Run | Immutable accepted recipe copy, mode/status, elapsed virtual time, trace, active stage |
| Simulated controller | Session generation, readings/validity, service/fault state, request outcomes |
| UI | Route, selection, pagination, modal, keyboard, run view; never the source of furnace observations |
| Demo controls | Scenario, clock multiplier/freeze, viewport preference; outside faithful HMI |
| Session defaults | Immutable seed fixtures; reset creates fresh library/preferences/favourites/recents and invalidates pending callbacks |

Use stable IDs for lookup; indexes are only presentation positions. Seed existing
fixtures once, preserve them as defaults, and allocate unique IDs for new copies.
All graph summaries derive from recipe data. Handle zero programs/stages without
invalid array access, `Math.max(...[])`, NaN, or infinite graph coordinates.

Use one small request entry point for save/delete/start/pause/resume/stop and
controller-owned Settings/Maintenance actions. Default: immediate pending feedback,
then a short deterministic simulated response. Optional scenario outcomes:
accepted, rejected, unknown, unsent. These are browser examples, not a protocol.
Each response carries a request token and simulator generation. Reset/scenario
replacement invalidates old callbacks. Repeated taps do not duplicate effects.
Reconnect reconstructs current simulated state and never replays commands.

Save means accepted by the simulated controller into this page session. Keep a
concise outer explanation that edits reset on reload; do not claim durable
storage. Mutating user actions use explicit Save/Cancel semantics.

## Functional completion contract

### Programs and stages

- Complete New, rename, duplicate, favourite, save, delete and empty-library flows.
- Complete stage add, rename, kind change, duplicate, delete and move up/down.
  Preserve existing grid/editor style and tap navigation; dragging is optional.
- Save all commits a detached draft atomically; Discard restores saved data.
  Reorder/delete recalculates inherited Hold targets and adjacent rate/duration
  assistance. Report invalid Heating/Cooling directions rather than silently
  changing recipe intent. Never erase a dirty draft on scenario/route changes.
- Handle deleting the final program, final stage, current page's final item,
  favourites/recent entries and a recipe whose copy is already running.
- Retain meaningful syntax and finite-number checks. Bounds are explicit demo
  capabilities, not production limits. User input is rendered as text, not HTML.
- Fastest cooling must become runnable: the simulator supplies a deterministic
  estimated cooling duration in the accepted run copy. Preserve the draft's
  fastest mode; never convert unknown duration to zero or claim physical accuracy.
- Preserve initial Hold/ambient assistance and existing increments. Freeze a
  single documented demo calculation policy shared by estimate and execution.

### Runs, Manual, Settings and Device

- Start progresses automatically through Heating/Hold/Cooling to completion.
  Retain Graph/Split/Furnace and elapsed/remaining toggle. Add external 1x/10x/
  60x/300x speeds and Next stage. The implemented default is 60x: one
  wall-clock second advances one virtual program minute; the one-minute manual
  Advance control remains available alongside Pause and Freeze.
- The current reversible saved-program fixtures use 0-200 C targets and
  heating/cooling rates no greater than 3.0 C/min. These values guide the
  browser and host-only LVGL preview only; controller validation remains
  authoritative and is not changed by the demo.
- Use one monotonic virtual clock. Freeze demo time while the page is hidden;
  reset the wall-clock baseline on return. No large surprise catch-up.
- Separate Freeze simulation from the faithful Pause request. Document synthetic
  paused behavior; do not present it as actual cooling/control physics.
- Tick only model/readings/trace elements that changed; avoid whole `render()`
  calls while a keyboard/dialog is open. Bound trace samples (for example 512),
  decimate consistently, and keep Stop responsive. No unbounded history per tick.
- Graph starts with the full accepted predicted domain plus headroom, expands
  for overrun, and never draws missing sensor observations as measured data.
- Manual uses its own mode and the native lab's presentation, with simulated
  Start/target/rate/Pause/Resume/Stop where those controls exist. Do not silently
  invent a redesigned screen. Document any absent native interaction before adding it.
- Replace reachable Settings/Maintenance request previews with coherent simulated
  outcomes: preferences, deletion scope, display restart, controller restart
  assessment/results and recovery. Preserve confirmations and visual layout.
- Display restart preserves the simulated controller/run and reconstructs the
  UI. Controller restart is a named synthetic scenario with an explicit resulting
  state, not a production recovery policy. Reset preferences, delete programs,
  reset session and restore sample library remain separate operations.
- Keep Device observations, service counters, fault notices and service
  acknowledgements consistent with simulator state. Do not invent manufacturer
  maintenance procedures. Keep intentionally read-only information read-only.

### Scenario matrix (verified mapping)

Sources: [keyboard mapping](../../simulator/ui_lab/src/main.c) and
[scenario fixtures](../../simulator/semantic_controller/src/simulated_controller.c).
Read `fill_current_common` and `fill_graph` as well as the scenario switch when
porting values; use the native fixtures as evidence, not only this summary.

| Key | Name | Required presentation |
| --- | --- | --- |
| 1 | disconnected | Unavailable observations; no invented current readings |
| 2 | idle | 24 C, door closed, fan off; existing service acknowledgement example |
| 3 | running-normal | Native running fixture and planned/measured graph |
| 4 | manual | Manual mode/session, no synthetic program/stage identity |
| 5 | paused | Paused state, zero fixture demand/power and fan off |
| 6 | fault | Existing safety-stop/door-open fixture, zero demand/power, fan off |
| 7 | stale | Last-known observations visibly stale; no fresh fabricated trace |
| 8 | running-overrun | 70 min elapsed, zero remaining, 190 C reading, final fixture graph sample 195 C; expanded graph domain |

Provide named buttons on phones and 1-8 shortcuts on PCs. Ignore shortcuts while
typing or using a keyboard/modal. Baseline presets set complete coherent state.
Separate them from fault injection into an existing run. Prompt before replacing
a dirty edit or a run; preserve saved programs. Restore normal/reset session is
always accessible through outer demo controls, including disconnected scenarios.

Additional requested fault variety: sensor unavailable, overtemperature, and
link interruption/reconnect. Label these as synthetic extensions, with fixture
tables specifying cause, readings/validity, demand, run state, notice and recovery.
Acknowledging a fault must not magically repair its cause. Provide clear-cause
and reset/recover actions outside the HMI; automatic resume is not assumed.
Use a short event list within the existing notification presentation, not a new
dashboard. Unresolved production alarm semantics stay unresolved.

### Session lifetime and expo reset

Use ordinary JavaScript memory only. Fresh page loads clone the premade library
and start in Idle. Saved edits remain available throughout the loaded session;
Save/Discard retains its normal meaning inside that session. Never silently
resume a run after a page reload.

Keep two clearly named outer controls: Reset run/scenario preserves the current
session library; Reset demo session restores all examples/preferences and clears
runs, drafts, faults, recents and pending requests. Confirm full session reset
if the user has edits. Restore sample programs can be a separate optional action
that preserves session-created programs, but is not required for launch.

Do not reset on visibility loss or physical rotation. Cancel/invalidate timers,
request callbacks and pending keyboard commits on explicit reset. Independent
tabs are independent demos. No multi-tab synchronization or storage migration.

## Ordered work packets

Each packet updates the progress table below and records actual checks. Do not
start a later dependent packet with a broken earlier one. Any agent editing this
worktree is not alone: preserve other agents' and the user's existing changes.

| Packet | Owned files / responsibility | Completion gate |
| --- | --- | --- |
| P0: baseline | This plan; read-only HTML/native/capture inspection | Capture current UI and catalogue all reachable preview/no-op actions; apply the confirmed user choices and record exact local test tools; resolve visual discrepancies against native evidence |
| P1: faithful viewport | Canonical HTML CSS/wrapper/dialog/viewport sections | All three logical sizes unchanged at scale 1; fit/rotation/fullscreen fallback works; keyboards and dialogs survive resize; no clipped critical controls |
| P2: stable library and drafts | HTML fixtures/library/editor sections | Stable IDs; New/Add/Delete/Duplicate/Reorder work; Save/Discard and empty states verified; existing run copy preserved |
| P3: request simulator and clock | HTML simulator/run/render sections | Pending/outcomes, automatic run, Manual fixture, fastest cooling, timing, completion and reset work; no keyboard loss on ticks |
| P4: scenarios and remaining actions | HTML scenarios/Settings/Device sections | Exact keys 1-8 parity; added fault cases; coherent readings/notifications; service/restart outcomes; recoverable from every scenario |
| P5: session/reset and expo usability | HTML bootstrap/defaults/reset and outer controls | Fresh load restores premade examples; in-session Save works; run reset preserves library; full reset clears everything safely; tab switching and rotation preserve session |
| P6: website adaptation and polish | Website sync script and generated copy; build script only if required | Sync is repeatable, copy reflects latest source, footer wording truthful, no engineering links in client copy, local standalone build works |
| P7: release verification | Browser tests and evidence; documentation | Matrix below passes or precise limitations are recorded; actual screenshots inspected and real phones checked; no deployment in this packet |

P1 and P2 are intentionally separate: avoid modifying layout, identity and run
logic all in one patch. P6 must update the sync transformation in the same work
packet as any client-wrapper assumption changes; never hand-maintain two demos.
Offline support, additional languages, persistent/shared storage and file
import/export are excluded by the confirmed delivery scope.

Recommended agent prompt:

> Read WEB_DEMO_CLIENT_BUILD_PLAN.md, repository instructions and current progress.
> Implement only packet Pn, preserving existing unrelated edits. Keep the HMI
> faithful and the application in one HTML. Use the stated source owner and
> interfaces. Verify that packet's acceptance cases and relevant regressions.
> Record files changed, actual commands/results, deviations and remaining work
> in the plan. Do not deploy or change production firmware/protocol.

## Verification and delivery

Add a small durable browser regression driver using existing available browser
tooling; do not rely solely on scratch `build/` scripts. Use fake time for clock
tests. Test behavior through user actions, with focused pure-function checks for
calculations/identity/reset behavior. No new framework merely for testing.

| Area | Required evidence |
| --- | --- |
| Visual parity | Screenshots of Home, program list/detail/stages/editor, Load, all Running views, Device, Settings, dialogs and keyboards at 800 x 480, 800 x 640, 1024 x 600 compared with current reference captures |
| Phones | Portrait and landscape at representative 360/390/430 px widths; actual iPhone Safari and Android Chrome; rotate with keyboard/modal open; notches, browser bars, fullscreen rejection/exit, zoom and touch |
| Desktop | Chrome/Edge plus Firefox and Safari where available; mouse, keyboard focus, Escape/Tab, sizing and fullscreen |
| Authoring | New/save/reopen/rename/duplicate/delete; last item/page; stable favourites/recents; add/delete/reorder Hold/Heating/Cooling; invalid inputs and discarded edits |
| Execution | All stage kinds, fastest cooling, speed changes, pause/resume/stop, next stage, completion, hidden-tab freeze, immutable run while editing/deleting library |
| Request/recovery | Double taps, pending reset, stale callback after new scenario, rejection/unknown/unsent, no reconnect replay, restart reconstruction |
| Scenarios | Exact 1-8 fixtures plus extra faults; all views coherent; graph overrun/gaps; acknowledgement versus cause clearing; recovery without losing library |
| Session | Save/reopen during visit; reload restores examples; run reset versus full reset; late callback after reset; background/rotation preserve state; independent tabs |
| Client build | Repeated sync produces identical output; local HTTP route works; no runtime exceptions/missing resources, no external runtime dependency; noindex retained |

Known existing commands (run only during implementation where appropriate):

```powershell
# In C:/dev/projects/Telamorph_website, after reading its local instructions:
python scripts/sync_hmi_demo.py C:/dev/Furnance_project/furnace_hmi_version_2/docs/images/theme/hmi-web-demo.html
python scripts/build_site.py
python -m http.server 8000 --directory dist
```

Extract and syntax-check inline JavaScript with the available Node installation.
Inspect actual captures; geometry tests alone do not establish visual fidelity.
Browser viewport emulation does not substitute for real-phone fullscreen/touch
testing. If devices are unavailable, record those checks as NOT RUN and leave
the client-readiness claim pending. HTML-only work needs no Zephyr/native rebuild.

Follow [review policy](../06-testing/REVIEW_POLICY.md) for significant completed
implementation. Update this plan, [publishing record](HMI_WEB_DEMO.md), status,
roadmap and session log; regenerate indexes through their owner and check links.
Keep evidence claims limited to checks actually observed. Publishing remains a
separate task using the website's established workflow.

## Rollback

Keep canonical-source and sync-script changes paired. Restore the previous HTML
and sync transformation together to roll back; do not overwrite unrelated local
changes. There is no persistent user schema, service-worker cache or data
migration. Reloading the restored page recreates its premade session fixtures.

## Progress and evidence

| Item | State |
| --- | --- |
| Planning source inspection | Complete: HTML copies, source ownership/sync/build, HMI records, emulator mapping inspected |
| User fidelity requirement | Confirmed: 1:1 HMI with simulated controller responses |
| Delivery choices | Confirmed: session-only premade library; ordinary online page; English only |
| P0 baseline | Complete: source/copy/route/capture inspection and reachable preview-action inventory recorded in this plan |
| P1 faithful viewport | Implemented in the canonical HTML; durable local Chrome smoke passed for controls, sizes, fit/inspect, keyboard, dialog, mobile fit and fullscreen fallback |
| P2-P7 implementation | Not started |
| Browser/phone verification for this extension | Chrome headless/CDP smoke passed; real phones, other browsers and visual screenshot comparison are NOT RUN |
| Deployment | Not requested or attempted |

### P1 implementation evidence (2026-09-18)

Fact: [hmi-web-demo.html](../images/theme/hmi-web-demo.html) now has a common
outer demo toolbar, expandable advanced fixtures, a shared viewport wrapper,
logical 800 x 480 / 800 x 640 / 1024 x 600 selection, proportional fit,
inspect mode, portrait rotate hint, fullscreen fallback and explicit native
dialog positioning. The HMI remains the inner logical rectangle; no production
protocol, firmware or controller behavior changed.

Fact: the durable
[P1 browser smoke driver](../../tools/hmi_web_demo_p1_smoke.cjs) starts a local
HTTP server and isolated headless Chrome session and verifies the P1 interaction
matrix through user-facing DOM actions.

Observed command and result:

    & 'C:\Program Files\nodejs\node.exe' tools\hmi_web_demo_p1_smoke.cjs
    PASS: P1 HTML demo viewport, controls, keyboard, dialog, mobile fit,
    fullscreen fallback and exception checks.

The check used a 390 x 800 emulated portrait viewport and desktop emulation for
the three logical sizes. It did not establish real-device touch, browser-bar,
notch, Safari/Firefox/Edge or visual screenshot parity. Website synchronization,
site build, deployment, native builds and firmware checks were not run.

Planning validation results are recorded in the session log. Future agents append
packet results here, including failed/not-run checks and deviations.
