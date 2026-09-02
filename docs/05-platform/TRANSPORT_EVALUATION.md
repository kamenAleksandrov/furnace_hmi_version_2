# Transport evaluation

## Known baseline

The expected physical connection is full-duplex, point-to-point RS-422 between
one HMI CPU and one Control CPU, over a likely few-metre cable in an industrial
environment. The baud rate is open and expected to be materially above 9600.
Exact UART/peripheral selection and software-visible transceiver behavior are
also open.

These facts justify a UART/RS-422 production adapter but do not define the
application protocol. [ADR-0006](../07-decisions/0006-transport-selection.md)
tracks the still-proposed transport decision.

## Evaluation criteria

Later selection and measurement should cover:

- sustained and burst throughput for commands, telemetry, programs, and
  history without starving important events;
- latency and bounded Stop-command handling;
- corruption, loss, partial-write, and reconnect behavior;
- buffering, backpressure, DMA/interrupt/thread ownership, and memory budget;
- detection of peer disappearance and controller boot/session change;
- devicetree/Kconfig representation across candidate boards;
- software-visible transceiver enable/direction details, if any;
- diagnostics and fault-injection support.

## Required abstraction

The transport port moves framed bytes and reports transport-level outcomes. It
must not claim application acceptance or expose a fixed UART to reusable HMI
code. A memory/loopback implementation must exercise the same upper protocol
layers in desktop tests.

No baud rate, UART number, driver API, or buffering policy is selected here.
