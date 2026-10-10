# HMI web demo

**P1 implementation update (2026-09-18):** The [client demo build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) now has its faithful viewport packet implemented in the canonical single HTML. The remaining authoring, simulator, scenario, website-adaptation and release packets are still pending.

## Operator feedback round (2026-10-06)

**Demo page only (parity exception):** the website bar and the tab icon use
the Telamorph logo exactly as the website shows it (`assets/images/brand/logo.svg`,
#566e7e and #34393c; the earlier mark was lightened for the dark bar). A faint
diagonal "Software in development · Not a final product" watermark covers the
page without taking input. Entering the page shows a short intro (logo,
Telamorph, HMI Demo V2, Work in progress) that a tap or key skips and
`?intro=off` leaves out (the smoke test uses `skipDemoIntro()`). The Door
opened scenario is removed (the HMI alert stays). Scenarios now run the epoxy
programs; the Program library switch offers 3 programs or none (programs
created in the session stay listed). Changing the screen size redraws the page,
so Quick launch fits again.

**HMI (both presentations):** see [status](STATUS.md) and the
[feedback round plan](OPERATOR_FEEDBACK_2026_10_06_PLAN.md).

**Round 2 (demo page only):** the tiled watermark became one faint line at
the bottom of the page footer ("Software in development · Not a final
product"), and the bar pill reads Work in progress (amber) instead of
Interactive demo.

## Simulated readings and side menu (2026-10-02)

The simulated measured temperature follows the plan within 0.5 C, as a
controller tracking its setpoint would: a brief lag and a small overshoot
where the rate changes at a stage corner, then it settles. Scenario profiles
add the larger deviations of faults. The fullscreen Controls tab is 40 px wide
with a gentler glow, and the side-menu scrim fades with the slide (demo page
only; the native lab has no side menu).

## Theme and language readiness (2026-10-05)

The demo's colours are the generated `hmi-theme` CSS tokens and its text the
generated `hmi-strings` catalogue copy (`t()`, `tp()`, ids passed to
`button()`/`tell()`); `setHmiTheme(id)` and `setHmiLanguage(tag)` switch at
runtime. Do not hand-edit the generated blocks; see the
[readiness plan](THEME_AND_LANGUAGE_READINESS_PLAN.md).

## Settings menu clean-up (2026-10-05)

Settings Maintenance is renamed System (power icon); Connections is listed
only when the controller enables server/Bluetooth (the demo fixture
`demoFixtures.connection` is `'disabled'`); Date & time has no Clock source
card; the brightness gauge shows no percentage; Dim when idle defaults to Off.
Both presentations; see [status](STATUS.md).

## Device and Settings card pages (2026-10-01)

Device and Settings sub-pages use full-page card grids, a brightness gauge,
dim and Running view choice dialogs and a Set date & time dialog (the demo
clock follows the browser clock plus the set offset). Both presentations; see
the [card pages plan](DEVICE_SETTINGS_CARDS_PLAN.md).

## Showcase page, scenarios and controller alerts (2026-10-01)

**Decision (product owner):** the page around the HMI becomes a client
showcase in the Telamorph website style (tokens from the website `main.css`,
ambient glow, frosted panel, website buttons), with Back to website
(`industrial.html`) and Get a quote (`contact.html?type=industrial`). See the
[showcase plan](WEB_DEMO_SHOWCASE_PLAN.md).

**Fact (demo wrapper, parity exception):** one control panel beside the HMI
(below it under 1180 px) holds Scenarios, Playback (status, previous stage,
pause/play of the simulation clock, next stage, speed Real/60x/300x/1200x with
300x default, Start example run), Screen (800x480, 800x640, 1024x600; Fit to
window or Actual pixels; Fullscreen), Program library (12, 7 or no programs),
On-screen keyboards and Reset demo. Fullscreen hides the website bar, scales
the HMI to the display and moves the panel into a side menu opened from a
Controls handle. The device-data, connection and restart fixtures left the
page; their values now live in `demoFixtures`, and scenarios drive them.

**Fact (scenarios, demo-only):** Complete run, Overheating, Not heating enough,
Door opened, Sensor failure, Power loss and Connection lost each fast-forward a
program to its event in about 5 s (power loss ~7 s with a blackout, connection
lost ~8 s including the restore). Faults go through `reportControllerFault`,
link changes through `setLinkLost`, exactly as a controller report would.

**Fact (HMI behaviour, both presentations):** the controller alert dialog,
Running fault state and Return Home, and the link-lost Running state described
in the [UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md#controller-alerts-proposal-2026-10-01).
Native: `fault_kind` in the dashboard model, `overtemperature`, `heating-slow`,
`sensor-fault` and `power-loss` semantic scenarios, and the existing `fault`
scenario tagged as door opened.

**Website sync:** the HMI page keeps its engineering links between
`engineering-links` markers; `scripts/sync_hmi_demo.py` in the website repo now
removes that block (it previously matched the removed selector paragraph).

**Layout update (2026-10-01, product owner):** outside fullscreen the page is
one window tall (minimum 560 px) and scrolls only when it cannot fit; the
panel scrolls inside itself. Beside the screen only from 1181 px; below that,
on phones and in fullscreen the controls are a side menu ordered Screen,
Playback, Scenarios (folded until opened), On-screen keyboards, Program library.
It opens from a Controls button in the website bar, or in fullscreen from a
glowing Controls tab on the right edge. On phones the website links move into
the menu (Get a quote stays in the bar).

**Second layout update (2026-10-01, product owner):** one panel order
everywhere (Screen, Playback, Scenarios, On-screen keyboards, Program library);
Scenarios is always listed, never folded; the HMI floats with no frame ring;
the website bar is its own frosted box like the panel. The page has one static
blue glow in the website glow material (glow.css/composite.css colours, 40 px
blur), centred and anchored to the bottom of the page like a lit floor; the
earlier drifting blobs and the amber light under the screen were removed. Units now follow the
[UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md#values-and-validity) (`°C`
in labels), in both presentations; the Manual readings strip gap is 8 px in
both so six readings with unit labels fit at 800 px.

**Fixed (both presentations):** stale Device Components/Info pushed the pager
14 px below the page; the stale banner now shows on Overview only and list tabs
keep their "Last known" footer marker.

**Known differences (native vs web, unchanged here):** the native modal leaves
the Running controls uncovered so Stop stays reachable, while the web dialog
covers the page; a single native dialog button is not centred.

**Verification:** `tools/hmi_web_demo_p1_smoke.cjs` passed (panel, playback,
three scenarios end to end, alerts, fullscreen side menu, phone fit, no
exceptions); all seven scenarios and 1600x900, 1024x768, 390x844 and 844x390
layouts were reviewed in headless Chrome captures. Native MSVC CTest passed at
800x480, 800x640 and 1024x600 apart from the known local `tools.codebase_index`
failure, including the new alert checks and captures. Not run: physical touch,
Safari/Firefox, the website sync/build/deploy.

## Soft-tile UI adopted (2026-09-29)

**Decision (product owner):** the soft-tile style preview replaces the previous
web demo and is the reference the LVGL presentation will be ported to. It now
lives at [`hmi-web-demo.html`](../images/theme/hmi-web-demo.html); the preview
file was removed. The previous demo was untracked and is not kept in the repo.

**Fact (visual):** dark page, lighter tiles, square corners; each button has a
2px fading edge on the side facing away from where it sits (side edges on rail,
Quick launch, device and settings rows). Colour roles: amber primary/Cancel,
yellow Edit, red destructive (Delete, Stop, Clear, Discard), steel-blue
navigation, purple Hold, green OK. Shared original icons live in
[`app/hmi/presentation/icons`](../../app/hmi/presentation/icons/README.md) and are
embedded by `tools/build_icon_sprite.py`.

**Fact (behaviour changes to carry into LVGL and the UX documents):**

- Program details offers **Start program** with a confirmation dialog; the Load
  screen remains only for Quick launch, with Start under the facts panel.
- The stage editor is a fixed-height dialog over the page it came from: delete,
  pager with editable stage position (moves the stage), add; Heating/Hold keep
  the same rows (Hold locks Target and shows Rate "None"); delete confirms.
- Home Quick launch shows all favourites then recents, trimmed to fit.
- Running: readings strip above the graph; the graph shows only the start of
  cooling, with a Show/Hide cooling toggle once measured cooling passes that
  window; measured cooling is a blue dashed line; planned cooling ends at its
  estimate.
- Manual: readings strip above the graph (state, elapsed, to target, current,
  controller setpoint and heater demand, estimated power); measured line plus
  prediction; one-piece Target/Rate steppers.
- Pagers are one component (‹ 1 / 2 ›); Device and Settings lists show up to
  five rows; keyboards group Cancel/Clear left and Apply right, with a blinking
  cursor and centred entry text; the numeric keypad has no sign key.

**Closed (2026-09-30):** LVGL parity is implemented; the
[LVGL port plan](LVGL_SOFT_TILE_PORT_PLAN.md) is complete and lists the few
remaining known differences. The UX baseline, screen concepts and
program/startup model record the changed behaviour. Setpoint and heater demand
are protocol-listed controller values; the demo synthesises them, and the native
Manual Setpoint shows "--" until the observation model carries it
(CTRL-SETPOINT).

**Fact (demo wrapper, not HMI):** the fit now uses the window height left after
the toolbar, so the HMI scales from phone landscape up to 2x on large screens
(previously it could never exceed its own height). The toolbar aligns with the
HMI width, switches to short labels when narrow (container query), and becomes
one scrollable row without title/status on short landscape screens. Its full
restyle is deferred.

**Verification:** `tools/hmi_web_demo_p1_smoke.cjs` passes against the new demo
after updating four assertions for the new layout (Stages label, stage-card
order, manual readings strip, stepper caption) and measuring the manual graph in
layout pixels, since the demo now scales above 100%. `tools/build_icon_sprite.py
--check` passes. Viewports 360x740, 740x360, 844x390, 768x1024, 1280x720,
1366x768 and 1920x1080 were reviewed in headless Chrome. Real touch hardware and
non-Chromium browsers were not tested.

## Inter typeface (2026-09-30)

**Decision (product owner):** the HMI uses Inter in both presentations. The
demo embeds the Inter Regular/SemiBold subsets as WOFF2 between its
`hmi-fonts` markers (generated by `tools/build_fonts.py`; do not edit) and
applies them to the HMI screen and dialogs only; the demo toolbar keeps the
system font. Inter is wider than Segoe UI, so the Settings rail icon gap is
8 px (right padding 4 px) and the Running view switch takes a 1.8 share of its
row. See the [LVGL port plan](LVGL_SOFT_TILE_PORT_PLAN.md).

**Verification:** `tools/hmi_web_demo_p1_smoke.cjs` passed; a headless-Chrome
overflow probe found no clipped or wrapped HMI labels on Home, Programs,
Program details, Device, Settings, both keyboards or Running.

## Program screens ported (2026-09-30)

**Fact:** the native LVGL lab now implements Start program with confirmation on
Program details, Load from Quick launch only, the Program stages toolbar and the
stage cards. The UX baseline and program/startup model record the start flow;
the stage-editor dialog is recorded in the UX baseline and screen concepts and
implemented natively as of LVGL milestone 4. Web
fix found while comparing: the program editor's "Edit stages" wrapped in its
124 px slot, so the compact actions are 140 px.

## Decision and scope

Rename the shared browser presentation to
[`hmi-web-demo.html`](../images/theme/hmi-web-demo.html). Maintain it alongside
native LVGL presentation changes. The browser remains a synthetic client demo;
[ADR-0001](../07-decisions/0001-authority-boundary.md) still governs the real HMI.
No controller, protocol, firmware, or storage behavior changes here.

## P1 faithful viewport implementation (2026-09-18)

**Fact:** Added a common outer toolbar and viewport wrapper around the existing
HMI. The wrapper provides Fullscreen/Exit with an in-page fallback, advanced
fixture disclosure, 1x/10x/60x/300x speed selection, simulation freeze,
Next stage, Return to run, Reset demo session, logical-size selection and
Inspect / zoom. The HMI stays a proportional logical rectangle and the native
dialog is positioned explicitly against the scaled viewport.

**Boundary:** This is browser presentation/demo behavior only. It does not add
controller authority, protocol traffic, firmware behavior, persistence or a
second mobile editor. The source fragment is materialized at bootstrap only so
the shared shell can own the HMI, fixtures and dialog layer.

**Verification:** tools/hmi_web_demo_p1_smoke.cjs passed with headless Chrome:
all three logical sizes at scale 1, toolbar state changes, text keyboard open,
dialog bounds, portrait 390 x 800 fit with no horizontal page overflow, rotate
hint, fullscreen fallback and zero runtime exceptions.

**Not run:** real phones, physical touch, Safari/Firefox/Edge, visual
screenshot comparison, website synchronization/build/deployment, native
builds, firmware and protocol checks. Website publication remains a separate
P6/release task.

## Advanced scenarios and working program envelope (2026-09-18)

**Decision:** The saved demo and host-only LVGL fixtures now use a working
envelope of 0-200 C and up to 3.0 C/min in either direction. This is a
reversible illustration choice for local previews and authoring assistance,
not a production furnace protection limit or controller validation contract.
The Control CPU remains the authority for real program limits and execution.

**Fact:** The HTML advanced-scenario area is grouped into Actions, Data
fixtures, and Review and recovery. Letter-keyboard and numeric-keypad previews
are explicit buttons. A started demo advances automatically once per wall-clock
second at the default 60x setting (one virtual program minute); Pause, Freeze,
manual one-minute Advance, speed selection, and Next stage remain available.

**Fact:** All twelve HTML catalogue entries and the native LVGL program
fixtures/stage previews were rescaled to the working envelope. The native
semantic running and overrun fixtures, dashboard graph scale, and concept SVG
were aligned with the same illustrative range.

**Verification:** The extended browser smoke driver
tools/hmi_web_demo_p1_smoke.cjs passed, including grouped controls, all twelve
program bounds, automatic ticking, and manual advancement. The native
tools/run_ui_lab.py check build and CTest run passed all 10/10 tests, including
UI-lab smoke and geometry, and the codebase-index check passed. Website
synchronization, embedded builds, physical touch, and production controller
validation remain not run.

## Ownership and update workflow

The HMI repository owns the shared HTML. The Telamorph website owns its client
adaptation in `scripts/sync_hmi_demo.py` and the resulting `hmi-web-demo.html`.
Keep future client-specific transformations in that script so refreshing the
shared UI does not overwrite them. The website build uses its checked-in copy
and does not require this HMI checkout.

From `C:/dev/projects/Telamorph_website`:

```powershell
python scripts/sync_hmi_demo.py C:/dev/Furnance_project/furnace_hmi_version_2/docs/images/theme/hmi-web-demo.html
python scripts/build_site.py
python -m http.server 8000 --directory dist
# Open http://localhost:8000/hmi-web-demo.html and review interactions.
# After review:
firebase.cmd deploy --only hosting --project telamorph
```

Fact: the existing site deploys `dist/` and uses an allowlisted Python build.
The demo is copied as a standalone page without the marketing navigation/footer.
It is absent from the navigation, sitemap and llms.txt, and requests no indexing
through both HTML metadata and a Hosting header. It is publicly accessible by
URL; unlinked/noindex does not provide access control.

The client adaptation removes engineering-document links and adds client-facing
simulation wording. No new Firebase service or console setup is needed. The
existing contact-function rewrite uses `pinTag`, so a Hosting deployment can
also synchronize that function revision under the established website workflow.

## Execution and verification (2026-09-15)

1. Rename the source and update references: complete.
2. Add repeatable client adaptation and standalone build inclusion: complete.
3. Build: passed, 163 files / 7 product pages; deployment inventory and local
   links validated.
4. Headless Chrome: passed HTTP load, four main navigation routes, text keyboard
   open/cancel, example run, advance, pause/resume, and all three HMI sizes
   (800 x 480, 800 x 640, 1024 x 600), with no runtime exceptions.
   Captures are local under `build/hmi-web-demo-review/`; the 800 x 480 capture
   was visually inspected.
5. Hosting deployment: attempted but blocked by expired Firebase credentials.
   Firebase reports `Authentication Error: Your credentials are no longer valid`.
   Run `firebase.cmd login --reauth` in an interactive terminal, then repeat the
   Hosting deploy command above. Live URL verification was not possible.
6. Generated index regeneration and `--check`: passed. Local Markdown file
   links checked; demo JavaScript syntax checked with Node.

Limitations: the fixed-size device presentation still targets desktop review;
phone adaptation and further client polish remain future work. Native/Zephyr
builds and physical hardware checks were not run for this HTML/hosting change.

Rollback: remove the standalone page from the website build and its Hosting
header, rebuild and redeploy; alternatively restore the previous Hosting
release. No persistent data migration is involved.
