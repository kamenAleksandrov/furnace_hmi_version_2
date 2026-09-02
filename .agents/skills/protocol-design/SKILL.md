---
name: protocol-design
description: Design or review Furnace HMI V2 communication requirements, semantic contracts, command lifecycles, synchronization, framing, and transport profiles. Use for any HMI and Control CPU interface work, reconnect behavior, retries, state snapshots, or protocol test planning.
---

# Protocol Design

## Establish the boundary

1. Read `docs/01-architecture/BOUNDARIES_AND_INVARIANTS.md`, `docs/01-architecture/STATE_OWNERSHIP.md`, `docs/02-protocol/PROTOCOL_REQUIREMENTS.md`, and relevant ADRs.
2. Treat the HMI as an untrusted external requester. The Control CPU revalidates every meaningful request and remains the authority.
3. Keep furnace meaning independent of the physical link while documenting transport capabilities such as stream or datagram behavior, MTU, ordering, reliability, latency, and duplex properties.

## Design semantics before bytes

1. Define typed intents, authoritative observations, responses, events, snapshots, and faults before choosing serialization or framing.
2. Keep delivery, parse outcome, acceptance or rejection, application, and completion distinct. Transmission success is never execution success.
3. Define controller boot or session identity, state revision, request correlation, pending-operation invalidation, freshness, full resynchronization, version negotiation, and capability negotiation.
4. Never replay stale operator intent after reconnect or either endpoint reboot. Permit retries only with explicit idempotency and deduplication semantics; otherwise query and resynchronize.
5. Require an authoritative snapshot sufficient to reconstruct the HMI without relying on event history.
6. Prioritize alarms and operational commands over coalescible telemetry. Make every queue, payload, parser, timeout, and retry policy bounded.

## Select encoding and transport deliberately

Do not place raw C structs on the wire. Specify fixed-width fields, units, bounds, byte order, compatibility, integrity, malformed-input behavior, and transport-specific profiles only after their requirements are known. A checksum detects accidental corruption; it does not provide authenticity.

## Make the contract independently testable

Plan golden vectors and negative cases for valid messages, boundaries, truncation, corruption, unknown versions and types, duplicates, timeouts, sequence wrap, disconnects, endpoint reboot, resynchronization, and unsupported capabilities. Require an independent oracle or independently authored endpoint test so shared code cannot merely agree with itself.

Record normative behavior only in `docs/02-protocol/`; use ADRs for choices and `docs/09-progress/OPEN_QUESTIONS.md` for unresolved inputs. Do not invent a wire schema or furnace command inventory unless that design phase is explicitly approved.
