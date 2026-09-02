# ADR-0003: Separate protocol concerns

- Status: Accepted principle
- Date: 2026-08-24
- Governing requirements: PROTO-001 through PROTO-009

## Context

The production link is expected to use UART/RS-422 while desktop integration
needs memory/loopback transport. Native C layout is unsafe as a cross-CPU wire
contract, and transport success must not be confused with application success.

## Decision

Separate application semantics and lifecycle, serialized representation,
framing/integrity, and physical transport. Portable HMI/application state
consumes semantic values, not frames or UART buffers. Transport reports
transport facts and does not invent controller acceptance.

This is a dependency and responsibility decision only. It does not select a
wire schema, message identifiers, serialization library, framing algorithm,
integrity mechanism, retry scheme, or concrete C interfaces.

## Consequences

- Production and simulator transports can exercise the same upper behavior.
- Codecs and framers can be tested with deterministic vectors and fault
  injection independently of a UART driver.
- Layer boundaries add interfaces that must remain narrow; empty ceremony is
  not required for operations that do not need it.

## Verification and follow-up

Resolve [serialization/framing](0007-serialization-and-framing.md) and
[source sharing](0009-protocol-source-sharing.md) with measured constraints.
Protocol tests must cover lifecycle, versioning, duplicate/retry, corruption,
and session-change behavior.

## Review record

The separation principle is approved by the project brief. Exact lower-layer
choices remain explicitly deferred.
