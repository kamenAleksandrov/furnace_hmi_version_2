# Project status

**Fact (2026-10-28, AGENT-OPT-03; configuration/documentation only):**
bounded assignments, explicit subscription-included model/effort choices where
supported, direct routine work before inherited delegation, revision-bound
review reuse and concise evidence handoffs. Existing specialist/human review
gates remain required. [Policy](../agent-delegation.md),
[execution record and client results](../agent-client-compatibility.md).
Branch is stacked on unmerged AGENT-OPT-02 PR 10; no product requirement, UI,
simulator, firmware, phase or work-item maturity changed. Validation in progress.


**Fact (2026-10-27, AGENT-OPT-02; configuration only):** the product owner
granted standing authorization for autonomous native delegation, pushing
feature branches and opening pull requests (draft while blockers remain,
stacked on unmerged dependencies); merging always needs explicit owner
authorization, and delegation stays within the current subscription
entitlement with no paid usage. Recorded in the
[shared delegation policy](../agent-delegation.md) (byte-identical to the
furnace copy) and AGENTS.md; the [client evidence](../agent-client-compatibility.md)
notes that project agents are read at session start. Owner prompt: furnace
`prompts/agent-opt-02-standing-authorization.md`. Production UI/firmware/simulator
behavior and development phases are unchanged.

**Fact (2026-10-26, AGENT-OPT-01; configuration only; native delegation validated):**
Added native Claude/Codex roles and a [shared delegation policy](../agent-delegation.md),
kept the six existing domain reviewers and required review gates. See
[execution record](AGENT_OPT_01.md) and [client evidence](../agent-client-compatibility.md).
Codex native custom delegation and changed-document link checks passed.
After quota reset, Claude explorer checks passed in both repositories; the
other three role types passed read-only configuration checks. Native traces
confirm model inheritance, five completed children and no nested spawns.
The canonical generated index check remains blocked by missing build metadata
in this worktree. See the
client evidence for exact commands, changed paths and follow-up.
Production UI/firmware/simulator behavior and development phases are unchanged.


**Fact (2026-10-26, documentation only; product owner request):** recorded the
required diagnostic presentation after the furnace diagnostics audit: a
controller-backed Device log viewer with filters, paging and export where
supported, a system-health view in which unknown is never shown as healthy,
and separate controller-validated recording-schedule controls, without
accounts: REQ-FUN-029, TEST-017,
[ADR-0020](../07-decisions/0020-diagnostics-presentation.md),
CTRL-HEALTH, CTRL-RECORDING-SCHEDULES, CTRL-EVENT-LOG and the Device-log open
question. Furnace references: DIAG-06…DIAG-14, API-10, WI-H12 (Phase 21),
WI-H13 (Phase 25). No UI, simulator, firmware, font or build file changed.

**Fact (2026-10-23, documentation only; product owner request):** recorded the
approved duration, quality-decision and runtime presentation: advisory versus
maximum program duration with acknowledgement at Start, accepted duration,
overrun and runtime limit, a bounded quality decision with Continue or Stop,
fault categories, tolerated Hold excursions, the Start measurement
qualification, chamber temperature as the process measurement, completion not
shown as a verified cure, and no roles: REQ-FUN-028, TEST-016,
[ADR-0019](../07-decisions/0019-duration-decisions-and-runtime-presentation.md) (amends ADR-0018),
CTRL-DURATION-DECISIONS, the Program execution section, the command lifecycle
and the open questions. Fields follow the furnace interface specification
0.8.0 draft. No UI, simulator, firmware, font or build file changed.

**Fact (2026-10-22, documentation only; product owner request):** recorded the
controller fault manager and its presentation (fault entries, acknowledgement
and recovery as separate requests, inhibition reasons, event history), the
corrected rule that a required-sensor failure is a latched critical fault, and
the Hold setpoint approach: REQ-FUN-027, TEST-015, REQ-FUN-022 amended,
[ADR-0018](../07-decisions/0018-fault-manager-and-hold-approach-presentation.md) (amends ADR-0014 and ADR-0017),
CTRL-FAULT-MANAGER, the Program execution section and the open questions.
Fields follow the furnace interface specification 0.7.0 draft. No UI,
simulator, firmware, font or build file changed.

**Fact (2026-10-22, documentation only; product owner request):** recorded the
approved fault classes and their presentation (recoverable fault pause with
explicit Resume; latched critical fault and protective trip with Stop only;
acknowledgement never restarts heating; transition deviations reported;
control diagnostics only in a service view): REQ-FUN-026, TEST-014,
[ADR-0017](../07-decisions/0017-fault-classes-and-control-diagnostics-presentation.md),
ADR-0016 amended, CTRL-FAULT-CLASSES, the Program execution section and the open
questions. Fields follow the furnace interface specification 0.6.0 draft. No UI,
simulator, firmware, font or build file changed.

**Fact (2026-10-20, documentation only; product owner request):** recorded the
approved program execution behavior and its presentation (temperature-driven
Heating completion, ramp holdback, already reached Heating stages, recoverable
Hold interruption, operator Pause and explicit Resume, distinct outcomes, no
curing acceptance without a policy): REQ-FUN-025, TEST-013, REQ-FUN-024 and
TEST-012 amended, [ADR-0016](../07-decisions/0016-program-execution-presentation.md),
CTRL-RUN-EXECUTION and the open questions. No UI, simulator, firmware, font or
build file changed.

