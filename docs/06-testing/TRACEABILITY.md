# Requirements traceability

This baseline maps requirements to intended evidence. `Not implemented` is an
honest state, not a failed test. Update this file as design and tests land.

| Requirement group | Intended verification | Current state |
| --- | --- | --- |
| ARCH-INV-007, ARCH-INV-008 | Ownership review plus request/rejection component tests (`TEST-001`) | Documentation baseline; controller request behavior not implemented |
| ARCH-INV-001; REQ-FUN-001; REQ-NFR-006 | Table-driven availability rendering, fail-closed invalid/zero/NULL input, non-interactive tree, and parent-lifetime checks in `desktop.state_availability_smoke` | Partial UI evidence in the [availability component](../../app/hmi/presentation/state_availability/); no controller/session data path is implemented |
| ARCH-INV-002, ARCH-INV-006 | Architecture review and controller contract/system priority tests | Controller implementation outside current scope |
| ARCH-INV-003, ARCH-INV-004; REQ-FUN-007 | Reconnect/session/snapshot component and integration tests (`TEST-002`, `TEST-003`) | Protocol behavior not implemented |
| ARCH-INV-005; REQ-NFR-004 | Static ownership review, queue-bound tests, host/Zephyr concurrency tests (`TEST-005`) | Foundation rule documented |
| ARCH-INV-009; REQ-FUN-002 | Revision/draft scenario tests | Domain behavior not implemented |
| REQ-FUN-003 | Dedicated active-Program navigation-containment smoke plus later controller-permission scenarios | Partial host-only evidence: the UI-lab smoke verifies that ordinary bottom navigation is hidden and rejected on the dedicated Running view. It is a presentation rule only; controller permission behavior remains unimplemented. |
| ARCH-INV-010; REQ-FUN-012 | Immutable history and create-draft scenario tests | Domain behavior not implemented |
| REQ-FUN-004, REQ-FUN-005 | Manual lifecycle/deadline controller contract tests | Semantics not implemented; expiry transition open |
| REQ-FUN-016; PROTO-010; TEST-009 | Controller service-policy/preflight and one-time acknowledgement lifecycle tests; recorded-maintenance visibility and session/reconnect tests | Partial host-only presentation evidence: a bounded controller-observation field and a simulated warning dialog exist. The controller-owned one-time policy and maintenance record are specified but not implemented. |
| REQ-FUN-017; PROTO-011; TEST-010 | Host Program-editor route/type/field applicability smoke, then controller capability/validation/rejection integration tests | Partial host-only visual evidence: Program/Stage pages use New program/Add stage terminology and show local HEATING target and rate/duration cross-calculation at 1 °C, 0.1 °C/min, and 1-minute controls, HOLD duration with inherited preview target, and FASTEST/controlled COOL modes. No controller capability snapshot, persistence, validation, or request semantics are implemented. |
| REQ-FUN-006; REQ-NFR-001, REQ-NFR-002 | Interaction-latency and overloaded-path tests (`TEST-006`) | Behavior not implemented |
| REQ-FUN-008 through REQ-FUN-012; REQ-FUN-014; REQ-NFR-006, REQ-NFR-007 | Model/formatting/graph scenario tests (`TEST-007`) | Partial host-only evidence: bounded dashboard observation model and planned/measured graph presentation exist; program semantics, persistence, controller agreement, and production graph behavior remain unimplemented |
| REQ-FUN-013 | Catalog completeness and invalid-ID tests on the [host](../../tests/host/ui_strings_smoke.c) and [Zephyr](../../tests/zephyr/foundation/src/main.c), plus desktop source review | Partial foundation evidence: initial English proof strings are separated in the [catalog](../../app/hmi/presentation/strings/); production-screen coverage and additional locales are not implemented |
| REQ-FUN-009; REQ-NFR-006, REQ-NFR-007 | Host-only semantic UI-lab scenarios and trajectory presentation smoke at 640 x 360, then accepted-semantics/controller-agreement tests (`TEST-007`) | Partial host-only evidence: denser rectangular axis/grid with numeric range labels, solid planned path, solid measured segments above it, unavailable-value gaps, synthetic color-coded preview-stage nodes, and overrun-only domain expansion render in the canonical UI-lab smoke. Manual 800 x 480 review, controller agreement, and production telemetry semantics remain deferred. |
| REQ-FUN-015 | Ownership/permission contract tests per future operation | Operations intentionally not designed |
| REQ-NFR-005, REQ-NFR-009 | Host simulator and declared Zephyr target builds (`TEST-008`) | Foundation build evidence tracked in [status](../09-progress/STATUS.md) |
| REQ-NFR-008; ARCH-INV-011 | Health-supervisor design review and fault-injection tests | Watchdog production logic intentionally deferred |
| PROTO-001 through PROTO-009 | Protocol vectors, codec/framer tests, lifecycle and fault-injection suites (`TEST-004`) | Requirements only; no wire schema approved |

