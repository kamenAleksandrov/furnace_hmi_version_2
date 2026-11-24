# Phase 21: controller client over a simulated link

- **Status:** Active from 2026-11-10. Living record ([PLANS.md](../../PLANS.md)).
- **Furnace work items:** WI-H06 (client), WI-H03 (execution, Hold, faults), WI-H04 (Manual), WI-H02 (program library), WI-H10 (UTF-8 names), WI-H12 (Device log and health), WI-H07 (end-to-end workflow tests). Phase 21 closes the furnace roadmap's M3, an integrated prototype controller-HMI baseline over a host link.
- **Controller side:** furnace firmware `main` at `721f3cb` (Phase 20 merged), interface and wire format 0.16. Its Phase 20d (program library, settings and diagnostics messages) is built inside this phase.

## 1. Outcome and non-goals

**Outcome:**
- The HMI runs as a Zephyr application with its LVGL screens.
- It talks to the real controller application over a byte link, using the controller's 0.16 protocol, with both sides simulated on one PC until the boards arrive.
- Every operator workflow of M3 runs against controller data, never against HMI-invented state: Start with its readiness, Stop, Pause, Resume, hot start, Manual, faults and recovery, program library, Device log.
- The web demo shows the same screens and the same behavior, with the controller simulated inside the page.

**Non-goals:**
- **No production link.** RS-422, RS-485, Modbus or Ethernet is the electrical engineer's decision, and stays open.
- **No HMI board bring-up and no energized furnace.**
- **No new protocol semantics from the HMI.** Phase 20d's controller messages bring their own decisions, made in the controller repository (M21-6).
- **No change to the authority boundary** (ADR-0001).

## 2. Governing requirements, invariants and decisions

- **Product owner, 2026-11-10:**
  - Codec option A: a pinned copy of the controller's codec.
  - Simulate the link while waiting for the boards.
  - The web demo and the LVGL application stay the same visually and in behavior. The demo lacks the back end and safety, and simulates them. Parity covers presentation and operator behavior. The demo's simulated controller is not controller truth (ADR-0004), and can differ from the real controller: the comparison is the scenario checks of M21-5 and M21-8, run in both.
  - Finish the LVGL and Zephyr project with the same test-first and simulation approach as the controller.
