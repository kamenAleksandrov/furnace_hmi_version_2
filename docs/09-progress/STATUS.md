# Project status

**Fact (2026-09-27, editor rework, 12 px borders, operator text):** In both
presentations the light strip is a 12 px top border, so content sits 12 px from
every edge. The program editor has Back / name / + Add stage / Edit stages in one
row, tappable stage cards, no statistics and no bottom navigation, and asks before
discarding changes. Emulator and concept explanations were removed from HMI text
(web and native catalogue); the native simulation marker remains. Native overlays
now dim the full screen, and native editor Cancel/Save no longer return to the
editor after a stage was opened.

**Verification:** Browser smoke passed with new editor, dirty-prompt and border
checks. Native MSVC 10/10 CTest passed at 800x480, 800x640 and 1024x600 with new
leave-with/without-changes assertions; desktop preset 10/10 and the Zephyr
workflow passed. Captures of both presentations were inspected. Physical touch
was not tested.

**Fact (2026-09-26, touch keyboard layout):** In both presentations the text
keyboard has no empty slots (`-` completes the a-l row; Caps and Delete are
1.5 keys wide around z-m) and its rows fill the panel height. The numeric
keypad is a 460 px centred panel with full-height rows; unavailable sign/decimal
keys are dimmed. Native captures `keyboard-text` and `keyboard-numeric` were added.

**Verification:** Browser smoke passed. Native MSVC builds and 10/10 CTest passed
at 800x480, 800x640 and 1024x600 without warnings; keyboard captures of both
presentations were inspected. Physical touch was not tested.

**Fact (2026-09-26, Program details thirds and Running reading line):** In both
presentations Program details has View stages / Edit program / Load program as
three equal bottom actions, with the graph over the first two and the facts over
the third. Running shows a single reading line in every view; automatic cooling
keeps curing Elapsed and adds Cooling and Est. remaining only. The shared HMI
model gained a fail-closed run phase, cooling elapsed, estimated cooling remaining
and a per-sample planned-cooling flag; the semantic simulator gained a `cooling`
scenario (key 9), and native Running draws the planned cooling as a dashed line.

**Verification:** Browser smoke passed with new one-line curing/cooling checks.
Native MSVC UI-lab builds and 10/10 CTest passed at 800x480, 800x640 and
1024x600 with no compiler warnings, including new cooling-reading and overlay
assertions; the MSVC desktop preset passed 10/10. Captures of both presentations
were inspected. `run_foundation.py zephyr` (qemu_x86 build, 3/3 test cases and
static analysis; only the two known upstream malloc.c warnings) and `tooling`
passed. Firmware, protocol and physical touch were not run.

**Fact (2026-09-26, Program details and cooling preview):** In both
presentations the light strip is 24 pixels high. Program previews show only the
start of automatic cooling as a dashed green segment (at most a quarter of the
curing time), while Running keeps the full estimate. Native Program details
places Duration above its values. View stages is outline-only, and Edit program
is icon-only on details and stages.

**Verification:** Browser smoke passed. Native MSVC builds and 10/10 CTest passed
at 800x640 and 1024x600, and the detail/stages captures of both presentations
were inspected. 800x480 compiled but could not link while the UI-lab window was
open. No firmware, protocol, physical-touch or controller checks were run.

**Fact (2026-09-26, light strip and Manual graph):** The top accent/light
target was reduced from 48 pixels (later to 24, above) in both presentations. The browser Manual graph now renders at its own pixel size
instead of stretching a fixed 420 x 180 view box, and uses the native axis
labels. The native Manual page capture is now `manual-setup`; the scenario
capture named `manual` had been overwriting it.