**Fact (2026-10-18, documentation only; product owner request):** recorded the
approved Hold timing policy (treatment timer only after qualification, no credit
before it, full duration after a hot start) and its presentation: REQ-FUN-024,
TEST-012, [ADR-0015](../07-decisions/0015-hold-qualification-and-treatment-time.md),
CTRL-HOLD-QUALIFICATION and the open questions. No UI, simulator, firmware, font
or build file changed.

**Fact (2026-10-16, Phase 05 documentation only; product owner request):**
recorded the live hot-start confirmation without a normal timeout, sensor-fault and
degraded-status presentation, and the Manual six-hour default versus the 48-hour
maximum: REQ-FUN-005 and REQ-FUN-019 amended, REQ-FUN-021 to REQ-FUN-023 added,
PROTO-012 and TEST-011 updated, [ADR-0014](../07-decisions/0014-live-hot-start-confirmation-and-sensor-faults.md).
No UI, simulator, firmware, font or build file changed.

**Fact (2026-10-16, documentation only; product owner request):** recorded the
explicit-Start and hot-start requirements coordinated with the furnace
controller: REQ-FUN-018 to REQ-FUN-020, PROTO-012, TEST-011 and
[ADR-0013](../07-decisions/0013-explicit-start-and-hot-start.md). **Start again**
is now an explicit Start evaluated by the controller. No UI, simulator, firmware or
build file changed; the labs still start at once (recorded gap).

**Fact (2026-10-06, feedback round 3; product owner request):** in both
presentations Stop on a program run now cuts the run where it stands and lets
the controller cool automatically on Running ("Stopped · automatic cooling"),
with **Start again** (the same program from the start) and **Return Home**;
Settings > System > Delete programs removes every program after one
confirmation naming them (single programs are deleted from their details).
See round 3 of the [feedback round plan](OPERATOR_FEEDBACK_2026_10_06_PLAN.md).

**Verification:** native CTest 13/13 at three sizes without build warnings;
web smoke, generator checks and lint pass. Physical panel not tested.

**Fact (2026-10-06, feedback round 2; product owner request):** in both
presentations Manual Stop (and the 48 h end) now goes into a Cooling state on
the Manual page with Exit and Start new; the Manual readings fit 800 px (label
over value); Settings > System Delete programs and Reset settings act for real
(program toggles; display or date & time format scopes); a Date format
preference (five formats) drives every date on screen. Maintenance done is
recorded as a quality-of-life aid, not warranty-based. Web page only: one
faint development note at the bottom and a Work in progress pill. See round 2
of the [feedback round plan](OPERATOR_FEEDBACK_2026_10_06_PLAN.md).

**Verification:** native CTest 13/13 at 800x480, 800x640 and 1024x600 without
build warnings; web smoke, generator checks and lint pass. Physical panel not
tested.

**Fact (2026-10-06, operator feedback round; product owner request):** in both
presentations the program library is three epoxy cure programs (**Epoxy system
1/2/3**, replacing the twelve ceramic/glass fixtures); a program created or
edited in the editor is saved for the session, opens its details, is listed,
can be started, run through automatic cooling to Complete, and deleted (Save
stays disabled with the first problem shown). The native lab now has a real
program store, and Start, Pause, Resume and Stop are controller requests that
the lab hands to the simulator, which runs any program. A Maintenance task opens
with a corner X and **Maintenance done** (a controller request; the task starts
a new interval and the record leads Service history). Device info drops the
Controller link card; Support opens the support QR code (theme tokens
`qr-plate`/`qr-ink`, generated by `tools/build_support_qr.py`). The Device log
is an event feed in the style of the Ajax Systems app: filter tabs, date
headings with their age, one month of history and this session's events, pages
that fit the screen. Web demo page only: the real Telamorph logo colours, the
logo as tab icon, a development watermark, an intro, no Door opened scenario,
and a page redraw when the screen size changes (Quick launch clipped before).
Details and decisions in the [feedback round plan](OPERATOR_FEEDBACK_2026_10_06_PLAN.md).

**Verification:** native MSVC builds without warnings and CTest 13/13 at
800x480, 800x640 and 1024x600 (`-E codebase_index`), with new smoke checks
(create/save/list/start/complete/delete, pause/resume/stop requests,
Maintenance done, Support QR, Device log history and filters) and captures;
web smoke passed with the matching checks; all generator `--check`s and the
theme/language lint pass; the support code decodes to `https://telamorph.com/`.
Physical panel and phone scanning of the on-screen code not tested.

**Fact (2026-10-05, theme and language readiness):** both presentations take
every colour from one theme source (`app/hmi/presentation/theme/themes.json`,
98 tokens, contrast-checked) and every visible text from one string catalogue
(`app/hmi/presentation/strings/catalogue/en.json`, 749 strings, checked for
printf shape, `{field}` patterns and font glyphs); generators write the C
modules and the web copies. Both switch theme and language at runtime (screen
rebuild in the GUI context); the Settings Theme and Language cards become
choices as soon as a second option exists. Number formats, date/time order,
the keypad decimal key and the keyboard rows are catalogue strings. Proposed
[ADR-0012](../07-decisions/0012-theme-and-language-catalogues.md); rules in the
[theme-and-language skill](../../.agents/skills/theme-and-language/SKILL.md);
details, small deliberate visual changes and follow-ups in the
[readiness plan](THEME_AND_LANGUAGE_READINESS_PLAN.md). The light theme itself
is the next step.

**Verification:** native MSVC builds without warnings and CTest 13/13 at
800x480, 800x640 and 1024x600 (`-E codebase_index`), including new palette,
language, generator, lint, probe-theme and pseudo-language checks; web smoke
passed with probe-theme, pseudo-language and Theme/Language choice checks;
captures of both presentations compared against a pre-work baseline (at most
4 of 255 levels apart from listed wording alignments); Zephyr qemu_x86 build
and Twister 3/3 passed. Physical panel not tested.

