# Settings screen proposal

Status: **UI direction approved by operator; maintenance extension proposed**,
2026-09-14. This is not an accepted architecture decision or implemented
controller behavior. The operator approved the preceding design and requested
the maintenance extension below; unspecified defaults and protocol details
remain open.

## Basis and ownership

**User mandate:** Settings must not override furnace-controller configuration.
The operator selects preferred maximum rate, temperature, and program duration
to guide program creation and stage correction. Date/time editing and server /
Bluetooth interaction are desired where configuration permits them.

**Facts:** At initial proposal time, `build_settings()` contained four
informational panels. Native Settings has since been implemented in the host
lab; see [current status](../09-progress/STATUS.md).
The [visual baseline](UX_AND_DOMAIN_BASELINE.md) specifies graphite surfaces,
deep-blue navigation, 8 px spacing, 6 px corners, touch-sized tap controls, and
no required dragging. English and Celsius are the initial supported choices.
The current Program-running workflow suppresses ordinary navigation, including
Settings, until normal Stop; Manual does not impose that presentation lock.

**Governing records:** [Requirements](../00-project/REQUIREMENTS.md)
REQ-FUN-002, 003, 010, 011, 013, 014, 015, 017; REQ-NFR-002, 004, 006, 009;
ARCH-INV-001, 003, 005, 007, 008, 009; and
[ADR-0001](../07-decisions/0001-authority-boundary.md) /
[ADR-0005](../07-decisions/0005-lvgl-single-owner.md).

**Recommendation:** HMI-owned preferences persist on this panel, not as
controller configuration or a shared multi-client policy. Controller capability
metadata remains session-bound. Date/time is controller-owned; the screen may
collect a change request, but displays success only after authoritative
confirmation. Server/Bluetooth ownership, hardware support, enablement source,
permissions, credentials, and time synchronization support remain unverified.

## Composition

Keep the existing state line, transparent Settings header, Notifications,
dotted state pill, and continuous Home / Programs / Device / Settings bar.
Use a roughly 184 px left category rail and a right content panel within the
existing 800 x 480 shell. Five 48 px category buttons fit without scrolling.
At taller/wider sizes preserve the same hierarchy and increase breathing room.

Categories: **Program preferences**, **Display & interaction**, **Date & time**,
**Connections**, and **Maintenance**. Open Program preferences first. Ordinary
pages have no more than three main rows, brief contextual text, and a stable action footer. Display has
two explicit subpages if needed; editing connections opens a detail page.
Maintenance uses four compact 48 px action rows and no preference-save footer.
Keep important state text and connection errors visible; do not rely on color.

Use background `#171B1E`, panels `#242A2E`, controls `#30383D`, text `#F1F3F2`,
supporting text `#BBC3C7`, and primary/selected controls `#285D7A`. Amber marks
a preference issue, red a failed operation. Header state colors continue to
describe observed furnace state. Settings categories do not introduce machine
states. Use existing type styles and at least 48 px tap targets / 56 px primary
actions. Numeric entry uses a keypad, plus/minus, explicit units, and Apply /
Cancel; a slider alone is insufficient.

See the [clickable concept](../images/theme/hmi-web-demo.html). It is a
standalone browser sketch with illustrative values and local temporary edits,
not an LVGL render, actual device state, or a working connection implementation.

## Program preferences

| Field | Proposed meaning | Editor effect |
| --- | --- | --- |
| Maximum heating rate | Preferred HEATING rate in degrees Celsius per minute; initial increment 0.1 | Check both direct rate edits and rates calculated after duration/target changes |
| Maximum temperature | Preferred program target ceiling in degrees Celsius; initial increment 1 | Check explicit and inherited targets across all affected stages |
| Maximum program duration | Preferred planned total in hours/minutes; initial increment 1 minute | Recompute the whole plan after insertion, deletion, reorder, target, rate, or duration edits |

**Recommendation:** Use “Maximum program duration” rather than “Maximum
operational time,” which could imply an enforced run cutoff. Label the page
“Your limits guide program editing.” Include controller bounds as separate
read-only context in field details when current metadata exists. Never label a
preference as a safety limit.