- **HMI ADRs:** ADR-0001 (authority), ADR-0003 (layering), ADR-0004 (shared sources; desktop simulator), ADR-0005 (one LVGL owner), ADR-0006 (transport; development link added), [ADR-0007](../07-decisions/0007-serialization-and-framing.md) (wire format: the controller's 0.16, accepted), [ADR-0009](../07-decisions/0009-protocol-source-sharing.md) (source sharing: pinned copy, accepted), ADR-0012 (theme and strings), ADR-0013 to ADR-0020 (presentation decisions).
- **HMI requirements:** ARCH-INV-001 to ARCH-INV-010, PROTO-001 to PROTO-009 (PROTO-010 and PROTO-011 have known gaps: ADR-0007), REQ-FUN-007, REQ-FUN-021.
- **Controller interface:** furnace firmware `docs/interfaces/controller-hmi-interface.md`, including the client obligations of Start readiness: `START_PREPARE`, Start enabled only on `READY`, and no automatic resend of a refused Start.
- **Parity:** the [UI-parity skill](../../.agents/skills/ui-parity/SKILL.md) applies to every user-visible change.

## 3. Facts, assumptions, open decisions and risks

| Kind | Item |
|---|---|
| Fact | The HMI's Zephyr application (`app/zephyr`) is a starting point: configuration and an empty `main`. The LVGL screens run in the desktop program only |
| Fact | The controller's codec (`core/wire`, `core/crc`) is about 1,600 lines of portable C17 with only standard headers, and ships golden test vectors (`tests/vectors/wire`) |
| Fact | The controller's simulation already exchanges framed messages over a native_sim pseudo-terminal |
| Assumption, gated | Zephyr 4.4.0 on native_sim in WSL can run the HMI's LVGL screens through its SDL display (WSLg). native_sim is Linux-only, while the HMI's recorded environment is Windows (MSYS2, MSVC): M21-2 opens with an evidence gate and records the WSL environment in `DEVELOPMENT_ENVIRONMENT.md`. Fallback if it fails: the desktop program over a host serial adapter, decided then. **Gate passed** (M21-2, see Progress) |
| Fact (found in M21-2) | The operator screens (Home, Programs, Running, Manual, Settings, Device) exist only in the host UI lab (`simulator/ui_lab`, about 13,000 lines; its build is Windows-only). The shared presentation holds the state-availability and dashboard views. The lab's screen code is plain LVGL on the shared fonts, icons, theme and strings; only its `main.c` uses SDL and the simulated controller. Hence M21-2b |
| Unproven | The copied codec under MSVC: M21-1; a failure is fixed in the controller first (ADR-0009) |
| Open, electrical engineer | The production link (HW-BLOCK-006 in the furnace register) |
| Open, product owner | Physical boards over USB for the first two-board tests: which boards, when |
| Risk | Two code paths for one presentation (LVGL and the web demo) can drift. Mitigation: the parity skill, and one checklist per slice |
| Risk | The copied codec can drift from the controller's. Mitigation: a manifest with hashes, a check against a firmware checkout, and the golden vectors in the HMI's own tests |
| Risk | 20d inside this phase adds controller work to an HMI phase. Mitigation: its own firmware plan, review gates and pull request, test-first, before the milestones that need it |
| Risk | During the draft series `HELLO` catches only a change of minor; compatibility is the synced revision (ADR-0007). Any controller wire change, 20d included, needs a lockstep HMI resync |

## 4. Affected modules and ownership

| Module | Owner of the change | Change |
|---|---|---|
| `app/hmi/protocol/wire` (new, copied) | Controller repository; the HMI only syncs | The 0.16 codec |
| `app/hmi/client` (new) | HMI | Session, snapshot, command lifecycle, link state: plain data to the model |
| `app/hmi/transport` (new) | HMI | A byte-stream port; a Zephyr UART adapter, a loopback for tests |
| `app/hmi/model` | HMI | Fed by the client instead of the semantic simulator, in the Zephyr application |
| `app/zephyr` | HMI | The application with LVGL, the GUI context and the client thread. The client posts plain data through a bounded queue; only the GUI context changes the model and touches LVGL (ADR-0005); codec structures never reach the model (ADR-0003) |
| `simulator/semantic_controller` | HMI | Stays for fast UI work and as the web demo's back end |
| Web demo | HMI | The same behavior, simulated |
| Furnace firmware | Controller repository | 20d messages; a runnable simulated controller program for the link |

## 5. Milestones

Each is a draft pull request, test-first, with its completion criteria checked.

| # | Milestone | Done when |
|---|---|---|
| M21-0 | This plan; ADR-0006, ADR-0007, ADR-0009 | Reviewed by the protocol and architecture reviewers and the decision challenger; merged on the owner's authorization |
| M21-1 | The codec in the HMI | Copied at a pinned controller revision with a manifest and HMI build files; a sync check; the golden vectors pass in the desktop MSVC build, the host GCC build and on Zephyr |
| M21-2 | The HMI as a Zephyr application | Evidence gate first (native_sim with SDL in WSL); then the shared presentation that exists runs on native_sim from the shared sources (the operator screens: M21-2b); the desktop program keeps working; the environment recorded |
| M21-2b | The operator screens in the shared presentation | The lab's screens move from `simulator/ui_lab` into `app/hmi/presentation`, fed only by the HMI model; simulator data stays in the lab; the lab and the Zephyr application compose the same screens; parity kept, with the intentional behavior changes in the design below in both presentations. Before M21-5 |
| M21-3 | Transport and client against the controller (WI-H06), with the former M21-4 | The controller and the Zephyr HMI run as separate programs in WSL over the simulated link; the 0.16 handshake and compatibility check; authentic `CAPABILITIES`, snapshot and `STATUS`; faults, measurement qualification, run state and freshness in the client's observation; disconnect, reconnect, controller restart and stale sessions handled; nonblocking; nothing simulated by the HMI; automated tests and an end-to-end demonstration. No heating command (design below) |
| M21-4 | Merged into M21-3 | Owner decision 2026-11-10: the end-to-end link is part of M21-3 |
| M21-5 | Run, Manual and fault workflows (WI-H03, WI-H04) | Each workflow runs against the controller; the same behavior in the web demo, simulated |
| M21-6 | Controller 20d (firmware) | A controller-side plan with its open decisions for the product owner and the controller's reviewers (for example the health rules still proposed); the messages built test-first with the controller's verification; the HMI resynced to the new revision in lockstep, ADR-0007 checked against its reopen triggers, before M21-7. **2026-11-19:** firmware 20d in #56–#60 (merged) and drafts #63–#65; pin moved to `a96d138` with the codec byte-identical; ADR-0007 trigger not met (result values only, no layout change; STATUS) |
| M21-7 | Library, names, Device log (WI-H02, WI-H10, WI-H12) | Against the controller; parity in the web demo. **2026-11-19:** M21-7a (client) in draft, with firmware fix #66; M21-7b (screens) in draft, stacked on it; **2026-11-20:** M21-7c (Device log and health) in draft, stacked on M21-7b; M21-7d (the controller's limits and the planning reference) added; M21-7e (the controller's clock, firmware and HMI) added |
| M21-8 | End-to-end workflow tests (WI-H07) | Scripted operator workflows over the simulated link; M3 closed by the furnace roadmap's criteria. **2026-11-24:** owner decisions and design below; M3's other criteria stay with their reviews |

### M21-8, the end-to-end operator workflows

**Owner decisions (2026-11-24):**
- The workflows go through the screens: an operator's taps on the shared screens, the command
  mapping, the link and the real controller, never the client alone.
- Recurrence summaries (furnace WI-F03) stay out. SD-04 is undecided, so WI-F03 is blocked; it is
  recorded as open for M3, with no guessed policy.
- The web demo runs the same scenarios against its simulated controller.

**What exists:**
- `tests/zephyr/controller_workflows` runs 10 workflows against the controller, but submits
  commands to the client as a screen would, so no screen is in the path.
- The screen checks (`tests/presentation/operator`) tap the screens, but against fixed observations,
  not a controller.

Neither shows that an operator's tap reaches the controller and that its answer reaches the screen.

**Design:**
- **One GUI host.** The body of `gui_main.c`'s loop moves into `app/zephyr/src/gui_host.c`, with
  an init function and one pass:
  - the pass takes the link's observation, library, limits, clock and diagnostics into the screens;
  - it hands the screens' requests to the link.

  `gui_main.c` keeps the display and the loop. The test calls the same pass, so the tested path is
  the application's. No behavior change; the GUI build and the lab captures show it.
- **`tests/zephyr/operator_workflows`** (new, not run by Twister: it needs the controller):
  - the screens on a memory display, as in `tests/zephyr/operator`;
  - the real link thread, and the controller's `link_pty` program over a pseudo-terminal pair;
  - taps sent with `lv_obj_send_event` to widgets found by their label strings;
  - each step waits for what the controller reports and checks what the screen shows.

  `tools/controller_workflow_check.py` runs it, with a suite argument.
- **Link loss without a restart.** The runner relays bytes between the two pseudo-terminals, as
  `controller_link_demo.py` does. A control line holds the relay and later releases it.
- **Scenarios**, each from Home, with no session executing (a final cooling may still run: the
  controller accepts a new Start during it). Pages are reached by the rail, Manual by Home's Start
  manual, a program by its row. W8 runs last but for the Device log, because a protective trip
  stays latched for the controller session.
  - **W1 Program edit and save:** Programs, New, a name of 31 characters typed on the keyboard,
    Add stage, Save all changes, Save program; Programs shows "Program saved" and the controller
    lists it with one stage. Edit it, add a Hold stage, save: the controller lists revision 2 with a
    Heating and a Hold stage. Delete it.
  - **W2 Start with hot-start confirmation:** the scenario offset raises the reading to about
    110 °C. Start "Sim cure and post-cure"; the proposal panel names the program and the skipped
    Heating to 100.0 °C; Confirm; Running shows the controller's run, at or after the proposed stage.
  - **W3 Pause and Resume** of that program run.
  - **W4 Stop and cooling:** Stop; Running shows the run stopped with Return Home while the
    controller cools. Return Home shows Home, and the cooling goes on.
  - **W5 Contained navigation:** while a run executes, the rail's Programs is not offered, or a tap
    on it leaves Running. The controller's refusal of a library change (PROG-15) stays covered by
    the furnace's tests.
  - **W6 Quality decision and Continue:** an offset below a Manual ramp brings the decision panel;
    Continue; the session goes on.
  - **W7 Faults:** a sensor fault; the alert; Faults (n) lists it; only Stop offered; Stop; Exit;
    with the sensor back, Acknowledge and Recover from Home's banner (at least one of each).
  - **W8 Protective trip:** acknowledged, the trip is not offered for recovery, its external reset
    is named, and the controller keeps it latched.
  - **W9 Manual:** Start for the controller's longest session, a retarget by stepper, Pause,
    Resume, Stop, Exit.
  - **W10 Link loss and reconnection during a session:** the relay holds for 5 s on Manual, longer
    than the controller's 3 s link silence. The screens show "Connection lost" and offer no
    command. After the release they show "Connection restored" and the same session (same run ID),
    and Stop works.
  - **W11 Device log** after the others: the controller's log is read and not shown as empty or
    unavailable.
- **Web demo:** `tools/hmi_web_demo_workflows.cjs` (headless Chrome) runs the operator steps of
  W1 to W11 against the demo's simulated controller, with its scenarios for the faults. This checks
  the parity of the presentation, not controller truth (ADR-0004). W10 is the demo's simulated link
  loss.
- **Firmware:** no change expected. The simulated controller already takes offset, sensor,
  interlock and protection lines. A gap gets its own furnace pull request.

**Not covered, recorded for M3:**
- Recurrence summaries (WI-F03, SD-04).
- An operator end of the final cooling: the controller has `RUN_END_COOLING`, the screens have no
  action for it.
- Cyrillic names (owner: later).
- The production link and the boards.

**M3:** M21-8 gives WI-H07's evidence only. M3 also needs:
- WI-I01 reviewed by both repositories and frozen as 1.0;
- the open review gates;
- the merges;
- the maturity updates in the furnace register, with the evidence its release criteria require.

**M21-8 done (2026-11-24, draft), stacked on M21-7e-2; controller: furnace 7e-1 (`fb1fb79`):**
- **Delivered:**
  - `app/zephyr/src/gui_host.c`: the GUI context's pass, shared by the application and the test.
    No behavior change.
  - `tests/presentation/operator/src/screen_probe.c`: the screen checks' widget finders, shared.
  - `tests/zephyr/operator_workflows`: W1 to W11 in 8 tests.
  - `tools/controller_workflow_check.py --suite operator`, with the relay for link loss.
  - `tools/hmi_web_demo_workflows.cjs`, and `tools/hmi_web_demo_browser.cjs` shared with the
    smoke test.
- **Found in the web demo, now as the screens:**
  - a new program was saved without the controller's "Program saved" and opened its details
    (the screens return to Programs);
  - Add stage repeated the previous target, so the draft could not be saved (the screens add
    1 °C above it, at least 120 °C, at 1.0 °C/min, within the controller's limits).
- **Parity gaps, recorded, not changed:**
  - the demo ends a stopped faulted run at once and shows Home; the controller stops it into
    its final cooling, which the screens show until Exit or Return Home;
  - the demo simulates no protective trip (fault 7) and no external reset; W8 runs its
    over-temperature fault (16) instead;
  - the demo raises faults only during program runs, so its W6, W7, W8 and W10 run on a program
    where the screens' run on Manual; its sensor fault is fault 3, the controller's fault 1;
  - the demo's hot start is Start again during its cooling: its furnace is never hot at rest;
  - Add stage in the demo gives every new Heating stage 95 min; the screens derive it from the
    rate and the previous target;
  - Cancel of a new program returns the demo to a program's details; the screens return to
    Programs;
  - the demo's state is reset between scenarios; the controller's carries over.
- **Found operating the screens through the test:**
  - after a link loss the "Connection restored" alert covers Stop until it is closed (UX baseline,
    as designed). A fault alert does the same. Each costs the operator one tap before Stop; the
    decision panel has its own Stop. For the HMI reviewers.
  - The screens keep pointers to a deleted page's widgets (`manual_exit_button`,
    `manual_steppers`). The screens' own code does not use them then; the test guards them.
- **Reviews:** AI agents in the HMI's embedded-reviewer and error-auditor roles, and the
  software-critic and UI-parity roles. These are not the required human reviews. No High finding.
  Fixed:
  - W1 tapped Save program under the stage editor, which an operator cannot do (and a demo
    change had followed that tap; withdrawn);
  - pages and programs were reached by setting state, not by taps;
  - the fault answers could pass with no tap;
  - W8 did not show that Recover is withheld;
  - W9 did not check the controller's longest session;
  - W10 held 2 s and could pass off Manual;
  - W11 did not look at the page;
  - the relay could lose bytes on a full pseudo-terminal;
  - the runner passed a suite that ran no tests.

  Kept: a tap is an LVGL click event on the widget, not a touch at its position (the screens act
  on clicks only); the client-level and screen suites keep their own scenario-channel code.
- **Evidence** (at `c344978`):
  - the screen suite 8 of 8 and the client-level suite 10 of 10, against `link_pty` at `fb1fb79`;
  - web workflows and smoke test pass;
  - Twister in WSL 38 of 38 executed test cases, 3 configurations built only (the two workflow
    suites and the application);
  - MSVC core 15 of 15, desktop 18 of 18, the three UI lab presets 17 of 17 each; GCC 18 of 18;
  - mutation probes, each breaking one link from tap to controller, all 8 killed: nothing submitted,
    Pause sent as Resume, Continue as Pause, Confirm as Cancel, Stop as Pause, a stale link shown
    as current, the library never shown, the relay ignoring hold. Each fails the scenario it
    should (Continue W6, Confirm W2, hold W10, stale W10 and W8).

### M21-2b design

Owner decision 2026-11-10 (after M21-3): migrate the lab's Home, Programs, Running, Manual, Settings
and Device screens into the shared application, preserving their design and navigation where they
agree with the controller's lifecycle; no state-changing command yet.

- **One implementation:** `app/hmi/presentation/operator` (moved from `simulator/ui_lab`, renamed).
  The UI lab, the desktop program's check and the Zephyr application compose it.
- **The data seam:** the screens take the status model (`furnace_hmi_dashboard_state_t`) and the
  controller-owned content (`furnace_hmi_operator_content_t`: library, recent programs, clock, event
  log, program limits, Device catalogue), each with a "known" flag.
  - The lab fills the content from `simulator/ui_lab/src/operator_fixtures.c` (SIMULATION ONLY).
  - The Zephyr application has none yet, so those parts are shown unavailable, never as empty.
- **Simulated answers:** only the lab answers controller actions itself (`simulated_responses`).
  - Elsewhere, an action that would change controller state sends nothing and says it is not
    connected yet. This covers runs, Manual, the light, the library, maintenance, acknowledgements,
    the clock, resets and restarts.
  - The screens never write the controller's log themselves.
- **No assumed values:** Manual forecasts only from a current measured temperature; the former
  25 C fallback is gone, in the web demo too.
- **Lifecycle:** navigation never ends the controller's final cooling. Return Home while it cools
  shows Home, and the run goes on; a run is closed only after its cooling. This applies in the
  screens and in the web demo; before, both dropped the cooling run.
  - The controller's `navigation_permitted` is followed on every observation (interface:
    Lifecycle). While it is false, the screens stay on Running, at creation and on every update;
    `navigate` and Return Home do nothing, a faulted run included.
  - Where no controller reports it (the UI lab), the lab's own containment applies.
- **The status adapter** (`app/hmi/client`, `furnace_hmi_controller_dashboard_state`) maps only
  what the controller reported:
  - unknown codes map to safe defaults;
  - controller-reported execution always maps to a running or paused run, so Running stays
    authoritative;
  - STOPPED is a finished run with `run_stopped` (a new model field);
  - AWAITING_DECISION is paused, and named "Awaiting operator decision" (`awaiting_decision`);
    the decision itself is M21-5.
  - A finished run that still cools is shown as cooling, never as complete.

### M21-7 design

Started 2026-11-19 (owner: "start M21-7"; the M21-6 review points wait). The controller's 20d
messages are in the pinned codec (`a96d138`); nothing in the wire format changes.

