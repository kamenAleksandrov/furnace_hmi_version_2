# Operator feedback round (2026-10-06) plan

**Status (2026-10-06):** complete (results below). Product owner request,
worked in chunks; each chunk was verified before the next started.

## Outcome

Product owner requests of 2026-10-06, in both presentations unless marked
*demo page only* (the web wrapper outside the HMI rectangle, the UI-parity
exception):

| # | Chunk | Scope |
| --- | --- | --- |
| 1 | Website wrapper | Real Telamorph logo colours; logo as the browser-tab icon; a "software in development" watermark; a short intro (logo, company name, "HMI Demo V2", "Work in progress", then the demo); remove the Door opened scenario. *Demo page only.* |
| 2 | Quick launch after a screen-size change | Changing the screen size redraws the page so Quick launch fits again (bug: going from 800x640 to 800x480 left extra recent programs clipped). *Web only*: the native lab's size is fixed per build, so it never resizes at run time. |
| 3 | Epoxy programs | The program library is three realistic epoxy cure programs, **Epoxy system 1/2/3**; the demo library switch offers 3 programs or none. Replaces the twelve ceramic/glass fixtures. |
| 4 | Program creation that runs | A program created (or edited) in the editor is stored for the session, listed in the library, can be favourited, started and run like a library program, and deleted. Native lab: a real in-memory program store and a start request that the simulated controller runs. No persistence across sessions. |
| 5 | Maintenance task dialog | Tapping a Maintenance card opens a dialog with an X close button and **Maintenance done**; done is a controller request that, when confirmed, resets the task, adds a Service history record and a Device log event. |
| 6 | Device info | Remove the Controller link card; Support opens a dialog with the support QR code (`https://telamorph.com/`). |
| 7 | Device log | Event feed in the style of the Ajax Systems app: filter tabs, events grouped under date headings with their age, a type icon and time per event, program names in the text; one month of history; paged. |
| 8 | Close-out | Status, UX baseline, web demo record, open questions; full verification. |

**Non-goals:** persistence of created programs, a production program store or
log storage contract, a real support URL policy, the website sync/build/deploy.

## Answers to the product owner's questions

**Controller link card (fact):** it showed whether the Device data on screen
is a live controller snapshot or a last-known one, and the snapshot age. That
is already shown where it matters (the stale banner on Overview, "Last known"
on paged lists, the link-lost state on Running), so for an operator the card
repeated information. **Decision (recommended, applied in chunk 6):** remove
it; Device info keeps Model, HMI firmware, Controller firmware, Catalogue and
Support.

**Ajax Systems (fact):** a manufacturer of wireless security alarms. Its phone
app's Notifications feed (product owner screenshot) lists events newest
first, grouped under date headings such as "1 Oct 2026 (4 days ago)", with
filter tabs (All, Alarms, Malfunctions, Security modes, System events). Each
event is a card with a coloured type icon and time, one line saying what
happened and who/which device, and the device or user glyph on the right.
**Furnace mapping (recommendation, applied in chunk 7):** tabs All, Programs,
Alarms, Maintenance, System; events such as "Program completed: Epoxy system 2",
"Over-temperature stop: Epoxy system 3", "Maintenance done: Door seal",
"Display restarted".

**Support QR (fact):** the supplied image decodes to `https://telamorph.com/`.

## Governing rules

- [ADR-0001](../07-decisions/0001-authority-boundary.md): the controller owns
  program storage, run start, maintenance records and the event log; the HMI
  sends requests and shows confirmed results. The simulators stand in for
  the controller and claim no production behaviour.
- [ADR-0012](../07-decisions/0012-theme-and-language-catalogues.md) and the
  theme-and-language skill: new colours are tokens, new text is catalogue
  strings, in both presentations.
- UI-parity skill; operator text policy (production wording inside the HMI).
- Cards fill the page (product owner preference, 2026-10-01).

## Assumptions

- Epoxy cure schedules are plausible public-domain shapes (room-temperature
  laminating resin post-cure, 120 C prepreg cure with a flow dwell, 180 C
  high-Tg cure with step post-cure) within the furnace limits (180 C,
  3.0 C/min, 8 stages). They are illustrations, not resin datasheet values.
- The QR code is presentation data until a controller/device-configuration
  field for the support link exists (open question).
- Native user-started runs advance at 60x (one simulated minute per second)
  so a run is watchable; the web keeps its playback speeds.

