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

The accepted reversible host-lab presentation direction is a graphite
instrument panel: Background `#171B1E`, panel `#242A2E`, secondary control
`#30383D`, main text `#F1F3F2`, supporting text `#BBC3C7`, and selected
navigation `#285D7A`. Current temperature is the dominant reading, followed by
target and remaining time; estimated power and elapsed time are subordinate.
Related readings share a panel with restrained dividers, common 6 px corners,
and an 8 px spacing rhythm. Frequent actions target at least 48 px in the host
lab, with 56 px run and primary actions as the starting physical-touch proxy.
This is a presentation decision for the reversible simulator/lab pass, not a
production display or controller-behavior decision.

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

**Decision (product owner, 2026-10-01; both presentations):** temperatures use
the degree sign, `°C` and `°C/min`. When a value has a label, the unit sits in
the label and the value is the number alone (`Current °C` / `151`,
`TARGET °C` / `120`, `RATE °C/min` / `2.0`, `Max. °C` / `180`); unlabelled
values such as graph axes, stage cards and dialog text keep a `°C` suffix.
Estimated power follows the same rule (`Est. power kW` / `3.40`). `°` (U+00B0)
is in both Tomorrow HMI fonts.

Celsius is initially implemented. Keep conversion at the presentation boundary
where appropriate, without implementing Fahrenheit now. English is initially
implemented; visible strings use a localization abstraction rather than being
scattered through screen logic. No user-role/access-control system is required
at this time.

## Programs and Manual

**Decision (product owner, 2026-10-06):** the delivered library is three
epoxy cure programs, Epoxy system 1 (laminating resin with a 60 C post-cure),
2 (120 C prepreg cure with a flow dwell) and 3 (180 C high-Tg cure in steps),
within the furnace limits. They are illustrations, not resin datasheet
values. Programs the operator creates are kept by the controller for the
session in both lab presentations (no persistence yet) and behave like
library programs: listed, favourite, start, run, delete.

**Decision (product owner, 2026-10-06):** Stop on a program run cuts the run
where it stands (the plan ends there) and the controller cools automatically
on Running, labelled "Stopped · automatic cooling"; Stop during automatic
cooling marks the run stopped the same way. Pause and Stop give way to
**Start again** (the same program from the start, at once) and **Return
Home** (closes the run). Navigation stays locked until one is chosen.

**Decision (product owner, 2026-10-06):** stopping Manual heating (or the
controller ending it at 48 h) leaves the session on the Manual page in a
Cooling state: heaters off, the readings show heating time, cooling time and
its estimate, the reading and the 30 C cooling target, and the forecast is the
cooling line. The steppers are off; **Exit** goes Home and **Start new**
returns to the Manual page with the same target and rate. Navigation stays
locked until one is chosen. The Manual readings show each label over its value
so all of them fit 800 px.

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

### Controller alerts (proposal, 2026-10-01)

**Recommendation (reversible presentation; implemented in both presentations):**
when the observation reports a new controller fault kind, the HMI shows an
alert dialog over the current page: a warning icon, the fault title, one
sentence on what the controller did, and **Close**. The same dialog reports
**Connection lost** when this display loses the controller link, and
**Connection restored** if that alert is still open when the link returns.

- **Close is a local dismissal only.** It is not an acknowledgement, clears
  nothing and sends nothing; the fault stays on Running and in Device
  notifications. Acknowledgement, latching, severity and history remain the
  open "Faults/alarms" question and belong to the controller.
- A Program run the controller ended with a fault stays on Running with the
  graph frozen at the fault; the fault title replaces the stage label (red),
  Pause disappears and Stop becomes **Return Home**.
- While the link is lost, Running shows the reading values as `-- stale`, the
  stage label reads Connection lost, and Pause and Stop are unavailable; nothing
  is replayed when the link returns.
- Initial fault kinds: over-temperature, target not reached, door opened,
  temperature sensor fault, power interrupted. Titles and texts are HMI-local
  presentations of a controller-reported category; final codes, wording and
  severities follow controller fault semantics (CTRL-ALERTS).
- Alerts are not animated (see Motion and navigation feedback).

## Interaction response

Provide immediate perceptible feedback. For ordinary controller interactions,
show a meaningful result or pending/progress state at about 500 ms rather than
leaving the operator uncertain. Normal Stop has a stronger, bounded path and
must not wait behind low-priority UI work. Emergency Stop is not an HMI feature.

## Verified host graphite composition

**Decision (presentation only):** Reserve a separate simulation identity band,
use a 56-pixel header, 48-pixel ordinary controls, and fixed editor/run actions.
Primary readings use 40-pixel type, secondary readings 32, and engineering labels
16. At the short review viewport, furnace readouts sit beside the proportional
drawing; taller views place them below it. Program facts stay together beside
the preview graph, with management actions separated from Load/Start.


### Local program-edit presentation follow-up

