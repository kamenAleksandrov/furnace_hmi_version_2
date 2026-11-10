# ADR-0006: Select the production transport

- Status: Proposed (production link); development link accepted 2026-11-10 (product owner)
- Date: 2026-08-24; amended 2026-11-10
- Governing requirements: PROTO-005 through PROTO-007, REQ-NFR-002, REQ-NFR-003

## Context

Known product assumptions describe a full-duplex, point-to-point connection
between one HMI and one Control CPU, probably a few metres in an industrial
environment. RS-422 is expected. Baud rate, UART/peripheral, software-visible
transceiver behavior, buffering, and production target hardware remain open.

## Development link (accepted 2026-11-10, product owner)

Until the boards are available, the HMI and the controller run as two Zephyr
native_sim programs on one PC, joined by a simulated serial cable (a
pseudo-terminal pair). The HMI's transport is a byte-stream port: the same
client and codec later run over a USB serial connection between two boards and
over the production link. See the [Phase 21 plan](../09-progress/PHASE21_CONTROLLER_CLIENT_PLAN.md).

**Production link: open.** The product owner reports that RS-422, RS-485,
Modbus and Ethernet are all viable and that the electrical engineer has not
decided (furnace HW-BLOCK-006). What each would mean for the software:

- **full-duplex serial** (RS-422, 4-wire RS-485, USB serial): the byte-stream
  port carries the framed messages directly;
- **2-wire RS-485** is half-duplex: direction control and turnaround, and
  either side sending unprompted (the controller pushes `STATUS` and events)
  would collide, so it needs a polling or arbitration scheme: a protocol
  change, not only an adapter;
- **Ethernet (TCP)** is a byte stream: another adapter beneath the same
  framed messages;
- **Modbus** is master/slave and cannot carry pushed status: a protocol
  decision of its own (ADR-0007 reopen trigger).

**Test levels:** the pseudo-terminal adds an integration level above ADR-0004's
in-memory loopback; it is lossless and untimed, so fault injection stays in the
loopback tests. How the two native_sim pseudo-terminals are joined (for example
a bridge in WSL) is checked in Phase 21's M21-3 (formerly M21-4).

## Original proposal (2026-08-24; its link assumption superseded above)

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
