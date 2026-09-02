# Glossary

**HMI CPU**  
Processor that runs Zephyr, LVGL, the HMI application, operator interaction,
local UI state, the controller client, and the HMI watchdog. Do not call this
processor the controller.

**Control CPU / Furnace Controller / controller**  
Processor that owns authoritative furnace state and behavior, including
program persistence and execution, Manual execution, validation, setpoints,
timing, PID, actuators, fan policy, safety/interlocks, faults, statistics,
authoritative time, and its own watchdog. In this repository, unqualified
*controller* means this processor.

**authoritative state**  
State the Control CPU owns and against which requests are validated. An HMI
cache is a presentation copy and may be stale.

**capability snapshot**  
Controller-provided, session-bound facts used for controller-informed UX, such
as ranges, writable state, and supported operations. It is not valid across an
unverified boot/session change.

**editable draft**  
HMI-local working data that the operator may modify before submitting it. It
does not become authoritative until the Control CPU validates and persists it.

**accepted program revision**  
The immutable program snapshot the controller accepted for an active run.
Editing the library can create a later revision but cannot mutate this one.

**Run Session**  
An immutable historical record of one Manual or program run. Its final fields
and retention policy are open.

**Manual mode**  
A first-class controller execution mode, not a synthetic long-duration
program.

**normal Stop**  
A high-priority software request from the HMI. It is distinct from the
independent emergency/safety-stop path.

**boot/session identity**  
Controller identity used to determine whether cached state and pending
requests still belong to the currently running controller instance.

**state revision**  
A monotonic controller-owned marker considered for detecting newer
authoritative snapshots. Its wire representation is not yet defined.

**measured / commanded / estimated / calculated / configured**  
Different engineering-value semantics that must remain explicit in the model
and UI. Staleness and unavailability are separate validity qualities.
