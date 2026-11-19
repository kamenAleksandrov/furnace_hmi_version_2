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
| M21-2b | The operator screens in the shared presentation | The lab's screens move from `simulator/ui_lab` into `app/hmi/presentation`, fed only by the HMI model; simulator data stays in the lab; the lab and the Zephyr application compose the same screens; the lab's smoke tests and web demo parity unchanged. Before M21-5 |
| M21-3 | Transport and client against the controller (WI-H06), with the former M21-4 | The controller and the Zephyr HMI run as separate programs in WSL over the simulated link; the 0.16 handshake and compatibility check; authentic `CAPABILITIES`, snapshot and `STATUS`; faults, measurement qualification, run state and freshness in the client's observation; disconnect, reconnect, controller restart and stale sessions handled; nonblocking; nothing simulated by the HMI; automated tests and an end-to-end demonstration. No heating command (design below) |
| M21-4 | Merged into M21-3 | Owner decision 2026-11-10: the end-to-end link is part of M21-3 |
| M21-5 | Run, Manual and fault workflows (WI-H03, WI-H04) | Each workflow runs against the controller; the same behavior in the web demo, simulated |
| M21-6 | Controller 20d (firmware) | A controller-side plan with its open decisions for the product owner and the controller's reviewers (for example the health rules still proposed); the messages built test-first with the controller's verification; the HMI resynced to the new revision in lockstep, ADR-0007 checked against its reopen triggers, before M21-7 |
| M21-7 | Library, names, Device log (WI-H02, WI-H10, WI-H12) | Against the controller; parity in the web demo |
| M21-8 | End-to-end workflow tests (WI-H07) | Scripted operator workflows over the simulated link; M3 closed by the furnace roadmap's criteria |

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
  with M21-2b and M21-5.
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
