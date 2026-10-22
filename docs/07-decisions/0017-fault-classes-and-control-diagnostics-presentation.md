# ADR-0017: Fault-class, stage-transition and control-diagnostics presentation

- Status: Accepted (product-owner mandate) for the behavior below; field lists Proposed in the shared draft
- Date: 2026-10-22
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-026, REQ-FUN-025, REQ-FUN-022, REQ-FUN-011, REQ-FUN-013, TEST-014; ARCH-INV-002; furnace FAULT-03, FAULT-04, RUN-12, CTRL-09, CTRL-10, GUARD-05
- Amends: [ADR-0016](0016-program-execution-presentation.md) (a protective trip no longer appears as a fault pause)

## Context

**Fact:** on 2026-10-22 the product owner asked both projects to distinguish recoverable faults, critical faults and independent protective trips. A recoverable fault stops heating and allows an explicit Resume after revalidation. A critical fault stops heating and latches: an ordinary Resume is refused, and no recovery procedure is defined yet, so only Stop ends the run. A protective trip is always effective and recorded distinctly. A critical or protective fault must not become resumable merely because its trigger disappears.

**Fact:** the furnace controller core implements this: run state `FAULTED`, `fault_class` (`RECOVERABLE`, `CRITICAL`, `PROTECTIVE_TRIP`), events `RUN_FAULTED`, `PROTECTIVE_TRIP` and `STAGE_TRANSITION_DEVIATION`, rejection reason `FAULT_LATCHED` and outcome `INTERRUPTED_BY_CRITICAL_FAULT`. It also has a first PI temperature controller that reports its power request and saturation. Which fault reasons are critical is per-model configuration, and only test fixtures exist. Fields and codes are in the furnace interface specification `docs/interfaces/controller-hmi-interface.md`, version 0.6.0 (draft).

**Fact:** ADR-0016 lists "prevented by protection" as an outcome and shows every fault as a fault pause with Resume. The labs have no latched-fault, protective-trip, transition-deviation or control-diagnostics presentation. `PROGRAM_AND_STARTUP_MODEL.md` already places raw controller terms in a role-appropriate service view.

## Decision drivers

- A latched fault must never look resumable, and nothing on the HMI may suggest that acknowledging it restarts heating.
- The protective function is independent of the controller software, and the presentation must not suggest that software can clear it.
- A temperature gap at a stage transition must be visible, not smoothed into a clean curve.
- Diagnostics are information for qualified users, never inputs to HMI decisions (REQ-FUN-011).
- Every label is localizable through the shared catalogue (REQ-FUN-013, ADR-0012).

## Options considered

1. **Present the controller's fault class, state and events as received (selected).** Matches the authority boundary.
2. **Map latched faults onto the existing fault-pause view with Resume disabled.** Rejected: operators would read it as a temporary pause and expect it to clear.

## Decision

Mandated (product owner, 2026-10-22):

- A `FAULTED` run is shown in its own state, distinct from any pause, with its class and reasons. It offers no Resume action, only Stop. The view states that recovery needs an approved procedure, of which none exists yet. If a Resume request is nevertheless sent (for example from another client), the `FAULT_LATCHED` rejection is shown.
- A protective trip is shown as an action of the independent protection, recorded distinctly from other faults; the HMI never offers to clear or bypass it.
- Acknowledging or dismissing any fault message never resumes or restarts heating.
- At the end of a run the additional outcome "interrupted by a critical fault" is shown; "prevented by protection" now follows a latched protective trip.
- A stage transition with a deviation above the controller's threshold (`STAGE_TRANSITION_DEVIATION`) is listed as a review reason of the run, with the measured deviation. The run summary may show each stage's setpoint step and transition deviation from the stage records.
- The proposed `control_diagnostics` (heating-power request in 0.01 %, control error, saturation and integration flags) are shown only in a diagnostics or service view. The HMI never derives decisions from them, and a power request is never presented as proof that power flowed.

## Consequences

- New presentation states, outcome, review reason, events and catalogue strings in both presentations (Running screen, run summary, Device log, diagnostics view).
- Implementation gaps, recorded in [traceability](../06-testing/TRACEABILITY.md): no latched-fault or protective-trip view; the simulator treats every stop as resumable; no transition-deviation or diagnostics presentation.
- The ADR-0016 text that a fault pause covers every fault is narrowed: a protective trip or a critical fault is presented by this ADR.

## Verification and follow-up

TEST-014. Open with the controller: the classification of each fault reason per model and process; the recovery or service procedure for a latched fault and the roles allowed to perform it; permissions for the diagnostics view; whether a large transition deviation should also pause the run.

## Review record

Mandated product decision of 2026-10-22. HMI domain review and decision-challenger review of the presentation design and of the shared 0.6.0 lists are required before implementation.