When adding a test, link its stable path or target here rather than copying the
requirement text. When changing behavior, update the requirement, ADR (if
consequential), implementation, test, and this mapping in one change.

## Graphite host presentation evidence - 2026-09-09

**Fact:** [Native verification](../09-progress/GRAPHITE_UI_VERIFICATION.md)
adds 165 inspected captures and `desktop.ui_lab_visual_geometry` alongside
`desktop.ui_lab_smoke`. This supplies partial host evidence for REQ-FUN-003
(Program lock/Manual navigation), REQ-FUN-009 (planned/measured graph) and REQ-FUN-013 (shared English
string catalog), REQ-FUN-017 (editor fields and local draft containment),
REQ-NFR-002/006 (Stop priority and invalid observations), and
ARCH-INV-005 (owner-only rendering and callback cleanup). It does not establish
physical touch performance, controller validation, production request latency,
protocol recovery, persistence, or safety. Those intended tests remain pending.

## Settings host-only presentation evidence - 2026-09-14

**Fact:** The host-only LVGL Settings route now supplies partial presentation
evidence for REQ-FUN-010, REQ-FUN-013, REQ-FUN-015, REQ-NFR-001,
REQ-NFR-004, REQ-NFR-006, ARCH-INV-001, ARCH-INV-005, and ARCH-INV-007.
The focused smoke path exercises numeric editing, unsaved navigation,
Display paging/save, and scoped reset confirmation. Native capture coverage
exercises the default Settings page, Display page 2, Date & time, Connections,
and Maintenance at 800 x 480 with zero geometry errors.

This evidence is limited to HMI-local presentation and simulated outcomes. It
does not establish controller validation, persistence/recovery, protocol
lifecycle, clock ownership, connectivity, deletion, restart, physical touch,
or safety behavior.

## Device host-only presentation evidence - 2026-09-14

**Fact:** The native LVGL Device route supplies partial presentation evidence
for REQ-FUN-010, REQ-FUN-011, REQ-FUN-013, REQ-FUN-016, REQ-NFR-004,
REQ-NFR-006, ARCH-INV-001, ARCH-INV-004, ARCH-INV-005, ARCH-INV-007,
PROTO-008, and PROTO-010. The route covers the five proposed Device
categories, generic component/task rendering, filters, three-row pagination,
read-only details/history/info, stale/unavailable states, and dirty Settings
navigation containment.

The editable host-only catalogue at
`simulator/ui_lab/data/device_service_catalogue.txt` is loaded before LVGL
startup, parsed into bounded records, and copied beside each build by CMake.
Changing an interval in that file changes the rendered cards without a
component-specific renderer branch. Direct native smoke verifies catalogue
backed interval display, all Device categories, filters, details, notices,
history, info, and the Settings guard. The complete host UI-lab preset passes
10/10 at 800 x 480, 800 x 640, and 1024 x 600; Device capture geometry reports
zero errors at each size.