**Fact (2026-10-05, Settings menu clean-up; product owner request):** in both
presentations the Settings **Maintenance** tab (reset, delete programs,
restart) is renamed **System** with a new power icon, so it is no longer
confused with Device Maintenance (service tasks, wrench). **Connections** is
hidden by default and listed only when the controller reports server/Bluetooth
enabled (simulated as disabled; the disabled/unsupported/checking rows are
removed). Date & time drops the Clock source card; Time format fills the
column. The Brightness card shows only the gauge, without a percentage, with
its heading just above the bars. Dim when idle defaults to Off, including
after Reset settings. Recorded in the
[Settings proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md) and
[UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md#device-and-settings-card-pages-2026-10-01).

**Verification:** web smoke passed with new rail (no Connections, System with
power icon, Connections when enabled, fallback to Display), no-percentage and
Dim-Off checks; native MSVC builds without project warnings and
`ctest -E codebase_index` passed 9/9 at 800x480, 800x640 and 1024x600 with the
same checks; icon sprite and LVGL icon `--check` pass; captures of both
presentations compared. Physical panel not tested.

**Fact (2026-10-03, typeface and LVGL parity):** both presentations use
**Tomorrow HMI**, our modified Tomorrow (SIL OFL 1.1, no Reserved Font Name;
commercial bundling allowed, the copyright notice and licence must ship with
the product): equal-width digits, Medium for body text and SemiBold for strong
text (product decision; details in the
[fonts README](../../app/hmi/presentation/fonts/README.md)). Grey text is at
least 13 px and the grey is slightly lighter (#b9c1c5). LVGL caught up with
the web on the remaining known gaps: the Running status reads "3 / 4 - Heating"
from a new HMI-only `stage_kind` in the model (falls back to the controller's
stage name when unknown), the furnace illustration uses the web drawing and
panel share, and single-button dialogs are centred. The web Manual strip now
reads "Current °C" like the rest.

**Verification:** web smoke passed; native MSVC builds without warnings and
CTest passed at 800x480, 800x640 and 1024x600 (codebase-index excluded, known
local failure), including the new stage-kind model checks; font generator
`--check` passes; side-by-side captures of both presentations compared.
Physical panel rendering not tested.

**Follow-up:** the product must carry the font's copyright notice and OFL text
(manual or an on-device licences screen) before it ships.

**Fact (2026-10-02, simulated readings and side menu):** in both simulators a
healthy run's measured trace now tracks the plan like a controller holding a
setpoint: within 0.5 C on the web (a short lag and overshoot at stage
corners), and on the plan itself natively, whose readings are whole degrees.
It previously trailed up to 8 C (web) and 3-18 C (native) below. Fault
scenarios still depart from the plan. Demo page only: the fullscreen Controls
tab is narrower with a softer, slower two-pulse glow, and the side-menu scrim
fades instead of snapping on and off.

**Verification:** web smoke passed; worst healthy web tracking error over all
twelve library programs is 0.5 C; native MSVC CTest passed at 800x480,
800x640 and 1024x600 (codebase-index excluded, known local failure).

**Fact (2026-10-01, Device and Settings card pages):** in both presentations
the Device and Settings sub-pages are grids of rectangular cards that fill the
page; whole cards are tappable. Display & interaction is one page with a
tappable brightness gauge, dim choices (Off, 1, 2, 5, 10, 30 min) and a Running
view choice; Date & time has a large clock and a new **Set date & time**
dialog; Device Maintenance, deferred since 2026-09-23, now lists every
catalogue task as a status card (the web placeholder is gone). A SemiBold 48
Inter size was added for the clock. See the
[card pages plan](DEVICE_SETTINGS_CARDS_PLAN.md).

**Verification:** web smoke passed with new card, gauge, choice and date/time
checks; native MSVC builds without warnings and CTest passed at 800x480,
800x640 and 1024x600 except the known local `tools.codebase_index` failure;
captures of both presentations compared. Physical touch not tested.

**Follow-ups:** light theme (requested for later); the native single-button
dialog centring noted on 2026-10-01 is unchanged.

**Fact (2026-10-01, units and demo page look):** both presentations show
`°C` / `°C/min`, with the unit in the field label where there is one (Running,
Manual and Home readings, Manual stepper captions, `Max. °C`) and a suffix
elsewhere; estimated power follows the same rule (`Est. power kW`). The web
demo uses one panel order everywhere, always-listed scenarios, a frameless
floating HMI, a boxed website bar and one static blue floor glow.

**Verification:** web smoke passed; native MSVC CTest passed at 800x480,
800x640 and 1024x600 except the known local `tools.codebase_index` failure.

**Fact (2026-10-01, demo layout follow-up):** the web demo page fits one
window without scrolling (the panel scrolls inside); phones, widths below
1181 px and fullscreen use a side menu (Screen, Playback, folded Scenarios,
keyboards, Program library) with a more visible fullscreen tab. Stale Device
Components/Info no longer push the pager off the page (web and native).

**Verification:** web smoke passed with new layout, menu-order and stale-Device
checks; native MSVC CTest passed at 800x480, 800x640 and 1024x600 except the
known local `tools.codebase_index` failure.

**Fact (2026-10-01, web demo showcase and controller alerts):** The web demo
page is a Telamorph-styled showcase: one control panel (seven fast scenarios,
playback with previous/next stage, pause and 300x default speed, screen size,
fit/actual pixels, fullscreen, program library, keyboards) that becomes a side
menu in fullscreen, plus Back to website and Get a quote. Both presentations
gained a **controller alert** dialog (fault kinds, connection lost/restored)
whose Close is a local dismissal only, a Running fault state with Return Home,
and a link-lost Running state with stale readings and Pause/Stop unavailable.
The model has an HMI-only `fault_kind`. Alarm acknowledgement and latching stay
open (CTRL-ALERTS). See the [showcase plan](WEB_DEMO_SHOWCASE_PLAN.md).

**Verification:** web smoke passed; native MSVC CTest passed at 800x480,
800x640 and 1024x600 except the known local `tools.codebase_index` failure.
Website sync not run (the website copy has uncommitted local changes);
physical touch not tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 6 of 6; port complete):**
Native Home, Device, Settings, keyboards and dialogs now match the web demo:
Quick launch trimmed to fit, sentence-case door/fan wording and "1,450 rpm",
five-row Device and Settings pages with status-edged Device rows, the web
keyboard groups and numeric grid without a sign key, 430 px dialogs with equal
centred buttons, and the web Delete program wording. The unreachable native
Stage detail page is removed. A Running page rebuild when measured cooling
crossed the graph window during telemetry is fixed. Remaining known
differences are listed in the
[port plan](LVGL_SOFT_TILE_PORT_PLAN.md) (controller stage name in the Running
status, furnace illustration, device catalogue texts).

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
without project warnings; web smoke passed; font/icon generators and codebase
index `--check` pass; Zephyr workflow 3/3. `tools.codebase_index` CTest still
fails locally (MinGW). Physical touch not tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 5 of 6):** Native Running and
Manual now match the web demo: readings above the graph, the start-of-cooling
window with Show/Hide cooling and blue dashed measured cooling, the Manual
readings strip, forecast with a "now" marker and one-piece steppers, and the web
graph caption, legend and axes on every graph. Manual Setpoint shows "--"
because the observation model has no controller setpoint (tracked as
CTRL-SETPOINT).

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
without project warnings, including rewritten Manual geometry and new
cooling-toggle checks; Zephyr workflow 3/3. `tools.codebase_index` still fails
locally (MinGW). Physical touch not tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 4 of 6):** The native stage
editor is now the web's fixed-height dialog over the page it came from, with
delete / movable position / add, Heating and Hold on the same rows (Hold locks
Target and Rate), validation that disables Save, and Discard restoring the whole
session. The UX baseline and screen concepts record the dialog. Stage edits
remain a local draft; the Control CPU alone accepts programs.

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
without project warnings, including new add/move/delete/discard/save/Hold
checks. `tools.codebase_index` still fails locally (MinGW). Physical touch not
tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 3 of 6):** Programs match the
web demo in both presentations. Program details start a program directly after
a confirmation dialog; the Load page is reached only from Home Quick launch.
Program stages, stage cards, library cards and the program editor toolbar use
the web layout. The UX baseline, screen concepts and program/startup model
record the new start flow; the Control CPU still decides whether a run starts.

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
without project warnings, including new start-confirmation checks; web smoke
passed. `tools.codebase_index` still fails locally (MinGW). Physical touch not
tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 2 of 6):** Both presentations
now use the Inter typeface (Regular and SemiBold, SIL OFL), generated for LVGL
and embedded in the web demo by `tools/build_fonts.py`. The native UI lab shows
the shared icons (A8 images from `tools/build_lvgl_icons.py`) on the same
buttons as the web, bold important/selected actions, the web pager, segmented
Running view switch and Settings/Device rows. Two web layouts were widened
slightly because Inter is wider than the previous system font.

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
without project warnings; web smoke passed; all three generators pass
`--check`. `tools.codebase_index` still fails locally (MinGW compiler cannot
compile any C file). Physical touch not tested.

