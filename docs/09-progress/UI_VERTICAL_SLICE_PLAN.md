# UI vertical-slice execution plan

- Status: Complete
- Started: 2026-08-26
- Completed: 2026-08-27
- Owner: HMI software

## Outcome and non-goals

Build the first reusable, desktop-visible LVGL presentation slice: an
authoritative-state availability screen that starts in `UNAVAILABLE`, is fed
only plain immutable view data, and clearly withholds furnace values and
controls until current controller state exists.

This slice does **not** implement controller communication, session/reconnect
behavior, protocol semantics, furnace values, navigation, commands, alarms,
programs, Manual mode, or production target drivers. It does not claim that
the current 800 x 480 desktop default is the production display choice.

## Governing records

- `REQ-FUN-001`: present authoritative state received from the Control CPU.
- `REQ-FUN-013`: keep visible strings outside screen logic.
- `REQ-NFR-005` and `REQ-NFR-006`: preserve portable source boundaries and
  honest validity/provenance presentation.
- `ARCH-INV-001`, `ARCH-INV-004`, `ARCH-INV-005`, and `ARCH-INV-007`: preserve
  controller authority, reconstruction, single-owner LVGL access, and
  controller rejection authority.
- [ADR-0004](../07-decisions/0004-desktop-simulator.md): use the project-owned
  desktop simulator and reusable presentation sources.
- [ADR-0005](../07-decisions/0005-lvgl-single-owner.md): only the GUI context
  may manipulate LVGL objects.

## Facts, working assumptions, and deferred decisions

**Facts**

- The current LVGL/SDL executable already builds, renders, accepts pointer and
  keyboard input, runs headlessly in CTest, and source-debugs through MSVC.
- The current visible foundation screen and SDL adapter are coupled in one
  desktop entry source.
- `CURRENT`, `STALE`, and `UNAVAILABLE` are accepted presentation concepts for
  controller-observed state.

**Working assumptions for this reversible slice**

- Use relative LVGL sizing within an explicitly supported 640 x 360 minimum;
  the configurable 800 x 480 default remains only a development viewport.
- Show English copy through the existing string catalog.
- Start in `UNAVAILABLE`; invalid input also fails closed to `UNAVAILABLE`.
- Keep execution single-threaded until an actual bounded cross-context handoff
  is required.

**Safely deferred**

- Production display/touch/MCU selection and memory budgets.
- Session identity, connection mapping, retry, and freshness thresholds.
- Dashboard fields, graphs, stage semantics, faults, history, navigation, and
  all controller-request workflows.
- The protocol, semantic simulator, queues, and embedded LVGL target wiring.

## Modules and ownership

- `app/hmi/presentation/`: reusable LVGL screen composition and immutable
  availability view data; no SDL or Zephyr APIs.
- `app/hmi/presentation/strings/`: all visible English text.
- `simulator/desktop/`: SDL display/input/timer adapter, startup wiring, and
  diagnostic smoke modes; no furnace authority.
- `tests`/CTest: headless rendering, invalid-state fail-closed, absence of
  actionable furnace controls, input diagnostic, and string-catalog coverage.

## Milestones and acceptance

| Milestone | Observable completion criterion | State |
| --- | --- | --- |
| Developer runbook | One linked command reference covers setup, visible UI, debug, host/Zephyr tests, analysis, and indexing | Complete |
| Reusable presentation | Default executable renders the availability screen from `app/hmi/presentation/` | Complete |
| Diagnostic separation | Existing SDL pointer/text exercise remains available only through a test mode | Complete |
| Fail-closed tests | Headless test verifies all valid states, invalid/NULL/zero fallback, no actionable widgets, object lifetime, and bounds | Complete |
| Cross-build verification | Host Debug/Release and MSVC desktop build/tests pass with warnings as errors | Complete |
| Visual evidence | The actual built window is captured and reviewed | Complete |
| Documentation/index | Status, roadmap, session log, traceability, links, and deterministic index validate | Complete |

## Verification and recovery

- Run the host Debug, Release, and MSVC desktop presets and their CTest suites.
- Run the shared string catalog on host and Zephyr; rerun the Zephyr build and
  Ztest because target-shared strings change.
- Check that `app/hmi/presentation/` has no SDL or Zephyr includes.
- Run link/config validation and deterministic index generation/check.
- Obtain architecture/software/error review for this first production-facing
  presentation boundary.

The change is additive and contains no persisted state or compatibility
migration. Rollback consists of removing the new component and restoring the
desktop default to the foundation diagnostic; dependency pins and build
environment remain unchanged.

## Completion evidence

- Host Debug, Release, and MSVC desktop presets each passed 6/6 CTest cases;
  the portable MSVC preset passed 4/4.
- A clean Zephyr `qemu_x86` build completed 135 steps; Twister executed one
  configuration and passed both Ztest cases.
- GCC static analysis completed with the two recorded upstream Zephyr libc
  warnings and no repository-owned warning.
- Architecture recheck found no remaining authority, portability, lifetime,
  or semantic blocker after remediation.
- The generated index and Markdown/link/configuration checks were refreshed
  from the final source/build snapshot.
