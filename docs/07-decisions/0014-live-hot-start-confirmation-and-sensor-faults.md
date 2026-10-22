# ADR-0014: Live hot-start confirmation, sensor-fault and degraded-status presentation

- Status: Accepted (product-owner mandate) for the behavior below; message details remain a Proposed shared draft
- Date: 2026-10-16
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-005, REQ-FUN-019, REQ-FUN-021, REQ-FUN-022, REQ-FUN-023, PROTO-012, TEST-011; ARCH-INV-002, ARCH-INV-003; furnace START-03, START-05, START-09, START-11, SENS-01 to SENS-06, MAN-01
- Amends: [ADR-0013](0013-explicit-start-and-hot-start.md) (the confirmation has no normal timeout)
- Amended by: [ADR-0018](0018-fault-manager-and-hold-approach-presentation.md) (a failure of a required sensor is a latched critical fault)

## Context

**Fact:** on 2026-10-16 the product owner finalized the hot-start and sensor-fault decisions for both projects. Controller requirements are in the furnace repository (`docs/requirements/requirements.md`) and message semantics in its interface specification `docs/interfaces/controller-hmi-interface.md`, now version 0.2.0 (draft).

**Fact:** this repository's protocol requirements already provide controller session identity, stale-data and restart detection, prioritized state/fault events and fast telemetry near 1 Hz (PROTO-002, PROTO-006 to PROTO-008). A link-liveness threshold, the temperature reporting interval and transport evidence (ADR-0006 proposed, ADR-0007 deferred) are not yet defined.

## Decision drivers

- An operator must be able to read and decide on a hot-start proposal without an arbitrary deadline.
- A confirmation must never rest on stale or unverified information.
- Sensor faults must be visible, structured and localizable; degraded operation must never look normal.

## Options considered

1. **Live proposal without a normal timeout, invalidated when live data cannot be assured (selected).** Uses the existing session, stale-detection and telemetry requirements.
2. **Fixed confirmation timeout (for example 60 s).** Rejected by the product owner as normal behavior; kept only as a configurable fallback where link liveness cannot be assured.

## Decision

Mandated (product owner, 2026-10-16):

- A hot-start confirmation stays open while valid live data and a live controller link exist. The HMI shows the current temperature with validity, refreshed at the defined reporting interval; when the controller publishes a changed proposal (new proposal revision), the HMI shows the change clearly and the operator must confirm the new proposal. Confirm always refers to the proposal revision on screen (REQ-FUN-019).
- The HMI never starts anything when the temperature becomes eligible; a waiting display is informational (REQ-FUN-018).
- Telemetry or link loss is shown as such; Confirm is disabled while data is stale or unavailable. If the controller invalidates the proposal, or a configured fallback timeout applies, the HMI informs the operator with the reason and closes the confirmation; it never dismisses it silently or shows it as accepted (REQ-FUN-021).
- Sensor faults arrive as structured controller events (channel, classification, consequence, required action) and are shown as localized warnings; degraded operation is shown distinctly from normal operation; acknowledging a warning never bypasses a fault, and a cleared fault never restarts heating (REQ-FUN-022).
- A cooling-time estimate is optional and informational; when unreliable, "no reliable estimate" is shown (REQ-FUN-023).
- Manual: the six-hour value in REQ-FUN-005 is the default session selection; the controller's configurable maximum (initially 48 h) bounds every selection.

## Consequences

- New catalogue strings and presentation states in both presentations: live proposal, changed proposal, invalidated, link/telemetry loss, sensor-fault warnings, degraded mode.
- Implementation gaps, recorded in [traceability](../06-testing/TRACEABILITY.md), not changed here: the labs start without hot-start checks; authored COOL stages; leading delayed-start Hold; 8-stage capacity versus the controller's 18; 32-character / 40-byte name handling versus 31 code points / 124 bytes; missing Cyrillic glyphs (ADR-0012). The furnace repository's `docs/architecture/hmi-reference.md` lists them as HMI-DIFF-02, -03, -06, -07 and -08 (plus -01 for the stage count).

## Verification and follow-up

Extend TEST-011 with live-update, changed-proposal, invalidation, telemetry-loss, sensor-fault and degraded-mode presentation cases. Define the liveness threshold and telemetry interval with the protocol work (ADR-0007).

## Review record

Mandated product decision of 2026-10-16. HMI domain review and decision-challenger review of the presentation design and of the shared 0.2.0 message draft are required before implementation.
