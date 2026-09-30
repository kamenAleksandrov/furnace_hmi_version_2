# Device implementation plan

Status: **Host-only implementation complete; production integration proposed**,
2026-09-14. Outcome: the worker inspection page described in the [Device
proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md) is implemented in the native
LVGL UI laboratory using the existing graphite design and shared browser
concept. The production controller contract remains open.

## Governing baseline and scope

The proposal records user mandates, facts, assumptions, recommendations,
requirements and ADR constraints. [ADR-0011](../07-decisions/0011-component-service-catalogue.md)
is proposed, not accepted. Controller ownership covers counters, service policy,
faults, history and time; LVGL has one execution owner. Worker access is
read-only, and active Program navigation containment remains unchanged.

Exclude worker interval editing, component creation, counter reset, service
completion certification, fault clearing, controller calibration, actuator
tests, safety claims, and changes to Settings Maintenance action contracts.
No manufacturer schedule is inferred from illustrative examples.

## Ownership and affected modules

- `docs/images/theme/hmi-web-demo.html`: shared clickable Settings/Device
  browser sketch and visual reference; it has no device I/O.
- `simulator/ui_lab/data/device_service_catalogue.txt`: editable host-only
  line-oriented catalogue. The UI lab reads it before lv_init() at startup,
  validates and copies bounded records, then populates generic component/task
  cards. Developers change intervals in this file; the renderer has no
  component-specific interval table.
- `simulator/ui_lab/src/ui_lab_explorer.c` / `.h`: native Device routing,
  bounded catalogue parser/view model, generic rows/details, filters,
  pagination, stale/unavailable presentation, and read-only actions. The
  existing native Settings implementation remains intact.
- `simulator/semantic_controller/`: later deterministic catalogue, counters,
  task states, errors/history fixtures; no production service engine.
- `app/hmi/model/` and presentation/string catalogue: portable bounded
  observation types, selectors/formatting, generic rows/details, localized text.
- Future product configuration/controller side: verified component/task
  manifest, authoritative counter providers, persistence, installed-part
  baselines, task evaluation and service-gate integration. This HMI plan does
  not authorize changes in the sibling controller repository.
- Future controller client/platform adapters: revision-bound paged reads,
  time/source metadata and bounded delivery to the GUI; hardware drivers remain
  at their native platform boundary.

## Ordered milestones

1. **Browser review.** Add Device and Settings navigation, five Device sections,
   generic catalogue lists/details, three counters, notices and read-only
   history/info. Exercise current, no-attention, fault, stale, unavailable,
   empty, and condition-based/missing-source cases. Done when the worker scope
   and terminology are reviewable in the same HTML and checks are recorded.
2. **Counter and catalogue contract.** Resolve powered-on source, paused Program
   inclusion, confirmed versus commanded fan runtime, persistence accuracy,
   manufacturer/BOM references, task triggers, warning leads, and replacement
   baselines. Review ADR-0011 and update requirements/ownership/traceability
   before normative implementation. Define wire-independent semantics before
   selecting serialization. Done when unknown values and policy owners are explicit.
3. **Bounded portable read models.** Define stable device/component/instance/task
   IDs, catalogue revision, counters with source/validity/generation, task states,
   notices and event records. Choose limits for catalogue/task counts, field
   lengths, page size, queues and history windows from budgets. Done when
   invalid/oversized inputs and revision changes fail visibly without overflow
   or assembling mixed snapshots.
4. **Native LVGL lab.** **Complete for the host-only presentation.** The approved
   composition has generic rows, details, filters, pagination, a dirty Settings
   navigation guard, deterministic observations, and startup-loaded catalogue
   data. Existing Settings interactions remain working; native 800 x 480,
   800 x 640, and 1024 x 600 capture/geometry and relevant host tests pass.
   The production controller-backed data path is a later conditional milestone.
5. **Controller catalogue/counter integration (conditional).** Obtain actual
   coherent metadata/counters/task status and invalidate old sessions. No HMI
   accumulated totals or authoritative due calculations. Done when new parts
   using existing counter kinds require configuration/localization only and
   unsupported sources visibly remain unknown. Real counter-source accuracy,
   power-loss retention, persistence wear and replacement behavior need target
   evidence owned by the controller workstream.
