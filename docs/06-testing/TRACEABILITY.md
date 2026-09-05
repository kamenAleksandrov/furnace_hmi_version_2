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