**Decision (presentation only):** The host lab places the transparent header a
few pixels below its coloured state line and uses only a bottom divider. The
state indicator is a compact coloured pill with a dot; its colour follows the
observed state. The notification control is labelled **Notifications** and has
no filled background.

**Fact:** Quick launch hides empty Favourites and Recent programs headings. If
both local lists are empty, it displays **Add a favourite program**. Program
details place a compact favourite marker and rename icon beside the name, use a
red delete action, and give **View stages** a muted amber treatment. The bundled
LVGL symbol set supplies the pencil but has no star symbol, so the host-lab
marker currently uses an asterisk fallback.

**Fact:** Program stages render up to nine direct-edit cards in a three-column,
three-row grid. Each card keeps its name at upper-left, carries a type-coloured
outlined ordinal badge at upper-right, and presents type/duration plus either a
temperature or explicit inherited target temperature. Graph nodes and cards enter the stage editor directly. The editor
shows its ordinal and total beside the stage name, with previous/next controls.
It holds a bounded HMI-local set of edits while cycling; **Save all changes**
retains the local preview and **Discard changes** clears it. Stage-name keyboard
input is intentionally deferred to the next requested batch. No controller
request, persistence, validation, or furnace semantics are implied.**Fact:** The host lab uses explicit Start/Save/acknowledgement request previews.
Observed active Program state controls the Running navigation lock; Manual
retains normal navigation. Modal coverage leaves normal Stop accessible.
These are host presentation choices and evidence, not production permissions.
See [native verification](../09-progress/GRAPHITE_UI_VERIFICATION.md).

### Soft-tile Programs presentation (2026-09-30)

**Decision (product owner, 2026-09-29; recorded with the LVGL port):** Program
details offer **Start program** directly, behind an HMI confirmation dialog
("Start <name>?", with stage count, curing time and maximum temperature). The
Load program page remains only for Home Quick launch, with Start program under
the program facts. This supersedes "management actions separated from
Load/Start" above: Start now shares the details action row with View stages,
Edit program and Delete program.

**Invariant (unchanged):** the confirmation is an operator check in the HMI, not
validation or permission. Confirming sends a start request; only Control CPU
acceptance starts a run ([ADR-0001](../07-decisions/0001-authority-boundary.md)).

**Fact:** the favourite marker is now the shared star icon (outline or filled),
replacing the asterisk fallback noted above. Stage cards follow the screen
concepts (tinted ordinal, kind label and glyph, large target, duration and
rate). Both presentations implement this; see the
[LVGL port plan](../09-progress/LVGL_SOFT_TILE_PORT_PLAN.md).

### Stage editor dialog (2026-09-30)

**Decision (product owner, 2026-09-29; recorded with the LVGL port):** the stage
editor is a fixed-height dialog over the page it was opened from, replacing the
full-page editor described above. Its top row holds Delete stage, a pager whose
stage position can be tapped to move the stage, and Add stage (a Heating stage
20 C above the current one). Heating and Hold use the same three rows, Target,
Rate and Duration; a Hold locks Target to the inherited temperature and shows
Rate "None". Deleting asks first. The first validation problem is shown under
the fields and disables Save all changes. Discard changes asks, then restores
every stage of the session (edits, moves, adds and deletes); Save all changes
keeps them in the local draft.

**Invariant (unchanged):** stage edits remain an HMI-local draft; saving does not
submit anything to the Control CPU, which alone validates and accepts programs.

### Soft-tile Home, Device, Settings, keyboards and dialogs (2026-09-30)

**Fact (both presentations):** Home Quick launch lists favourites, then recent
programs, and drops entries from the end until Start manual fits, so a taller
screen shows more. Door and fan read "Closed" / "Open" / "Unknown" and
"1,450 rpm". Device and Settings lists show up to five 48 px rows per page with
the shared pager and an "N items" count; Device rows carry a status-coloured
left edge (amber due, red fault, green up to date, neutral otherwise) and a
status icon beside the tag. The Display settings page shows Brightness, Dim when
idle, Reduced motion, Default Running view and Language / units first, and
Theme under More.

