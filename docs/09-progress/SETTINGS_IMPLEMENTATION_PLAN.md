# Settings implementation plan

Status: **Host-only LVGL presentation implemented**, 2026-09-14. Production
controller, protocol, storage, and hardware behavior remain outside this plan.

## Outcome and non-goals

Deliver the [Settings proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md) using the
existing graphite shell, then connect local authoring preferences to editor
assistance and the requested Maintenance tab. Keep production clock/connectivity
and maintenance action integration as separate gated
milestones. No controller-configuration override, furnace logic, safety limit,
actual runtime enforcement, or accepted-program mutation is authorized here.

The host-only LVGL laboratory now renders the HTML concept’s five-category
Settings screen, Display subpages, local edit transaction, numeric controls,
and simulated Maintenance confirmations. These are presentation previews only;
they do not send requests or persist device state.

## Evidence and decisions

Facts, user mandate, recommendations, open questions, governing requirement
IDs, and ownership are listed in the proposal. Production runtime remains
unchanged; only the host-only presentation lab was extended.
Before behavioral implementation, resolve warning versus strict local gating,
unset/default values, cooling scope, and total-duration uncertainty. Record the
consequential preference/save policy in an ADR and update the canonical
requirements, state ownership, authoring model, and traceability deliberately.
Clock and connection integration need their own evidence and protocol review;
do not infer them from simulator fixtures.

## Modules and execution ownership

- `simulator/ui_lab/src/ui_lab_explorer.c` / `.h`: category routing, temporary
  forms, keypad/dialog integration, simulated outcomes. Split settings-specific
  presentation into a focused file if implementation would enlarge the explorer
  substantially. Preserve existing unrelated work in this dirty worktree.
- `app/hmi/presentation/strings/`: all new operator text and formatting.
- Portable HMI model/application modules (exact names chosen during milestone
  2): preference values/version, edit transaction, pure issue reporting against
  a candidate and current capabilities. Never own program acceptance.
- HMI storage adapter: versioned local preferences and recoverable persistence;
  hardware API stays outside portable code. Host memory/file implementation is
  distinct from a future Zephyr target adapter.
- Controller client / platform connectivity adapters: only after ownership,
  metadata, permissions, lifecycle, and target support are defined.

The GUI owner alone accesses LVGL. Async completion carries copied bounded
data with request/form identity, not widget pointers. Bound field count, issue
count to the editor's stage capacity, input lengths, queues, retries, and
timeouts. Coalesce status updates; never drop a save result silently. Failed
enqueue reports busy without claiming a request was sent. Navigation/disposal
invalidates old UI recipients. Storage and network work must not delay Stop.

## Ordered milestones and completion criteria

1. **Review design.** Use the clickable browser concept and proposal to settle
   product choices. Done when the field meanings and correction policy are
   recorded, with unsupported features explicitly deferred.
2. **Local model and policy.** Define optional-value encoding, units, increments,
   preference version, current-capability inputs, and structured issue output.
   Record the ADR/requirements before adopting consequential behavior. Done
   when focused tests cover bounds, unavailable metadata, calculations, whole
   program revalidation, and no mutation of accepted programs.
3. **LVGL lab presentation.** Replace the placeholder with categories, form
   editing, saved/dirty/error feedback, and explicit simulator connection/clock
   scenarios. Done when native captures and interaction checks pass at 800 x
   480, 800 x 640, and 1024 x 600, with no clipping/scroll dependence and usable
   keypad/modal targets. Operator review is separate from author inspection.
4. **Editor integration.** Check rate-driven and duration-driven heating,
   inherited targets, stage reorder/delete, and total duration. Implement the
   chosen issue review and before/after correction preview. Done when all
   affected stages revalidate and controller rejection cannot be bypassed by
   preference acknowledgement. Retain lab preview labels until real save exists.
5. **Local persistence.** Define a small versioned record, validation, atomic
   replacement or equivalent recovery, and the last successfully saved state.
   On failure retain the dirty candidate and old committed values. On invalid
   or unknown-version data show recovery status; never silently replace chosen
   authoring preferences with invented limits. Done when restart, interrupted
   write, corrupt data, and unsupported schema tests demonstrate recovery.
6. **Clock integration (conditional).** Specify controller clock metadata,
   request acceptance/result, timezone/time-source semantics and permissions.
   Done when real or contract-faithful tests cover acceptance, rejection,
   timeout, late response, session change, and no automatic replay, while run
   timing/history retain their defined semantics.
7. **Connections integration (conditional).** Verify actual config source and
   hardware/CPU ownership; define service purpose, credentials, status,
   permissions, and reconnect behavior. Done when unsupported/disabled/unknown
   states cannot trigger operations and config changes invalidate pending UI
   actions appropriately. Production hardware evidence is required separately.

8. **Maintenance presentation and action contracts.** Add four action rows and
   scoped reset/delete/display-restart confirmations plus the three furnace
   assessment branches. Use explicit simulation fixtures first. Before real
   actions, review/accept [ADR-0010](../07-decisions/0010-maintenance-request-workflows.md),
   define normative protocol semantics, reset defaults, deletion scope/atomicity,
   pending-write handling, and platform restart support. Done when cancellation
   sends nothing, double confirmation sends once, final rejection is respected,
   busy/unknown never grants authority, and recovery never replays old intent.
   Preserve Program navigation containment and normal Stop visibility.