Where a current controller upper bound applies, editing guidance uses the lower
of that bound and the selected preference, together with controller lower
bounds, increments, and field applicability. An empty feasible range is an
explicit conflict. Do not overwrite a stored preference when controller
capabilities change. Show the effective bound and explain the restriction.
Unknown or stale capabilities are never replaced by hard-coded furnace limits.
Local preferences can be edited offline, but controller compatibility remains
unverified until current metadata returns.

**Recommendation:** Apply rate preference to HEATING first. Controlled COOL
requires a separate decision and potentially a separate field; “fastest
permitted cooling” has no user-selected numeric rate and must not silently
become controlled cooling. A heating maximum must not be claimed to bound it.

Total duration includes all modeled heating, holds (including any initial wait),
and modeled cooling. Fastest cooling and physical execution can have uncertain
duration. Show “Estimated” or “Cannot determine total duration” as applicable;
never treat unknown as zero or certify an actual run deadline from a forecast.
The controller owns actual execution time and any enforced deadline. Manual's
separate deadline rule remains unchanged.

### Save and correction workflow

1. Edits remain in a local form until **Save preferences**. **Discard** restores
   the previously saved values. Leaving a changed form offers Save / Discard /
   Stay. Show a storage error instead of claiming success on a failed write.
2. Saving preferences never rewrites a saved program, history, or active run.
   Re-evaluate an open editor and mark its affected stages. Existing library
   programs are checked when opened/loaded, avoiding an unbounded background scan.
3. At stage editing and program-save review, show concrete issues, for example
   “Stage 2: 8.0 degrees C/min exceeds your 5.0 preference.” Recheck downstream
   inherited targets, rate/duration relationships, and total duration.
4. **Recommendation pending operator choice:** require correction or an explicit
   one-time **Keep these values** acknowledgement for preference-only issues.
   This acknowledgement applies only to the reviewed program content and
   preference version; edits invalidate it. It never bypasses controller
   restrictions or grants permission to run. This warning policy best matches
   “guide”; an HMI-local strict save gate is a viable alternative.
5. Offer individual corrections with a before/after preview. Reducing a heating
   rate can increase total duration; lowering a target can affect following
   stages. Recheck the whole candidate after correction. Never silently clamp
   values or shorten holds / raise rates to make the total fit.
6. Applying a correction changes the unsaved editor only. Actual program save
   remains a controller request and can fail or be rejected. The current lab
   must keep its explicit preview wording until that lifecycle is implemented.
7. At load/start review, explain relevant preference issues without modifying
   the accepted revision. Actual permission and acceptance remain controller
   decisions. Do not interrupt or stop an already running program because a
   preference changes.

No numeric defaults are selected by this proposal. The concept's numbers are
illustrative only. Decide whether each preference may be explicitly **Not set**;
never overload zero to mean unlimited. If allowed, explain that the user
preference is absent and controller validation still applies.

## Additional settings

| Section | Recommended contents | Ownership / availability |
| --- | --- | --- |
| Display & interaction, page 1 | Brightness; idle dim delay; reduced motion | HMI local; brightness/dimming require target support |
| Display & interaction, page 2 | Default Running view: Graph / Split / Furnace; touch feedback if hardware supports it | HMI local; preserve existing graph semantics and state visibility |
| Date & time | Current controller date/time; edit date/time; 12/24-hour display format | Controller clock; format preference HMI local |
| Date & time detail, later | Timezone and automatic time source / last successful synchronization | Add only after clock representation and synchronization ownership are defined |
| Connections | Server and Bluetooth status rows, each opening details | Controls require supported hardware/software, configuration enablement, and current permission |

Dimming should preserve legibility; propose no blanking during operation or
fault presentation. Wake interaction must not also activate the covered
control. Reduced motion suppresses decorative transitions only. Touch-feedback
volume is not an alarm mute. Keep graphite, English, and Celsius as current
information instead of offering unavailable themes, languages, or units.