**Fact (2026-09-30, LVGL soft-tile port, milestone 1 of 6):** The native UI lab
now uses the soft-tile theme of the web demo: its palette, square corners, tile
faces with hairlines and lift, the fading 2 px edge on the side facing away from
where a tile sits, amber selection glow, and the role colours (amber primary and
Cancel, yellow Edit, red destructive, steel navigation). Page layouts, Inter and
icons are still the previous native ones; see the
[port plan](LVGL_SOFT_TILE_PORT_PLAN.md) for the remaining milestones.

**Verification:** MSVC builds and CTest 9/9 at 800x480, 800x640 and 1024x600
with no project warnings; captures reviewed against the web demo. The
`tools.codebase_index` test was not passing because the local MinGW compiler
fails on any C file; the index itself was regenerated and `--check` passes.
Web demo unchanged, so its smoke was not rerun. Physical touch not tested.

**Fact (2026-09-27, editor rework, 12 px borders, operator text):** In both
presentations the light strip is a 12 px top border, so content sits 12 px from
every edge. The program editor has Back / name / + Add stage / Edit stages in one
row, tappable stage cards, no statistics and no bottom navigation, and asks before
discarding changes. Emulator and concept explanations were removed from HMI text
(web and native catalogue); the native simulation marker remains. Native overlays
now dim the full screen, and native editor Cancel/Save no longer return to the
editor after a stage was opened.

**Verification:** Browser smoke passed with new editor, dirty-prompt and border
checks. Native MSVC 10/10 CTest passed at 800x480, 800x640 and 1024x600 with new
leave-with/without-changes assertions; desktop preset 10/10 and the Zephyr
workflow passed. Captures of both presentations were inspected. Physical touch
was not tested.