Milestones 6 through 8 can use reversible, clearly simulated UI before the
separate production behavior contracts and hardware evidence are complete.

## Verification and failure cases

Use existing host/UI-lab tests and native capture tooling. Add focused behavior
tests at implementation, not tests that merely mirror the screen construction.

- Equal-to-limit, just-over-limit, invalid/empty/overflow numeric inputs,
  quantization, and impossible preference/controller range intersections.
- Rate correction increasing duration; target changes altering later stages;
  unknown cooling duration not counted as zero; no automatic destructive fix.
- Preference-only acknowledgement invalidated by edits/preference version;
  controller bounds always remain separately enforced by the controller.
- Stale capabilities, changed controller identity, lower replacement bounds,
  local edits offline, and existing programs unaffected until explicitly edited.
- Save/discard/navigation, storage failure/restart, duplicate taps, disposed
  forms, late completion, queue full, and no worker LVGL access.
- Date/time rejection or unknown outcome; no changed wall clock used as a
  substitute for operational elapsed time.
- Connection config off, unsupported radio, permission revoked, source stale,
  failed pairing/authentication, and recovery without replaying old commands.
- Active Program navigation containment and Manual navigation remain intact;
  ordinary low-priority settings work cannot obstruct normal Stop.

Run the relevant host/UI-lab suite and capture review for actual UI changes;
build each supported target touched by portable/platform changes. Physical
brightness, dimming, touch and radio behavior require target testing and cannot
be certified by the browser concept or desktop simulator.

### Maintenance verification extension

- Reset exact selected preference scope; credentials/controller values/programs
  excluded; atomic write failure retains old values and approved defaults only.
- Delete selected/all confirmation matches authoritative IDs/revisions/count;
  stale selection, controller rejection, unsupported bulk deletion, partial
  batch outcomes, timeout, history and active-revision preservation.
- Display restart resolves unsaved work and local writes; a subsequent boot
  reconstructs authoritative state without replaying pending furnace actions.
- Restart assessment OK/busy/timeout, Cancel at each step, late response,
  expired assessment, changed controller session/state during confirmation,
  transport unavailable, duplicate clicks, final rejection after prior OK,
  accepted then disconnected, lost acceptance, same-session reconnect, new
  session snapshot, and inability to attribute an independently observed reboot.

## Documentation and migration

Update the canonical proposal/model when decisions are accepted; update
requirements/traceability and ADRs together, then status, roadmap, session log,
and documentation index with actual results. Use `tools/codebase_index.py` to
regenerate owned metadata and `--check` plus `git diff --check` to verify it.
Do not manually edit generated documentation.

Current planning artifacts introduce no stored state and need no migration.
Future storage uses an explicit schema version and migration policy; a rollback
must preserve a recoverable prior record and must never reinterpret unsupported
preferences as controller configuration. Removing the lab screen must not
alter saved programs or controller operation.

## Planning verification record

**Fact:** Owning index generation and `tools/codebase_index.py --check` passed.
Local link-target checks for the new proposal, plan, concept, and documentation
index passed. `node --check build/settings-concept-script.js` passed for the
extracted concept script. `git diff --check` passed with existing CRLF-to-LF
notices. Playwright was unavailable in the project environment.

No LVGL implementation, firmware build, native interaction tests,
browser-render inspection, or physical-panel verification was performed.
The concept is review material, not verified native screen evidence.

**Maintenance extension:** Added the requested action flows and proposed ADR-0010.
Local link targets, extracted JavaScript syntax, generator regeneration/check,
and diff whitespace validation passed. A Node mocked-DOM interaction check
passed maintenance routing, all 12 assessment/result combinations, cancellation,
scoped preference reset, and delete/display-restart confirmation previews.
The first harness run failed because its fixture elements were not initialized;
a corrected harness passed. This is script-level evidence only, not browser
layout, native LVGL, actual transport, deletion, or reboot verification.

## Host-only LVGL implementation record - 2026-09-14

**Fact:** The native Settings route is implemented in the host-only LVGL
explorer. It includes the five category rail, Program preferences, the two
Display pages, Date & time, Connections availability states, local
Save/Discard/Stay behavior, bounded numeric editors with plus/minus controls,
and the requested scoped Maintenance previews. All visible Settings text is
in the English string catalog.

**Fact:** The host MSVC UI-lab build passed. The full
`host-msvc-ui-lab-debug` CTest preset passed 10/10, including Settings smoke
interactions and native geometry captures. The Settings capture set contains
the default page, Display page 2, Date & time, Connections, and Maintenance;
all reported zero geometry errors at 800 x 480.

**Limit:** The browser concept remains the visual source of truth, but browser
render inspection was unavailable in this environment. No Zephyr graphical
target, controller request path, persistent storage, production clock,
connectivity, deletion, or restart behavior was implemented or certified.