**Verification:** Browser smoke passed, including a new 1:1 graph-scale check.
Native MSVC builds and all 10 CTest checks passed at 800x640 and 1024x600. The
800x480 link was blocked by an open UI-lab window and was not rerun. No
firmware, protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-26, time/keyboard/light refinement):** The Manual page title is
Manual mode. Both browser and native Running views show elapsed and remaining
together on one line, without the former time toggle. Estimated readings use
Est.; the text keyboard has one digits-plus-letters layout and numeric entry
keeps its separate keypad. The persistent top accent has a 48-pixel, full-width
click target in both presentations.

**Verification:** Browser smoke passed keyboard persistence, time-line overflow
and light-target dimensions. Native build and all 10 CTest checks pass at
800x480, 800x640 and 1024x600, including the light-target bounds, both time
values, unavailable remaining time and compact-stage geometry. No firmware,
protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual action and scale stability):** Manual Start, Stop,
Pause and Resume now preserve the existing native LVGL page and trajectory;
only state text and action visibility change. Both Manual renderers show the
full supplied 0--180 C controller range instead of auto-zooming around the
chosen target.

**Verification:** Browser smoke passed. Native MSVC builds and all 10 configured
CTest checks passed at 800 x 640 and 1024 x 600 after the change. The default 800 x 480
rerun remains pending while its UI-lab executable is open; no firmware,
protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual graph/input stability):** The browser Manual graph
now fills its card and its lower-band live text is larger. Browser simulation
does not redraw the route while a touch keyboard is open, so numeric entry
remains available. Native Manual target/rate adjustments retain the existing
LVGL trajectory and update its labels/points in place rather than rebuilding
the page and replaying the graph's minimum-to-final-size layout.

**Verification:** Browser smoke passed keyboard-persistence and graph-fill
checks. Native build and all 10 CTest checks passed at 800 x 640 and 1024 x
600, including the assertion that a Manual target adjustment retains the same
trajectory object. Default 800 x 480 rerun remains pending until the currently
open UI-lab executable is closed. No firmware, protocol, physical-touch or
furnace-control verification was run.

**Fact (2026-09-24, Manual lower-band alignment):** Both Manual renderers now
keep the prediction graph free of operational text. Live state and Curing
elapsed are at the top of the lower control band, with Start/Pause/Stop on the
left and target/rate adjustment controls on the right. The shared run labels
are Start, Pause, Resume and Stop; operator controls do not carry a `preview`
or demo qualifier.

**Verification:** The browser smoke passed. Native LVGL build plus all 10 CTest
checks passed at 800 x 480, 800 x 640 and 1024 x 600, including geometry
assertions for the new context and control ordering. No firmware, protocol,
physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual layout refinement):** The browser Manual forecast
now receives roughly the upper 70 percent of a vertical split; the lower 30
percent is a compact control band. In both browser and native LVGL source, the
temperature/rate label leads a larger, dark, industrial-yellow-outlined
plus/minus control band anchored above the bottom run actions. Native Manual
also presents `Curing elapsed` from the observed dashboard state; the browser
uses its declared session-scoped simulator for the same presentation.

**Verification:** Browser smoke passed with graph-height, control-size,
label-placement and bottom-action assertions. Native MSVC builds and all 10
CTest checks passed at 800 x 480, 800 x 640 and 1024 x 600, including the
Manual elapsed/control geometry capture path. Whitespace and regenerated-index
checks passed. No firmware, protocol, physical-touch or furnace-control
verification was run.

**Fact (2026-09-24, shared UI parity refinement):** Both the native LVGL
UI-lab and canonical browser demo now remove the page/status bezel from every
page, expanding content below a persistent industrial-yellow top line. The
line is the simulated interior-light request control and glows on the observed
fixture state. Manual now has only a current-target prediction graph, direct
and +/- target/rate controls, and Pause/Stop; its 48-hour browser simulation
ends heating, returns to Manual setup and states that cooling is automatic.
Stage cards and Program details share the revised composition: type/ordinal/
duration/temperature cards, then stage count/max temperature and three Duration
fields. The native lab remains presentation-only; the browser remains a
session-scoped controller simulation.