## Progress

- [x] 1 Website wrapper (web smoke: tab icon and bar mark in the logo colours,
  intro texts and skip, watermark takes no input, six scenarios)
- [x] 2 Quick launch refit (web smoke: 800x640 -> 800x480 fits, back restores)
- [x] 3 Epoxy programs (both; native fixtures replaced by a session program
  store with real stages; twelve ceramic program strings removed)
- [x] 4 Program creation that runs (both: save adds or replaces, details of the
  saved program open, Save disabled with the first problem shown, library
  capacity 12, Delete removes it from the library, Favourites and Recent.
  Native: Start, Pause, Resume and Stop are controller requests the lab hands
  to the simulator, which now runs any program through cooling to Complete;
  Return Home closes a completed run. Smoke checks in both)
- [x] 5 Maintenance task dialog (both: corner X, Maintenance done as a
  controller request; the task starts a new interval, condition-based tasks
  keep their state, last baseline becomes today, a Maintenance record leads
  Service history and Last service; disabled without current data. Smoke
  checks in both. The task detail text no longer says viewing does not record
  service.)
- [x] 6 Device info and support QR (both: five cards, Support spans two
  columns and opens the QR code; `tools/build_support_qr.py` makes one A8
  image for LVGL and a CSS mask for the web from the supplied image and
  verifies it decodes to `https://telamorph.com/`; new theme tokens `qr-plate`
  and `qr-ink` with a 7:1 contrast pair. Controller link strings removed.)
- [x] 7 Device log (both: filter tabs All/Programs/Alarms/Maintenance/System
  fill the toolbar; date headings with their age; one-line rows with the type
  icon in its tone and the time; pages fit the measured height; tapping a row
  shows date, time and source. History: a seeded generator draws the same
  month in both presentations relative to each clock (verified in captures:
  the same days, times, programs and alarms). Session events: web from the
  demo's controller actions; native from controller reports (run started,
  paused, resumed, completed, faulted, stopped; link lost/restored) plus
  program saved/deleted, maintenance done, manual start/stop, settings saved
  and display start. The Overview Errors card opens the log on Alarms.)
- [x] 8 Close-out (status, web demo record, UX baseline, Device proposal
  decision, open questions, controller backlog, icons README, index)

## Verification

Per chunk: `node tools/hmi_web_demo_p1_smoke.cjs`; for HMI chunks also the
native MSVC build and CTest at 800x480, 800x640 and 1024x600
(`-E codebase_index`), generator `--check`s, `tools/check_theme_language.py`
and captures of both presentations.

Baseline before work (2026-10-06): native 800x480 CTest 13/13; web smoke
passed.

## Results

- Native MSVC builds without warnings; CTest 13/13 at 800x480, 800x640 and
  1024x600 (`-E codebase_index`). New native smoke checks: create, save, list,
  start, complete and Return Home, delete; Start/Pause/Resume/Stop requests;
  Maintenance done (X, record, history); Support QR image and X; Device log
  history (more than 40 events, newest first, within 30 days), filters and
  details. New captures: maintenance-task, maintenance-recorded, device-info,
  device-support, device-log, device-log-alarms, start-running, program-run.
- Web smoke passes with the matching checks plus intro, favicon, watermark,
  six scenarios, Quick launch refit, library switch, log filters/headings.
- `build_strings`, `build_theme`, `build_support_qr`, `build_lvgl_icons`,
  `build_icon_sprite`, `build_fonts` `--check` and `check_theme_language.py`
  pass; `build_support_qr.py --verify` decodes `https://telamorph.com/`.
- Side-by-side captures of both presentations compared for Home, Programs,
  program details, Running, the Maintenance dialogs, Device info, Support and
  the Device log.

## Deviations and known differences

- The web dialog X takes the native free-standing tile edge (amber line
  under it) because LVGL tiles have no edge-less style.
- Native times come from the lab's fixed controller clock (14 Sep 2026
  14:32), so session events all carry that minute; the web uses the browser
  clock.
- Native user-started runs advance 60 times faster than real time (lab
  speed); scenarios keep real time.
- The web keeps its older `controller.events` request trail (no longer shown);
  the Device log uses the new event list.

## Follow-ups

- Settings > System > Delete programs is still a preview; it does not remove
  the session's programs.
- Phone scanning of the on-screen code (both presentations) and the physical
  panel were not tested.