Three steps, each its own draft pull request, stacked:

1. **M21-7a, the library and the program Start in the client.** The portable client mirrors the
   controller's library, sends saves, deletes and the program Start transaction, and reports
   their outcomes. The Zephyr link thread publishes the mirror. No screen changes.
2. **M21-7b, the screens (WI-H02, WI-H10).** Programs, the editors, quick launch and Program
   Start use the controller's library on Zephyr; UTF-8 names; the web demo and the UI lab in
   parity.
3. **M21-7c, the Device log and health (WI-H12).** `HEALTH` and `EVENT_HISTORY_PAGE` in the
   client, and the Device log and health view from them (ADR-0020); the web demo in parity.

**M21-7a, the library mirror** (controller interface: Program library):
- **When:** while the link is current, the client lists the library whenever the
  `library_revision` of the newest `STATUS` or `SNAPSHOT_END` is newer than the revision it holds,
  and after every new controller session. Revisions only increase within a controller session, and
  status is coalesced, so a `STATUS` that trails a page never causes a listing. Reads change
  nothing, so the client may send them by itself.
- **An empty library** (total 0) is a complete listing (review M1).
- **How:** `PROGRAM_LIST_REQUEST` pages of six from index 0, then `PROGRAM_READ_REQUEST` for each
  program that is not damaged. A page with another `library_revision` than the first, or a read
  that gives another revision or `NOT_FOUND`, means the library changed: the listing starts
  again from index 0.
- **What is shown:** only a complete listing, every program read, at one revision. Until a new
  one is complete, the previous one stays, marked as updating. A new controller session
  discards it.
- **States:** unknown (nothing complete yet), current, unavailable (`STORAGE_UNAVAILABLE`, until
  the revision or the session changes) and too large (more programs than the HMI holds, 12).
  Damaged programs are listed, without stages, and cannot be started.
- **Names:** the controller's UTF-8 bytes as received, with a flag when they are not well-formed
  UTF-8 of at most 31 code points (WI-H10; the screens decide how to show them, M21-7b).
- **Lost answers:** a step without an answer for `FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS` is asked
  again under a new request ID. A link that stops being current stops the listing; it starts
  again from index 0 once the link is current.
- **Data path:** the mirror is plain data (ADR-0003) beside the observation, with its own
  generation. The link thread publishes it only when it changes.

**M21-7a, library changes and the Start** (the command rules of M21-5 apply unchanged):
- **Save and delete** use the ordinary slot. The result is accepted for `SAVED` or `DELETED`.
  `UNRESOLVED` (10) from the controller is reported as unresolved: the change may still take
  effect. Any other result is rejected, with the controller's code and, for a save, the first
  validation error and the estimate (`ACKNOWLEDGMENT_REQUIRED`).
- **Retry timing:** the controller answers a change once its write has ended, or with
  `UNRESOLVED` after its own deadline. The interface gives no number; the firmware's is two control
  periods plus 10 s, about 10.2 s (`app/runtime/src/runtime.c`, `library_client_deadline`). The
  client therefore waits
  `FURNACE_HMI_CLIENT_LIBRARY_RETRY_MS` (12 s, provisional) before it sends a change again under
  the same request ID. A retry sent earlier would find the change still being written and get
  `DUPLICATE_REQUEST`.
- **Program Start:** `START_REQUEST` names the program and the revision the operator saw. The
  answer is `START_ACCEPTED` (the run started), `START_REJECTED` or `START_PROPOSAL`. A proposal
  is reported as its own outcome, never as accepted: the controller has not heated.
- **The proposal** is kept with its transaction until it ends. A newer revision replaces it.
  It ends with `START_PROPOSAL_INVALIDATED` (with the reason), `START_CANCELLED`,
  `START_ACCEPTED`, a session change, or a `STATUS` that shows the same transaction ended or a
  newer transaction. A `STATUS` that shows no transaction or an older one is ignored, because
  status is coalesced and can trail the proposal.
- **Confirm and cancel:** `START_CONFIRM` names exactly the proposal revision shown, its stage
  and, when the proposal requires a duration acknowledgement, the accepted duration.
  `START_CANCEL` names the transaction.
- **Refused Starts:** `STORE_NOT_READY` and every other refusal end the Start; nothing is re-sent
  by itself (Start readiness, rule 3). `DUPLICATE_REQUEST`, as a `START_REJECTED` reason or a
  service refusal, is unresolved, as for every command.
- **After the proposal:** the command slot keeps `PROPOSED` until the next command; where the
  proposal stands is `start_proposal.state`.
- **No invisible transaction (review M2):** a proposal answering the latest Start of this client
  session is shown also when that Start already became unresolved; the outcome stays unresolved.
  `STATUS`'s start transaction (ID, revision, state, reason) is always observed, so M21-7b can show
  and cancel a transaction whose proposal never arrived. The HMI never cancels one by itself: that
  would be a new client behavior, for the owner (M21-7b).
- **Listed again after a change (review M3):** after `SAVED`, `DELETED` or `UNRESOLVED` the library
  is listed again once, also when the revision has not risen (the interface: "the client lists the
  library again"). A refusal lists nothing.
- **Names the codec refuses:** a page or program whose name is not strict UTF-8 fails to decode as
  a whole frame (wire format §7.1), so that step is asked again after its timeout; `name_valid`
  flags only a name over 31 code points. The controller validates every name it saves.
- **A new client session** ends an open proposal (the controller invalidates it,
  `CLIENT_SESSION_CHANGED`). An unavailable or too large library is held at the newest revision
  seen, never below `STATUS`'s.
- **The receive buffer (done in M21-7b):** the polled UART of the Zephyr port would lose bytes of
  ~1 KB library pages during a sleep on a real UART. Since M21-7b the receive interrupt fills a
  4 KB ring (`controller_link.c`); the board's UART, line and rate stay HW-BLOCK-006.

**Verification:** client scenarios against the scripted controller (host and Zephyr), and
workflows against firmware `link_pty` at the pinned revision: the library of the three
delivered fixture programs, a save, a delete and a program Start with Stop.

### M21-7b, the screens

Started 2026-11-19 (owner: "proceed with M21-7b", the two open 7a choices to be finished in it).
One draft pull request, four steps and the review fixes:
1. **The library on the screens.** `furnace_hmi_operator_screens_set_library` replaces the
   library whenever the controller's changes; Programs says when it is being read, unavailable
   or too large, and marks a listing being read again as updating. Programs carry the
   controller's ID, revision, UTF-8 name and planning estimate (the duration shown, PROG-14). A
   damaged program is listed and never opened. A program with a value the editors cannot show
   exactly (a target in tenths of a degree, a rate or Hold outside their range) is listed and
   startable, never edited: nothing is rounded into a save. 18 stages, nine to a stage page,
   in both presentations and the simulator (owner question: the screens had eight).
2. **Saves and deletes.** The editor holds the program it was made from and saves with its ID
   and base revision; the controller's answer decides: SAVED closes the editor, an invalid
   program names the controller's first error, ACKNOWLEDGMENT_REQUIRED asks and saves again with
   the controller's estimate, UNRESOLVED says it is not known. Settings > Delete programs
   deletes the programs its dialog named, one at a time by ID.
3. **Program Start.** Start names the program and revision shown. The controller's proposal opens
   a panel that is only confirmed or cancelled: the measured temperature, the start stage, every
   skipped stage, the estimate, a duration to accept and the warnings; Confirm is off without a
   valid temperature and names exactly the proposal drawn. **Owner question (a transaction
   without a proposal on this display), as recommended 2026-11-19:** it is pointed out once,
   and again when a Start is refused because of it, with Cancel and Keep; the HMI never cancels
   by itself, and does not offer Confirm for a proposal the operator has not seen.
4. **Names** of 31 characters in up to 124 bytes in the keyboard, the editor, the request and
   the running program's name; the web demo counts by character.

**Not in M21-7b, recorded:** the labs (UI lab and web demo) still answer every Start, save and
delete at once: no proposal, acknowledgement, refusal, inexact or waiting view (TRACEABILITY,
REQ-FUN-018 to -023). The font has no Cyrillic glyphs (ADR-0012), so a Cyrillic name is held
and sent whole but not drawn. The editors' own bounds (180 C, 3.0 C/min) are not yet the
controller's limits (CTRL-LIMITS). The program graph still draws a ramp from 25 C.

**Owner answers on the M21-7b report (2026-11-20):**
- Cyrillic glyphs wait (ADR-0012); nothing changes in M21-7c or M21-7d.
- The editors must take the controller's limits: M21-7d (below).
- Program durations at creation come from an assumed ambient temperature the operator sets;
  a Start uses the actual reading. This is the controller's planning reference already (furnace
  PROG-12, PROG-13, PROG-14; owner 2026-11-04): an operator setting, default 23.0 C, settable
  within the controller's reported range (5.0 to 40.0 C today, a provisional firmware fixture until
  T-08), read and changed with `SETTINGS` and `SETTING_CHANGE`, and stored with every
  program's estimate. The HMI has not shown or changed it yet: M21-7d.

### M21-7c, the Device log and health

Started 2026-11-20 (owner: "begin M21-7c"). Furnace WI-H12, ADR-0020, controller interface
"Diagnostics" (20d-4). One draft pull request, stacked on M21-7b, three steps:

1. **The client reads health and history** (`app/hmi/client`, module `controller_diagnostics`):
   - **Health:** `HEALTH_REQUEST` while the link is current, every
     `FURNACE_HMI_CLIENT_HEALTH_INTERVAL_MS` (5 s, provisional) and again after
     `FURNACE_HMI_CLIENT_SYNC_TIMEOUT_MS` without an answer. The newest report is kept with its
     controller session; a report naming another session is ignored, a new session discards it,
     and a link that is not current marks it stale (ADR-0020: never implied that recording stopped).
   - **History, on request:** the screens ask for a read (the Device log opened, or Refresh). The
     client reads the critical, general and audit logs one after the other, one request at a
     time: a first page gives `first_retained` and `next_sequence`; the reading then starts at
     the newest `FURNACE_HMI_CONTROLLER_HISTORY_PER_LOG` entries (provisional) and follows
     `scanned_to` while `more` is set. `BUSY` is asked again after
     `FURNACE_HMI_CLIENT_HISTORY_BUSY_MS`; an unanswered page after the sync timeout.
     `UNAVAILABLE` and `READ_FAILED` are kept per log and shown, with what was read.
   - **One window:** the logs are merged newest first by controller start and monotonic time.
     When a log held more than was read, entries older than its oldest one read are left out of
     every log, so the window shown is complete in every log (a run's general entries never
     appear without its critical ones).
   - **Kept until replaced:** a finished read stays until the next one ends; a new controller
     session discards it; a link that is not current stops a read in progress (the previous
     read stays, marked as not current).