Useful later additions are quick-launch arrangement (favourites already exist),
an estimation-only ambient default once its ownership is settled, and the
explicit reset scopes under Maintenance below. Do not introduce an ambiguous
factory reset. Put firmware versions, diagnostics,
service counters, and maintenance records in Device, linked if useful.

Exclude PID tuning, sensor calibration, protection thresholds, actuator policy,
controller configuration enable switches, counter resets, and generic remote
control permission from this screen. These require their own controller-owned
scope and permission design.

## Date/time and connection behavior

Date/time edits use a separate **Set date & time** action, not the local
preference-save transaction. Show current source/validity, input validation,
pending, accepted, rejected, and outcome-unknown states. After a timeout or
session change, read current authoritative state; do not replay automatically.
Changing wall-clock time must not redefine elapsed run time or rewrite history.
Whether edits are permitted during Manual/operation is controller metadata,
not a hard-coded HMI rule. Timezone/DST handling needs an explicit agreement
before implementation; do not invent a local authoritative clock.

Connections distinguish **Not supported**, **Disabled by configuration**,
**Checking availability**, **Not connected**, **Connecting**, **Connected**, and
**Connection failed**. Unknown is not disabled. Keep a compact disabled row
with the reason instead of an enabled switch that does nothing. A “Connected”
server status means the relevant service connection is established, not merely
that a network link exists. Bluetooth distinguishes radio availability from a
paired/connected device. Show Connect / Disconnect / Pair only when advertised
and permitted. Do not invent a Wi-Fi setup requirement before transport is known.

Server/Bluetooth availability and furnace-controller link status are separate.
Invalidate only metadata owned by the source that became stale. No credentials
in ordinary preferences, logs, or this concept. Pairing, endpoint editing,
credential storage, reconnect policy, and remote-control scope are deferred
until the actual connectivity owner and supported service are known.

## Maintenance

**User request:** Add Reset settings, Delete programs, Restart display, and
Attempt furnace restart. **Design recommendation:** name the tab Maintenance
and keep these four independent actions. See [proposed ADR-0010](../07-decisions/0010-maintenance-request-workflows.md).
This section describes the proposed interaction; it is not a production protocol.

| Action | Scope and confirmation | Completion / failure |
| --- | --- | --- |
| Reset settings | Opens explicit choices: Display & interaction preferences, Program preferences, or both; show exact fields/defaults before Reset preferences | HMI-local recoverable write; keep prior values on failure. Excludes programs, history, controller configuration/clock, credentials, pairing, and counters |
| Delete programs | Opens selection/review for selected or all library programs; final dialog shows names/count and says deletion cannot be undone | Controller validates IDs/revisions and permissions, persists and reports outcome. Do not remove local rows merely because a request was sent; no effect on immutable run history or active accepted snapshot |
| Restart display | Confirm temporary loss of display/controls and disposition of unsaved edits; restart HMI only | Resolve local save/discard first, finish or report pending local writes, then platform restart; reconstruct from a current controller snapshot after startup |
| Attempt furnace restart | Ask whether restart is appropriate now, then use the three branches below | Send an attempt only after confirmation; final execution/rejection belongs to the Control CPU |

Reset defaults are not yet selected. Preview the exact scope and defaults;
never use the concept's illustrative numbers as shipping defaults. Program
preference reset rechecks editor guidance without rewriting saved programs.
For program deletion, recommendation is selected/all review within this tab,
with the same individual deletion path already available under Programs.
If bulk deletion is not advertised, do not simulate an atomic operation by
silently issuing many deletes. Define partial outcomes if a batch is supported.
Changes to reviewed IDs/revisions invalidate the confirmation. The controller
may reject deletion of programs in use; HMI confirmation cannot override that.

Display restart is not a Stop request. Explain that it does not ask the furnace
to stop and observation/control through this display is temporarily unavailable.
Do not promise a particular furnace outcome; controller behavior remains its
own policy. Never replay pending furnace actions after HMI startup. Unresolved
controller operations remain unknown until authoritative reconciliation.

