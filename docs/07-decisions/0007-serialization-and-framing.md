# ADR-0007: Select serialization, framing, and integrity

- Status: Deferred
- Date: 2026-08-24
- Governing requirements: PROTO-001 through PROTO-009, TEST-004

## Context

The wire format must be explicit, bounded, versionable, robust to corruption
and duplicates, and independent of native C layout. Exact message semantics,
payload sizes, memory budgets, throughput, error environment, and compatibility
expectations are not yet defined well enough to make a defensible selection.

## Decision to defer

Do not select or implement serialization, framing, integrity checking, message
identifiers, version negotiation, or retry representation during repository
foundation work. Raw C structures are prohibited on the wire in the meantime.

## Options to evaluate later

Evaluate bounded hand-written or generated representations and suitable framed
binary approaches against explicit schemas, tooling on both CPUs, code/RAM
cost, deterministic bounds, evolution rules, diagnostic quality, and golden
test-vector support. Select an integrity mechanism based on the physical error
model; do not assume framing and integrity are the same concern.

## Consequences

Foundation code may define abstract semantic/transport ports but must not
crystallize a temporary byte format. Protocol feature implementation waits for
the required evidence and accepted ADR.

## Evidence required to decide

Approved semantic operations and lifecycle, target memory/flash budgets,
maximum payload classes, measured link characteristics, compatibility/version
policy, duplicate/retry model, and cross-CPU implementation constraints.

## Review record

Protocol-reviewer, embedded-reviewer, and decision-challenger review are
required before acceptance.
