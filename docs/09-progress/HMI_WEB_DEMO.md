# HMI web demo

**P1 implementation update (2026-09-18):** The [client demo build plan](WEB_DEMO_CLIENT_BUILD_PLAN.md) now has its faithful viewport packet implemented in the canonical single HTML. The remaining authoring, simulator, scenario, website-adaptation and release packets are still pending.

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

**Open:** LVGL parity has not been implemented (the AGENTS web/LVGL parity rule
applies to that port). The UX baseline and program/startup documents still
describe the previous behaviour; the Start-from-details and stage-dialog changes
should be recorded there (or in an ADR) before or with the LVGL port. Setpoint and
heater demand are protocol-listed controller values; the demo synthesises them.

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