**Fact (2026-09-26, touch keyboard layout):** In both presentations the text
keyboard has no empty slots (`-` completes the a-l row; Caps and Delete are
1.5 keys wide around z-m) and its rows fill the panel height. The numeric
keypad is a 460 px centred panel with full-height rows; unavailable sign/decimal
keys are dimmed. Native captures `keyboard-text` and `keyboard-numeric` were added.

**Verification:** Browser smoke passed. Native MSVC builds and 10/10 CTest passed
at 800x480, 800x640 and 1024x600 without warnings; keyboard captures of both
presentations were inspected. Physical touch was not tested.

**Fact (2026-09-26, Program details thirds and Running reading line):** In both
presentations Program details has View stages / Edit program / Load program as
three equal bottom actions, with the graph over the first two and the facts over
the third. Running shows a single reading line in every view; automatic cooling
keeps curing Elapsed and adds Cooling and Est. remaining only. The shared HMI
model gained a fail-closed run phase, cooling elapsed, estimated cooling remaining
and a per-sample planned-cooling flag; the semantic simulator gained a `cooling`
scenario (key 9), and native Running draws the planned cooling as a dashed line.

**Verification:** Browser smoke passed with new one-line curing/cooling checks.
Native MSVC UI-lab builds and 10/10 CTest passed at 800x480, 800x640 and
1024x600 with no compiler warnings, including new cooling-reading and overlay
assertions; the MSVC desktop preset passed 10/10. Captures of both presentations
were inspected. `run_foundation.py zephyr` (qemu_x86 build, 3/3 test cases and
static analysis; only the two known upstream malloc.c warnings) and `tooling`
passed. Firmware, protocol and physical touch were not run.

**Fact (2026-09-26, Program details and cooling preview):** In both
presentations the light strip is 24 pixels high. Program previews show only the
start of automatic cooling as a dashed green segment (at most a quarter of the
curing time), while Running keeps the full estimate. Native Program details
places Duration above its values. View stages is outline-only, and Edit program
is icon-only on details and stages.

**Verification:** Browser smoke passed. Native MSVC builds and 10/10 CTest passed
at 800x640 and 1024x600, and the detail/stages captures of both presentations
were inspected. 800x480 compiled but could not link while the UI-lab window was
open. No firmware, protocol, physical-touch or controller checks were run.

**Fact (2026-09-26, light strip and Manual graph):** The top accent/light
target was reduced from 48 pixels (later to 24, above) in both presentations. The browser Manual graph now renders at its own pixel size
instead of stretching a fixed 420 x 180 view box, and uses the native axis
labels. The native Manual page capture is now `manual-setup`; the scenario
capture named `manual` had been overwriting it.

**Verification:** Browser smoke passed, including a new 1:1 graph-scale check.
Native MSVC builds and all 10 CTest checks passed at 800x640 and 1024x600. The
800x480 link was blocked by an open UI-lab window and was not rerun. No
firmware, protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-26, time/keyboard/light refinement):** The Manual page title is
Manual mode. Both browser and native Running views show elapsed and remaining
together on one line, without the former time toggle. Estimated readings use
Est.; the text keyboard has one digits-plus-letters layout and numeric entry
keeps its separate keypad. The persistent top accent has a 48-pixel, full-width
click target in both presentations.

**Verification:** Browser smoke passed keyboard persistence, time-line overflow
and light-target dimensions. Native build and all 10 CTest checks pass at
800x480, 800x640 and 1024x600, including the light-target bounds, both time
values, unavailable remaining time and compact-stage geometry. No firmware,
protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual action and scale stability):** Manual Start, Stop,
Pause and Resume now preserve the existing native LVGL page and trajectory;
only state text and action visibility change. Both Manual renderers show the
full supplied 0--180 C controller range instead of auto-zooming around the
chosen target.

**Verification:** Browser smoke passed. Native MSVC builds and all 10 configured
CTest checks passed at 800 x 640 and 1024 x 600 after the change. The default 800 x 480
rerun remains pending while its UI-lab executable is open; no firmware,
protocol, physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual graph/input stability):** The browser Manual graph
now fills its card and its lower-band live text is larger. Browser simulation
does not redraw the route while a touch keyboard is open, so numeric entry
remains available. Native Manual target/rate adjustments retain the existing
LVGL trajectory and update its labels/points in place rather than rebuilding
the page and replaying the graph's minimum-to-final-size layout.

**Verification:** Browser smoke passed keyboard-persistence and graph-fill
checks. Native build and all 10 CTest checks passed at 800 x 640 and 1024 x
600, including the assertion that a Manual target adjustment retains the same
trajectory object. Default 800 x 480 rerun remains pending until the currently
open UI-lab executable is closed. No firmware, protocol, physical-touch or
furnace-control verification was run.

**Fact (2026-09-24, Manual lower-band alignment):** Both Manual renderers now
keep the prediction graph free of operational text. Live state and Curing
elapsed are at the top of the lower control band, with Start/Pause/Stop on the
left and target/rate adjustment controls on the right. The shared run labels
are Start, Pause, Resume and Stop; operator controls do not carry a `preview`
or demo qualifier.

**Verification:** The browser smoke passed. Native LVGL build plus all 10 CTest
checks passed at 800 x 480, 800 x 640 and 1024 x 600, including geometry
assertions for the new context and control ordering. No firmware, protocol,
physical-touch or furnace-control verification was run.

**Fact (2026-09-24, Manual layout refinement):** The browser Manual forecast
now receives roughly the upper 70 percent of a vertical split; the lower 30
percent is a compact control band. In both browser and native LVGL source, the
temperature/rate label leads a larger, dark, industrial-yellow-outlined
plus/minus control band anchored above the bottom run actions. Native Manual
also presents `Curing elapsed` from the observed dashboard state; the browser
uses its declared session-scoped simulator for the same presentation.

