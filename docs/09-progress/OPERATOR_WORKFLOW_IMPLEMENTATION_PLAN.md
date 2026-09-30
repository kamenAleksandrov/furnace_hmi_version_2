# Operator workflow implementation plan

Date: 2026-09-26. Status: browser/native host UI implementation complete;
controller integration and physical-touch verification remain separate.

## Outcome and evidence

Deliver simplified navigation, Manual setup/run, program creation and automatic
cooling in browser demo and native LVGL lab. Firmware/protocol implementation
and website deployment are separate. See [concepts](../03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md)
and searchable [controller backlog](CONTROLLER_INTEGRATION_BACKLOG.md).

Facts: review sizes are 800 x 480, 800 x 640 and 1024 x 600; existing touch
baseline is 48 x 48 logical pixels. Preserve substantial prior worktree changes.
Affect [browser source](../images/theme/hmi-web-demo.html) renderHome,
renderLoading, renderPrograms, renderStageEditor, renderRunning, switchMain,
graph/keyboard helpers, [native explorer](../../simulator/ui_lab/src/ui_lab_explorer.c),
host main, service catalogue and shared strings. Inspect current source before
assuming packet status in the [previous browser plan](WEB_DEMO_CLIENT_BUILD_PLAN.md).

Governing [requirements](../00-project/REQUIREMENTS.md): REQ-FUN-001 through 007,
009/010/011/013/014/017; REQ-NFR-002/004/006; ARCH-INV-001 through 009;
PROTO-001/002/003/008/011; TEST-001 through 007/010. Preserve accepted
[authority ADR](../07-decisions/0001-authority-boundary.md),
[protocol layering](../07-decisions/0003-protocol-layering.md),
[simulator](../07-decisions/0004-desktop-simulator.md) and
[GUI ownership](../07-decisions/0005-lvgl-single-owner.md).

## Confirmed decisions

- Quick Launch retains favourites/recents, preview then explicit Start; preview
  Back returns Home. Only Choose a program becomes Start manual, opening inert setup.
- Programs tab always opens library; protect dirty edits before leaving. Program
  active/paused navigation remains contained. Remove the status/current-page
  bezel on every page; retain only the yellow top accent line as the
  persistent light-request control.
- Maxima: 180 C, 3 C/min, 48 h. Use simulated controller metadata now; production
  limits/steps come from current controller session. Remove local preference limits.
- Save allows curing <=48 h excluding cooling. Controller ends Manual heating at
  48 h and handles automatic cooling; HMI notice begins ten minutes before and
  returns to Manual setup after the limit. Stop then fresh accepted Start can
  begin another Manual session.
- Author Heating/Hold only, no stage names/material. One automatic final cooling
  phase excluded from authored stage count and curing duration; no controlled cooling.
- Details separate curing, Est. cooling/total. Stage cards/preview graph
  nodes are read-only. Show parent name and Number of stages. Pencil opens full editor.
- Pause/Stop gain selector width. Show curing elapsed and remaining together on
  one line; during cooling retain curing elapsed with separate cooling elapsed
  and Est. remaining.
- Manual is a 70/30 top-graph/bottom-control composition. In both presentation
  implementations, live state and elapsed sit at the top of the lower band;
  Start/Pause/Stop are left of target/rate. Target/rate labels sit above their
  direct and plus/minus controls, with larger symbol-centred hit targets.
  Operator-facing labels use production wording rather than demo/preview
  qualifiers. Native elapsed is observed state; browser elapsed is demo-only.
- Manual parameter and Start/Pause/Resume/Stop edits retain the graph widget and
  update plotted values, labels and action visibility in place. Its temperature
  axis uses the supplied full controller range rather than auto-zooming around
  a selection. Browser simulation defers route redraw while a touch keyboard owns
  input focus; it does not close the keyboard for a periodic presentation tick.
- Yellow accent; letters/numbers plus space/edit keyboard controls, Caps, correct
  numeric range/step. Components becomes Device log; Notifications becomes Maintenance.
  Detailed maintenance and Restart furnace deferred; preserve fault visibility.
- Light is always accessible and requests controller action. Fan/door work deferred.

## Required reconciliation and open inputs