6. **Notifications/history integration (conditional).** Define event identity,
   severity/order, active/resolved distinction, controller-issued service
   acknowledgements, history retention/pagination and approved guidance mapping.
   Done when reading/dismissing never clears a fault or marks service complete,
   history is immutable in Device, and unknown codes retain their raw code.

## Verification and recovery

- Catalogue extension with a new ordinary component without view changes;
  duplicate IDs, absent source, empty catalogue, retired/replaced part, maximum
  configured size, missing localization/procedure, and incompatible revision.
- Counter large values/precision, unknown versus zero, no HMI ticking, counter
  rollback/generation mismatch, measured versus commanded fan source, Program
  versus Manual/paused definitions, last-known stale totals and new device identity.
- Multiple tasks/triggers, whichever-first behavior, due with another trigger
  unknown, unknown calendar baseline, overdue display, condition-based tasks,
  and inspection-due not being shown as a diagnosed fault. Controller-side due
  tests use independent expected outcomes rather than HMI reimplementation.
- Filters and page bounds after data changes, task/detail back navigation,
  empty/error/loading states, stable notice/event IDs, stale history and no
  mixed-revision pages. Mid-view reconnect revalidates or closes affected detail.
- Every worker action is inspection only; no unauthorized write command,
  service reset, bulk fault acknowledgement, interlock override, or hidden edit.
- Single-owner LVGL, copied bounded worker updates, lifecycle-safe response
  correlation, queue full/coalescing behavior, cancellation and no stale replay.
  Reading large histories cannot starve current faults or normal Stop.
- Preserve Running containment, Settings dirty prompts, and previously added
  Maintenance dialogues. Physical touchscreen and actual controller behavior
  remain distinct gates from browser/native layout inspection.

## Documentation, migration and rollback

Update the canonical Device proposal and accepted ADR/requirements only after
review; preserve existing global service acknowledgement semantics until its
relationship to per-task schedules is agreed. Update status, roadmap, session
log, indexes and traceability with actual evidence. Regenerate metadata using
`tools/codebase_index.py`, run `--check`, local link checks, and `git diff --check`.

This concept introduces no device persistence. Future catalogue migration must
preserve stable component/task IDs, installed-instance history and baseline
meaning; never reuse retired IDs for unrelated parts. Unknown schema/revision
is unavailable, not factory defaults. HMI rollback cannot reset controller
counters or maintenance history; older clients display unsupported metadata
honestly. Controller policy/storage migration needs its own rollback plan.

## Verification record

**Fact:** Local Chrome headless browser review passed 90 Device views (five
categories, six data fixtures, three viewport sizes: 800 x 480, 800 x 640,
1024 x 600). Checks covered screen bounds and content overlap, catalogue
extension without renderer changes, pagination, error filtering/details,
Settings dirty navigation, and all 12 existing restart assessment/result
combinations. Component detail views were checked at each size as well.

Actual captures were inspected for overview, component list/detail,
notifications, stale overview, and wider history. The first visual pass found
separator glyphs damaged by shell encoding; escaped Unicode corrected them
and fresh browser checks/captures passed. Review also aligned the illustrative
fan interval with its counter baseline. Example intervals remain synthetic.

Local review driver: `node build/device-concept-review.cjs`, using a dedicated
headless Chrome debugging session on localhost port 9227; captures are under
`build/device-concept-review/`. Representative overview and component captures
are retained under `docs/images/theme/`. This is browser evidence, not native
LVGL or controller verification.

**Fact:** The native LVGL implementation was built with the editable catalogue
loaded before lv_init(). Direct native smoke passed catalogue-backed interval
rendering, all five Device categories, filters, pagination, component/task
details, notifications, history, info, stale presentation, unavailable state,
and the dirty-Settings navigation guard.

Native capture/geometry verification reported zero geometry errors for every
Device page and state at 800 x 480. The complete host UI-lab preset passed
10/10 at 800 x 480, 800 x 640, and 1024 x 600, including build, smoke,
index, workflow, and foundation checks. The editable source catalogue was
copied beside each built executable by CMake.

This is host-only presentation evidence. No controller catalogue/counter
transport, production persistence, manufacturer interval validation, firmware
behavior, or physical touchscreen/device test was performed. ADR-0011 remains
Proposed; authoritative controller ownership and the catalogue deployment/wire
contract remain unresolved.