**Verification:** Native MSVC UI-lab build and configured CTest passed 10/10,
including visual geometry and top-line-light smoke coverage. The strengthened
headless browser smoke passed. No physical touch review, firmware build,
website deployment or real Control CPU/light integration was run.

**Fact (2026-09-23, first operator-workflow implementation slice):** The native
LVGL UI-lab and canonical browser demo now include Manual mode, controller-style
demo request acceptance, 48-hour curing limits, automatic cooling estimates,
program creation/editing, read-only authored stage cards, Device log and
Maintenance routes, and the industrial-yellow accent. The browser demo keeps
session events in volatile memory so it can be demonstrated without a furnace.
Native LVGL remains a host presentation fixture; production controller/protocol
integration, furnace-side light/door behavior, and real maintenance records
remain deferred in [the controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md).

**Verification:** Native MSVC UI-lab build passed; configured CTest passed 10/10
including geometry and semantic smoke; the extended headless browser smoke
passed; and `git diff --check` passed. Physical touch review, website deployment,
firmware builds, and real Control CPU integration were not run.

**Fact (2026-09-23, planning only):** [Workflow plan](OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md)
and [screen concepts](../03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md) cover Manual,
program creation, automatic cooling and simpler navigation. The searchable
[controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md) preserves deferred tasks.
REQ-FUN-005/017 reconciliation is the first milestone. Accepted requirements and
application behavior are unchanged; layouts are not yet visually verified.

**Fact (2026-09-18, browser demo P1):** The canonical
[hmi-web-demo.html](../images/theme/hmi-web-demo.html) now has the shared
outer viewport/toolbar packet: proportional logical-size fitting, inspect mode,
portrait rotate hint, fullscreen fallback, advanced fixture disclosure and
explicit dialog geometry. The durable
[hmi_web_demo_p1_smoke.cjs](../../tools/hmi_web_demo_p1_smoke.cjs) passed in
headless Chrome for 800 x 480, 800 x 640, 1024 x 600 and emulated 390 x 800
portrait. Real phones, other browsers, visual screenshot comparison, website
sync/build/deployment and native/firmware checks were not run. P2-P7 remain
pending. No production authority, protocol or controller behavior changed.

**Fact (2026-09-18, planning only):** A [client web demo agent build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) now specifies ordered single-HTML work packets for faithful HMI presentation, simulated controller responses, authoring, scenarios and phone/fullscreen use. The confirmed scope is session-only programs with premade examples, an ordinary online webpage and English only. No demo, native code or website deployment changed in this planning pass.

## Fact (host and browser presentation)

The [running readings refinement](RUNNING_READINGS_REFINEMENT_PLAN.md) adds a
tappable elapsed/remaining total, filled readings, and a transparent outlined
time control without a trailing swap glyph. Power is presented as one reading
without a secondary Estimate caption. It also supplies live full-graph
readings.
Native builds and 10/10 tests pass at all three review sizes (67 captures each);
Chrome running layout/toggle checks pass at the same sizes. Existing keyboard,
program title, star and graph refinements are preserved. Host/browser only.

Last updated: 2026-09-16

**Fact (planning only):** A [Settings screen proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md),
clickable browser concept, and [implementation plan](SETTINGS_IMPLEMENTATION_PLAN.md)
have received operator approval of the UI direction and a requested Maintenance
extension (reset preferences, delete programs, display restart, and assessed
furnace restart attempts). The updated concept remains simulated;
[ADR-0010](../07-decisions/0010-maintenance-request-workflows.md) is proposed.
User preference limits remain separate from
controller configuration. Warning policy, defaults, cooling scope, clock and
connectivity integration remain proposals/open choices; runtime is unchanged.

**Fact (host-only UI-lab):** The Settings concept is now implemented in native
LVGL for the five category routes, Display subpages, local preference editing,
and simulated Maintenance confirmations. The host MSVC build, full 10/10 CTest
preset, Settings smoke interactions, and 800 x 480 native geometry captures
passed. Production controller, protocol, storage, clock, connectivity, and
hardware behavior remain unimplemented.