REQ-FUN-005 still specifies six-hour default/open expiry; REQ-FUN-017 permits
authored COOL/controlled rate. Program model allows stage labels/graph editing;
old Settings/browser plans retain preferences and old authoring/keyboard/palette.
Milestone 0 records a consequential ADR and updates canonical requirements/model,
protocol capability wording and traceability together. Follow domain/decision
review before ADR acceptance. Product decisions are already supplied. This plan
does not silently alter accepted requirements.

Recommendations: summary rows plus existing stage editor; Manual Pause/Resume
under REQ-FUN-004; active Manual/cooling containment recorded explicitly as an
extension to REQ-FUN-003. Idle Manual keeps navigation. Label countdown Until cooling.
Whether pause consumes deadline remains controller input; edits never reset it.

The persistent top accent line is the Light utility slot; it must remain
reachable without covering Stop or content.
Device log initially uses bounded volatile session memory; distinguish intent
from confirmed events. Recommend semantic actions rather than each keyboard key.
Capacity/verbosity and persistent history need later decisions.

Cooling endpoint/averages need engineers/Svetlio. Smoothed live ETA is a proposal
requiring validation; one-minute slope is not an approved whole-cycle model.
Estimate never determines completion. Discord image unavailable here; Bambu Lab
is inspiration, not a copied specification.

## Ordered milestones

1. Reconcile baseline via ADR/canonical updates. Inventory routes/fixtures/tests;
   prototype both screens at all sizes. Done: conflicts explicitly resolved and
   idle/run/cooling Manual plus empty/populated editor/keyboard layouts reviewable.
2. Separate library, detached drafts, immutable accepted run, observations and
   route state. Stable IDs, current/stale/unavailable capabilities, origin-aware
   Back, Programs reset, containment. Done: dirty/empty navigation works; no Start
   on entry or renderer-owned operational limits; missing metadata fails closed.
3. Program editor: name/list/Add/Save/Cancel, existing Heating/Hold editor, stage
   Apply changes draft only; final Save awaits simulated acceptance. Add/delete/
   reorder update Hold inheritance and durations. Done: create/save/reject/discard,
   exactly-48-hour/over-limit, final-stage deletion, long names and read-only previews.
4. Manual: inert setup, graph/controls, pending/accepted/rejected Start/Apply/
   Pause/Resume/Stop. Simulator owns expiry; notice at 47:50 reconstructs on
   reconnect. Done: edits alone never operate; stale/late/rejected requests,
   deadline, Stop/restart work; HMI restart cannot reset controller deadline.
5. Running/cooling: reallocate action width without shrinking touch height;
   separate timers; labelled estimated cooling band/cone. Unknown ETA does not
   block permitted Start. Start with synthetic/engineering averages; live ETA
   waits for validation. Done: count/summary/graph agree; flat/rising/missing/stale
   samples cannot fabricate finite ETA or completed state.
6. Device/common UI: bounded log with source/time/intent/outcome, deduplication,
   empty/overflow states; new routes preserve fault access. Light observed/pending/
   rejected/unavailable states. Yellow accent/keyboards retain semantic colors.
   Done: log cannot block GUI, Light/Stop reachable, numeric metadata reflected.
7. Native parity and verification: implement browser packets, mirror approved
   behavior in native lab. Shared strings, one LVGL owner, bounded plain-data
   handoff, safe widget lifetime. Browser interactions/captures and native MSVC
   build/CTest/geometry at all three sizes; inspect images/keyboards/dialogs.
   Recheck phone/fullscreen/rotation after shell edits. Cover rejection, stale,
   reconnect/reboot, duplicate taps, no replay and immutable run. Record actual
   results and unrun physical/production checks; deployment remains separate.

## Recovery, resources and rollback

Controller owns limits/timing/validation/persistence/actuators/safety. HMI owns
drafts/navigation/presentation/local log; simulator truth is synthetic. Bound
stages, telemetry/log history and pending requests. Propose drop-oldest log with
retention indicator, never blocking Stop. Session changes invalidate capabilities/
pending state. Reconstruct from snapshot without history or automatic command
replay. Rejection preserves draft. Browser data stays page-session only.

Rollback scoped packets without resetting unrelated work. Real legacy COOL/named
records need versioned compatibility, never silent stage deletion. Update status,
roadmap/questions/session log/traceability and regenerate owned metadata. Planning
verification is in SESSION_LOG.md; runtime/visual/native/thermal/hardware tests
were not run in this documentation-only pass.
