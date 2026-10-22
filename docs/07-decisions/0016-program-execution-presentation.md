# ADR-0016: Program execution, ramp holdback, Pause and outcome presentation

- Status: Accepted (product-owner mandate) for the behavior below; field and command lists Proposed in the shared draft
- Date: 2026-10-20
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-024, REQ-FUN-025, REQ-FUN-006, PROTO-012, TEST-012, TEST-013; REQ-FUN-011, REQ-FUN-013; ARCH-INV-002, ARCH-INV-003; furnace PROF-01, PROF-02, PROF-08, PROF-12, PROF-13, PROF-16, START-15, RUN-07 to RUN-11, GUARD-01
- Amends: [ADR-0015](0015-hold-qualification-and-treatment-time.md) (a Hold interruption is now recoverable)

## Context

**Fact:** on 2026-10-20 the product owner approved the program execution behavior for both projects: Heating stages complete when the measured process temperature arrives at the target (not when the nominal time has elapsed); the controller holds back the setpoint when the furnace lags the ramp and reports it; a Heating stage whose target is already reached is passed without heating; an interrupted Hold keeps its credited time and must qualify again before crediting resumes; the operator can pause deliberately and must resume explicitly; a run ends with a distinct outcome, and reaching the final stage is never presented as accepted curing. The controller core implements this (furnace `core/run`); the status fields, commands and event codes are in the furnace interface specification `docs/interfaces/controller-hmi-interface.md`, version 0.5.0 (draft), sections "Program execution" and "Hold progress".

**Fact:** the labs show a run-level "Remaining" reading and Pause/Resume/Stop actions from the simulator; they have no holdback, review or outcome presentation, and the simulator pauses and resumes without controller revalidation.

## Decision drivers

- Delayed execution must never look like a clean, on-schedule run.
- Operators must see why a run is paused and what blocks a Resume.
- The controller remains the only authority for state, times and outcomes (REQ-FUN-011, ARCH-INV-002).
- Every label is localizable through the shared catalogue (REQ-FUN-013, ADR-0012).

## Options considered

1. **Present controller run status, events and outcomes as received (selected).** Matches the authority boundary.
2. **Derive progress and remaining time in the HMI from the program and the clock.** Rejected: contradicts temperature-driven completion, holdback and qualification.

## Decision

Mandated (product owner, 2026-10-20):

- During execution the HMI shows the current stage and phase, the requested setpoint and the measured process temperature, and whether the ramp is progressing, held back (with the tracking deviation and holdback time) or waiting at the target (REQ-FUN-025).
- Actual elapsed time and the nominal (planned) time are shown separately; a delay beyond the nominal time is shown as such.
- A paused run shows whether the operator paused it or a fault paused it, with the reasons; Resume and Stop are requests to the controller; a rejected Resume shows the blocking conditions. The HMI never resumes or retries on its own.
- Warnings (holdback, waiting at the target, overshoot) are shown from controller events.
- At the end the HMI shows the outcome: completed, completed with deviations requiring review, stopped by the operator, interrupted by a fault, or prevented by protection. It never presents curing as accepted while the controller reports it as not assessed.
- Hold presentation (amends ADR-0015): after an interruption the credited time stays visible; "requalifying" is shown while the temperature is back in the band but not yet qualified; the countdown resumes from the kept time only in active treatment.

## Consequences

- New presentation states, events and catalogue strings in both presentations (Running screen, run summary, Device log entries).
- Implementation gaps, recorded in [traceability](../06-testing/TRACEABILITY.md): no holdback, delay, review or outcome view; simulated Pause/Resume without revalidation; run-level remaining time derived without temperature-driven completion.
- The interface references in older documents still name 0.2.0; moving them follows the review of the 0.3.0 to 0.5.0 changes.

## Verification and follow-up

TEST-013. Open with the controller: controlled stopping and cooling, fan behavior during Pause (provisional), limits for Hold interruptions, the process-acceptance policy, Resume permissions and roles.

## Review record

Mandated product decision of 2026-10-20. HMI domain review and decision-challenger review of the presentation design and of the shared 0.5.0 lists are required before implementation.