**Fact (Device host-only presentation):** The shared browser concept now has a
native LVGL Device implementation covering Overview, Device log, Maintenance,
Service history and Device info. The editable
[`device_service_catalogue.txt`](../../simulator/ui_lab/data/device_service_catalogue.txt)
is read before LVGL startup, validated into bounded records, and rendered by
generic device/task views; service intervals are not hardcoded in the
renderer. Native smoke and geometry checks pass at 800 x 480, 800 x 640, and
1024 x 600. The [Device proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md),
[plan](DEVICE_IMPLEMENTATION_PLAN.md), and [proposed ADR-0011](../07-decisions/0011-component-service-catalogue.md)
still govern the unresolved production contract. Example values are synthetic;
controller-owned counters, service policy, history, persistence, protocol, and
hardware behavior are not implemented.

**Fact (Programs host-only presentation):** Native LVGL Programs, Program
details and Program stages now implement the approved styling pass while
preserving the existing fixture content, route meanings, graph-node/editor
routing, Load boundary and local preview semantics. Library cards use neutral
aligned summaries; paging is in the action toolbar; detail facts are grouped
beside the graph; Delete is a restrained red-outline action; and stage cards
use outlined ordinal badges with a separate inherited-target caption. The
[style proposal](../03-hmi/PROGRAMS_STYLE_PROPOSAL.md) and
[plan](PROGRAMS_STYLE_PLAN.md) govern this presentation-only change.

The native MSVC UI-lab build and full CTest preset pass at 800 x 480,
800 x 640 and 1024 x 600, with zero capture-geometry errors. This remains
host-only HMI presentation evidence: no controller, protocol, firmware,
persistence, physical touchscreen or safety behavior changed.

**Fact (browser demo extension):** The shared HTML now opens Home and supports
quick launch, Load/Start demo, Graph/Split/Furnace Running, pause/resume/Stop,
and local stage editing through cards/graph nodes. All-stage Save/Discard and
run-copy isolation are browser-only. See the [demo plan](HOME_RUNNING_DEMO_PLAN.md).
Native firmware and website deployment are unchanged by this extension.

**Fact (browser and native keyboards):** Full text/numbers/symbols and
numeric/sign touch keyboards now open from program/stage names, Settings
values and editable stage values. Apply/Cancel preserve local transaction
boundaries. The host-only native LVGL lab mirrors the layouts, uses an active
up-arrow Shift key, page-matching `#+=`/`123` More labels, and routes program,
stage and Settings edits into bounded local state. Browser verification passed
15 layouts at three sizes; native smoke and geometry checks pass at the three
native review sizes. The latest refinement displays Delete, keeps the
number/symbol cycle closed, and retains active Shift feedback. See the
[keyboard plan](TOUCH_KEYBOARD_PLAN.md). Embedded keyboard/controller behavior
is not implemented.

## Current gate

**Fact:** The [Graphite UI completion plan](GRAPHITE_UI_COMPLETION_PLAN.md)
has completed implementation and native visual inspection at all three review
sizes. The [verification record](GRAPHITE_UI_VERIFICATION.md) contains the
per-page matrix, actual screenshots, logs, launch commands, and remaining limits.
Final documentation/index verification passed. Operator approval and
physical-panel testing remain separate pending gates.

A local program-edit follow-up is also complete at 800 x 480: it refines the transparent notification header and dotted status pill, empty quick launch, program management controls, direct 3 x 3 stage editing, and bounded local all-stage save/discard preview. The host MSVC UI-lab CTest preset passed 10/10 and its capture run reported zero geometry errors; wider viewport and physical-panel review were not repeated for this follow-up.

A Running-view presentation follow-up is complete at all three review sizes:
unboxed separated view selectors, centered full-graph statistic cards without
vertical separators, greater bottom-axis clearance, slightly inset
temperature labels, and high-contrast amber measured trace. Fresh captures
reported zero geometry errors and each host MSVC UI-lab CTest preset passed
10/10.

