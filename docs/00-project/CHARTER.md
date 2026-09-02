# Project charter

## Purpose

Build a maintainable HMI software platform for a furnace system. The HMI CPU
runs Zephyr, LVGL, the operator interface, local drafts and preferences, a
communication client, and HMI health monitoring. It is a sophisticated remote
operator interface, not the source of furnace truth.

The Control CPU/Furnace Controller owns furnace state, execution, validation,
timing, control, actuators, interlocks, faults, operational counters, persisted
programs, and authoritative time. [ADR-0001](../07-decisions/0001-authority-boundary.md)
records this mandated boundary.

## Foundation outcome

The foundation establishes:

- a Zephyr 4.4.0 project that can evolve toward multiple supported boards;
- portable HMI source boundaries and target-specific adapters;
- a project-owned LVGL desktop simulator;
- repeatable build, test, editor, and code-indexing infrastructure;
- traceable requirements, ADRs, review guidance, and progress records.

## Current non-goals

This phase does not implement furnace control, program or Manual execution,
heater/fan/PID policy, controller commands, production protocol semantics,
furnace state machines, or production furnace screens. It also does not design
PCB, power electronics, mechanical details, EMC, isolation, protection, or
heater circuitry.

Hardware facts are tracked only where software depends on them, such as MCU
resources, display/touch interfaces, UART exposure, storage, RTC, watchdog,
debug access, and software-visible RS-422 transceiver behavior.

## Success criteria

The engineering platform is useful when a developer can reproduce the pinned
environment, build and inspect the proof targets, run the available tests,
navigate authoritative documentation, and extend the portable HMI without
moving furnace authority into the HMI.