**Verification:** Browser smoke passed with graph-height, control-size,
label-placement and bottom-action assertions. Native MSVC builds and all 10
CTest checks passed at 800 x 480, 800 x 640 and 1024 x 600, including the
Manual elapsed/control geometry capture path. Whitespace and regenerated-index
checks passed. No firmware, protocol, physical-touch or furnace-control
verification was run.

**Fact (2026-09-24, shared UI parity refinement):** Both the native LVGL
UI-lab and canonical browser demo now remove the page/status bezel from every
page, expanding content below a persistent industrial-yellow top line. The
line is the simulated interior-light request control and glows on the observed
fixture state. Manual now has only a current-target prediction graph, direct
and +/- target/rate controls, and Pause/Stop; its 48-hour browser simulation
ends heating, returns to Manual setup and states that cooling is automatic.
Stage cards and Program details share the revised composition: type/ordinal/
duration/temperature cards, then stage count/max temperature and three Duration
fields. The native lab remains presentation-only; the browser remains a
session-scoped controller simulation.

**Verification:** Native MSVC UI-lab build and configured CTest passed 10/10,
including visual geometry and top-line-light smoke coverage. The strengthened
headless browser smoke passed. No physical touch review, firmware build,
website deployment or real Control CPU/light integration was run.

**Fact (2026-09-23, first operator-workflow implementation slice):** The native
LVGL UI-lab and canonical browser demo now include Manual mode, controller-style
demo request acceptance, 48-hour curing limits, automatic cooling estimates,
program creation/editing, read-only authored stage cards, Device log and
Maintenance routes, and the industrial-yellow accent. The browser demo keeps
session events in volatile memory so it can be demonstrated without a furnace.
Native LVGL remains a host presentation fixture; production controller/protocol
integration, furnace-side light/door behavior, and real maintenance records
remain deferred in [the controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md).

**Verification:** Native MSVC UI-lab build passed; configured CTest passed 10/10
including geometry and semantic smoke; the extended headless browser smoke
passed; and `git diff --check` passed. Physical touch review, website deployment,
firmware builds, and real Control CPU integration were not run.

**Fact (2026-09-23, planning only):** [Workflow plan](OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md)
and [screen concepts](../03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md) cover Manual,
program creation, automatic cooling and simpler navigation. The searchable
[controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md) preserves deferred tasks.
REQ-FUN-005/017 reconciliation is the first milestone. Accepted requirements and
application behavior are unchanged; layouts are not yet visually verified.

**Fact (2026-09-18, browser demo P1):** The canonical
[hmi-web-demo.html](../images/theme/hmi-web-demo.html) now has the shared
outer viewport/toolbar packet: proportional logical-size fitting, inspect mode,
portrait rotate hint, fullscreen fallback, advanced fixture disclosure and
explicit dialog geometry. The durable
[hmi_web_demo_p1_smoke.cjs](../../tools/hmi_web_demo_p1_smoke.cjs) passed in
headless Chrome for 800 x 480, 800 x 640, 1024 x 600 and emulated 390 x 800
portrait. Real phones, other browsers, visual screenshot comparison, website
sync/build/deployment and native/firmware checks were not run. P2-P7 remain
pending. No production authority, protocol or controller behavior changed.

**Fact (2026-09-18, planning only):** A [client web demo agent build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) now specifies ordered single-HTML work packets for faithful HMI presentation, simulated controller responses, authoring, scenarios and phone/fullscreen use. The confirmed scope is session-only programs with premade examples, an ordinary online webpage and English only. No demo, native code or website deployment changed in this planning pass.

## Fact (host and browser presentation)

The [running readings refinement](RUNNING_READINGS_REFINEMENT_PLAN.md) adds a
tappable elapsed/remaining total, filled readings, and a transparent outlined
time control without a trailing swap glyph. Power is presented as one reading
without a secondary Estimate caption. It also supplies live full-graph
readings.
Native builds and 10/10 tests pass at all three review sizes (67 captures each);
Chrome running layout/toggle checks pass at the same sizes. Existing keyboard,
program title, star and graph refinements are preserved. Host/browser only.

Last updated: 2026-09-16

**Fact (planning only):** A [Settings screen proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md),
clickable browser concept, and [implementation plan](SETTINGS_IMPLEMENTATION_PLAN.md)
have received operator approval of the UI direction and a requested Maintenance
extension (reset preferences, delete programs, display restart, and assessed
furnace restart attempts). The updated concept remains simulated;
[ADR-0010](../07-decisions/0010-maintenance-request-workflows.md) is proposed.
User preference limits remain separate from
controller configuration. Warning policy, defaults, cooling scope, clock and
connectivity integration remain proposals/open choices; runtime is unchanged.

**Fact (host-only UI-lab):** The Settings concept is now implemented in native
LVGL for the five category routes, Display subpages, local preference editing,
and simulated Maintenance confirmations. The host MSVC build, full 10/10 CTest
preset, Settings smoke interactions, and 800 x 480 native geometry captures
passed. Production controller, protocol, storage, clock, connectivity, and
hardware behavior remain unimplemented.

