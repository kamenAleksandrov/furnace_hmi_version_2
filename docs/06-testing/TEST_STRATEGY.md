# Test strategy

Testing follows the authority and failure boundaries rather than only nominal
screens. The foundation supplies portable host tests and Zephyr-compatible test
entry points; production suites grow with implementation.

## Test layers

**Portable unit tests** cover model, application, codec, and other platform-free
C. They should run quickly on the host and avoid LVGL or a physical UART unless
that dependency is the subject under test.

**Component tests** cover controller-client session/cache behavior, bounded
queues, presentation formatting, and adapter contracts with fakes.

**Desktop integration tests** use the semantic simulated controller for UI
scenarios and later use the actual codec/framer over loopback. They inject
disconnects, latency, corruption, dropped/duplicate data, and controller
session changes.

**Zephyr tests/build proofs** verify Kconfig/devicetree integration, compiler
constraints, target adapters, threading primitives, and declared boards. Use
Zephyr's test tooling where suitable.

**System/contract tests** eventually run against the real Control CPU contract
and hardware, particularly priority, timing, recovery, and safety-boundary
observations. They do not attempt to validate controller safety solely through
the HMI.

## Essential scenarios

- controller rejects syntactically valid but operationally illegal input;
- link fails with commands pending and later reconnects without replay;
- new HMI reconstructs from a snapshot after missing events;
- controller boot/session changes while caches and responses are in flight;
- duplicate, timeout, late, rejected, and long-running request outcomes;
- telemetry overload cannot obstruct important events or Stop interaction;
- only the GUI context touches LVGL and its work remains bounded;
- editing creates a new revision while an active accepted revision stays fixed;
- history remains immutable and reuse creates a draft;
- planned graph and executed semantics agree after stage rules are defined;
- stale/unavailable/estimated values are presented honestly.

## Evidence

Record the exact command, target/configuration, result, and relevant artifact.
Do not report a check as passed when it was not run. Generated test output is a
build artifact unless a specific report is intentionally retained. The
[developer runbook](../05-platform/DEVELOPER_RUNBOOK.md) is the copy/paste
command reference for the supported test paths.