### Furnace restart assessment and confirmation

1. **Attempt furnace restart** begins a read-only suitability assessment with
   immediate pending feedback and a bounded deadline. Canceling this phase
   sends no restart request. Lack of reply is not an affirmative assessment.
2. Present one of the following confirmations, always with Cancel as the
   default focus and an explicit action label rather than ambiguous Yes:

| Assessment | Proposed dialog | Confirm action |
| --- | --- | --- |
| OK now | **Restart furnace controller?** The controller reports that restart is allowed now. Restart will temporarily interrupt communication. Continue? | Request restart |
| Busy / not advisable now | **Furnace is busy.** Show the controller reason. Restarting during work may interrupt the program, leave work unfinished, lose unsaved data, or damage the load/equipment. Do you still want to request a restart? | Attempt restart |
| No response before deadline | **No response from furnace.** Restart suitability could not be checked. The furnace may still be operating. Attempt a restart request if communication permits? | Attempt restart |

3. Confirmation sends at most one fresh correlated restart request through the
   defined controller request path. Carry the assessment context and the fact
   that the operator acknowledged busy/unknown status when the contract defines
   them. These describe intent, not authority. The controller rechecks its
   current condition even after an earlier OK. It can accept or reject any
   branch. A final rejection is shown as **Restart refused: [reason]**; it never
   leads to a bypass/force-reset action.
4. If transport/capability support makes an attempt impossible, report
   **Could not send restart request**. Do not queue it for reconnect. A timed-out
   assessment can still precede an explicit attempt while an eligible transport
   remains available. Never use stale session permissions or introduce an
   unapproved GPIO reset, power cycle, or alternate hardware path.
5. Distinguish sending, sent/awaiting result, accepted/awaiting reconnection,
   rejected, could-not-send, and outcome-unknown. Delivery is not acceptance;
   loss of communication is not proof of restart. A final-response timeout
   means **Restart outcome unknown**, not failed or successful. Recover by
   reading current session/state, never automatically repeating the request.
6. Bind assessment/confirmation to controller session and a bounded lifetime.
   Session changes or expiry invalidate the old confirmation; repeat assessment
   on fresh operator intent. Changed assessment information must not silently
   replace the dialog the operator is confirming. Ignore late/superseded replies.
   Disable repeated confirmation immediately. Define duplicate handling in the
   controller contract before enabling real retries.
7. After acceptance, display **Waiting for furnace controller**. A newly observed
   boot/session and coherent snapshot can establish that a restart occurred;
   causal attribution to this request requires correlated evidence. Same-session
   reconnection alone proves neither reboot nor request completion. Reconstruct
   the actual idle/Manual/Program/fault view and invalidate old capabilities and
   pending actions normally.

**Boundary interpretation:** The requested attempt after busy/no response is
supported as a controller-validated request. Forcing a hardware reset despite
controller refusal would violate ADR-0001 / ARCH-INV-001, 002, 007 and requires
explicit conflict resolution; it is not part of this proposal.

**Existing navigation fact:** Active/paused Program presentation suppresses
Settings until normal Stop (REQ-FUN-003). This proposal does not introduce a
backdoor to Maintenance from Running. Busy scenarios still matter for Manual,
other controller work, and state changes while an action is pending. Any
additional Running-to-Maintenance route requires a separate product decision.
Do not let confirmation dialogs obscure normal Stop where it is available;
underlying observed-state changes can dismiss/invalidate a maintenance flow.

## Review choices

- Preference violation: acknowledge once and continue (recommended), or require
  changing the program/preference before this HMI will submit a save?
- Heating-only rate initially (recommended), or also a separate controlled
  cooling preference?
- What defaults should ship, and may any preference be Not set?
- Does maximum duration mean a planned-duration preference as proposed, or is
  an additional controller-enforced deadline desired as a separate feature?
- Which CPU owns server/Bluetooth; what existing configuration and APIs expose
  enablement and permission; what is the server for?

These questions refine the implementation, not the user's established mandate
that preferences must not override controller configuration.