- Website sync/build/deploy (`scripts/sync_hmi_demo.py` in the website repo)
  not run; the new head link and intro need no sync changes.
- Open questions recorded: Maintenance done permissions, support link source,
  Device log retention/query, program library and capacity.

## Round 2 (2026-10-06, later; product owner feedback on round 1)

| # | Chunk | Result |
| --- | --- | --- |
| R2.1 | Website: one development note, out of the way | The tiled watermark is replaced by one faint line ("Software in development · Not a final product") at the bottom of the page in the footer; the bar pill now reads Work in progress (amber). *Demo page only.* |
| R2.2 | Maintenance done is quality of life | Recorded: a tracking aid for machine health (recommendations), not warranty-based; the earlier note stays because this may change (Device proposal, UX baseline, open questions, backlog). |
| R2.3 | Manual readings at 800 px | Each reading is its label over its value in equal columns, so the state and six readings fit 800 px (both). |
| R2.4 | Manual Stop goes into cooling | Stop (and the 48 h end) keeps the session on screen in a Cooling state: readings Elapsed, Cooling, Est. remaining, Current, Target 30; a dashed cooling forecast; steppers off; Exit (Home) and Start new (Manual page with the same target and rate) replace Pause/Stop; navigation stays locked until one is chosen (both). Native shows the lab's observation, so its cooling is static. |
| R2.5 | Settings > System | Delete programs lists the programs as toggles with Delete (n) and Delete all (n); the confirmation names them; they are removed (with Favourites, Recent, log events). Reset settings offers Display preferences, Date & time formats or All preferences, with true defaults (the old program-preference scope showed 200 C / 12 h, settings no page shows) (both). |
| R2.6 | Date formats | Date format preference with five choices (6 Oct 2026, 06.10.2026, 06/10/2026, 10/06/2026, 2026-10-06), saved with the Save bar; the clock card, Maintenance done records, Service history, Device log headings and details follow it (both). |

**Verification (round 2):** native MSVC builds without warnings and CTest
13/13 at 800x480, 800x640 and 1024x600 (`-E codebase_index`), with new smoke
checks (Manual Stop/cooling/Exit/Start new, Date format choice and Save,
Delete programs toggles/Delete (1)/Delete all/nothing left, Reset of the
formats) and captures (manual-heating, manual-cooling, settings-time,
settings-delete; the geometry check caught a clipped "Delete selected (1)",
now "Delete (1)"); web smoke passes with the matching checks; generator
`--check`s and the theme/language lint pass. Physical panel not tested.

## Round 3 (2026-10-06, later)

| # | Chunk | Result |
| --- | --- | --- |
| R3.1 | Program Stop goes into cooling | Stop during heating or holding cuts the run where it stands (the plan ends there) and the controller cools automatically on Running, labelled "Stopped · automatic cooling"; Stop during automatic cooling marks the run stopped. **Start again** runs the same program from the start at once; **Return Home** closes the run. A stopped run's cooling does not log Program completed (both). Native: the simulator's Stop cuts a started program and keeps cooling; a new close-run request ends it; a start is accepted while a run cools. |
| R3.2 | Delete programs removes all | Settings > System > Delete programs asks once ("Delete all 3 programs?", naming them) and removes every program; a single program is deleted from its details (both). The toggle dialog is gone. |
| R3.3 | Date format choices fit | The choice buttons keep one line ("2026-10-06") (web). |

**Incident (fixed):** an edit script for R3.2 replaced a range bounded by
anchors far apart and deleted unrelated code in both presentations (web: the
device pages, log, program catalogue and settings helpers; native: the
restart-assessment, restart-outcome and reset-scope dialogs). The web demo was
restored byte-for-byte from the last passing smoke run's browser cache copy
(16:55) and only this round's edits were re-applied (verified by diff); the
three native functions were restored from the committed copy plus the
round-2 changes. Both suites pass afterwards.

**Verification (round 3):** native MSVC builds without warnings and CTest
13/13 at 800x480, 800x640 and 1024x600 (`-E codebase_index`) with new checks
(Stop -> cooling at 70 min, Start again from 0, Return Home; Delete all with
Cancel first) and a program-stopped capture; web smoke passes with the
matching checks (cut at 40 min, cooling advances, Start again, Return Home,
Delete all, one-line date choices); generator checks and lint pass.
