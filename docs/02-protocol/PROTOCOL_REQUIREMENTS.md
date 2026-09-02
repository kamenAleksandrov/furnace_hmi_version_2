# Protocol requirements

The protocol is not implemented and no wire schema is approved. The normative
constraints are `PROTO-001` through `PROTO-009` in the
[requirements register](../00-project/REQUIREMENTS.md#protocol-requirements).
This document frames the later design work without selecting serialization,
framing, identifiers, packet sizes, or UART parameters.

## Required outcomes

The eventual protocol must let the HMI:

- establish communication and identify the current controller boot/session;
- obtain capabilities, setting metadata, and enough authoritative state to
  reconstruct the operator view after restart or missed events;
- submit requests and distinguish rejection, acceptance, resulting state, and
  completion when relevant;
- receive important state/fault events without starvation by telemetry;
- detect stale data and a controller restart;
- transfer larger program/history/diagnostic data without impairing important
  controller work;
- run over production and in-memory simulator transports.

## State recovery baseline

The design shall investigate controller boot/session identity, monotonic state
revision, run and program identity/revision, mode, phase, temperature validity,
setpoint, demand, faults, readiness, capabilities, and setting metadata. This
is a list of required concepts to evaluate, not a message definition.

Reconnect follows this semantic sequence:

1. establish a new communication relationship;
2. determine controller/session identity;
3. invalidate incompatible pending requests;
4. mark prior cache data stale;
5. request authoritative state;
6. reconstruct presentation;
7. resume ordinary traffic.

No step authorizes automatic replay of a pre-disconnect command.

## Traffic behavior

- Immediate/event-driven: command outcomes, state transitions, faults, and
  important lifecycle changes.
- Fast periodic: initially near 1 Hz for temperature, setpoint, run state,
  demand, and live graph samples.
- Slow periodic: initially near 5 seconds for counters and slow statistics.
- On request/very slow: detailed diagnostics and history.

Rates are configurable policy, not permanent protocol constants. Appropriate
telemetry may be dropped or coalesced; important outcomes and state/fault
events must retain priority.

## Deliberately unresolved

Serialization, framing, integrity mechanism, retry windows, transaction IDs,
compatibility/version negotiation, exact snapshot partitioning, and transport
parameters require evidence and ADRs. See the
[deferred serialization ADR](../07-decisions/0007-serialization-and-framing.md).
