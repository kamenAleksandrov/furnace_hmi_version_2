# Manual and program editor concepts

Date: 2026-09-24. Concepts with current host-LVGL/browser presentation notes;
production controller, protocol and hardware integration remain separate.
See [implementation plan](../09-progress/OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md).

## Basis

Design for 800 x 480 first, verify 800 x 640 and 1024 x 600. Ordinary touch
targets at least 48 x 48 logical pixels. Keep graphite surfaces, industrial
yellow interaction accents and distinct semantic colors.

## Implemented presentation adjustment (2026-09-24)

- Remove the page/status bezel on every HMI page. The yellow top accent line
  remains and is the persistent interior-light request control; it glows while
  the presentation fixture reports light on. It is not a hardware state owner.
- Manual uses a graph on the left and controls on the right. Its forecast ends
  at the current target using the current rate; it does not invent holding or
  cooling time. Target and rate have decrement, direct numeric entry and
  increment controls. Pause/Stop are the only Manual run actions.
- Manual is a vertical split: the graph receives roughly the upper 70 percent
  and the lower 30 percent is a single control band. Target and rate labels sit above
  their dark, yellow-outlined control bands; 48 x 52 minimum plus/minus
  touch targets are symbol-centred. Live state and Curing elapsed sit at the top
  of that lower band, above the controls; Start/Pause/Stop occupy its left side
  and target/rate occupy the right. Operator-facing labels use shipping terms:
  never add a demo or preview qualifier to a control. Running Manual shows
  controller-observed Curing elapsed time; the browser fixture supplies the same
  presentation from its explicitly simulated session.
- Manual target/rate and Start/Pause/Resume/Stop changes update the existing
  graph data, labels and action visibility in place; they do not rebuild the
  Manual page or animate a temporary-size graph. Manual uses the supplied full
  controller temperature range (currently 0--180 C), rather than auto-zooming
  around the selected target. The browser graph fills its assigned card. While a browser touch keyboard is open,
  simulation may advance behind the modal but must not redraw or dismiss the
  operator's entry.
- At the 48-hour Manual limit, controller simulation ends heating, while
  furnace cooling remains automatic. The HMI returns to Manual setup and shows
  the automatic-cooling notice rather than a separate Manual cooling view.
- Stage cards show white Heating/Holding type, coloured ordinal, white duration
  and coloured temperature. Details show Number of stages and Max. temp first,
  then Duration: Curing, Est. cooling and Est. total.

## Manual setup

Quick Launch keeps favourites/recents and replaces only Choose a program with
Start manual. Entry opens idle setup. Reuse Running split structure, replacing
furnace illustration with controls.

```text
+------------------------------------------------------------------+
| Manual                                     Ready / Not running    |
|                                                                  |
| Temperature graph / preview                                  |
| Current 24 C, target/rate forecast                            |
|---------------------------------------------------------------|
| Target [ - 120 C + ]  Rate [ - 2.0 C/min + ]   [ Start ]     |
| [Back]                         [Start                  ] [Light]  |
| [Home]       [Programs]       [Device]       [Settings]            |
+------------------------------------------------------------------+
```

Width proposal: 16-pixel margins, 12-pixel gap, 440-pixel graph and 316-pixel
controls. Target/rate rows at least 56 pixels tall. Numeric keyboard receives
controller bounds/steps. Current temperature and requested target are distinct.
Start becomes pending, transitioning only on acceptance. Missing capabilities
explain unavailable actions rather than inserting guessed production limits.

## Manual running and cooling

```text
+------------------------------------------------------------------+
| Manual                   Heating                  State current   |
|                                                                  |
| Planned / measured graph                                        |
| Heating, Curing elapsed 00:42                                    |
|-------------------------------------------------------------------|
| Target [ - 120 C + ]  Rate [ - 2.0 C/min + ]  [Pause] [Stop]    |
+------------------------------------------------------------------+
```

Recommendation: active Manual shares Program containment, explicitly recorded
in requirements. Pause becomes Resume. Editing creates candidates; Apply sends
the request, retaining observed values and rejected candidate input. Repurpose
split-view controls; no Graph/Split/Furnace bar here. Reserve run action band so
Stop stays accessible with keyboard/dialog open.

Notice begins ten minutes before controller deadline; reconnect within that
window shows actual remaining time. At the deadline the controller ends manual
heating and owns automatic cooling; Manual returns to setup. New Manual needs
fresh accepted Start after Stop.
Prefer clear labels to ambiguous elapsed(parenthesized elapsed):

```text
Curing elapsed        48:00:00
Cooling elapsed       00:18:20     Est. remaining         ~00:42
```

Completed curing can be smaller secondary text. Cooling elapsed is observed
timing; remaining is estimated. Show Estimating or Unavailable when appropriate.
For Program running retain selectors but reduce width, not touch height. The
top accent line replaces the old action-band Light button.

## Program creation/editing

