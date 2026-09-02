# Assumptions and constraints

## Accepted requirements, decisions, and constraints

The following items are normative; they are not working assumptions:

- **Decision:** production code is C and the runtime is pinned to Zephyr 4.4.0
  by [ADR-0002](../07-decisions/0002-zephyr-4-4-0.md).
- **Requirement:** LVGL is the UI framework and only one designated GUI context
  may access its objects (`ARCH-INV-005`).
- **Requirement:** one source project supports target-specific builds; it is
  not one portable binary for unlike MCU architectures (`REQ-NFR-005`).
- **Requirement:** Zephyr devicetree, Kconfig, board configuration, drivers,
  and adapters carry hardware differences. Portable HMI code must not encode a
  fixed UART, display, touch controller, resolution, or vendor-only API
  (`REQ-NFR-009`).
- **Requirements:** the HMI cannot be a safety dependency and cannot
  authoritatively validate a furnace operation (`ARCH-INV-002`,
  `ARCH-INV-007`).
- **Requirement:** Celsius is the initial product display unit. The internal
  model must permit future presentation conversion, but Fahrenheit is not
  implemented now (`REQ-FUN-014`).
- **Requirement:** English is the initial implemented language. Visible strings
  require a localization boundary rather than distribution through screen
  logic (`REQ-FUN-013`).
- **Requirement:** fast telemetry starts near 1 Hz and slow counters near five
  seconds, with configurable rates rather than fixed wire constants
  (`PROTO-007`).
- **Decision:** SDL2 is the desktop host-driver baseline unless stronger
  evidence supports another driver
  ([ADR-0004](../07-decisions/0004-desktop-simulator.md)).
- **Project constraint:** V1 `.prg` import/backward compatibility is not
  required unless explicitly added later.

## Current working assumptions

These guide exploration but are not final design decisions:

- ESP32-P4 and STM32 are likely target families; neither is yet the selected
  first production target.
- The physical HMI-to-controller link is expected to be point-to-point,
  full-duplex RS-422 over a few metres in an industrial environment.

## Evidence discipline

Do not convert a working assumption into accepted behavior by implementing it.
Resolve consequential choices through the [ADR process](../07-decisions/README.md)
and update [open questions](../09-progress/OPEN_QUESTIONS.md).