2. **The screens** (both presentations, parity):
   - **Device log** shows the controller's history when the screens are on the controller:
     every entry with its text, tone and filter.
     - Filters: Programs (every RUN entry), Alarms (FAULT, SENSOR, ACTUATOR, and anything of
       severity ERROR or above), so a faulted run is in both (owner 2026-11-20); System (everything
       else: SYSTEM, DIAG, COMM, CONFIG; CONTROL traces are not stored).
     - Maintenance shows that the controller does not record maintenance yet (CTRL-MAINTENANCE).
     - A time from a valid or adjusted wall clock is shown as date and time. Without one (the
       firmware's simulated controller, `link_pty`, has no clock), the entry is grouped under its
       controller start and shown as the time since that start; no date is invented.
     - Run entries name the run by its ID: the controller does not record the program's name
       (new backlog gap, CTRL-EVENT-NAMES).
     - An event this HMI does not know is listed by its ID and severity, never dropped.
     - Refresh reads again. "Updating" while a read is in progress; evicted entries and a log
       that is unavailable or failed are said, not hidden.
   - **Health:** a new Device rail category. It shows the overall state, each subsystem with its
     state and detail, the fault counts, uptime, controller starts, the reset cause and service
     hours. `UNKNOWN` is shown as unknown, never as healthy (DIAG-09). Stale when the link is not
     current. The HMI's own link state is shown beside it, not merged (ADR-0020).
   - **Tab availability:** each Device category is available when its data is: Device log and
     Health from the controller; Overview, Maintenance, Service history and Device info stay
     unavailable on the controller until it reports them (CTRL-MAINTENANCE, CTRL-HEALTH).
   - **The labs** (UI lab, web demo) simulate the controller's entries and health in the same
     form, so both presentations draw the same rows.
3. **Verification:** client scenarios against the scripted controller (host and Zephyr), the
   screens' content checks, the lab captures, the web smoke test, and a workflow against
   firmware `link_pty` (#66): the controller start, a Manual or program run and a fault appear
   in the Device log, and the health is reported.

**Done (2026-11-20, draft):** as above, with these choices made in the work:
- Health is the sixth Device category, after Device info (the lab's capture numbers stay).
- The Device log draws only the controller's form. The labs map their simulated events to what a
  controller records, so they lose program names, library changes, maintenance and display
  settings, as the controller does (CTRL-EVENT-NAMES, CTRL-MAINTENANCE). Their health is
  simulated as the controller reports it: the health thread is never judged, so the whole is never
  OK.
- Review fixes (three AI reviews in the HMI roles, not the required human reviews): a page is asked
  again only after 12 s, beyond the controller's 10 s read deadline; a log unanswered five times is
  given up as failed; a read asked while the link is down starts once it is current; an unknown
  severity is named by its number; a stale report is drawn neutral and claims nothing about
  recording; older entries left out by the window are said; texts of `RUN_FAULTED` and the stage
  deviation corrected.

**Not in M21-7c, recorded:** filters by time, run or severity, export, occurrence counts and active
versus historical faults (ADR-0020, Phase 25); recording schedules (WI-I09). The labs' health
simulates no latched or unacknowledged fault counts (shown as 0).

**Questions for the owner (M21-7c review), answered 2026-11-20:**
- A faulted run appears under Programs and under Alarms. Changed in M21-7c (both presentations).
- The controller keeps the time as universal time (UTC), and the operator sets it from the display
  (M21-7e). The Device log shows it as the controller gives it.
- An `ADJUSTED` wall clock looks the same as a valid one (as built).

**Proposed in M21-7c, the owner may change:** the 5 s health period; reading the logs on
request instead of following new entries; the complete-window rule; Health as its own Device
category; run entries named by run ID.

### M21-7d, the controller's limits and the planning reference

Added 2026-11-20 (owner: the editors' limits must be the furnace's). After M21-7c, one draft
pull request stacked on it:
- **Limits from `CAPABILITIES`** (CTRL-LIMITS, furnace API-07, CFG-06): the program and stage
  editors and Manual take `max_stages`, the allowed stage kinds, `target_min`/`target_max`,
  `rate_min`/`rate_max`, `curing_max_minutes`, `recommended_curing_minutes` and the name limits
  from the controller session. An unconfigured controller (limits 0) offers no editor. The
  HMI's own constants stay only as the lab's simulated capabilities. The controller still
  validates every request; the HMI never raises a limit.
- **The planning reference** (furnace PROG-12, PROG-13): Settings shows the controller's value
  and lets the operator change it within `planning_reference_min` to `planning_reference_max`;
  the controller's answer decides (`APPLIED`, `OUT_OF_RANGE`, `SESSION_ACTIVE`, `BUSY`). The
  program graph and the editor's preview ramp from it, labelled as an estimate (interface:
  "No assumed temperature"); the 25 C start goes. A program's card keeps showing the
  controller's stored estimate and the reference it used.
- **Lab parity** (open since M21-7b): the UI lab and the web demo simulate the controller's
  proposal, acknowledgement and refusals, so they show the same Start and save dialogs as the
  controller.
- **Two small text fixes:** an information dialog is dismissed with "Close", not "Cancel"; one
  stage reads "1 stage".

Owner 2026-11-20: the lab parity stays for now (it may be removed later); the text fixes are
quality and are required; the planning reference's range is the controller's, not the HMI's.

**Design (started 2026-11-20, owner: "ready for M21-7d").** Two draft pull requests, stacked:
M21-7d-1 (limits, planning reference, text fixes) on M21-7c, and M21-7d-2 (the labs' Start and
save dialogs) on M21-7d-1, so each stays reviewable.

1. **Client:** the observation keeps every `CAPABILITIES` program limit (0 without a
   configuration) and the planning reference from the snapshot's `SETTINGS`; a new controller
   session discards it. `SETTING_CHANGE` is a command in the ordinary slot; `SETTING_RESULT`'s
   value in effect becomes the reference whatever the result; `APPLIED` is accepted, the others
   rejected with the controller's code; `COMMAND_REJECTED` (for example `NOT_CONFIGURED`) refuses.
   The HMI does not ask for `SETTINGS` again on its own: another client's change is seen at the
   next snapshot (a gap while the HMI is the only client).
2. **The screens' limits** (one plain-data type, the lab's simulated values and the
   controller's, never mixed):
   - The editor holds whole degrees: `target_min` is rounded up and `target_max` down to whole
     degrees, so the HMI never offers a value the controller refuses; rates are tenths, exact.
   - The stage rules are the controller's (furnace `core/program`): each Heating target within
     the limits and above the previous Heating target, the first only within the limits; a rate
     within the limits; a Hold after a Heating stage, 1 minute or more. The HMI's own rule that the
     first target lies above 25 C goes.
   - The total: the curing estimate from the planning reference, computed as the controller does
     (each Heating stage from the previous target, rounded up to a second; a stage already at its
     target takes no time), at most `curing_max_minutes`; the HMI's 48 h goes.
   - Stages: at most `max_stages`; a kind the controller does not allow is not offered.
   - Manual's target and rate take the same limits (MAN-03).
   - Without limits (not yet reported, or an unconfigured controller) New program, Edit and
     Manual Start are not offered, with a note; the controller decides anyway.
   - The library's programs are checked against the controller's limits when shown, not the
     HMI's.
3. **The planning reference:** Settings gains a "Programs" category (interface Q17) with the
   assumed start temperature: its value, the controller's range, and Change (a keypad in
   0.1 C, within the range); the controller's answer is shown (applied, out of range, refused
   during a run, busy, refused). Not offered while execution is active
   (`settings_changes_permitted`). The labs simulate it with the controller's fixture (23.0 C,
   5.0 to 40.0 C).
4. **The estimate:** the program graph, the editor's preview and the stage minutes ramp from
   the planning reference, labelled as an estimate from it; a program card keeps the controller's
   stored estimate.
5. **Text fixes:** an information-only dialog closes with "Close"; one stage reads "1 stage".
6. **M21-7d-2:** the labs simulate the controller's Start proposal, the acknowledgement of a long
   program and refusals, so their Start and save dialogs are the controller's.

**M21-7d-1 done (2026-11-20, draft):** as designed, with these choices made in the work:
- Settings > Programs is the fifth category, after System, so the earlier categories keep their
  numbers and captures. Its range reads "5.0–40.0 °C"; the longer form did not fit its card.
- Manual needs the limits, not the reference. Without the limits Manual shows a note; New
  program, Edit and Save show "Program limits not available", a dialog that closes.
- An unresolved change is shown as unknown, and the client reads `SETTINGS` again (also when a
  snapshot lacked them); an earlier answer is cleared when the value in effect changes; Applied
  says when the controller has no settings store (`settings_state` `NONE`).
- A Hold beyond `curing_max_minutes` is refused as a Hold, as the controller reports it; the
  error texts name the controller's limits ("target must be 20–180 °C").
- Found operating the HMI against the controller: a tenths keypad stated its range in raw tenths
  ("50 to 400"); it now states 5.0 to 40.0, step 0.1 (also the rate keypads).
- Review fixes (three AI reviews in the HMI roles, not the required human reviews): the above,
  and a keypad open during a page update keeps working (it already broke on library updates).

**Not in M21-7d-1, recorded:**
- The editors' stage minutes start at the reference rounded to whole degrees; the total is
  checked with the exact reference, so the minutes shown can differ by a minute from the check.
- `recommended_curing_minutes` and `name_max_code_points` are kept but not yet used (the save's
  acknowledgement and the 31-character keyboard already cover them).
- The labs' delivered programs keep minutes authored from 25 C.
- Another client's change of the reference is seen at the next snapshot, or when a change of this
  HMI is unresolved; the HMI does not poll `SETTINGS`.
- Seen, not changed: the Display card "Default Running view" overlaps its value on the HMI.

