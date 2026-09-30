# Host-only UI laboratory vertical slice

- Status: Host-only graphite software and native inspection complete; operator approval and production recovery slice pending
- Started: 2026-09-02
- Owner: HMI software

## Outcome and non-goals

Provide a clearly host-only environment in which the reusable HMI can display
deterministic synthetic controller observations. The current pass makes Home a
graphite instrument-panel furnace overview with quick launch, applies the same
visual system to Program loading/library/overview/stage authoring, and gives
active programs a dedicated Running page with planned/measured graph and
visual modes. Device and Settings remain existing placeholders while their
workflow is deliberately deferred.

This slice does not implement the production wire protocol, controller control
loops, safety behavior, program persistence, controller validation, or embedded
LVGL display wiring. Lab buttons and editable fields are request/draft previews;
synthetic behavior is not evidence of furnace performance.

## Governing records

- `ARCH-INV-001`, `ARCH-INV-003`, `ARCH-INV-004`, `ARCH-INV-005`, and
  `ARCH-INV-007`: controller authority, snapshot recovery, no replay, and one
  LVGL owner.
- `REQ-FUN-001`, `REQ-FUN-002`, `REQ-FUN-009`, `REQ-FUN-011`, `REQ-FUN-013`,
  `REQ-FUN-016`, and
  `REQ-NFR-005` through `REQ-NFR-007`.
- `PROTO-002`, `PROTO-005`, `PROTO-006`, and `PROTO-007`.
- [ADR-0001](../07-decisions/0001-authority-boundary.md),
  [ADR-0004](../07-decisions/0004-desktop-simulator.md),
  [ADR-0005](../07-decisions/0005-lvgl-single-owner.md), and deferred
  [ADR-0008](../07-decisions/0008-memory-policy.md).

## Facts, assumptions, and open decisions

**Facts**

- The host and Zephyr targets are separate CMake compositions.
- The existing desktop path is the approved LVGL/SDL2 host baseline.
- The current model uses bounded arrays and explicit value provenance/validity.

**Working assumptions for this reversible visual slice**

- The default development viewport remains 800 x 480, with the existing 640 x
  360 structural floor; deliberate review variants are 800 x 640 and 1024 x
  600.
- The simulator's ambient temperature field is a draft-only estimation input
  captured when a program is saved. A run starts from the current measured
  controller temperature and does not reuse the saved ambient value as a live
  sensor reading.
- The first synthetic program uses illustrative HEATING/HOLD/COOL values only;
  no production stage calculator or controller execution semantics are being
  accepted.
- The current reversible fixture set is intentionally bounded to 0-200 C
  targets and no more than 30 tenths of a degree C per minute for heating or
  cooling. These bounds are preview/editor guidance, not controller safety
  limits or a protocol contract.
- A controller-reported service condition and acknowledgement preview are
  visual-lab data only. The lab neither evaluates service policy nor emits a
  controller request.

**Still open**

- Exact stage limits, the HEATING duration calculation inputs/rounding, and
  the controller's execution semantics.
- Program and Run Session schemas, bounds, retention, permissions, and
  controller persistence.
- The production display/MCU and memory policy.

## Boundary and build isolation

The host-only `furnace_hmi_ui_lab` target links the shared model and
presentation with `simulator/semantic_controller`. The Zephyr application does
not list simulator sources, scenario fixtures, or the UI-lab entry point. The
lab contains a compile-time `__ZEPHYR__` guard and displays a permanent
simulation marker. A future build/link check must continue to prove this
separation.

## Current workflow pass (2026-09-02)

The following ordered plan supersedes the older dashboard-first presentation
description above for this active host-only pass. It deliberately stops before
Device and Settings implementation.

1. Record the clarified one-time controller-owned service acknowledgement,
   ProgramDraft boundary, stage authoring assistance, and graph rules in their
   canonical requirements/design records.
2. Keep the semantic controller fixture host-only and deterministic. Extend
   presentation fixtures only enough to demonstrate bounded favourites/recent
   programs, program summaries/stages, and observed running data; no fixture
   validates or persists a furnace program.
3. Replace Home's run dashboard with a 2.5D observed furnace overview and
   quick launch. Selecting an item opens a Program-loading preview; `Start`
   remains a controller-request preview.
4. Add a dedicated Program-running page with fixed top status, bottom run
   request controls, and a three-mode graph/visual area (graph, split, visual).
   During a started program the laboratory hides ordinary bottom navigation;
   scenario changes remain available only as host-lab input.
5. Rework Programs into bounded two-column cards with tap paging. Provide a
   program overview (facts, graph, edit/delete, stages route), a stage list,
   and a type-aware local StageDraft editor. HEATING/HOLD/COOL fields are visual
   capability/relationship assistance only until a controller contract exists.
6. Use a reusable rectangular trajectory presentation: axis labels and grid,
   muted solid planned path, contrast solid measured path above it, gaps for
   invalid values, and a full-plan domain retained until a live measurement
   leaves its headroom. In-graph stage
   annotations are recorded as a later capability, not faked.
7. Add host smoke/interaction coverage for every route, scenario, graph mode,
   local draft field applicability, and service dialog entry. Then run host,
   target-shared, documentation, and index verification and record actual
   outputs.

### Current constraints and non-goals

- A service acknowledgement is issued and recorded by the Control CPU once per
  configured period. The UI may only render the current event and submit later
  intent; Device/maintenance visibility is recorded but not implemented here.
- Local range checks are assistive. They cannot determine that a program is
  runnable, particularly where HEATING target and either rate/duration drive a
  calculated counterpart
  depend on controller-defined inputs.
