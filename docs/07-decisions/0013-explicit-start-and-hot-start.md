# ADR-0013: Explicit Start, hot start and skipped-stage confirmation

- Status: Accepted (product-owner mandate) for the start policy and the HMI presentation duties; message details are a Proposed shared draft
- Date: 2026-10-16
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-018, REQ-FUN-019, REQ-FUN-020, PROTO-012, TEST-011; ARCH-INV-001, ARCH-INV-003, ARCH-INV-007; furnace START-01 to START-10
- Amended by: [ADR-0014](0014-live-hot-start-confirmation-and-sensor-faults.md) (2026-10-16): the confirmation has no normal timeout; it stays open while live data is valid and is invalidated otherwise

## Context

**Fact:** the product owner decided on 2026-10-16 that a furnace must never begin or resume heating merely because its measured temperature reached a threshold, and that a hot start which skips program stages needs an explicit, current operator confirmation. The decision applies to both projects. The Control CPU requirements are in the furnace repository (`docs/requirements/requirements.md`, START-01 to START-10) and the message semantics in its single interface specification (`docs/interfaces/controller-hmi-interface.md`, version 0.1.0 draft).

**Fact:** existing HMI decisions already require that the Control CPU validates every operation (ARCH-INV-007) and that reconnection never replays a command (ARCH-INV-003). The 2026-10-06 UX decision described **Start again** after Stop as "the same program from the start, at once", and the labs accept any start immediately.

## Decision drivers

- No unexpected heating after cooling, cancellation, expiry, link loss, HMI or controller restart, or power restoration.
- One authority for stage eligibility: the Control CPU.
- Operators see exactly what a hot start skips before they confirm it.

## Options considered

1. **Controller-proposed hot start with explicit confirmation (selected).** The HMI presents the controller's proposal and sends a confirmation bound to that proposal; the controller revalidates.
2. **HMI-side eligibility with a single Start command.** Rejected: it would duplicate the controller's stage-selection logic and let a client decide a safety-relevant transition (ARCH-INV-001/007).
3. **Armed or waiting start that fires when the furnace cools enough.** Rejected by the product owner: no deferred, queued, armed or temperature-triggered start is permitted.

## Decision

Mandated (product owner, 2026-10-16):

- The HMI sends Start only for an explicit operator action and never stores, queues, arms, schedules or repeats a Start or confirmation (REQ-FUN-018). Waiting-for-cooling presentation is informational.
- The Control CPU alone decides the starting stage and skipped stages. For a hot start the HMI shows the measured temperature, proposed stage, skipped stages, skipped Hold durations, warnings and explicit Confirm/Cancel, all from the shared string catalogue (REQ-FUN-019).
- A confirmation is bound to the controller session, start transaction, program revision and proposed stage; expired, duplicated, stale or mismatched confirmations never start heating (PROTO-012).
- Operator-entered text is UTF-8, including Cyrillic (REQ-FUN-020).
- **Start again** after a stopped run is an explicit Start request evaluated by the controller like any other Start; while the furnace is hot it may produce a hot-start proposal or a rejection. This amends the 2026-10-06 "from the start, at once" wording.

Proposed (not authority to implement): the message set and reason codes of the shared specification 0.1.0; a future operator-selected starting stage; an informational cooling-time estimate; mandatory non-skippable stages. These are recorded in the [program and startup model](../03-hmi/PROGRAM_AND_STARTUP_MODEL.md#explicit-start-and-hot-start) and the furnace START-04, START-06 to START-08.

## Consequences

- New presentation work: proposal view, confirmation dialog, rejection reasons, informational waiting state, new catalogue strings in every supported language.
- The labs' current immediate **Start again** and "accept every start" simulator behavior no longer match the requirements; they are recorded as gaps in [traceability](../06-testing/TRACEABILITY.md) and changed only in a separately approved implementation task.
- Conflicts recorded, not changed here: a first-stage HOLD "delayed start" preview (this repository) versus the furnace rule that the first stage is a Heating stage; REQ-FUN-017's authored COOL and REQ-FUN-005's six-hour Manual default versus the furnace decisions of 2026-10-16. They need separate reconciliation.

## Verification and follow-up

TEST-011 defines the HMI tests. Controller-side tests (temperature below, at and above Hold targets, boundaries at the tolerance, multi-stage skips, above the final target, confirmation expiry and mismatch, reconnect, power-loss recovery) belong to the furnace repository. Update the shared specification version reference when it changes.

## Review record

Mandated product decision of 2026-10-16. HMI domain review and decision-challenger review of the presentation design and of the 0.1.0 message draft are still required before implementation.
