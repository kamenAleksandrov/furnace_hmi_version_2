# Requirements baseline

This file is the normative requirements register for accepted project behavior.
An ID remains stable after publication; change its text deliberately and record
consequential changes in an ADR. Implementation and verification status are
tracked separately in [traceability](../06-testing/TRACEABILITY.md).

## Functional requirements

| ID          | Requirement                                                                                                                                                                                                                                        |
| ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| REQ-FUN-001 | The HMI shall act as a remote operator interface and shall present authoritative operational state received from the Control CPU.                                                                                                                  |
| REQ-FUN-002 | Program browsing and editing shall use an HMI-local draft, while the Control CPU shall own validation, identifiers, revisions, persistence, and the authoritative program library.                                                                 |
| REQ-FUN-003 | Operators shall generally be able to navigate, inspect data, and manage program drafts while a run is active, subject to explicit controller permissions rather than a global UI lock.                                                             |
| REQ-FUN-004 | Manual shall be a first-class controller mode supporting Start, target/rate changes, Pause, Resume, and Stop; it shall not be represented as a synthetic program.                                                                                  |
| REQ-FUN-005 | The Control CPU shall enforce the selected Manual session deadline. The default selection shall be six hours and the UI shall permit an explicit longer duration; target/rate changes shall not reset the deadline. Exact expiry behavior is open. |
| REQ-FUN-006 | The operator interface shall support Start, Pause, Resume, and normal software Stop as controller requests.                                                                                                                                        |
| REQ-FUN-007 | After link loss or HMI restart, the HMI shall identify the current controller session, invalidate incompatible pending/cache state, obtain authoritative state, and reconstruct its presentation.                                                  |
| REQ-FUN-008 | The HMI information model shall be able to represent the dashboard baseline described in [UX and domain baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md) without imposing a rigid fixed screen layout.                                               |
| REQ-FUN-009 | The HMI shall support planned and live measured temperature trajectories; their semantic relationship to controller execution shall be explicit.                                                                                                   |
| REQ-FUN-010 | Each setting shall have an explicit HMI or Control CPU owner. Controller-owned settings shall be presented from controller-provided value, writability, and applicable metadata.                                                                   |
| REQ-FUN-011 | The Control CPU shall own operational counters and authoritative clock/time; the HMI shall display controller-provided values.                                                                                                                     |
| REQ-FUN-012 | Run history shall consist of immutable Run Sessions. Reuse shall create an editable draft rather than alter history.                                                                                                                               |
| REQ-FUN-013 | The UI shall be localizable from the start, with English as the only initially implemented language and visible strings separated from screen logic.                                                                                               |
| REQ-FUN-014 | Celsius shall be the initially implemented display unit; canonical model representation and presentation boundaries shall not prevent later display-unit support.                                                                                  |
| REQ-FUN-015 | Future reset, clear, and restart operations shall have separately defined ownership, scope, and permission semantics rather than one ambiguous factory-reset operation.                                                                            |

## Non-functional requirements

| ID | Requirement |
| --- | --- |
| REQ-NFR-001 | Operator interaction shall produce perceptible local feedback immediately. Ordinary controller operations should provide meaningful response or pending/progress presentation within approximately 500 ms. |
| REQ-NFR-002 | Normal Stop handling shall have stronger responsiveness than low-priority operations and shall not be blocked by filesystem, graph, telemetry, modal, or draft-editing work. |
| REQ-NFR-003 | Low-priority HMI requests and droppable/coalescible telemetry shall not delay important command responses, fault/state events, or furnace-critical Control CPU work. |
| REQ-NFR-004 | The GUI context shall not perform long or unbounded filesystem, controller, storage, retry, or bulk-processing work. Cross-context mechanisms shall be explicit and bounded. |
| REQ-NFR-005 | The source architecture shall support multiple target-specific Zephyr builds and a desktop build without redesigning the portable HMI application. |
| REQ-NFR-006 | Every displayed engineering value shall distinguish its semantic provenance (measured, commanded, estimated, calculated, or configured) and validity (including stale or unavailable). |
| REQ-NFR-007 | An estimate derived from heater demand shall not be presented as measured electrical power. |
| REQ-NFR-008 | HMI health supervision shall eventually detect failure of critical HMI subsystems; an unrelated healthy thread shall not mask GUI, communication-owner, or critical-application failure. |
| REQ-NFR-009 | Hardware-specific values and APIs shall remain outside portable application code and be supplied through Zephyr-native configuration, drivers, and platform adapters. |