**Implemented (2026-09-27), superseding the sketch below.** One toolbar row
holds Back, the editable program name (tap to rename; no separate "Edit
program" title), a compact `+ Add stage` and `Edit stages`. Stages use the same
cards as Program stages, and tapping a card opens that stage in the stage editor
(no per-stage Edit button). The bottom navigation is hidden; Cancel and Save
program are the bottom row. Leaving through Back or Cancel with a changed name
or stages asks `Discard changes?` first; an unchanged draft leaves directly.
Curing/cooling/total statistics are not shown here; they appear after Save on
Program details. The editor shows no emulator or draft-state explanations.

Original recommendation: name above summary rows; each has an Edit action
opening current full stage editor. Multiple inline numeric inputs/units/type/
actions would crowd 800 x 480. Do not implement the dense version at that size.

```text
+------------------------------------------------------------------+
| New program / Edit program                                       |
| Name [Panel curing                                         ]     |
|                                                                  |
| 1  Heating    120 C     2.0 C/min                    [Edit]       |
| 2  Hold       120 C     01:30                        [Edit]       |
| 3  Heating    180 C     1.0 C/min                    [Edit]       |
|                                                                  |
| [+ Add stage]                 [Previous]  1/2  [Next]             |
| Cooling follows automatically - estimate shown in preview        |
| [Cancel]      Curing: 03:18       [Save program                ]  |
+------------------------------------------------------------------+
```

Example height budget: 32 margins + 32 title + 56 name + 192 three rows + 48
toolbar + 24 cooling note + 56 footer + 40 gaps = 480. Paginate more stages;
taller screens may show more rows with identical editing behavior. Arithmetic
supports the proposal; actual LVGL/text/keyboard geometry still needs review.

Empty state: Add the first stage. Save requires valid name/stages and current
capabilities. No material/stage names. Name keyboard: letters, numbers, space,
Caps, Delete, Apply, Cancel. Add stage chooses Heating/Hold then existing editor;
title includes parent name and ordinal/type. Heating retains applicable target/
rate/duration assistance; Hold inherited temperature and editable duration.
Stage Apply modifies program draft only. Stage Cancel restores previous values;
cancelling new stage leaves no empty row. Move up/down and Delete stage belong
inside editing flow, not crowded summary rows. Final Save submits whole program;
rejection preserves edits. Leaving dirty editor offers Save/Discard/Stay.

Read-only details/stage view remain separate: non-editing cards/graph nodes,
parent name visible, explicit Pencil/Edit program entry. Details example:

```text
Number of stages 3        Max. temp               180 C
Duration
Curing 03:18              Est. cooling            ~01:10
Est. total ~04:28
```

## Operator text policy (2026-09-27)

Operator-facing HMI text describes the furnace and what the operator can do.
It does not explain the emulator, the demo, the concept status or how the
simulated controller behaves (for example "Draft only until the controller
accepts Save", "Simulated assessment", "Illustrative value", "- preview").
Validation errors and operational guidance stay. Exceptions: the native lab's
required `SIMULATION - NOT DEVICE DATA` marker and the web demo's outer toolbar,
which sit outside the HMI screen. Fixture data values (part numbers, versions)
are content, not explanation.

## Light, log and review

The persistent yellow top accent line is the light request control. Its visible
stroke is 4 pixels high over a 12-pixel full-width hit area (2026-09-27). The
strip is the top border itself, so content starts 12 pixels from every screen
edge. It is deliberately below the 48 x 48 ordinary-control baseline: it spans
the full width and has nothing else near its centre, so the operator decision is
that a thin, easy-to-find edge is acceptable (physical-panel confirmation
pending). It glows for the
observed/simulated on state. Actual on/off, pending/unavailable follows
controller observation; furnace actuation remains outside this HMI concept.

Device log: timestamp, source, action/event, outcome, newest first. Bounded
session memory; button click is not confirmed furnace event. Maintenance detail/
reset/check deferred; retain fault access when replacing Notifications.

Review idle/run/pause/cooling, notice, empty/populated editor, long names,
keyboard/dialog, stale and rejected states at all sizes. Discord reference
unavailable; physical touch review not performed.

## Display refinements (2026-09-26)

The Manual page title is Manual mode. Use Est. for estimated UI readings.
During program curing, elapsed and remaining are displayed together on one
line; the former time-switch button is gone. The text keyboard has one layout:
digits across the top and letters below, with no symbols screen or toggle.
Numeric entry continues to use its separate numeric keypad. The persistent top
line remains a visible 4-pixel accent over a full-width light control target,
reduced from 48 to 12 pixels high by 2026-09-27 (see Light, log and review);
native geometry and browser smoke verify its size. The browser Manual graph is
drawn at its own layout size rather than stretched from a fixed view box, with
the same axis labels as native.

Program previews (details and load) end with a dashed green automatic-cooling
segment that shows only the start of cooling: at most a quarter of the curing
time, following the estimated cooling slope, so the authored stages keep most
of the width. A running program shows the whole cooling estimate. In Program
details the Duration heading sits above Curing, Est. cooling and Est. total;
View stages is an outline-only action beside the filled Load program; Edit
program is a pencil icon-only button on Program stages.

Program details (2026-09-26 revision): the bottom action row holds three equal
actions, View stages, Edit program (text, outline) and Load program (filled);
Edit program no longer sits in the title bar. The graph and facts share the
same three-column grid and gap as that row, so the graph spans exactly the
first two actions and the facts card sits above Load program. Duration values
are label/value rows to fit the narrower card.

Running readings: every Running view (Graph, Split, Furnace) shows one line of
`Label value` readings between the visual and the actions, labels muted and
values emphasized at one shared size. During curing: Elapsed, Remaining,
Current, Target, Est. power. During automatic cooling the line keeps curing
Elapsed and adds only Cooling (elapsed) and Est. remaining, then Current and
Target; no second row and no repeated Curing/Estimated wording. The native lab
no longer shows reading cards above the Split/Furnace visual. Native LVGL has
no bold Montserrat, so values use a slightly larger, brighter font instead.
