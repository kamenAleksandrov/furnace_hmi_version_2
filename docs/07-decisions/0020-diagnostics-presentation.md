# ADR-0020: Diagnostics presentation: controller-backed Device log, health dashboard and recording schedules

- Status: Accepted (product-owner mandate) for the behavior below; operations, fields and screen placement Proposed
- Date: 2026-10-26
- Owners: product owner; HMI and Control CPU maintainers
- Governing requirements: REQ-FUN-029, TEST-017, REQ-FUN-010, REQ-FUN-011, REQ-FUN-027, ARCH-INV-001, ARCH-INV-004, PROTO-007; furnace DIAG-06…DIAG-14, API-10, API-01, FAULT-07
- Relates to: [Device screen proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md), [ADR-0018](0018-fault-manager-and-hold-approach-presentation.md) (fault entries and history)

## Context

**Fact:** on 2026-10-26 the product owner required a comprehensive diagnostic system for the finished controller. The furnace repository records this in `prompts/18-p14-review-merge-logging-audit.md` and the audit `docs/architecture/diagnostics-logging.md`. The controller will record three things:

- events when they happen;
- periodic telemetry streams, each with its own interval;
- health information, with separate check, snapshot and publication periods (furnace DIAG-06…DIAG-14).

Clients read health status and paginated history and update permitted recording preferences through furnace API-10. The owner approved that logging and health monitoring have separately configurable timing, and that different periodic logging streams have independent recording intervals.

**Fact:** the owner also required the HMI to include diagnostic functionality, not merely receive events:

- an event-log viewer;
- a system-health dashboard;
- separate recording-schedule controls validated by the controller.

There are no accounts or authorization tiers.

**Fact:** the labs already have a Device screen (Overview, Device log, Maintenance, Service history, Device info) against simulated data, with a one-month simulated Device log. There is no health dashboard and no recording-schedule control. Controller work is in the backlog as CTRL-EVENTS and CTRL-EVENT-LOG. The furnace plan schedules the following:

| Furnace item | Phase | Content |
|---|---|---|
| WI-I08 | 20 | Basic diagnostic API |
| WI-H12 | 21 | Basic controller-backed Device log and health overview |
| WI-I09 | 24 | Complete diagnostic API |
| WI-H13 | 25 | Full viewer, dashboard and recording schedules |

## Decision drivers

- The controller owns history, health, counters, clock and diagnostic settings (ARCH-INV-001, REQ-FUN-010, REQ-FUN-011).
- A restarted HMI reconstructs from controller data, never from its own history (ARCH-INV-004).
- Unknown or unmonitored parts are never shown as healthy (Device screen proposal).
- Large history reads must not starve commands, faults or status (PROTO-007).

## Options considered

1. **The HMI keeps its own log of received events.** This works without a history query, but it loses everything recorded while the link was down or the HMI was off, and contradicts ARCH-INV-004.
2. **The controller history, read on demand (selected).** One authoritative record; works after restarts and link loss; needs paged queries.

## Decision

- **Device log.** Becomes a viewer of controller history:
  - filters by category, severity, time range and run/session;
  - occurrence counts and fault details;
  - paged browsing bound to a controller history revision, so pages never mix;
  - active and historical faults distinguished;
  - export where the controller supports it.

  The one-month window of the labs is a presentation assumption, not a retention decision (furnace SD-22).
- **System health.** Overall controller status and subsystem health, each with validity: sensors, actuator, fan where reported, link, storage, runtime. Current warnings and faults, runtime and communication diagnostics, and relevant maintenance statistics are shown alongside. An unknown or unmonitored subsystem is shown as unknown. **Proposal:** extend the Device Overview and add a health category only if the content does not fit; placement is decided with the implementation in both presentations.
- **Recording schedules.** Separate controls for:
  - health reporting;
  - periodic temperature logging;
  - control-performance logging;
  - other controller-supported streams;
  - the HMI's own history display interval.

  Values, bounds and writability come from the controller, which validates every change (furnace DIAG-13). Engineering-only items are shown read-only. Any operator may change the operator-adjustable items; there are no accounts.
- **Link loss.** Diagnostics are shown as stale. The HMI never implies that recording stopped, because the controller keeps recording (furnace DIAG-14). The HMI never synthesizes history or health it did not receive.
- **Still open:** fields and messages (furnace interface specification, with WI-I08 and WI-I09); retention (furnace SD-22); which settings operators may change and the defaults (SD-23); export path (SD-25).

## Consequences

- The simulated Device log and Overview are replaced in Phase 21 (furnace WI-H12) and completed in Phase 25 (WI-H13). Web demo and LVGL parity apply.
- Recording-schedule controls are controller-owned settings under REQ-FUN-010, not HMI preferences. Only the HMI history display interval is HMI-owned.
- The HMI's own health supervision (REQ-NFR-008, ARCH-INV-011) stays separate from controller health. The dashboard may show HMI link state but does not merge the two.

## Verification and follow-up

- TEST-017 scenarios from scripted controller diagnostic data in both presentations; then integration against the furnace diagnostic API.
- Update this ADR when the furnace interface specification defines the diagnostic operations.

## Review record

Documentation only, from the owner's request. No UI, simulator, firmware, font or build file changed. Awaiting owner review of the planning pull requests in both repositories.
