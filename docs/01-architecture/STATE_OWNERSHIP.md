# State ownership

| State or behavior | Owner | HMI treatment |
| --- | --- | --- |
| Furnace state, mode, phase, readiness | Control CPU | Session-bound cached presentation |
| Temperature validity, setpoint, demand | Control CPU | Display with provenance, validity, and staleness |
| PID, actuators, heater/fan policy | Control CPU | Observe only when exposed; never implement authority locally |
| Interlocks, faults, protection | Control CPU/hardware | Present controller state; HMI is not a safety dependency |
| Program library, IDs, revisions | Control CPU | Browse remotely; edit an HMI-local draft; submit for validation |
| Active accepted program snapshot | Control CPU | Immutable view for the current run |
| Run Sessions/history | Control CPU | Immutable view; create a new draft to reuse |
| Operational settings and limits | Control CPU | Request value/range/writability metadata; submit changes |
| Runtime/service counters | Control CPU | Display authoritative monotonic values and selected derivations |
| Service-period condition, one-time acknowledgement, and maintenance record | Control CPU | Display current session-bound condition and later Device/maintenance record; collect acknowledgement intent only; never issue, record, suppress, authorize, or bypass locally |
| Clock/date | Control CPU | Display controller time |
| Navigation, focus, dialogs, transient forms | HMI CPU | Local state |
| Draft edit state | HMI CPU until submitted | Never imply persistence or acceptance before controller response |
| Language, brightness, theme, UI preferences | HMI CPU | Persist locally through an HMI adapter when designed |
| Capability/cache state | Control CPU source; HMI copy | Bind to session identity; mark stale/invalidate on loss or reboot |
| HMI and Control CPU watchdog state | Respective CPU | Independent; HMI detects controller disappearance/reboot |

## Shared workflow is not shared authority

A program save crosses both owners: the HMI owns the unsaved draft and editing
experience; the Control CPU validates, persists or rejects, and assigns the new
authoritative revision. The same separation applies to controller-owned
settings.

## Value semantics

Ownership and provenance are separate. A displayed value must identify whether
it is measured, commanded, estimated, calculated, or configured and whether it
is current, stale, or unavailable. For example, demand-derived heater power is
an estimate even though its input is controller-owned.

## Adding state

Document the owner, persistence, session scope, validity, update source,
failure behavior, and authoritative validation path before adding a stateful
feature. Update the [requirements register](../00-project/REQUIREMENTS.md) when
the behavior is externally meaningful.