The repository foundation and first desktop-visible UI vertical slice are
complete in the local worktree. The current host-only UI-lab follow-up is
implemented and headlessly verified: Home is a furnace overview with local
favourites/recent quick launch, Programs has two full six-card library pages
plus loading/overview/stage/editor routes, and a dedicated Running page has
graph/split/visual modes with ordinary navigation contained. The latest visual
pass applies the graphite instrument-panel visual system across every existing
route: shared charcoal surfaces, restrained deep-blue navigation, grouped
readings, deliberate state accents, stable planned/measured graph colors,
proportioned furnace visuals, and touch-sized controls. It keeps the running
graph's complete predicted domain until live data leaves its headroom and
gives the continuous bottom-navigation control an owner-only active-tab glow.
HEATING, HOLD,
and COOL are presentation/draft assistance only. The lab starts fail-closed in
`UNAVAILABLE`, withholds operational values and controls until current state
exists, and preserves the Control CPU authority boundary. Lab edits and action
buttons are local previews only; furnace business logic, production protocol
semantics, controller state machines, and operational screens remain absent.

## Foundation evidence

| Area | State | Evidence |
| --- | --- | --- |
| Documentation vault and governance | Complete | [Index](../INDEX.md), root `AGENTS.md`, root `PLANS.md` |
| Requirements and architecture | Baseline complete; architecture findings fixed | [Requirements](../00-project/REQUIREMENTS.md), [ADRs](../07-decisions/README.md), [review policy](../06-testing/REVIEW_POLICY.md) |
| Agent skills/reviewers | Three skills and six read-focused reviewer configurations validate | Repository `.agents/` and `.codex/` configuration |
| Zephyr 4.4.0 environment | Exact pin and project-local tools verified | [Development environment](../05-platform/DEVELOPMENT_ENVIRONMENT.md), root `west.yml` |
| Minimal Zephyr proof | `qemu_x86` build and compilation database passed | Foundation application under `app/zephyr/` |
| Desktop LVGL/SDL2 UI | Visible fail-closed availability screen, retained pointer/text diagnostic, and MSVC source-debug path passed | [Simulator strategy](../05-platform/DESKTOP_SIMULATOR_STRATEGY.md), `simulator/desktop/` |
| Desktop operator workflow | One Python command configures, builds, tests, and opens the exact MSVC desktop artifact; check-only mode is available | [Developer runbook](../05-platform/DEVELOPER_RUNBOOK.md), `tools/run_desktop_ui.py` |
| Host-only semantic UI laboratory | Separate MSVC preset and launcher render deterministic disconnected/idle/running/Manual/paused/fault/stale/overrun scenarios; the no-scroll explorer now applies the graphite visual system across Home, Programs, Program editor, Running, Device, Settings, dialogs, and shared navigation; simulator code is guarded from Zephyr | [UI-lab plan](UI_LAB_VERTICAL_SLICE_PLAN.md), `tools/run_ui_lab.py`, `simulator/ui_lab/` |
| First simulated dashboard | Bounded session/revision observation model, provenance/validity fields, planned/measured graph, controller-reported service gate, and host-only presentation | `app/hmi/model/`, `app/hmi/presentation/dashboard/`, `simulator/semantic_controller/` |
| First reusable presentation | `UNAVAILABLE`/`STALE`/`CURRENT` mapping, invalid-input fallback, non-interactive structure, parent lifetime, and minimum viewport verified | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md), `app/hmi/presentation/state_availability/` |
| Host/Zephyr tests | Shared host Debug/Release/MSVC desktop/core: 10/10, 10/10, 10/10, 8/8. All three UI-lab viewport suites: 10/10 each, with 165 native captures and zero geometry errors. | [Current verification record](GRAPHITE_UI_VERIFICATION.md) |
| Static analysis/debug | Zephyr GCC analysis completed; SDK GDB and desktop `cppvsdbg` stopped in project source | [Environment verification record](../05-platform/DEVELOPMENT_ENVIRONMENT.md#verification-record--2026-08-25) |
| Deterministic indexer V1 | Schema, tooling regression cases, fail-closed input validation, generation, and `--check` passed | [Generated policy](../_generated/README.md), [generated index](../_generated/CODEBASE_INDEX.md) |
| Foundation reviewer gate | Complete; architecture, embedded, indexer, software-critic, error-auditor, and workflow findings resolved | Phase 9 in [roadmap](ROADMAP.md) |
| UI slice reviewer gate | Complete; fail-open default, platform copy, state/link wording, allocation contract, lifetime, viewport, and coverage findings resolved | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md) |