**Fact (Device host-only presentation):** The shared browser concept now has a
native LVGL Device implementation covering Overview, Device log, Maintenance,
Service history and Device info. The editable
[`device_service_catalogue.txt`](../../simulator/ui_lab/data/device_service_catalogue.txt)
is read before LVGL startup, validated into bounded records, and rendered by
generic device/task views; service intervals are not hardcoded in the
renderer. Native smoke and geometry checks pass at 800 x 480, 800 x 640, and
1024 x 600. The [Device proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md),
[plan](DEVICE_IMPLEMENTATION_PLAN.md), and [proposed ADR-0011](../07-decisions/0011-component-service-catalogue.md)
still govern the unresolved production contract. Example values are synthetic;
controller-owned counters, service policy, history, persistence, protocol, and
hardware behavior are not implemented.

**Fact (Programs host-only presentation):** Native LVGL Programs, Program
details and Program stages now implement the approved styling pass while
preserving the existing fixture content, route meanings, graph-node/editor
routing, Load boundary and local preview semantics. Library cards use neutral
aligned summaries; paging is in the action toolbar; detail facts are grouped
beside the graph; Delete is a restrained red-outline action; and stage cards
use outlined ordinal badges with a separate inherited-target caption. The
[style proposal](../03-hmi/PROGRAMS_STYLE_PROPOSAL.md) and
[plan](PROGRAMS_STYLE_PLAN.md) govern this presentation-only change.

The native MSVC UI-lab build and full CTest preset pass at 800 x 480,
800 x 640 and 1024 x 600, with zero capture-geometry errors. This remains
host-only HMI presentation evidence: no controller, protocol, firmware,
persistence, physical touchscreen or safety behavior changed.

**Fact (browser demo extension):** The shared HTML now opens Home and supports
quick launch, Load/Start demo, Graph/Split/Furnace Running, pause/resume/Stop,
and local stage editing through cards/graph nodes. All-stage Save/Discard and
run-copy isolation are browser-only. See the [demo plan](HOME_RUNNING_DEMO_PLAN.md).
Native firmware and website deployment are unchanged by this extension.

**Fact (browser and native keyboards):** Full text/numbers/symbols and
numeric/sign touch keyboards now open from program/stage names, Settings
values and editable stage values. Apply/Cancel preserve local transaction
boundaries. The host-only native LVGL lab mirrors the layouts, uses an active
up-arrow Shift key, page-matching `#+=`/`123` More labels, and routes program,
stage and Settings edits into bounded local state. Browser verification passed
15 layouts at three sizes; native smoke and geometry checks pass at the three
native review sizes. The latest refinement displays Delete, keeps the
number/symbol cycle closed, and retains active Shift feedback. See the
[keyboard plan](TOUCH_KEYBOARD_PLAN.md). Embedded keyboard/controller behavior
is not implemented.

## Current gate

**Fact:** The [Graphite UI completion plan](GRAPHITE_UI_COMPLETION_PLAN.md)
has completed implementation and native visual inspection at all three review
sizes. The [verification record](GRAPHITE_UI_VERIFICATION.md) contains the
per-page matrix, actual screenshots, logs, launch commands, and remaining limits.
Final documentation/index verification passed. Operator approval and
physical-panel testing remain separate pending gates.

A local program-edit follow-up is also complete at 800 x 480: it refines the transparent notification header and dotted status pill, empty quick launch, program management controls, direct 3 x 3 stage editing, and bounded local all-stage save/discard preview. The host MSVC UI-lab CTest preset passed 10/10 and its capture run reported zero geometry errors; wider viewport and physical-panel review were not repeated for this follow-up.

A Running-view presentation follow-up is complete at all three review sizes:
unboxed separated view selectors, centered full-graph statistic cards without
vertical separators, greater bottom-axis clearance, slightly inset
temperature labels, and high-contrast amber measured trace. Fresh captures
reported zero geometry errors and each host MSVC UI-lab CTest preset passed
10/10.

The repository foundation and first desktop-visible UI vertical slice are
complete in the local worktree. The current host-only UI-lab follow-up is
implemented and headlessly verified: Home is a furnace overview with local
favourites/recent quick launch, Programs has two full six-card library pages
plus loading/overview/stage/editor routes, and a dedicated Running page has
graph/split/visual modes with ordinary navigation contained. The latest visual
pass applies the graphite instrument-panel visual system across every existing
route: shared charcoal surfaces, restrained deep-blue navigation, grouped
readings, deliberate state accents, stable planned/measured graph colors,
proportioned furnace visuals, and touch-sized controls. It keeps the running
graph's complete predicted domain until live data leaves its headroom and
gives the continuous bottom-navigation control an owner-only active-tab glow.
HEATING, HOLD,
and COOL are presentation/draft assistance only. The lab starts fail-closed in
`UNAVAILABLE`, withholds operational values and controls until current state
exists, and preserves the Control CPU authority boundary. Lab edits and action
buttons are local previews only; furnace business logic, production protocol
semantics, controller state machines, and operational screens remain absent.

## Foundation evidence