## Architecture invariants

| ID | Invariant |
| --- | --- |
| ARCH-INV-001 | The Control CPU is authoritative for furnace truth, programs, execution, validation, timing, control, faults, operational settings/counters, and safety; the HMI owns only presentation, local interaction state/drafts/preferences/cache, its client, and its own health. |
| ARCH-INV-002 | No furnace safety mechanism may depend on correct HMI operation. Emergency Stop is outside HMI software; normal Stop remains a distinct high-priority software request. |
| ARCH-INV-003 | Reconnection shall never automatically replay a previous command merely because the link returned. |
| ARCH-INV-004 | A newly started HMI shall be able to reconstruct correct operator-facing state from current authoritative controller information without requiring complete local event history. |
| ARCH-INV-005 | Only the designated GUI execution context may manipulate LVGL objects. |
| ARCH-INV-006 | Low-priority HMI/library work shall not impair controller control loops, sensors, watchdog, actuators, or safety/interlock handling. |
| ARCH-INV-007 | The Control CPU shall authoritatively validate every meaningful furnace operation; the HMI shall assume a request can be rejected. |
| ARCH-INV-008 | Operational limits shall not be hard-coded in the HMI as authority. Controller-informed constraints are usable only from a valid current-session capability snapshot. |
| ARCH-INV-009 | Active execution shall use an immutable controller-accepted program revision; later library edits shall not mutate it. |
| ARCH-INV-010 | Historical Run Sessions shall be immutable. |
| ARCH-INV-011 | The HMI and Control CPU watchdogs are independent; each processor owns its own health behavior. |

## Protocol requirements

These requirements constrain later protocol design; they do not define a wire
schema.

| ID | Requirement |
| --- | --- |
| PROTO-001 | Protocol semantics shall distinguish transmission, frame decode, queuing, acceptance/rejection, resulting state change, and completion when those distinctions matter. |
| PROTO-002 | The protocol shall provide controller boot/session identity and enough authoritative state to detect stale HMI state and reconstruct the operator view. |
| PROTO-003 | Request retry and duplicate behavior shall be explicit and shall not permit unintentional repeated operations. |
| PROTO-004 | Native C structures shall not define the wire format. Serialization shall use explicit fixed-width representations, units, bounds, versioning, and integrity behavior independent of padding, enum size, `size_t`, native `bool`, endianness, or SDK error values. |
| PROTO-005 | Protocol semantics, serialization, framing/integrity, and physical transport shall remain separable so the application can use RS-422/UART or an in-memory simulator transport. |
| PROTO-006 | Immediate command responses, state transitions, and faults shall take precedence over droppable/coalescible periodic telemetry where required. |
| PROTO-007 | Initial fast telemetry shall be configurable around 1 Hz, slow counters around 5 seconds, and large diagnostics/history on request; these rates shall not become fixed wire assumptions. |
| PROTO-008 | Controller capability and setting metadata shall be scoped to the identified current controller session and invalidated when that identity changes. |
| PROTO-009 | The protocol shall permit a controller rejection to carry sufficient stable meaning for the HMI to present the outcome without treating transport success as operation success. |

## Verification requirements

| ID | Requirement |
| --- | --- |
| TEST-001 | Tests shall demonstrate that HMI syntax assistance cannot bypass authoritative controller rejection. |
| TEST-002 | Reconnect tests shall demonstrate cache invalidation, current-session reconstruction, and absence of automatic old-command replay. |
| TEST-003 | Snapshot tests shall demonstrate reconstruction without a complete prior event stream. |
| TEST-004 | Protocol tests shall cover duplicate, retry, timeout, rejection, late response, and controller boot/session-change cases once semantics exist. |
| TEST-005 | Concurrency tests/reviews shall verify exclusive LVGL ownership and bounded cross-context communication. |
| TEST-006 | Priority/fault-injection tests shall verify that telemetry and low-priority work do not obstruct Stop handling or important state/fault delivery. |
| TEST-007 | Planned-trajectory tests shall verify agreement with the same accepted semantics used for controller execution once stage semantics are defined. |
| TEST-008 | Build verification shall cover portable host code, the desktop simulator, and each declared supported Zephyr target as those targets are introduced. |

## Deferred detail

Stage semantics, Manual expiry transition, alarm workflow, Run Session schema and
retention, serialization/framing, exact transport parameters, memory budgets,
and production hardware are deliberately unresolved. See
[open questions](../09-progress/OPEN_QUESTIONS.md); do not infer answers from
examples in this repository.