This remains host-only HMI presentation evidence. It does not establish the
controller-owned catalogue/counter source, service-policy evaluation,
revision/session transport, persistence, history authority, protocol
lifecycle, manufacturer intervals, physical touch, or safety behavior. ADR-0011
remains Proposed and those production tests are pending.

## Programs host-only styling evidence - 2026-09-15

**Fact:** The native LVGL Programs library, Program details and Program stages
routes now supply partial presentation evidence for REQ-FUN-002, REQ-FUN-003,
REQ-FUN-009, REQ-FUN-012, REQ-FUN-013, REQ-FUN-014 and REQ-FUN-017;
REQ-NFR-004 and REQ-NFR-006; and ARCH-INV-001, ARCH-INV-005, ARCH-INV-007,
ARCH-INV-009 and ARCH-INV-010. The implementation preserves existing
six-card/nine-stage paging, graph-node and editor routing, full-card targets,
Load-before-Start meaning, illustrative graph values and local preview
boundaries while applying the approved neutral cards, aligned facts, toolbar
pagination, restrained Delete action and outlined stage badges.

The native MSVC UI-lab build and complete CTest preset passed 10/10 at 800 x
480, 800 x 640 and 1024 x 600. The native capture geometry check reported zero
errors at every size. This is presentation-only evidence: it does not establish
controller-owned program truth, validation, persistence, protocol behavior,
thermal/safety behavior or physical touch performance. Manual native image
inspection was unavailable because the local image viewer could not open the
generated BMP captures.

## Programs visual and keyboard refinement evidence - 2026-09-15

**Fact:** The follow-up supplies partial host-only evidence for REQ-FUN-002,
REQ-FUN-003, REQ-FUN-009, REQ-FUN-013 and REQ-FUN-017; REQ-NFR-004 and
REQ-NFR-006; and ARCH-INV-001, ARCH-INV-005, ARCH-INV-007, ARCH-INV-009 and
ARCH-INV-010. It covers Delete labels on text and numeric keyboards, active
Shift feedback, closed number/symbol cycling, larger centered Program titles,
a drawn five-point favourite star, opaque View stages styling, graph-axis
clearance, and centered background statistic cards without separators.

The native MSVC build, complete CTest suite and capture harness passed 10/10
at 800 x 480, 800 x 640 and 1024 x 600; all native capture geometry checks
reported zero errors. The evidence remains presentation-only and does not
establish controller ownership, protocol behavior, persistence, safety,
embedded LVGL integration or physical touch performance.

## Running time control outline evidence - 2026-09-15

**Fact:** The native and browser implementations retain the elapsed/remaining
time button and its local toggle, while removing the trailing swap glyph and
background fill. The control is now transparent with a colored blue border.
This supplies additional partial host-only evidence for REQ-FUN-009,
REQ-FUN-013 and REQ-NFR-004/006, and ARCH-INV-001 and ARCH-INV-005.

Native MSVC builds, complete CTest suites and capture geometry checks passed
10/10 at 800 x 480, 800 x 640 and 1024 x 600. The standalone HTML script syntax
and targeted running-time style assertions also passed. Browser
interaction/capture was not rerun for this presentation-only edit. This does
not establish embedded LVGL integration, physical touch performance, controller
ownership, protocol behavior, persistence or safety.

## Running/program control styling evidence - 2026-09-16

**Fact:** Removed the secondary power Estimate caption; flattened the native
and browser Running Graph/Split/Furnace selectors to text plus an active
colored bottom border; matched quick-launch width to one bottom-nav item;
filled the Program detail View stages and Delete actions; and aligned browser
stage-type and numeric plus/minus colors with native LVGL.

Native MSVC builds and complete CTest suites passed 10/10 at 800 x 480,
800 x 640 and 1024 x 600; native capture geometry checks reported zero errors.
Standalone HTML syntax and targeted style assertions also passed. Browser
interaction/capture was not rerun for this presentation-only edit. This does
not establish embedded LVGL integration, physical touch performance, controller
ownership, protocol behavior, persistence or safety.
