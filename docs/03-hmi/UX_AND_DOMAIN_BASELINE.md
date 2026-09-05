# UX and domain baseline

This document records operator-facing information and behavior the future HMI
model must support. It is not a screen specification and does not authorize
production behavior in the foundation phase.

## Stakeholder interaction baseline (2026-09-02)

The current visual direction is a structured redesign of the supplied concept
sketches rather than a literal reproduction. Interaction is tap/click based;
dragging is not required. Interactive targets should normally be at least
48 x 48 px. The laboratory bottom navigation uses Home, Programs, Device, and
Settings; its final information architecture remains reviewable. A
controller-reported active Program opens a dedicated running page rather than
turning Home into a run dashboard.

The graph distinguishes a muted solid planned trajectory from a contrasting
solid measured trajectory. Missing sensor samples are represented as unknown
gaps, never invented values. A static status/icon treatment must remain usable
if the optional furnace animation is unavailable. Pause, Resume, and Stop are
first-class run actions; Emergency Stop remains outside the HMI safety
function. The HMI should expose user-useful control information (stage,
target/error, rate, demand, timing, fan/door/interlock and sensor validity)
before exposing raw PID internals, which belong in a role-appropriate service
or diagnostics view if eventually needed.

The visual theme is dark and neutral rather than light blue. A deeper blue is
reserved for ordinary navigation and actions. A high-contrast state accent is
present on every destination: running is warm amber, paused is distinct violet,
Manual is teal, idle is blue-grey, faults are red, and stale/unavailable state
uses an explicitly cautionary muted treatment. These are presentation cues, not
additional machine state.

### Motion and navigation feedback

The current host-lab baseline presents bottom navigation as one continuous tab
surface: only the selected destination is raised and receives a short (140 ms)
deep-blue glow. This is a recommendation for non-essential interaction
feedback, not a machine-state transition. It executes only in the LVGL owner
context and never delays navigation or a controller request.

Do not animate temperature values, measured/planned paths, controller state,
faults, alarms, Stop availability, or stale/unavailable transitions. Those
changes must be immediately readable even if decorative motion is disabled or
unavailable. A future reduced-motion preference and its scope remain open.

## Dashboard information

The model should be able to present current temperature, target/setpoint,
program name, current stage and count, phase, overall run state, elapsed and
remaining/estimated time, heater demand, estimated heater power, fan status,
service time and detailed counters, clock/date, controller communication state,
active faults/alarms, and planned/live graphs. More statistics may be added;
the model and layout should not assume this list is permanently closed.

## Values and validity

Every engineering value exposes whether it is measured, commanded, estimated,
calculated, or configured, plus current/stale/unavailable state. Labels must
not turn demand-derived estimated power into a measured electrical quantity.

Celsius is initially implemented. Keep conversion at the presentation boundary
where appropriate, without implementing Fahrenheit now. English is initially
implemented; visible strings use a localization abstraction rather than being
scattered through screen logic. No user-role/access-control system is required
at this time.

## Programs and Manual

The planned library experience includes browse, select, create, edit, copy,
rename, delete, preview, calculation assistance, save, load, and run. Home
quick launch presents a bounded sequence of favourites then recently run
programs, with a separate full library route. The HMI edits locally and submits
a draft; controller acceptance creates authoritative state. Editing can
continue during a run without altering its accepted revision.

The host-lab favourite action is HMI-local quick-launch preference only. It
uses explicit **Add favourite** / **Remove favourite** language and a state
accent rather than ambiguous `+`/`-` wording. It does not claim that a Control
CPU program has changed.

Operator-facing labels use **New program**, **Add stage**, and **Program
editor**, not “draft” or an HMI-owned “accepted revision.” A local editing
buffer is transient presentation state only. The authoritative library changes
only after the Control CPU validates the relevant request; whether validation
is per-stage or atomic at program save remains an interface decision.

The current stakeholder proposal is that a saved program may carry an ambient
temperature input used only for duration/trajectory estimation. A run must
start and continue from the controller's current measured temperature; the
saved ambient estimate must never masquerade as a live sensor value. Storage,
units, validation, and whether the controller returns this estimate in its
accepted revision remain to be specified.

The stage model is expected to make HEATING, HOLD, and COOL explicit and may
suggest a likely type from surrounding values. The stakeholder's target delta
means a temperature change in degrees Celsius per time period, normally °C/min.
HEATING lets the operator edit target and choose rate or duration as the local
preview driver, showing the calculated counterpart. Limits, rounding, actual start temperature, and controller
execution semantics remain open.

The clarified visual-authoring proposal makes HEATING target and rate/duration
editable with the other value calculated; HOLD inherits its target for preview and exposes
duration; COOL offers fastest-allowed or controlled-rate policy only when the
controller advertises it. Exact governing parameters, bounds, and rounding
remain controller-domain work.

Manual is a first-class controller mode with Start, target/rate adjustment,
Pause, Resume, and Stop. The controller enforces a default six-hour selected
deadline or an explicitly longer selection; adjusting target/rate does not
extend it. A warning/extension affordance is HMI presentation, not enforcement.

### Service-period preflight

The Control CPU owns service-period counters, maintenance policy, proposed-run
forecasting, and the decision to allow, block, or require acknowledgement for a
run. It issues the acknowledgement opportunity once per configured service
period, records an accepted acknowledgement for later Device/maintenance
inspection, and permits normal operation after acceptance subject to all other
controller checks. The HMI presents a clearly worded, correlated intent dialog;
it cannot grant permission. Threshold, re-arm after service, roles, and audit
retention remain open.

## Graphs

Present the planned temperature trajectory and live measured temperature. The
planned graph should be based on the same accepted stage semantics the
controller executes, not an unrelated HMI approximation. It has a time X axis,
temperature Y axis, readable rectangular grid, a muted solid planned path,
and a stronger solid measured path rendered above it. The initial bounds cover
the complete plan and live history with headroom and stay fixed until actual time
or temperature exceeds that domain. Stage annotations/floating labels are
feasible when timing data are specified. When a program is saved, its ambient
temperature may be retained as an estimation input only; a run starts and
updates from the controller's live measured temperature. History resolution
and graph architecture remain open.

## Faults and destructive operations

Reserve model boundaries for stable fault codes, severity, time/context,
acknowledgement, latching, history, and recovery guidance without inventing the
workflow. Similarly, do not create one universal factory reset. HMI preference
reset, HMI restart, program/history/log/credential/counter clearing,
controller-setting reset, and controller restart have different owners and
permissions that remain to be designed.

## Interaction response

Provide immediate perceptible feedback. For ordinary controller interactions,
show a meaningful result or pending/progress state at about 500 ms rather than
leaving the operator uncertain. Normal Stop has a stronger, bounded path and
must not wait behind low-priority UI work. Emergency Stop is not an HMI feature.
