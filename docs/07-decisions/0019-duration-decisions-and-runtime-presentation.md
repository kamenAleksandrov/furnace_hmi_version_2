# ADR-0019: Program duration, quality decisions and runtime-limit presentation

- Status: Accepted (product-owner mandate) for the behavior below; message, field and code lists Proposed in the shared draft
- Date: 2026-10-23
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-028, REQ-FUN-027, REQ-FUN-026, REQ-FUN-025, REQ-FUN-011, REQ-FUN-013, TEST-016; ARCH-INV-002; furnace PROG-11, PROF-15, PROF-17, RUN-14, RUN-15, RUN-16, START-16, START-17, FAULT-07, FAULT-08, FAULT-09, FAULT-02
- Amends: [ADR-0018](0018-fault-manager-and-hold-approach-presentation.md) (there are no operator roles at all; a continuable quality fault offers Continue)

## Context

**Fact:** on 2026-10-23 the product owner approved a fault philosophy and a runtime policy for the furnace controller (furnace repository `prompts/15-phase13-measurement-qualification-runtime-limits.md`):

- **Roles.** No accounts or authorization tiers.
- **Quality faults.** A quality-only fault may continue after an explicit decision. During a bounded wait the controller holds the last setpoint.
- **Equipment and safety faults.** Equipment-protective faults inhibit and cannot be acknowledged away; safety faults follow the protective response.
- **Durations.** An advisory (recommended) program duration is acknowledged, never confused with the non-overridable maximum. An accepted long program runs its full schedule. A hard runtime limit stops a run that cannot complete.
- **Measurement.** Chamber temperature is the authoritative process measurement. Completion is not a verified cure.
- **Persistence.** Unresolved faults must survive restarts; this is not implemented yet.

**Fact:** the furnace controller core implements the behavior. Fields, messages and codes are in the furnace interface specification `docs/interfaces/controller-hmi-interface.md`, version 0.8.0 (draft):

- `AWAITING_DECISION` and `RUN_CONTINUE`;
- duration acknowledgement in the Start proposal and confirmation;
- runtime fields and events, the outcome `STOPPED_AT_RUNTIME_LIMIT` and fault ID 14 `RUNTIME_LIMIT`;
- fault categories and `continuable`;
- Hold excursion fields and the Start measurement qualification.

All values on the furnace side are test fixtures, including the 2-minute decision wait.

**Fact:** the labs have no duration acknowledgement, decision wait, Continue action, runtime presentation or fault categories.

## Decision drivers

- An advisory warning must never look like a fault or like permission beyond the maximum.
- A quality decision must say what happened, that the cure may be affected, what to do, how much time is left, and offer exactly Continue or Stop.
- Equipment and safety faults must never offer Continue.
- Elapsed time, accepted duration, overrun and the runtime limit must stay distinguishable.
- The temperature used for execution is chamber temperature, not workpiece temperature.
- The controller remains the only authority (REQ-FUN-011, ARCH-INV-002). Every label is localizable (REQ-FUN-013).

## Options considered

1. **Present the controller's decision state, durations and categories as received (selected).**
2. **Let the HMI decide which faults may continue, or time the decision wait locally.** Rejected: it would duplicate controller policy, and the wait would drift from the controller's monotonic clock.

## Decision

Mandated (product owner, 2026-10-23):

- **Advisory duration.** When a Start proposal reports `duration_ack_required`, the HMI shows the recommended, estimated and maximum durations as a warning with an explicit acknowledgement. The confirmation carries the accepted duration. A program above the maximum is never offered.
- **Running time.** The HMI shows the accepted duration (`accepted_s`), the runtime used (`running_s`) and the hard boundary (`runtime_limit_s`) separately from elapsed time. Overrun is shown as a warning. A stop at the runtime limit is shown as such and is not overridable.
- **Quality decision.** In `AWAITING_DECISION` the HMI shows:
  - the quality reasons and their fault entries;
  - that curing quality may be compromised;
  - the recommended-action codes;
  - the remaining decision time from `decision_remaining_s`.

  It offers Continue (`RUN_CONTINUE`) and Stop. Continue is a request; the result is shown. A decision timeout is shown as a fault pause with heating off.
- **Fault categories.** Faults are grouped as quality, operational, equipment and safety. Equipment and safety faults never show Continue.
- **Measurement qualification.** Tolerated Hold excursions (`excursion_count`) and the Start measurement qualification (`MEASUREMENT_UNQUALIFIED`) are shown as information.
- **Process measurement.** Temperatures used for execution are labelled as chamber temperature.
- **Completion.** A completed program is never shown as a verified cure.
- **Roles.** No role, login, password or local override exists in the HMI (amends ADR-0018). Engineering configuration is not edited through operator screens.

## Consequences

- New presentation states, commands and catalogue strings in both presentations: duration acknowledgement, runtime readings, decision view with countdown, Continue, categories, excursion counts, chamber-temperature label.
- Implementation gaps are recorded in [traceability](../06-testing/TRACEABILITY.md).

## Verification and follow-up

TEST-016. Still open with the controller:

- the recommended-action code table per fault;
- the decision-wait and runtime values per furnace model;
- Save-time duration acknowledgement once a controller program store exists;
- the event that today also reports a failed re-qualification as `HOLD_INTERRUPTED` (furnace IMP-19);
- fault persistence across restarts (furnace FAULT-02).

## Review record

Mandated product decision of 2026-10-23. HMI domain review and decision-challenger review of the presentation design and of the shared 0.8.0 lists are required before implementation.
