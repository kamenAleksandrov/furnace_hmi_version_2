# ADR-0011: Render configured component service information generically

- Status: Proposed
- Date: 2026-09-14
- Governing: REQ-FUN-001, 010, 011, 016; REQ-NFR-004, 006, 009;
  ARCH-INV-001, 004, 005, 007; PROTO-008, 010; ADR-0001, ADR-0005

## Context

The operator wants developer-configured components and manufacturer-based
service tasks to appear on Device without writing a new screen for each part.
Workers need runtime/maintenance/error visibility without editing operational
policy or clearing maintenance obligations.

## Proposed decision

Use a bounded, declarative product component/task catalogue and generic Device
rendering. The controller owns counter accumulation, installed-instance and
service baselines, due-state evaluation, permissions, and persisted history.
The worker Device view is initially read-only. Use stable IDs and catalogue
revision/session metadata; product configuration controls which components exist.
Hardware selection remains in Zephyr/board/adapters. Catalogue definition is
not a pin map or HMI-owned policy engine.

The [Device proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md) owns the draft UX and
model detail. A code-owned manifest versus controller-exposed catalogue
transport remains open; neither permits divergent authoritative interval tables.
No wire schema or new controller commands are selected here.

## Alternatives

- **Generic configured catalogue (recommended):** adding an ordinary component
  is a data/localization change. New counter kinds still require controller
  support; the data table cannot manufacture measurements.
- **Individual screens and hard-coded branches:** simple for one part, but
  duplicates layout and grows with every component and task.
- **Worker-editable intervals/reset buttons:** convenient service editing but
  conflicts with this requested worker scope. A separately authorized service
  workflow can be designed later without putting it on the read-only page.
- **HMI accumulates counters and evaluates authoritative due policy:** violates
  controller ownership and makes restart/link loss corrupt service truth.

## Consequences and acceptance gate

Define per-counter semantics, source/validity, replacement identity, calendar
behavior, migration, bounds, pagination, and service-gate relationships before
production. Manufacturer references must be verified for actual selected parts;
example values are not approved service intervals. Unknown data must not look
healthy. Review performance with a full supported catalogue and stale/replaced
sessions. Record accepted normative changes in requirements, ownership, and
protocol documents with traceability after the required domain and
decision-challenger reviews. This draft is not an accepted architecture change.
