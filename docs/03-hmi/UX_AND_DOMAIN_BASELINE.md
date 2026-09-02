# UX and domain baseline

This document records operator-facing information and behavior the future HMI
model must support. It is not a screen specification and does not authorize
production behavior in the foundation phase.

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
rename, delete, preview, calculation assistance, save, load, and run. The HMI
edits locally and submits a draft; controller acceptance creates authoritative
state. Editing can continue during a run without altering its accepted
revision.

The stage model is expected to make RAMP, HOLD, and COOL explicit and may
suggest a likely type from surrounding values. Final semantics are open.

Manual is a first-class controller mode with Start, target/rate adjustment,
Pause, Resume, and Stop. The controller enforces a default six-hour selected
deadline or an explicitly longer selection; adjusting target/rate does not
extend it. A warning/extension affordance is HMI presentation, not enforcement.

## Graphs

Present the planned temperature trajectory and live measured temperature. The
planned graph should be based on the same accepted stage semantics the
controller executes, not an unrelated HMI approximation. History resolution
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
