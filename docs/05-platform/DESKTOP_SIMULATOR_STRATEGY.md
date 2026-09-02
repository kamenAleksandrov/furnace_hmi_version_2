# Desktop simulator strategy

Desktop simulation is an accepted core architecture requirement, recorded in
[ADR-0004](../07-decisions/0004-desktop-simulator.md).

## Foundation target

Provide a project-owned C/CMake executable using LVGL and SDL2 that proves a
window, rendering, input, build, and debug path. The foundation began with a
generic input diagnostic and now defaults to the first reusable, read-only
state-availability presentation. Furnace values, actions, and protocol behavior
remain absent.

The simulator is an application target, not a wholesale copy of an upstream
example. It should eventually compile the same portable presentation,
application, HMI model, and controller-client sources as embedded builds.
Host-specific entry, clock, storage, input/display, and transport adapters may
differ.

The foundation build accepts SDL2 2.32.10 or newer. Version 2.32.10 is the
tested compatibility floor, not a production dependency pin; newer compatible
releases remain eligible and any future floor change requires a host build and
smoke-test result.

The current desktop development viewport defaults to a fixed 800 x 480 window;
the reusable availability layout is tested down to 640 x 360. Neither size is a
production display selection. The executable modes are:

- no argument: visible state-availability UI, initially `UNAVAILABLE`;
- `--presentation-smoke-test`: headless state mapping, lifetime, read-only, and
  bounds checks;
- `--smoke-test`: the original SDL pointer/text input diagnostic.

## Planned simulation levels

1. A semantic simulated controller supplies state and request outcomes for fast
   deterministic UI development and scenario tests.
2. The actual approved codec and framer run over memory/loopback transport for
   protocol integration, corruption/loss/latency injection, and reconnect
   tests.

These levels are complementary. The semantic simulator must not become a
second implementation of controller truth used to define production behavior.

## Sharing and ownership rules

- LVGL access remains confined to the GUI context on the host too.
- Scenarios provide semantic inputs rather than directly mutating LVGL objects.
- The host build must not add furnace-only behavior absent from embedded code.
- Differences in adapters are explicit and testable.
- Exact LVGL source/version integration follows the pinned project dependency
  workflow so host and embedded semantics do not drift silently.