**M21-7d-2 done (2026-11-20, draft), stacked on M21-7d-1:**
- **The labs' simulated controller answers a Start as the controller does** (furnace `core/run`
  start selection and start transaction; SIMULATION ONLY): the first stage the measured
  temperature allows (a Heating stage below its target, a Hold within 5.0 C of its target, the
  firmware's fixture profile A), the estimate from there, the advisory duration and the maximum.
  The first stage within the advisory duration starts at once; a later stage, or a longer
  estimate, is a proposal; a run active, no valid temperature, no eligible stage or a longer
  estimate than the maximum is refused with the controller's reason.
- **One Start path in the screens:** Start, Start again, Confirm and Cancel go through the
  controller's command slot in the labs too (`answered_by_controller`); the labs show the same
  "sending" notice, proposal panel, refusal dialog and duration acknowledgement as against the
  controller. The program becomes recent when the run starts, not when Start is pressed.
- **The save acknowledgement:** above the advisory duration the labs ask, as the controller
  does, to acknowledge exactly the estimate before saving (one dialog, shared with the
  controller path). Closing the question saves nothing.
- **The labs' advisory duration is 12 h**, their own: the firmware's `link_pty` has none, so a
  controller workflow never asks. The screens' limits now carry `recommended_curing_minutes`
  (0 when the controller reports none or one above the maximum).
- Every run starts from the measured temperature, as the controller's does (the first ramp's
  minutes recomputed from it; a Hold holds at its target).
- **Review fixes** (three AI reviews in the HMI roles, not the required human reviews; fixed
  test-first):
  - A refused or cancelled Start no longer changes the program a stopped run's Start again
    restarts: the program becomes the running one when its run starts (both paths).
  - The simulated controller validates the program first (PROGRAM_INVALID instead of "a run is
    active"; a first target at or below 25 C is valid), answers BUSY while a transaction is open,
    TRANSACTION_MISMATCH and PROPOSAL_MISMATCH as the firmware, keeps the transaction when a
    Confirm is refused, and evaluates the live data at Confirm: a failure withdraws the proposal,
    a material change publishes a new revision. The web demo does the same at Confirm.
  - A Confirm refused because the proposal was withdrawn shows the controller's reason for it
    (both paths); a late Confirm tap no longer erases the withdrawal notice.
  - Confirm accepts the estimate the panel drew, not a newer one it did not show (both paths).
  - The web demo's proposal state cannot outlive its panel (Escape works on other dialogs; a
    reset clears it); an Acknowledge with no draft left closes its dialog.

**Not in M21-7d-2, recorded:**
- The labs evaluate an open proposal again only at Confirm, not while it is open; a scenario
  change closes it without a message.
- The UI lab's Cancel goes through the command slot with a sending notice; the web demo's is
  immediate. Reason 34 (a run active) cannot be provoked through the labs' screens.
- An open proposal is reopened over an alert that came meanwhile (both paths; the alert is lost
  after one observation).
- Seen, not changed: the web demo offers no program Start from Program details while a stopped
  run cools (only Start again); the UI lab does. The furnace register's HMI-DIFF-01/-02 still
  describe the labs as starting at once; they are the firmware repository's to update.

### M21-7e, the controller's clock (firmware and HMI)

Added 2026-11-20 (owner): the furnace controller keeps the time, in universal time (UTC); the HMI
only shows it and changes it when the operator asks; the Device log uses it. Today the controller has
no time source (furnace HW-BLOCK-013, WI-K08), and interface 0.16 has no message to read or set the
time: "clock set" is planned as a new message type (interface Q15, WI-H09). The firmware's
diagnostics can already hold a wall clock (`furnace_diag_set_wall_clock`, the `WALL_CLOCK` event,
`wall_state` `ADJUSTED` after a change). Proposed, for the owner's approval before it starts:
- **Firmware (a furnace plan and draft first):** a time read in `STATUS` or its own message, and
  `CLOCK_SET` (UTC milliseconds), validated by the controller, refused during execution or not
  (owner question), recorded as `WALL_CLOCK` in the audit log; the time is `UNKNOWN` after every
  start until set, while no hardware clock keeps it (HW-BLOCK-013 stays open). Monotonic time keeps
  every deadline (RUN-14). An interface minor version; the HMI's codec resynced (ADR-0007 check).
- **HMI:** Settings' Date & time shows the controller's time and sets it; Home's clock and the
  Device log take it; without a valid time nothing is invented (as now).
- **Owner questions:** is the time shown in UTC or in a local time zone the operator chooses on the
  display; may the time be set during a run; must the controller ask for the time after a restart.

**Owner answers (2026-11-20):**
- **Display:** UTC, unless offering every local time zone is practical. A full zone database with
  summer-time rules is not, on this display, so the time is shown in UTC; a local zone is a later
  option.
- **During a run:** the operator cannot leave the Running view during a run, so the HMI cannot set
  the time then. Remote (PLC) control while a run executes is open: it is to be thought through
  and handled when that control is designed.
- **After a restart:** a hardware clock is to be tried (HW-BLOCK-013). Without one the controller
  does not ask the operator for the time at every start; the time stays unknown until someone sets
  it.

The design takes these answers. M21-7e follows M21-7d-2 and starts with the furnace plan.

**Firmware (2026-11-23):** the plan is furnace `docs/architecture/wall-clock-plan.md` (draft PR
#67; owner O1 to O3 approved: amend wire 0.16 in place, refuse a set during execution, accept
2025 to 2099 UTC). Slice 7e-1 is furnace draft PR #68, stacked on #66: `CLOCK_SET` and
`CLOCK_RESULT`, the clock at the end of `STATUS`, unknown after every start, audited.

**M21-7e-2 done (2026-11-24, draft), stacked on M21-7d-2:**
- **Codec** synced from the furnace at 7e-1 (`fb1fb79`). An HMI that reads the clock needs a 7e-1
  controller: against an earlier 0.16 controller its `STATUS` is 9 bytes short and malformed.
- **Client:** the controller's clock from `STATUS` (state, time, the client's time of the reading)
  and from `CLOCK_RESULT`; after a result only a `STATUS` newer than the next one replaces it (one
  captured before the set can still arrive); a new controller session discards it;
  `CLOCK_SET` is a command in the ordinary slot.
- **Screens:** Settings > Date & time shows the controller's time to the minute with "UTC"; "Not
  set" while the controller reports it unknown (no prompt, owner); "Unavailable" without a
  controller clock. Set date & time sends the picked minute and shows the answer: set, refused
  outside 2025 to 2099, refused during a run, or no answer. The picker offers 2025 to 2099 and
  starts at 1 January 2025 when the clock is unknown. The Device log already shows the controller's
  record times; it now redraws when the minute changes.
- **Home** shows no clock (it never did): the plan's "Home's clock" is Settings and the Device log.
- **Labs:** their simulated clock is shown in UTC (the web demo reads it through UTC getters, so
  in any browser zone); the picker's year starts at 2025.
- **Found operating the GUI against the controller:** the controller's clock audits were listed as
  "Controller event 31" (now named); without a clock the Date format card read "Not set" (now an
  example date).
- **Review fixes** (three AI reviews in the HMI roles, not the required human reviews; tests first
  where a test could show it):
  - the time is shown only while the link is current, and only a state and time the controller
    can report (2025 to 2099); a reading published just after the GUI's own time no longer adds
    49.7 days;
  - a new minute redraws only Date & time and the Device log;
  - an unanswered set is "not confirmed", not "not set";
  - an unknown clock's picker starts at 1 January 2025 with Set off until the operator changes the
    date (an unchanged placeholder would set the furnace to it); a step at a limit is no change;
  - the date-format choices and a Device log entry dated after the clock (set back) show no zero
    or negative date;
  - the web demo reads its clock through UTC getters (the zone shift was wrong near summer-time
    changes) and sets whole minutes, as the HMI;
  - checks for the year limits, an out-of-range answer, the redraw scope and, against the
    controller, the range's edges.

**Not in M21-7e-2, recorded:**
- No local time zone (owner: UTC); no hardware clock (HW-BLOCK-013); remote (PLC) setting during
  a run is WI-I07's.
- The display advances the controller's time between statuses from the HMI's own uptime, to the
  minute; a set sends the picked minute (seconds 0), so it is up to a minute behind what the
  operator read.
- The labs do not answer a set as the controller does: the UI lab's clock is a fixed simulated
  time and every set is accepted; the web demo's follows the browser's clock. Neither shows Not
  set, Unavailable or a refusal (the desktop and Zephyr checks do, with the controller's answers).
- No pending notice between Set and the answer (the answer comes within a second; a second Set
  meanwhile is busy). The answer's dialog replaces a Faults dialog opened meanwhile.
- A setting is listed twice in the Device log: the controller's clock change and its audit.

### M21-5 design

Owner decisions 2026-11-11:
- **Program Start moves to M21-7.** Protocol 0.16 cannot list programs (the library is 20d, M21-6),
  so the HMI cannot name one without inventing it. M21-5 covers:
  - Manual: Start, retarget, length, Stop;
  - Pause, Resume, Stop and Continue;
  - fault acknowledgement and recovery.

  All of them can be exercised on a Manual session. Program Start stays gated on Zephyr.
- **A faulted run follows the controller everywhere:**
  - it stays on Running until the operator stops it;
  - the UI lab and the web demo drop Return Home on a faulted run;
  - the UX baseline is amended;
  - HMI-FAULT-RETURN-HOME is resolved and recorded as a CFL entry in the furnace requirements
    register.

Three steps, each a commit series on one draft pull request, stacked on M21-2b:

1. **M21-5a, commands in the client.** The portable client sends operator commands and reports
   their outcomes; the Zephyr link thread takes them from the UI thread. No screen changes.
2. **M21-5b, the screens.** Manual and Running send real commands and show the outcome. The
   faulted-run change lands in the lab, the web demo and Zephyr.
3. **M21-5c, faults.** Acknowledgement, recovery and the Continue/Stop decision (AWAITING_DECISION);
   fault IDs mapped to the screens' alerts (HMI-FAULT-CATEGORY).

**M21-5c, faults and the decision** (owner 2026-11-11: a Faults dialog; a decision panel that cannot be
closed; HMI ADR-0018 and ADR-0019):
- **Faults dialog:** opened from a red "Faults (n)" button on Running and Manual and a banner on
  Home whenever the controller lists entries. Each entry shows its name (from its fault ID), active
  or cleared, latched and acknowledged. An unacknowledged entry offers Acknowledge; an acknowledged
  latched entry the operator may recover offers Recover; a protective trip says it is reset at the
  protection device. Each is a request naming the entry by ID and source; the controller's answer
  is shown in the dialog. The actions wait for a pending command, a Stop and a fresh list. The
  dialog is redrawn only when what it shows changes, so a tap is not lost.
- **Decision panel:** while the controller awaits a decision it covers Running or Manual and cannot
  be closed: the fault, "the cure may be affected", the controller's time left (unknown when not
  current) and Continue and Stop, which follow the link and the commands on every observation. It
  closes when the controller leaves the decision; a fault alert that comes meanwhile is shown then.
