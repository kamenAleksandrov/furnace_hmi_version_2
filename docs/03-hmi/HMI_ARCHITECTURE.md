# HMI architecture

The HMI architecture separates reusable behavior from LVGL and Zephyr/desktop
adapters. These are responsibility boundaries, not a mandate to create empty
directories or abstractions before code needs them.

## Reusable areas

**Presentation** owns screen composition, formatting, localization lookup, and
binding presentation state to LVGL. It runs under one GUI owner.

**Application** coordinates operator intents, drafts, pending/presentation
states, and use cases. It does not declare a furnace operation valid.

**HMI model** represents session-bound authoritative snapshots, local drafts,
value provenance/validity, and UI-only state without LVGL objects or transport
buffers.

**Controller client** coordinates requests, lifecycle, cache/session validity,
and resynchronization over a semantic protocol interface. It does not expose
raw UART details to the application.

**Ports** define narrow boundaries for time, persistence of HMI preferences,
controller transport, and queued GUI/application exchange. Zephyr and desktop
implementations differ behind these ports.

## Execution ownership

Only the designated GUI context creates or manipulates LVGL objects. Transport,
storage, and other contexts publish bounded plain-data messages. The GUI
consumes a bounded amount of work and never waits synchronously for a controller
transaction, filesystem operation, retry sequence, or large calculation.

## State discipline

- Controller data is a session-scoped cache with explicit validity/staleness.
- Immediate HMI feedback is not represented as authoritative success.
- Program edit state remains a local draft until the controller returns a new
  authoritative revision.
- HMI-owned preferences do not leak into controller-owned operational settings.
- Operational limits come from a valid current-session capability snapshot and
  remain subject to controller validation.

The desktop and embedded builds should eventually reuse these areas. Platform
adapters and entry points differ; furnace semantics must not be duplicated for
the simulator.

## Host-only UI laboratory

The host-only UI laboratory composes a deterministic semantic controller with
the shared model and presentation. It supplies snapshots, graph samples,
validity changes, session changes, and later request outcomes without writing
LVGL objects directly. Its executable, fixtures, and development controls are
host-only; the Zephyr composition does not link them. A visible simulation
marker and a separate build preset prevent synthetic observations from being
mistaken for device data.

## Implemented first presentation slice

The first reusable LVGL component is the read-only state-availability view
under `app/hmi/presentation/state_availability/`. It accepts plain presentation
data with three values: `UNAVAILABLE`, `STALE`, and `CURRENT`. Zero, NULL, and
invalid input fail closed to `UNAVAILABLE`; no furnace value or control is
shown unless the input explicitly says the observed controller state is
current.

The view reports state availability, not transport connectivity. Its object
tree is non-interactive, owned by the GUI context, and cleared when its root or
parent is deleted. Desktop-only development identity remains in the simulator
adapter. The component is host-built and semantically smoke-tested at a
640 x 360 minimum viewport; embedded LVGL wiring remains deferred until a real
display target is selected.
