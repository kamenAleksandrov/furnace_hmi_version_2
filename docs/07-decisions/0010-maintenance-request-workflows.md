# ADR-0010: Separate maintenance actions and controller restart assessment

- Status: Proposed; product flow requested, production semantics not accepted
- Date: 2026-09-14
- Governing requirements: REQ-FUN-002, 003, 007, 015; ARCH-INV-001, 002, 003,
  004, 007, 009, 010; PROTO-001, 002, 003, 008, 009

## Context

The operator requests a Settings maintenance tab for resetting preferences,
deleting programs, restarting the display, and attempting a furnace restart.
The requested restart interaction first asks the controller whether restart is
appropriate now, then presents a confirmation for an affirmative response,
a busy response, or no response. Busy/unknown branches still offer an attempt.

## Proposed decision

Keep four distinct actions with explicit owner, scope, confirmation, and
outcome. Interpret furnace restart confirmation as authorization to send one
controller-validated request, including after a busy assessment or timeout.
It is never a force-reset permission, a power-cycle instruction, or a reason
to bypass a final controller rejection. The controller revalidates at execution
because circumstances may change while the confirmation dialog is open.

The [Settings proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md#maintenance)
owns the proposed UX, copy, action scope, and request/recovery behavior. It is
non-normative until domain/protocol review. Before production implementation,
record agreed normative semantics in `docs/02-protocol/`, update requirements
and ownership/traceability, and complete the required ADR reviews.

## Alternatives and consequences

- **Proposed: assess, confirm, request, revalidate.** Preserves the requested
  three-branch UX and controller authority. Requires explicit assessment versus
  final-rejection semantics and honest unknown-outcome presentation.
- **Disable every attempt after busy/unknown.** Simpler but removes the user's
  requested opportunity to submit a fresh attempt for controller evaluation.
- **Force reset after confirmation.** Conflicts with ADR-0001 and ARCH-INV-001,
  002, 007: the HMI would bypass controller validation and could interrupt
  furnace work. Not included; requires explicit architecture/requirement
  resolution and a separately justified hardware/service design.

## Open details and verification

Controller restart transition/recovery, persistence guarantees, assessment
freshness, correlation, deadlines, duplicate handling, and supported request
path must be defined by the controller/protocol contract. No numeric timeout,
wire command, restart guarantee, or hardware reset path is selected here.
Program deletion needs controller-defined scope and atomicity; preference
reset needs approved defaults and an explicit field list.

Verify cancellation sends no action, duplicate confirmation sends once,
assessment cannot authorize stale execution, final rejection is respected,
unknown outcomes cause resynchronization rather than replay, and a new
controller session reconstructs the current view. Test local reset/storage
failure and display-restart recovery independently of furnace restart.

## Review record

Drafted for the requested planning update. No acceptance review or production
implementation is claimed. The original authority and navigation rules remain
in force.