- **Alerts:** the adapter names the fault behind a faulted run and maps it to an alert kind only
  where the kind is right; any other controller fault is alerted by its name.
- **Parity:** the UI lab's simulation and the web demo raise an entry with each fault scenario,
  keep latched entries after the run, answer Acknowledge, Recover and Continue, and have a decision
  scenario.
- **Against the real controller:** firmware #55 raises a continuable LAG with the scenario line
  `offset`; the workflow check runs the decision with Continue.

**Command rules** (controller interface: "Session, link and delivery", "Start readiness"):
- **Outcomes:** a command is **accepted** or **rejected** only by the controller's result after it
  was applied (API-03). Sent is never shown as done.
- **Slots:** one ordinary command at a time, plus Stop in its own slot, so a pending command never
  delays a Stop. A second ordinary command while one is pending is refused locally (`BUSY`); the
  screen shows the pending one.
- **Retries:** an unanswered command is sent again under the **same** `request_id`; the controller
  returns its recorded result and runs nothing twice.
  - An ordinary command is retried while the link stays current. Its outcome becomes
    **unresolved** after the attempts run out, or when the link is re-opened in any way: the HMI's
    silence, a HELLO the HMI sends (for example after a NOT_CONNECTED), any HELLO_ACK, or a change
    of controller or client session. The operator's intent ends there (Start readiness, rule 4),
    and the HMI never re-sends it by itself.
  - Stop is retried while connected and while synchronizing to the same controller session (its
    HELLO_ACK has confirmed the session), also after NOT_CONNECTED or CLIENT_SESSION_MISMATCH.
    It ends unresolved when the controller or client session changes, its attempts run out, the
    controller reports STOP_UNRESOLVED, or the link is unusable for
    `FURNACE_HMI_CLIENT_STOP_PATIENCE_MS`. Another refusal by the service makes it refused.
  - **Order:** a Stop handed over before an earlier command was sent drops that command, which is
    reported as superseded: nothing the operator pressed before Stop goes out after it.
  - Results carry no client session, so a result completes a command only when its request ID
    and kind both match.
  - A result arriving after its command became unresolved is ignored; the state comes from
    STATUS.
- **Unresolved:** `DUPLICATE_REQUEST` on a retry, `STOP_UNRESOLVED`, a session change and lost
  answers are all reported as unresolved, never as accepted or rejected. The screens then show the
  state from `STATUS`.
- **Not sent:** while the link is not current, a command is refused locally (`NOT_CURRENT`) and
  nothing is sent. Each slot keeps its own latest refusal.
- **Start readiness:** `START_PREPARE` is sent when the operator opens a way to start (Manual).
  Manual Start is enabled only when `store_readiness` is READY. A Start refused as `STORE_NOT_READY`
  is not re-sent; the operator presses Start again.
- **Codes:** the outcome keeps the controller's reason codes as received; their texts are the
  screens' (M21-5b).

**M21-5b, the screens** (owner 2026-11-11: no session-length choice; retarget taps merged):
- **Only without simulated answers** (the Zephyr application). The UI lab and the web demo keep
  their own simulation; the faulted-run change applies to all three.
- **Requests:** the screens raise START_PREPARE, MANUAL_START, MANUAL_RETARGET, PAUSE, RESUME and
  STOP; the host hands each over and tells the screens its submission number
  (`furnace_hmi_operator_screens_request_submitted`, 0 when it could not). An ordinary request is
  taken before a Stop of the same pass, so the mailbox lets the Stop drop it.
- **Feedback:** the model carries the newest outcome of each slot (`command_feedback`,
  `stop_feedback`: pending, accepted, rejected, refused, unresolved, not sent, with the reason).
  A slot offers no second command until the outcome for its submission arrives. Every outcome other
  than accepted opens a dialog with the screens' text for the controller's reason, except for
  START_PREPARE, which is asked for in the background.
- **Stop first:** a Stop tap is never lost behind another tap. While a Stop is handed over or
  pending, no ordinary command goes out (START_PREPARE aside), a merged tap is dropped, and the
  steppers and Pause are disabled.
- **What the operator saw:** the Manual Pause button carries its meaning (Pause or Resume) from its
  label; a tap is dropped when the state changed before it was handled. Every request names the
  run shown when the operator acted.
- **Manual:**
  - opening Manual sends START_PREPARE; Start is enabled only when the controller reports READY and
    its Manual maximum (CAPABILITIES `manual_max_minutes`) is known, and asks for that maximum;
  - the session, its target and rate, pause and end follow STATUS (`session_kind` MANUAL,
    `manual_target`, `manual_rate`); a controller Manual session holds the Manual page (the UX
    baseline) instead of Running;
  - a stepper change during a session is a retarget; taps while one is pending are merged and the
    newest is sent after the answer; after a refusal or an unresolved outcome nothing is resent and
    the controller's values are shown;
  - a faulted session offers Stop only; while the controller awaits a decision, Pause and Resume are
    not offered (the decision is M21-5c);
  - a tap not sent when the session ends is dropped;
  - Exit and Start new leave an ended session that still cools; that mark ends when the controller
    leaves Manual, so a repeated run ID is a new session.
- **Running:** Pause, Resume and Stop are requests; a faulted run keeps Stop and offers no Return
  Home (owner 2026-11-11).

**Threads (ADR-0005):** the UI thread never calls the client. It puts a command into a two-slot
mailbox (ordinary, Stop) and gets a submission number; the link thread takes it at its next poll.
The outcome, with that number, comes back in the observation.