## Accepted baseline

- Control CPU authority and HMI/safety boundary: [ADR-0001](../07-decisions/0001-authority-boundary.md)
- Zephyr 4.4.0 pin: [ADR-0002](../07-decisions/0002-zephyr-4-4-0.md)
- Protocol separation principle: [ADR-0003](../07-decisions/0003-protocol-layering.md)
- Project-owned desktop simulator: [ADR-0004](../07-decisions/0004-desktop-simulator.md)
- Single LVGL execution owner: [ADR-0005](../07-decisions/0005-lvgl-single-owner.md)

## Known limitations

- The unrelated global MinGW GDB is broken because its expected Python runtime
  DLL is absent. It is not part of either verified debugger path: Zephyr uses
  SDK GDB, and the desktop proof uses MSVC PDBs with `cppvsdbg`.
- Python 3.13.3 passed the local Zephyr workflow, but upstream recommends Python
  3.12 specifically for Windows; new Windows setups should prefer 3.12.
- The proof uses `qemu_x86`; it provides no evidence for the still-unselected
  production MCU, board, display, touch device, or resource budgets.
- The reusable availability component is currently exercised by the desktop
  target. Zephyr builds the shared foundation/string catalog, but embedded LVGL
  display wiring is intentionally deferred until a display target is selected.
- The host-only UI laboratory is a visual and semantic HMI test environment;
  its synthetic controller cannot validate Control CPU safety, thermal physics,
  or production protocol behavior. Its program edits, service acknowledgement,
  and run controls are request previews, not persistent/controller behavior.
- The current UI-lab graph and stage values are illustrative. Exact
  HEATING/HOLD/COOL execution limits, rounding, and controller acceptance
  behavior remain open.
- The one-command UI runner is intentionally Windows/MSVC-only and does not
  imply a Zephyr build, an embedded graphical target, or visual correctness.
- Manual operator visual sign-off for the graphite pass at 800 x 480, 800 x
  640, and 1024 x 600 remains pending. Author inspection of actual native captures is complete; automated tests
  cover geometry and interaction. Physical touch-target review is not performed.
- Two GCC analyzer warnings remain in upstream Zephyr libc `malloc.c`; no
  analyzer warning was emitted from repository-owned sources.

## Scope guard

No foundation or availability proof should be interpreted as furnace behavior.
Questions are resolved at the decision gate that needs them: early presentation
work may continue with reversible simulated data, while operational controls,
protocol behavior, safety claims, and target integration wait for their
requirements.

See [open questions](OPEN_QUESTIONS.md), [session log](SESSION_LOG.md), and
[traceability](../06-testing/TRACEABILITY.md).

## 2026-09-15 - HMI web demo publishing

**Fact:** Renamed the shared browser source to `hmi-web-demo.html` and added
a repeatable client adaptation to the Telamorph website build. Build and
headless browser interaction checks passed. See the
[web demo record](HMI_WEB_DEMO.md) for workflow, deployment result and limits.

**Deployment:** Attempted; blocked by expired Firebase login. Local preview
is available; live publishing requires `firebase.cmd login --reauth`.