- The program's saved ambient temperature is estimation-only; live runs begin
  from controller-measured temperature.
- Graph labels and stage annotations require controller-defined accepted stage
  timing; the UI lab uses illustrated data and never presents it as a
  controller calculation.

## Graphite instrument-panel pass (2026-09-08)

**Facts**

- The host-only explorer uses the review palette: charcoal `#171B1E` background,
  graphite `#242A2E` panels, slate `#30383D` secondary controls, soft-white
  `#F1F3F2` main text, silver `#BBC3C7` supporting text, and deep-blue
  `#285D7A` selected navigation.
- Shared panel, spacing, corner, typography, and button helpers now cover every
  existing route: Home, Programs, Program loading/details/stages/editor, Running,
  Device, Settings, and dialogs.
- Home and Running group current temperature, target, and remaining time in a
  clear reading band; current temperature has the largest type. Estimated power
  and elapsed time have lower visual weight.
- Running uses stable muted planned and bright measured graph traces. The
  measured trace stays bright when the controller presentation changes from
  running to paused; the state accent remains in the status strip, badge, and
  controls.
- The furnace visual is width-aware, places fan/door/heater-demand facts around
  it, and keeps the bottom navigation continuous. Delete is in Program
  management, separated from the primary Start action.
- Generic actions use a 48 px minimum height and frequent/run actions use 56 px
  where space permits. The 800 x 480 default plus 800 x 640 and 1024 x 600
  presets make viewport assumptions explicit.

**Decision**

The graphite visual system is the accepted reversible presentation direction for
the host-only lab. It does not add controller authority, production protocol
semantics, persistence, safety logic, or embedded LVGL display integration.

**Remaining gate**

Manual operator review is still required at each supported viewport, including
physical touch-target measurement on the selected display. Headless smoke tests
prove construction and compact-layout structure, not final visual quality.

## Scenarios and inputs

The first fixture set is `disconnected`, `idle`, `running-normal`, `manual`,
`paused`, `fault`, `stale`, and `running-overrun`. The executable accepts a
named scenario and the interactive host window maps keys 1–8 to those
scenarios. The final case deliberately proves graph-domain expansion beyond
the illustrated plan. All updates are
semantic state changes consumed by the GUI owner; no scenario writes LVGL
objects directly.

The interactive window also maps `H/P/D/S` to the stable bottom destinations
and `I` to the service-warning preview. These keys alter only host-lab local
presentation state.

Future fixtures should add delayed/rejected requests, pending work across a
disconnect, same-session reconnect, controller reboot/new session, telemetry
gaps, queue pressure, and immutable active-revision editing. Fixture format
selection remains a design task; any external format must be versioned,
strictly validated, and bounded.

## Ordered milestones

| Milestone | Observable criterion | State |
| --- | --- | --- |
| Bounded shared observation | Plain model contains session/revision, provenance/validity, and bounded graph samples | Complete |
| Semantic fake | Host-only deterministic scenarios expose immutable observation snapshots | Complete |
| Dashboard presentation | Shared LVGL view shows run data, graph, visual overview, and four-item navigation shell | Complete |
| Clarified Home/Programs/Running pass | Host-only Home quick launch, Program loading/library/overview/stages/editor, dedicated Running view modes, and trajectory presentation are rendered | Graphite visual-system implementation is complete across all current routes. Default 800 x 480, 800 x 640, and 1024 x 600 suites pass 10/10; the smoke asserts the state strip, grouped Running readings/graph, compact shell sizing, and route coverage. Manual operator feedback remains pending. |
| Scenario input | CLI selection and host key switching select scenarios without widget mutation | Complete |
| Host isolation | UI-lab preset builds the fake; ordinary host and Zephyr compositions do not | Complete for current compositions |
| Visual evidence | Window reviewed at 800 x 480, 800 x 640, and 1024 x 600; structural smoke at 640 x 360 | Author native inspection complete: 165 frames, zero geometry errors; operator approval pending |
| Recovery tests | Session/reconnect/no-replay and pending-request scenarios exercise the shared client | Pending; protocol design required |

## Verification

- Build and run `host-msvc-ui-lab-debug` CTest, including all scenario smoke
  cases.
- Build and test the 800 x 640 and 1024 x 600 viewport presets.
- Run existing host Debug/Release/MSVC desktop and portable-core suites.
- Run the Zephyr build/Twister because the shared model and catalog are used by
  the target foundation.
- Run Python tooling tests, documentation/link checks, and regenerate the
  deterministic codebase index.
- Add negative build/link evidence showing that simulator sources cannot enter
  the Zephyr image.
- Add screenshot/manual review evidence; screenshots supplement, not replace,
  semantic and structural assertions.

## Rollback

The slice is additive. Disable `FURNACE_HMI_BUILD_UI_LAB` or remove the
UI-lab preset/targets and the existing desktop foundation remains usable.
Shared model code may remain only if its target-facing use is retained; no
persisted data or migration is introduced.

## Graphite completion execution - 2026-09-09

**Fact:** The follow-up [completion plan](GRAPHITE_UI_COMPLETION_PLAN.md) adds
actual SDL renderer captures and all-ancestor/text/graph geometry checks, fixes
shared layout, furnace geometry, editor/modal lifetime, observation validity,
and coherent program fixtures. Every existing page/dialog was inspected at
800 x 480, 800 x 640, and 1024 x 600. Detailed results and reproduction are in
[Graphite UI verification](GRAPHITE_UI_VERIFICATION.md). Production recovery,
controller acceptance, persistence, and hardware usability are not established.