**Verification:**
- client scenarios against the scripted controller (host and Zephyr);
- a headless end-to-end program against the real controller (`tests/sim/link_pty` of the furnace
  firmware). It runs each workflow and injects the controller's faults through its scenario channel
  (firmware #53): a sensor fault, an interlock and a protective trip during a Manual run.

### M21-3 design

Owner decision 2026-11-10: M21-3 comes first and connects the Zephyr HMI to the actual controller
program over the simulated link; M21-2b follows (in parallel only while independent). Heating
commands and operator workflows stay in M21-5 and later.

**The link.**
- The controller is the furnace firmware's own simulated program, `tests/sim/link_pty`, built at
  the revision in `CONTROLLER_SYNC.json`: the controller runtime with its simulated plant, serving
  its client link on a pseudo-terminal.
- A relay, `tools/controller_link_demo.py`, holds a second pseudo-terminal for the HMI and copies
  bytes between the two. A controller restart then creates a new controller device, while the HMI's
  stays the same. The relay is also where faults are injected (ADR-0006).
- The HMI's link is a Zephyr `native_tty` UART opened with `--controller_link_port=<device>`. The
  same driver opens a USB serial port for the first two-board tests.

**Modules** (§4):
- `app/hmi/transport`: a byte port (non-blocking read and write) and an in-memory loopback.
- `app/hmi/client`: portable C with no threads and no Zephyr. It is given bytes and the time, and
  produces frames and a controller observation.
- `app/zephyr`: the link thread polls the UART and the client, and publishes the observation to a
  single-slot mailbox (the latest value, with a generation counter). The GUI context reads the
  mailbox without waiting and alone updates LVGL (ADR-0005).
- The observation is HMI-owned plain data: the controller's codes are kept as received, and no
  codec structure reaches it (ADR-0003).

**Session rules**, from the controller interface (Session, link and delivery) and its service:
- **Handshake:** `HELLO` with a client session ID that is random at each HMI boot, repeated every
  1000 ms until `HELLO_ACK`. The 1000 ms is a provisional HMI value. Only `OK` connects. A refused
  version (`RX_HELLO_VERSION`) makes the link `INCOMPATIBLE`, and the HMI records the controller's
  version from the frame header.
- **Synchronization:** `CAPABILITIES`, then the snapshot (`STATUS`, `FAULT_LIST` parts, `SETTINGS`)
  up to `SNAPSHOT_END`. After a `HELLO_ACK` to the same controller session, the controller sends no
  snapshot, and the client asks for one (`SNAPSHOT_REQUEST`).
- **Freshness:** `CURRENT` only after `SNAPSHOT_END` and while the newest `STATUS` is younger than
  the controller's `link_silence_ms`; otherwise `STALE` (data kept, marked stale). On `STALE` the
  client sends `HELLO` again. Before `CAPABILITIES`, or for an incompatible controller,
  `UNAVAILABLE`.
- **Controller restart:** a different `session_id` (in `HELLO_ACK`, `CAPABILITIES` or `STATUS`)
  discards the capabilities, status and fault list, and marks a pending request unresolved.
  Nothing is replayed.
- **A repeated session ID** (improbable, not impossible): when the link falls silent, the client
  keeps the `status_sequence` it holds as a baseline. After reconnecting to the same session ID,
  nothing of that session can be older, because it was already published. A `STATUS` or
  `SNAPSHOT_END` below the baseline is therefore another session, and is handled as a restart.
  Data of the old session is never shown as current. While connected, an older sequence is
  normal: a periodic `STATUS` may arrive before the end of an older snapshot (spec: Snapshot).
  The rule misses a new session whose sequence has already passed the baseline. Its data is then
  taken as current, and that data is the new controller's own.
- **Ordering:** an older `status_sequence` is not shown. Outside a snapshot it asks for one (spec:
  Recovery). A gap in a `RUN_EVENT` or `FAULT_EVENT` sequence asks for a snapshot. Events within
  a snapshot are applied by their sequences.
- **The fault list** comes only from a snapshot. It is applied only when its pages are contiguous,
  add up to `total`, and carry the snapshot's `status_sequence` and `request_id`; otherwise it is
  not applied and is asked for again (no list is not an empty list). While a refresh is pending,
  the list is marked outdated.
- **Liveness:** `HEARTBEAT` when nothing was sent for `heartbeat_interval_ms`.
- **Intervals:** the controller's `status_interval_ms`, `heartbeat_interval_ms` and
  `link_silence_ms` are accepted only within HMI bounds:
  - 1 ms to 60 s for the status and heartbeat intervals;
  - for `link_silence_ms`, more than the status interval and at most 10 min.

  These are provisional HMI values. Outside them the capabilities are refused, and the link never
  becomes current.
- **Requests:** the client sends only `HELLO`, `HEARTBEAT`, `SNAPSHOT_REQUEST` and
  `CAPABILITIES_REQUEST` (after an identity change without capabilities).
  - **Lifecycle:** its snapshot request is pending, then completed (`SNAPSHOT_END` with its ID),
    rejected (`COMMAND_REJECTED`), or unresolved (a restart).
  - **One at a time:** at most one is outstanding. It is asked again after 3 s without an answer,
    in any connected state, and so is a rejected or incomplete one while the list is outdated.
  - **Request IDs** increase within a client session and never wrap: at 2^32-1 the client starts a
    new client session.
  - **A reused client session ID:** a `DUPLICATE_REQUEST` before the controller has answered any
    request of the client session means the ID was reused, and the client starts a new one.
- **Bounded work:** each poll reads a bounded number of bytes; all buffers are static.
- **The GUI** shows `STALE` when the link thread has published nothing for 1 s, for example when it
  is blocked in a write. It publishes at least every 200 ms.

**Presentation in M21-3.**
- The availability screen follows the link: `UNAVAILABLE`, `STALE` or `CURRENT`.
- The window carries a development marker for the simulated link.
- Values (temperature with validity and age, measurement state, run state, faults, lifecycle) are
  in the observation, its tests and the console trace. The operator screens that show them come
  with M21-2b (connected) and M21-5 (the workflows).
- The Zephyr application links no simulator: every value comes from controller frames. A value is
  current only when the controller marks it valid and the link is current.

**Open, not blocking:**
- On hardware, a unique client session ID at each HMI boot needs a random source or a persistent
  counter. On native_sim the HMI runs with `--seed-random`; the fixed default seed would repeat
  the ID.
- Found in M21-3, for the controller repository: its simulated entropy (`sim/app_io`,
  `sim_entropy`) starts from the same state in every process, so each start of the simulated
  controller draws the same `session_id`. A restart then looks like the same session. The HMI
  handles it by the repeated-ID rule, and the end-to-end demonstration exercises that rule. A
  per-process seed in the fixture would make restarts look like restarts; recorded as
  CTRL-SIM-SESSION-ID.

## 6. Verification, including failure and recovery

- **Host tests and Twister** for the codec, client and transport, with injected faults in the loopback:
  - a lost frame, a corrupt frame, overlong or garbage input;
  - a `HELLO` version mismatch;
  - a stalled output, and a telemetry flood against command results (PROTO-006);
  - a controller restart with a command pending: marked unresolved, never replayed (ARCH-INV-003);
  - an HMI restart in the middle of a command (ARCH-INV-004);
  - `DUPLICATE_REQUEST` on ordinary commands, and an exhausted request ID;
  - an unfresh session, a refused Start (no resend), `STOP_UNRESOLVED`;
  - a superseded snapshot;
  - the pseudo-terminal peer going away, counted as link loss.
- **The controller's golden vectors** run in the HMI on every build.
- **End to end** over the simulated link, with the controller's own verification unchanged.
- **Parity:** each slice records the LVGL and web demo checks of the parity skill.
- **Not claimed:** anything about the production link, the boards, or timing on hardware.

## 7. Documentation and generated artifacts

- **Per milestone:** status, the controller integration backlog (entries closed by the client), and the codebase index (`tools/codebase_index.py`).
- **Interface contradictions** go to the controller's requirements register as `CFL` entries, not into HMI code.

## 8. Rollback and compatibility

- **The semantic simulator stays,** so the UI can run without a controller at any point.
- **The codec is replaced only by a new sync** to a newer controller revision. The protocol version check (`HELLO`) refuses a mismatched controller rather than guessing.

## Progress

- 2026-11-11, M21-5a (draft): **commands in the client** (M21-5 design above).
  - **The client:** `furnace_hmi_controller_client_submit` and the outcomes in the observation
    (`command`, `stop`, `command_refusal`); `controller_command.h` holds the plain-data types.
  - **The link thread:** `furnace_hmi_controller_link_submit`, a two-slot mailbox; the link trace
    shows both outcomes.
  - **Against the real controller** (firmware #53, `tools/controller_workflow_check.py`; every
    rejection reason asserted against the interface, acceptances by state):
    - Manual: Start, retarget, length, Pause, Resume and Stop accepted; Resume after Stop
      rejected (NOT_PAUSED);
    - sensor fault (a not-finite reading): FAULTED, Pause rejected (FAULT_LATCHED), Stop,
      acknowledgement and recovery of every latched entry;
    - interlock: PAUSED with INHIBIT, nothing latched; Resume blocked while active
      (RESUME_BLOCKED), accepted after; Continue rejected (NOT_AWAITING_DECISION);
    - protective trip: FAULTED, latched class 4; recovery refused as
      PROTECTIVE_RESET_EXTERNAL during and after the trip;
    - Start then Stop handed over together: the Start is dropped as superseded, the Stop sent,
      nothing runs.
  - **Reviews** (AI agents in the HMI roles: embedded and protocol reviewer; error auditor with
    software critic). No path heats without an operator action. Fixed test-first, each with a
    mutation probe that the tests kill:
    - an ordinary command survived a reconnect the controller saw first (High);
    - a Stop could overtake an earlier, unsent command in the mailbox (High);
    - a Stop refused as not connected was lost; a Stop could wait forever while synchronizing;
      one refusal record served both slots (Medium);
    - a client-session change was labelled a controller restart; results were matched by
      request ID alone (Low);
    - test strength: retry loops in the workflows now retry only the expected rejection; the
      scenario channel has a deadline; the runner drains the controller's output and also
      checks the exit code.

    Kept as decided: a late result after unresolved is ignored. Also kept: a command submitted
    between a new client session and its HELLO_ACK is refused by the controller.
  - **Found:** a ztest build runs native_sim's clock freely unless
    `CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME` is set, and every link timeout then expires at once.
    The pseudo-terminals must be raw.
  - **Evidence:** `host-msvc-core-debug` 15 of 15, `host-msvc-desktop-debug` and `host-debug`
    (GCC) 18 of 18; Twister in WSL on both native_sim variants, 8 of 8 cases; Twister on
    Windows, `qemu_x86`, 5 of 5; the M21-3 demonstration 10 of 10; the workflow check 5 of 5;
    14 mutation probes (13 of the client, 1 of the mailbox through the workflow check), all
    killed.

- 2026-11-11, M21-5b (draft): **the screens** (M21-5b design above). Owner decisions 2026-11-11:
  no Manual length choice (REQ-FUN-005 amended); retarget taps merged.
  - **Screens:** Manual Start, retarget, Pause, Resume and Stop are requests on Zephyr; a
    controller Manual session holds the Manual page; every outcome other than accepted is shown.
  - **Model and adapter:** the run ID, the Manual values, start readiness, the Manual maximum and
    the feedback of both slots, with the HMI's own not-sent reasons.
  - **Zephyr:** `app/zephyr/src/operator_commands.c` maps a request to a command; `gui_main.c`
    hands it to the mailbox and reports its submission number.
  - **Faulted run:** Stop and no Return Home in the screens, the UI lab and the web demo; the UX
    baseline is amended.
  - **Reviews** (AI agents in the HMI roles). Fixed test-first, each with a mutation probe that
    the tests kill:
    - a Manual Pause tap could be sent as Resume when the controller paused the session before
      the tap was handled (High);
    - a merged retarget could go out after Stop; a Manual Stop tap could be lost behind another
      tap (Medium-High);
    - a merged tap outlived its session; a faulted session offered the steppers; Pause and Resume
      were offered while awaiting a decision (Medium);
    - requests now name the run shown; the left-session mark ends when the controller leaves
      Manual; the not-sent reasons are the model's (Low);
    - the UI lab's smoke test still expected Return Home on a faulted run, and the web demo showed
      Return Home on a run stopped and then faulted.

    Kept: the Running page ignores a Pause or Resume tap while a command is pending instead of
    disabling the button (program runs cannot start before M21-7); a request the host cannot map
    is reported as "busy" (unreachable).
  - **Evidence:** `host-msvc-core-debug` 15 of 15; `host-msvc-desktop-debug` and `host-debug` (GCC)
    18 of 18; the three UI lab presets 17 of 17 each; Twister in WSL on both native_sim variants,
    12 of 12 executed cases; Twister on Windows, `qemu_x86`, 4 configurations built (none run on
    Windows); the workflow check 5 of 5, with Manual started at the controller's maximum; the
    M21-3 demonstration 10 of 10; the web demo smoke test; 33 mutation probes (19 of the first
    implementation, 14 of the review fixes), all killed but one equivalent mutant (the flush's own
    Stop check, which the earlier Stop checks make unreachable). UI lab captures against M21-5a:
    only the four faulted-run captures change.
- 2026-11-11, M21-5c (draft): **faults and the decision** (M21-5c design above). Owner decisions
  2026-11-11: a Faults dialog; a decision panel that cannot be closed.
  - **Client and model:** each fault entry's recovery, category and continuation; the decision's
    time left and fault; the fault behind a faulted run and its alert kind (HMI-FAULT-CATEGORY).
  - **Screens:** the Faults dialog with Acknowledge and Recover, the decision panel with Continue and
    Stop, and their buttons on Running, Manual and Home. The Zephyr host maps the three commands.
  - **Parity:** the UI lab's simulation and the web demo raise and answer the same entries and have a
    decision scenario.
  - **Against the real controller:** furnace firmware #55 (simulation only) brings the controller to
    a decision with a continuable LAG; the workflow check runs it with Continue.
  - **Reviews** (AI agents in the HMI roles). No path sends Continue, Resume or a recovery without an
    operator tap. Fixed test-first, each with a mutation probe the tests kill:
    - the decision panel's buttons kept their first state, so a brief link loss could leave the
      operator without Stop (High);
    - an alert lost as a decision ended; a stale countdown looked live; Manual's Faults button was
      made anew on every observation; fault actions named a place in a changing list; the Faults
      dialog closed on Running as the run moved; an unavailable list read as "no faults"; the alert
      named the lowest inhibiting fault, with "target not reached" for lag and holdback (Medium);
    - texts for Continue rejections and unknown recovery replies; no reply for an entry not found;
      fault actions wait for a Stop and a fresh list; texts from the catalogue (Low).
  - **Found while testing:** on a faulted Manual session each observation enabled and disabled the
    steppers again, restarting their transitions; LVGL's heap grew from about 63 KB to 180 KB and the
    Zephyr test ran out at 192 KiB. Fixed; the heap stays flat (peak 85 KB) and a test checks that
    observations start no animations. A Stop tap now redraws an open Faults dialog at once.
  - **Evidence:** `host-msvc-core-debug` 15 of 15; `host-msvc-desktop-debug` and `host-debug` (GCC)
    18 of 18; the three UI lab presets 17 of 17 each; Twister in WSL, 14 of 14 executed cases;
    Twister on Windows, `qemu_x86`, 4 configurations built (none run); the workflow check 6 of 6
    against firmware #55; the M21-3 demonstration 10 of 10; the web demo smoke test; 26 mutation
    probes (22 of the change, 3 of the client and adapter, 1 of the stepper fix), all killed but one
    equivalent mutant (Recover can never be offered before Acknowledge). UI lab captures against
    M21-5b: the four faulted-run captures change and the decision capture is new; manual-cooling
    differs only in its known timing (HMI-CAPTURE-TIMING).

- 2026-11-10, M21-2b (draft): **the operator screens are shared** (design above).
  - **Moved without change first:** all 279 lab captures (93 at 800x480, 800x640 and 1024x600)
    were byte-identical after the move and after the content seam.
  - **Compiled with GCC `-Werror` for the first time.** This removed ten dead functions
    (`device_build_components`, `page_title`, `format_stage_card_summary`, then their helpers
    `device_add_row`, `device_component_summary`, `device_count_components`,
    `device_create_list`, `format_rate_tenths`, `device_component_visible` and
    `device_task_needs_attention`), and marked an intentional switch fall-through.
  - **Behavior changed on purpose, in both presentations:**
    - Return Home leaves a cooling run cooling. Two lab captures changed for it: Home now shows the
      stopped run at 115 C cooling, where it showed an idle furnace at 24 C.
    - Manual has no forecast without a live temperature.

    The smoke tests of the lab and of the web demo were updated to match. A third capture differs
    by timing alone: it was taken during LVGL's disabled-state animation, and is identical once it
    settles (checked in two probe builds).
  - **Evidence** (CTest per preset; Twister per test instance):
    - `host-msvc-ui-lab-debug`, `-800x640-debug` and `-1024x600-debug`: 17 of 17 each;
    - `host-msvc-core-debug` 15 of 15;
    - `host-msvc-desktop-debug` and `host-debug` (GCC): 18 of 18 each. They include the new
      `desktop.operator_content_smoke`:
      - content: no simulated date, clock or program; no assumed forecast; the unavailable
        presentation; a Start that sends nothing;
      - lifecycle: a held run stays on Running; a run that still cools is not complete and is left
        by Return Home; a decision is named; a controller Manual session holds Running;
    - `tools/hmi_web_demo_p1_smoke.cjs` passes;
    - Twister in WSL, `native_sim` and `native_sim/native/64`: every executed instance passes,
      including `furnace_hmi.presentation.operator_content` and the client test with its 11
      adapter scenarios;
    - Twister on Windows, `qemu_x86` (`--short-build-path`): 5 of 5 cases;
    - the end-to-end demonstration, 10 of 10, with the operator screens.
  - **Proven on the old code:** the protocol review's two presentation defects were run against
    the code before the fixes in a probe build, and both checks failed there: a zero clock
    (check 95) and "complete" while cooling (check 104).
  - **The Zephyr window:** Home from the controller, at 20 C current, with the target and remaining
    time unknown and the door and fan unknown (the simulated controller has no feedback).
  - **Follow-ups (owner 2026-11-10):**
    - correct the simulated controller's repeated session identity in a narrow firmware change,
      with explicit fixtures for intentional repetition (CTRL-SIM-SESSION-ID);
    - add end-to-end fault injection with the controller's simulation before Phase 21 acceptance
      (E2E-FAULTS);
    - the HMI's client-session entropy is a hardware requirement before live commands.
  - **The architecture review** (architect with software critic; AI agents in the HMI roles)
    escalated controller state shown that was never reported:
    - "complete" while cooling;
    - a zero clock;
    - the decision state shown as paused;
    - Running left during a held or Manual session.

    All are fixed with tests. Log writes and library changes outside the editor are also
    gated, for the library milestone.
  - **Open, recorded in the backlog:**
    - how Home presents a background cooling run, and a way back to Running;
    - the decision state (AWAITING_DECISION);
    - web parity for unknown controller content (with the library, M21-7);
    - the quick launch's hint while the library is unknown;
    - the lab capture harness's sensitivity to animation timing.
- 2026-11-10, M21-3 (draft), on the owner's decision to connect to the actual controller first:
  - **Built:** `app/hmi/transport` (the byte port and a loopback), `app/hmi/client` (session,
    synchronization, freshness, the snapshot request, the observation), and the Zephyr link thread
    with a single-slot mailbox. The availability screen follows the link, with a development marker
    for the simulated link (a shared component, also used by the desktop program).
  - **Tests:** 35 scenarios against a scripted controller over the loopback, in CTest
    (`host.controller_client`) and in Zephyr (`furnace_hmi.client.controller_link`):
    - the handshake: refused version, retry and a lost `HELLO_ACK`;
    - the link: silence, reconnect and restart, a repeated session ID and an unresolved request;
    - ordering, including a periodic `STATUS` within a snapshot and an older periodic `STATUS`;
    - events: gaps, and events during the first snapshot;
    - corruption and heartbeat;
    - rejections (`NOT_CONNECTED`, `STALE_SESSION`), duplicates on the first snapshot and the
      capabilities request, and an exhausted request ID;
    - synchronization timeout; a lost snapshot end while connected; a superseded snapshot;
    - the fault list: paged, a lost page, a missing list;
    - absurd intervals, bounded polls and clock wrap.
  - **Defects found and fixed,** each test-first:
    - a reconnect fell silent again at once;
    - a restart with a repeated session ID could have shown the old session's status as current.
  - **The protocol review** (protocol reviewer with software critic; AI agents in the HMI roles)
    found these, each fixed test-first:
    - the first repeated-ID rule misfired on normal snapshot traffic;
    - a lost snapshot end while connected was never asked for again;
    - fault list pages were not checked;
    - the controller's intervals were used unchecked;
    - a fault-event flood could keep a snapshot from completing;
    - reused client session IDs were detected only on the first request;
    - a blocked link thread would leave the GUI showing current;
    - the demonstration's fault and run-state check compared nothing.

    The error auditor's wording corrections were also applied.
  - **End to end:** `tools/controller_link_demo.py` runs the controller's own simulated program and
    the HMI as two processes in WSL. The program is furnace firmware `tests/sim/link_pty`, at main
    `721f3cb` (the merge of controller PR #51, the codec copy's revision). Ten checks pass:
    - connect;
    - authentic STATUS: the HMI's temperature, validity, run state, heating inhibition, latched
      class and unacknowledged count equal the fields the relay read in the controller's frame of
      the same sequence;
    - the fault list as sent (the controller's `FAULT_LIST` total);
    - cut and restore;
    - corruption;
    - controller restart (with the repeated session ID), and authentic STATUS after it;
    - HMI restart (a new client session);
    - only read messages from the HMI;
    - the controller's STATUS cadence: longest gap 0.21 s observed, against a 0.5 s threshold and a
      0.2 s interval.
  - **Limits of the evidence:**
    - The simulated controller is idle and fault-free, so the end-to-end fault comparison is of an
      empty list; non-empty lists are covered by the scripted scenarios.
    - The cadence is the controller's link service. Its control timing is the controller's own
      verification, not repeated here.
  - **Not done here:** heating commands, operator workflows and the operator screens (M21-2b,
    M21-5).
- 2026-11-10, M21-2 (draft): **evidence gate passed.** In WSL (Ubuntu 24.04, WSLg), Zephyr 4.4.0 on `native_sim/native/64` with LVGL `85aa60d1` builds, opens its SDL window and runs. The HMI's Zephyr application now builds the shared state-availability screen on that board (the main thread is the GUI context, ADR-0005; Zephyr's pointer input thread only reads display pointers set before main), and `qemu_x86` keeps the console probe. The desktop program's presentation checks moved to `tests/presentation/availability`, one definition run by the desktop smoke test and by the Zephyr test `furnace_hmi.presentation.availability` (a `zephyr,dummy-dc` display without SDL, `native_sim` and `native_sim/native/64`; each state rendered with `lv_refr_now`). The window path is built by Twister and was run by hand under WSLg, and headless with `SDL_VIDEODRIVER=offscreen` (it starts and runs; SDL's `dummy` driver has no renderer, so the display fails to start), which M21-3's demonstration uses. Reviews (embedded reviewer with software critic, and error auditor; AI agents in the HMI roles) were applied: gradient stops as the desktop, a build failure if that board builds without LVGL, a display check and a minimum sleep in the GUI loop, wording corrections. A window capture of each build shows the same screen, apart from the desktop program's development footer; the captures are visual evidence, not a pixel-exact comparison (the Zephyr window passes through WSLg's remote display). Found: the operator screens live only in the UI lab, so M21-2b was added. On Windows, `qemu_x86` Twister passes the foundation probe and the codec conformance (the run M21-1 left out) and builds the probe sample; the desktop program passes under MSVC (`host-msvc-desktop-debug`, 16 of 16) and GCC (`host-debug`, 16 of 16).
- 2026-11-10, M21-1 (draft): the codec copied at `721f3cb` with its manifest, `tools/sync_controller_wire.py`, and the conformance test. The golden vectors pass under the desktop MSVC build (`/W4 /WX /permissive-`, the codec unmodified), the host GCC build (`-Werror`) and Zephyr native_sim. Not run: `qemu_x86` on Windows.
- 2026-11-10: plan and ADR updates (M21-0), draft. Protocol and architect review, and the decision challenger, applied (records in ADR-0007 and ADR-0009).
