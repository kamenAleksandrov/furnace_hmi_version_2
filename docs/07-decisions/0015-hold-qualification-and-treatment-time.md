# ADR-0015: Hold qualification and treatment-time presentation

- Status: Accepted (product-owner mandate) for the behavior below; field list Proposed in the shared draft
- Date: 2026-10-18
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-024, PROTO-012, TEST-012; REQ-FUN-011, REQ-FUN-013; ARCH-INV-002; furnace PROF-03, PROF-07, PROF-09 to PROF-13, START-09, NFR-07

## Context

**Fact:** on 2026-10-18 the product owner approved the Hold timing policy for both projects. A Hold's programmed duration does not count from stage entry: treatment timing starts only after the measured temperature is inside the configured treatment band around the Hold target and has met a configurable stability qualification. Time spent approaching or qualifying is never credited. A Hold entered directly by a hot start still receives its full programmed duration after qualification. Controller requirements are in the furnace repository (`docs/requirements/requirements.md`, PROF-03 and PROF-09 to PROF-13); the Hold progress fields and their mapping to presentation states are in its interface specification `docs/interfaces/controller-hmi-interface.md`, section "Hold progress", version 0.4.0 (draft).

**Fact:** the treatment band is a separate quantity from the hot-start entry tolerance (furnace START-03, ±5.0 °C provisional); its value, the stability interval and any filtering remain provisional. The response to a deviation after qualification is unresolved (furnace PROF-12); until it is approved, time outside the approved conditions is never credited and such a Hold never completes (PROF-11, PROF-13).

**Fact:** the labs show a run-level "Remaining" reading supplied by the simulator; they have no Hold qualification state and no credited-treatment-time presentation.

## Decision drivers

- An operator must never read unqualified time as treatment.
- The controller is the only source of phase, elapsed time and credited time (REQ-FUN-011, ARCH-INV-002).
- Every label is localizable through the shared catalogue (REQ-FUN-013, [ADR-0012](0012-theme-and-language-catalogues.md)).

## Options considered

1. **Controller-owned Hold phase and times, displayed as received (selected).** Matches the authority boundary and the approved policy.
2. **HMI-computed countdown from stage entry.** Rejected: contradicts the approved policy and would show unqualified time as treatment.

## Decision

Mandated (product owner, 2026-10-18):

- During a Hold the HMI distinguishes six presentation states derived from the controller's Hold progress: approaching Hold temperature; waiting for stable temperature; stability qualification in progress; active Hold treatment; treatment interrupted or out of tolerance; Hold completed (REQ-FUN-024).
- The treatment countdown appears only in the active-treatment state and shows the controller's remaining treatment time. Before qualification no countdown is shown.
- Elapsed stage time and credited treatment time are always distinguishable; neither is computed by the HMI.
- An interrupted treatment is shown as such, with the controller's condition; the HMI never presents it as progressing or completed and never resumes it on its own.
- Labels and messages are semantic string IDs with typed parameters (durations as numbers) in the shared catalogue, in both presentations. Catalogue entries are added with the implementation, not by this decision.

## Consequences

- New presentation states and catalogue strings on the Running screen and in the stage list of both presentations.
- Implementation gap, recorded in [traceability](../06-testing/TRACEABILITY.md): the labs have no Hold presentation states or credited time, and their simulated run-level remaining time does not account for qualification time; a run-level remaining time is an estimate until every Hold is qualified.
- The existing references to interface version 0.2.0 in this repository (PROTO-012, `COMMAND_LIFECYCLE.md`, `PROGRAM_AND_STARTUP_MODEL.md`, CTRL-START-HOT, ADR-0014) are left unchanged by this decision: the 0.3.0 start-transaction changes in between need their own review before those references move.

## Verification and follow-up

TEST-012. The deviation-after-qualification policy (furnace PROF-12), the treatment band, the stability interval and their filtering rules are open; the presentation of a recovered or requalified treatment is defined when PROF-12 is approved.

## Review record

Mandated product decision of 2026-10-18. HMI domain review and decision-challenger review of the presentation design and of the shared 0.4.0 field list are required before implementation.
