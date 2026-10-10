# Web demo showcase and controller alerts plan

**Status (2026-10-01):** complete; results below.

## Outcome

1. The browser demo page becomes a client-ready showcase: one tidy control
   panel (scenarios, playback, screen size, program library, keyboards), a
   fullscreen "real screen" mode whose controls live in a slide-out side menu,
   Telamorph website styling outside the HMI, and Back to website / Get a quote
   links.
2. Fast scenarios (about five seconds each) fast-forward a run and end in a
   realistic event: complete run, over-temperature, target not reached, door
   opened, temperature sensor fault, power interrupted, connection lost.
3. The HMI gains a **controller alert** dialog and a Running fault state, in
   both the web demo and the native LVGL lab.

**Non-goals:** controller fault semantics, alarm acknowledgement, latching,
history or recovery (open question "Faults/alarms"); protocol or wire schema;
website deployment; a guided tutorial (answered separately, not built here).

## Governing rules

- [ADR-0001](../07-decisions/0001-authority-boundary.md): the Control CPU owns
  faults, run termination and recovery. The HMI only presents them.
- [Program and startup model](../03-hmi/PROGRAM_AND_STARTUP_MODEL.md): link
  loss marks observations stale and never replays commands.
- [UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md): faults, alarms and
  stale transitions are never animated.
- AGENTS.md web/LVGL parity. The wrapper, scenario player and blackout
  visualisation are demo-only (parity exception); the alert dialog, Running
  fault state and Return Home are HMI behaviour and ship in both.

## Facts, assumptions, decisions

- **Fact:** the alarm workflow is unresolved
  ([open questions](OPEN_QUESTIONS.md), "Faults/alarms", "Sensor loss").
- **Fact:** the native model already has `RUN_STATE_FAULT` and `STALE`
  availability; the native `fault` scenario is a door-open safety stop.
- **Decision (reversible presentation proposal):** an alert dialog appears
  when the observation reports a new fault kind, when the controller link is
  lost, and (if the link-lost alert is still open) when it is restored. Its only
  action is **Close**, a local dismissal. It is not an acknowledgement and does
  not clear anything; the fault remains on the Running page and in Device
  notifications.
- **Decision:** a fault ends the run's active controls. Running stays visible
  (graph frozen at the fault), Pause is hidden and Stop becomes Return Home.
- **Assumption:** the alert texts are HMI-local presentations of a
  controller-reported fault category. Final texts, codes and severities come
  from controller fault semantics (CTRL-ALERTS in the
  [integration backlog](CONTROLLER_INTEGRATION_BACKLOG.md)).
- **Decision (product owner, 2026-10-01):** Data fixtures, connection fixture
  and restart fixtures leave the demo panel; scenarios drive those states. The
  program library selector stays. Default playback speed is faster (300x).

## Affected modules

| Area | Change |
| --- | --- |
| `docs/images/theme/hmi-web-demo.html` | Wrapper, panel, drawer, scenario player, playback, alert, fault state |
| `app/hmi/model` | `fault_kind` observation field (HMI representation only) |
| `simulator/semantic_controller` | Fault kinds on `fault`; new over-temperature, heating-slow, sensor-fault and power-loss scenarios |
| `app/hmi/presentation/strings` | Alert titles/details, Return Home |
| `simulator/ui_lab` | Alert dialog, Running fault state, Return Home, smoke checks |
| `tools` | Scenario lists, browser smoke |
| Telamorph website `scripts/sync_hmi_demo.py` | Follow the new wrapper markers |

## Milestones

1. Web wrapper and panel, fixtures replaced by state, playback controls.
   Done when the smoke test drives every panel control at three sizes and
   fullscreen moves the panel into the side menu.
2. Web alert, fault state and the seven scenarios. Done when each scenario
   ends in its state within about 5-8 s with no exceptions.
3. Native model, scenarios, strings, alert and fault state. Done when CTest
   passes at 800x480, 800x640 and 1024x600 with new alert assertions.
4. Telamorph styling and glow; website sync script; documents and generated
   index.

## Verification

Browser smoke (all panel controls, scenarios, alert, fullscreen drawer, phone
fit); native MSVC CTest at the three review sizes with captures; generator
`--check`. Not planned: physical touch, Safari/Firefox, website deployment.

## Rollback

Revert the commit. No stored data, protocol or controller behaviour changes.

## Results (2026-10-01)

All four milestones are done. The web page, scenarios and alerts are recorded
in [HMI web demo](HMI_WEB_DEMO.md#showcase-page-scenarios-and-controller-alerts-2026-10-01);
the alert behaviour is in the
[UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md#controller-alerts-proposal-2026-10-01).

**Deviations:** native fault scenarios open on Running (Program) or Home
(power loss) with the alert already shown, as a first snapshot with an active
fault; the web scenarios reach the same states live. The website copy was not
re-synced because it has uncommitted local changes.

**Verification:** browser smoke passed, and every scenario plus desktop, tablet
and phone layouts were reviewed in headless Chrome captures. Native MSVC CTest
passed at 800x480, 800x640 and 1024x600 except the known local
`tools.codebase_index` failure; alert captures were compared with the web.

**Layout follow-up (2026-10-01):** one-window-tall page, side menu below
1181 px, on phones and in fullscreen, folded scenarios, a stronger fullscreen
tab; the stale Device footer overflow is fixed in both presentations. Web
smoke and native CTest at all three sizes passed again (same known
`tools.codebase_index` failure).

**Follow-ups:** native single-button dialogs are not centred; Home readings in the web demo are static
after a power-loss scenario; the guided tour is not built.
