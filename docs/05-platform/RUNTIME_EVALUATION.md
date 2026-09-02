# Runtime and target evaluation

## Accepted baseline

Production HMI software uses C, Zephyr 4.4.0, and LVGL. The pin is recorded in
[ADR-0002](../07-decisions/0002-zephyr-4-4-0.md). The effective compiler C
standard must be recorded from the configured Zephyr 4.4.0 build rather than
guessed or forced independently; see
[development environment](DEVELOPMENT_ENVIRONMENT.md).

Zephyr is the application runtime. A previously installed ESP-IDF environment
is not an architectural dependency. Espressif support may still cause Zephyr's
own board/toolchain workflow to use vendor components internally; if so, that
is a Zephyr implementation dependency and must be documented explicitly.

## Target strategy

The product is one source project with separate target-specific builds. Likely
families include ESP32-P4 and STM32, but the first production MCU/board is open.
The application must not require redesign when a supported target changes.

Use Zephyr-native boundaries:

- devicetree for peripherals, buses, display/touch, GPIO, storage wiring,
  interrupts, external memory, and watchdog hardware;
- Kconfig/project configuration for features, logging, software buffers,
  optional modules, tests/debug, and LVGL configuration;
- board definitions and overlays for supported hardware combinations;
- narrow platform adapters for behavior that is genuinely target-specific.

Portable code must not encode a UART number, display/touch model, fixed
resolution, or ESP32-/STM32-only API.

## Selection evidence still needed

Before naming a production target, record software-facing flash/RAM/external
memory, display/touch interface and resolution, storage, UART/RS-422 behavior,
RTC/time source, watchdog capability, debug/programming path, supported Zephyr
drivers, toolchain maturity, and measured build/runtime budgets. PCB and power
electronics design remain outside this software repository.
