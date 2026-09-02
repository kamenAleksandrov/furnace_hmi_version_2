# ADR-0006: Select the production transport

- Status: Proposed
- Date: 2026-08-24
- Governing requirements: PROTO-005 through PROTO-007, REQ-NFR-002, REQ-NFR-003

## Context

Known product assumptions describe a full-duplex, point-to-point connection
between one HMI and one Control CPU, probably a few metres in an industrial
environment. RS-422 is expected. Baud rate, UART/peripheral, software-visible
transceiver behavior, buffering, and production target hardware remain open.

## Proposal

Use a Zephyr UART-backed full-duplex RS-422 adapter for production and an
in-memory/loopback adapter for desktop integration. Keep application semantics,
codec, and framing independent of the adapter. Supply UART and transceiver
facts through devicetree/Kconfig/board configuration rather than portable code.

No baud rate, UART instance, DMA policy, buffer size, or transceiver-control
behavior is proposed yet.

## Alternatives to evaluate

- UART/RS-422 with interrupt-driven buffering;
- UART/RS-422 with asynchronous/DMA support where target drivers permit;
- an alternate physical link only if product hardware or measured reliability
  evidence invalidates the expected baseline.

## Consequences if accepted

The design gains a conventional deterministic point-to-point physical path and
testable host substitution, while target driver capabilities and buffering may
constrain latency/memory. Industrial physical-layer robustness is not proven by
software selection alone.

## Evidence required for acceptance

Confirm the first target's UART driver/API, software-visible transceiver
signals, throughput/latency budget, error reporting, queue/backpressure design,
memory budget, and fault-injection results. Resolve the relevant items in
[transport evaluation](../05-platform/TRANSPORT_EVALUATION.md).

## Review record

Domain and decision-challenger review are required before acceptance.
