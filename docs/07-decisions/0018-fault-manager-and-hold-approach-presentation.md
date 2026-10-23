# ADR-0018: Fault-manager and Hold-approach presentation

- Status: Accepted (product-owner mandate) for the behavior below; message and field lists Proposed in the shared draft
- Date: 2026-10-22
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-027, REQ-FUN-026, REQ-FUN-024, REQ-FUN-022, REQ-FUN-011, REQ-FUN-013, TEST-015; ARCH-INV-002; furnace FAULT-01, FAULT-05, FAULT-06, SENS-03, SENS-07, RUN-13, CTRL-11, CTRL-12
- Amends: [ADR-0014](0014-live-hot-start-confirmation-and-sensor-faults.md) (a failure of a required sensor is a latched critical fault), [ADR-0017](0017-fault-classes-and-control-diagnostics-presentation.md) (acknowledgement and recovery requests become explicit commands)

## Context

**Fact:** on 2026-10-22 the product owner approved a centralized fault manager in the controller and two corrections. First, the failure of a sensor required for control or protection is critical until a validated degraded-operation policy exists. The controller had treated the required process temperature as recoverable by default, which contradicted the approved sensor rule. Second, when a Heating stage completes on arrival, the Hold setpoint continues smoothly toward its target at the preceding ramp rate instead of stepping to it (Option B).

**Fact:** the furnace controller core implements this: `core/fault` with stable fault IDs, classes, entries with recovery eligibility, a bounded event history, and separate acknowledgement and recovery requests; the sequencer follows its verdict and reports `setpoint_approaching` and `setpoint_moving`. No recovery procedure, service role or password is defined: a recovery request for a latched fault is recorded and refused. Fields, messages and codes are in the furnace interface specification `docs/interfaces/controller-hmi-interface.md`, version 0.7.0 (draft), sections "Fault management" and "Program execution". The 0.2.0 draft `SENSOR_FAULT` message is replaced by `FAULT_EVENT`.

**Fact:** the labs show warnings and a Device log from simulated data; they have no fault entry list, acknowledgement command, recovery eligibility or Hold-approach presentation.

## Decision drivers

- Acknowledging must never look like fixing or resuming.
- An operator must see what blocks heating and which action, if any, exists.
- A Hold whose setpoint is still approaching must not look like a stalled or failing Hold.
- The controller remains the only authority for faults, latches and recovery (REQ-FUN-011, ARCH-INV-002).
- Every label is localizable through the shared catalogue (REQ-FUN-013, ADR-0012).

## Options considered

1. **Present the controller's fault entries, events and recovery eligibility as received (selected).**
2. **Keep the HMI's own alarm list and acknowledgement state.** Rejected: duplicates the controller's lifecycle and could show a fault as resolved that the controller still latches.

## Decision

Mandated (product owner, 2026-10-22):

- The HMI shows the controller's active fault entries with class, condition (active or cleared), latch, acknowledgement, occurrence count and recovery eligibility. It also shows the reasons heating is currently inhibited.
- Acknowledge is a request to the controller. The HMI shows the result. It never hides an entry that still inhibits heating, and it never treats acknowledgement as recovery or Resume.
- A recovery request is offered only as a request and shows the controller's answer. Today that answer is always "no approved procedure" or "protection reset outside the controller". No role, password or local override exists.
- A failure of a required sensor is shown as a latched critical fault, not as a warning with Resume (amends ADR-0014). A sensor approved as not required shows its configured class.
- The fault event history is shown from controller events in sequence order. A gap in the sequence numbers is shown as missed events, never filled in.
- During a Hold whose setpoint is still approaching its target, the setpoint is shown moving toward the target. Qualification can already run during that time, because it is judged on the measured temperature in the treatment band.

## Consequences

- New presentation states, commands and catalogue strings in both presentations: fault list, acknowledgement and recovery results, inhibition reasons, Device log, Hold approach.
- Implementation gaps, recorded in [traceability](../06-testing/TRACEABILITY.md): no fault entry list or acknowledgement command; the simulator's sensor warnings are resumable; no Hold-approach presentation.
- Older documents that cite `SENSOR_FAULT` are read through this ADR until they are revised.

## Verification and follow-up

TEST-015. Open with the controller: approved recovery procedures and roles for latched faults, whether latches survive a controller restart (persistence), the fault event delivery and history query, the per-model classification policy.

## Review record

Mandated product decision of 2026-10-22. HMI domain review and decision-challenger review of the presentation design and of the shared 0.7.0 lists are required before implementation.