| Area | State | Evidence |
| --- | --- | --- |
| Documentation vault and governance | Complete | [Index](../INDEX.md), root `AGENTS.md`, root `PLANS.md` |
| Requirements and architecture | Baseline complete; architecture findings fixed | [Requirements](../00-project/REQUIREMENTS.md), [ADRs](../07-decisions/README.md), [review policy](../06-testing/REVIEW_POLICY.md) |
| Agent skills/reviewers | Three skills and six read-focused reviewer configurations validate | Repository `.agents/` and `.codex/` configuration |
| Zephyr 4.4.0 environment | Exact pin and project-local tools verified | [Development environment](../05-platform/DEVELOPMENT_ENVIRONMENT.md), root `west.yml` |
| Minimal Zephyr proof | `qemu_x86` build and compilation database passed | Foundation application under `app/zephyr/` |
| Desktop LVGL/SDL2 UI | Visible fail-closed availability screen, retained pointer/text diagnostic, and MSVC source-debug path passed | [Simulator strategy](../05-platform/DESKTOP_SIMULATOR_STRATEGY.md), `simulator/desktop/` |
| Desktop operator workflow | One Python command configures, builds, tests, and opens the exact MSVC desktop artifact; check-only mode is available | [Developer runbook](../05-platform/DEVELOPER_RUNBOOK.md), `tools/run_desktop_ui.py` |
| Host-only semantic UI laboratory | Separate MSVC preset and launcher render deterministic disconnected/idle/running/Manual/paused/fault/stale/overrun scenarios; the no-scroll explorer now applies the graphite visual system across Home, Programs, Program editor, Running, Device, Settings, dialogs, and shared navigation; simulator code is guarded from Zephyr | [UI-lab plan](UI_LAB_VERTICAL_SLICE_PLAN.md), `tools/run_ui_lab.py`, `simulator/ui_lab/` |
| First simulated dashboard | Bounded session/revision observation model, provenance/validity fields, planned/measured graph, controller-reported service gate, and host-only presentation | `app/hmi/model/`, `app/hmi/presentation/dashboard/`, `simulator/semantic_controller/` |
| First reusable presentation | `UNAVAILABLE`/`STALE`/`CURRENT` mapping, invalid-input fallback, non-interactive structure, parent lifetime, and minimum viewport verified | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md), `app/hmi/presentation/state_availability/` |
| Host/Zephyr tests | Shared host Debug/Release/MSVC desktop/core: 10/10, 10/10, 10/10, 8/8. All three UI-lab viewport suites: 10/10 each, with 165 native captures and zero geometry errors. | [Current verification record](GRAPHITE_UI_VERIFICATION.md) |
| Static analysis/debug | Zephyr GCC analysis completed; SDK GDB and desktop `cppvsdbg` stopped in project source | [Environment verification record](../05-platform/DEVELOPMENT_ENVIRONMENT.md#verification-record--2026-08-25) |
| Deterministic indexer V1 | Schema, tooling regression cases, fail-closed input validation, generation, and `--check` passed | [Generated policy](../_generated/README.md), [generated index](../_generated/CODEBASE_INDEX.md) |
| Foundation reviewer gate | Complete; architecture, embedded, indexer, software-critic, error-auditor, and workflow findings resolved | Phase 9 in [roadmap](ROADMAP.md) |
| UI slice reviewer gate | Complete; fail-open default, platform copy, state/link wording, allocation contract, lifetime, viewport, and coverage findings resolved | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md) |

## Accepted baseline

- Control CPU authority and HMI/safety boundary: [ADR-0001](../07-decisions/0001-authority-boundary.md)
- Zephyr 4.4.0 pin: [ADR-0002](../07-decisions/0002-zephyr-4-4-0.md)
- Protocol separation principle: [ADR-0003](../07-decisions/0003-protocol-layering.md)
- Project-owned desktop simulator: [ADR-0004](../07-decisions/0004-desktop-simulator.md)
- Single LVGL execution owner: [ADR-0005](../07-decisions/0005-lvgl-single-owner.md)

## Known limitations

- The unrelated global MinGW GDB is broken because its expected Python runtime
  DLL is absent. It is not part of either verified debugger path: Zephyr uses
  SDK GDB, and the desktop proof uses MSVC PDBs with `cppvsdbg`.
- Python 3.13.3 passed the local Zephyr workflow, but upstream recommends Python
  3.12 specifically for Windows; new Windows setups should prefer 3.12.
- The proof uses `qemu_x86`; it provides no evidence for the still-unselected
  production MCU, board, display, touch device, or resource budgets.
- The reusable availability component is currently exercised by the desktop
  target. Zephyr builds the shared foundation/string catalog, but embedded LVGL
  display wiring is intentionally deferred until a display target is selected.
- The host-only UI laboratory is a visual and semantic HMI test environment;
  its synthetic controller cannot validate Control CPU safety, thermal physics,
  or production protocol behavior. Its program edits, service acknowledgement,
  and run controls are request previews, not persistent/controller behavior.
- The current UI-lab graph and stage values are illustrative. Exact
  HEATING/HOLD/COOL execution limits, rounding, and controller acceptance
  behavior remain open.
- The one-command UI runner is intentionally Windows/MSVC-only and does not
  imply a Zephyr build, an embedded graphical target, or visual correctness.
- Manual operator visual sign-off for the graphite pass at 800 x 480, 800 x
  640, and 1024 x 600 remains pending. Author inspection of actual native captures is complete; automated tests
  cover geometry and interaction. Physical touch-target review is not performed.
- Two GCC analyzer warnings remain in upstream Zephyr libc `malloc.c`; no
  analyzer warning was emitted from repository-owned sources.

## Scope guard

No foundation or availability proof should be interpreted as furnace behavior.
Questions are resolved at the decision gate that needs them: early presentation
work may continue with reversible simulated data, while operational controls,
protocol behavior, safety claims, and target integration wait for their
requirements.

See [open questions](OPEN_QUESTIONS.md), [session log](SESSION_LOG.md), and
[traceability](../06-testing/TRACEABILITY.md).

## 2026-09-15 - HMI web demo publishing

**Fact:** Renamed the shared browser source to `hmi-web-demo.html` and added
a repeatable client adaptation to the Telamorph website build. Build and
headless browser interaction checks passed. See the
[web demo record](HMI_WEB_DEMO.md) for workflow, deployment result and limits.

**Deployment:** Attempted; blocked by expired Firebase login. Local preview
is available; live publishing requires `firebase.cmd login --reauth`.