**Fact (both presentations):** confirmation dialogs are 430 px wide (narrower
screens leave 12 px each side), with equal centred buttons 112-220 px wide.
Delete program asks "Delete program?" naming the program; confirming shows
"Deletion request accepted". Keyboard layouts are recorded in the
[touch keyboard plan](../09-progress/TOUCH_KEYBOARD_PLAN.md#2026-09-30-soft-tile-layout-supersedes-the-action-row-and-keypad-above);
the numeric keypad has no sign key because no current field accepts a negative
value.

**Invariant (unchanged):** "Deletion request accepted" is the host preview of a
Control CPU response. The HMI does not delete programs itself.

### Device and Settings card pages (2026-10-01)

**Decision (product owner; supersedes the five-row lists above for these
pages):** Device and Settings sub-pages use rectangular cards like the Device
Overview runtime tiles, in a grid whose rows stretch to fill the page above the
footer. Each card shows a small label, a large value and a caption. Action
cards are tappable on their whole surface and carry a lit left edge (soft
amber; red for destructive actions; on Device the status colour: amber due,
red fault, green up to date, neutral otherwise). Information cards have no
edge. The Device log stays a list. The full layout is recorded in the
[card pages plan](../09-progress/DEVICE_SETTINGS_CARDS_PLAN.md); both
presentations implement it.

- **Display & interaction** is one page: a full-width Brightness card with a
  tappable ten-step gauge (10-100 % backlight) between - and +, with no number
  shown and the heading just above the bars; Dim when idle (default Off) opens a
  choice of Off, 1, 2, 5, 10 or 30 min; Reduced motion toggles On/Off; Default
  Running view opens a choice of Graph, Split or Furnace; Language, Units and
  Theme (Dark) are information. Edits stay in the unsaved form until Save.
- **Date & time**: a large clock card opens **Set date & time** (day, month,
  year, hour, minute, plus AM/PM in 12-hour format, each with + and -, and a
  weekday preview), beside a Time format card. A shorter month or a non-leap
  year pulls the day back (31 Jan -> 28 Feb). Set date & time is a request to
  the controller, separate from preference Save; Cancel changes nothing.
- **Connections** is listed only when the controller reports server/Bluetooth
  enabled (hidden by default): a wide Furnace controller card over Server and
  Bluetooth.
- Settings **System** (named Maintenance until 2026-10-05, renamed so it is
  not confused with Device Maintenance; power icon): four action cards (Reset
  settings, Delete programs, Restart display, Attempt furnace restart).
- **Theme and Language** (2026-10-05) are display preferences like the other
  Display cards: information while only one exists, a choice dialog once a
  second theme or language is added. The edited choice previews at once;
  Save keeps it, Discard and Reset restore. Each language is listed by its own
  name. Colours and text come from the shared theme and string catalogues
  ([ADR-0012](../07-decisions/0012-theme-and-language-catalogues.md)).
- Device **Overview**: the three runtime cards over Service attention, Errors
  and Last service, each opening its tab. Device **Maintenance** lists every
  catalogue task as a status card, worst first, six per page; Service history
  pages its records the same way; Device info shows six cards.

**Decision (product owner, 2026-10-06; supersedes parts of the list above):**
- A Maintenance task opens its details with a corner X (close only) and
  **Maintenance done**. Done is a request; when the controller confirms it the
  task starts a new interval ("Next in <interval>"; a condition-based task
  stays condition-based), its last baseline becomes today, and a Maintenance
  record leads Service history and the Overview Last service card. It is
  unavailable while data is stale or the link is lost. Latest decision: it is a
  quality-of-life aid for tracking machine health (recommendations), not
  warranty-based; this may change. Who may record it is open
  ([open questions](../09-progress/OPEN_QUESTIONS.md), Maintenance done).
- Device info shows five cards: Model, HMI firmware, Controller firmware,
  Component catalogue and a two-column Support card. Controller link is
  removed: link state already shows where it matters (stale banner, "Last
  known" footers, the link-lost Running state). Support opens the support QR
  code (dark code on a light plate in every theme) under a scan hint, with the
  corner X as its only control.
- The **Device log** replaces "stays a list": an event feed in the style of
  the Ajax Systems app. Filter tabs (All, Programs, Alarms, Maintenance,
  System) fill the toolbar; events sit newest first under date headings with
  their age ("1 Oct 2026 (5 days ago)"); each row shows the type icon in its
  tone, the time and one line naming the program, alarm or part ("Program
  completed: Epoxy system 2", "Over-temperature: Epoxy system 3",
  "Maintenance done: Air filter / Inspect filter"); tapping a row shows its
  date, time and source. It holds one month; pages hold as many rows as the
  screen fits. The Overview Errors card opens it on Alarms.
- **Date format** (2026-10-06) is a display preference on Date & time beside
  Time format: 6 Oct 2026, 06.10.2026, 06/10/2026, 10/06/2026 or 2026-10-06.
  Every date on screen follows it; it is saved with the Save bar.
- Settings **System** (2026-10-06): Delete programs removes every program
  after one confirmation naming them (the controller answers the request);
  a single program is deleted from its details. Reset settings
  resets Display preferences, Date & time formats, or All preferences.

**Recommendation (reduced motion):** the preference turns off decorative
motion only: button press/glow transitions, the navigation glow and slide-in
panels. It never hides or delays state, alerts, Stop availability or stale
indication, which are not animated anyway (see Motion and navigation
feedback). Whether the product needs it stays open
([open questions](../09-progress/OPEN_QUESTIONS.md), Motion/accessibility).

### Running-view presentation refinement

**Decision (presentation only):** The Running view uses unboxed Graph, Split,
and Furnace selectors with narrow straight separators. In full Graph view the
reading band becomes a compact unboxed footer beside Elapsed, allowing the
graph to use the released vertical space. The measured path is high-contrast
amber, distinct from the muted planned path. The graph's lower grid boundary
is lowered to clear its time labels, while temperature labels move slightly
right without shifting the plot.