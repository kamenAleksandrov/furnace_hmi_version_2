# Graphite UI verification record

Date: 2026-09-09. Scope: host-only LVGL laboratory.
Status: M0-M5 complete; software verified for operator review.

**Fact:** Existing uncommitted work was preserved. The initial dirty-file list,
baseline patch, and native baseline images are retained under
`build/graphite-review/`. No sibling controller repository was edited.

**Decision:** The implementation follows the accepted graphite presentation
and the supplied `build/ui-review-graphite-concept.svg`. Responsive composition,
spacing, and fixed footer sizes are presentation choices within the completion
plan; no architecture or production protocol decision was changed.

## Native evidence matrix

Every row is captured at 800 x 480, 800 x 640, and 1024 x 600. The gallery is
`build/graphite-review/final/index.html`; `manifest.json` records the source hash,
executable hashes, native sizes, deterministic simulator time, and frame names.
Each viewport directory contains configure/build/CTest logs and native PNGs.

| Surface | Captures and exercised variants |
| --- | --- |
| Home | Default, empty/few/full favourites, changed local recents, idle/fault/disconnected |
| Program loading | Default and long eight-stage program, Start request preview |
| Programs | First/last full pages, partial last page, empty library |
| Program details | Add/Remove favourite, management actions, complete grouped facts |
| Program stages | First/last pages; maximum eight-stage fixture last page |
| Program stages | Direct card/node entry to the stage editor; stage-detail presentation is retired from the operator flow |
| Program editor | Heating/Hold/Cooling; controlled cooling; New program/Add stage; local validation |
| Running | Graph/Split/Furnace, running/paused/stale/overrun, zero/invalid values, long name, negative sample/gap |
| Manual | Distinct Manual heading, measured-only graph, retained ordinary navigation |
| Device | Current and unavailable observations; unavailable acknowledgement record |
| Settings | Read-only HMI-local preferences and controller-owned placeholder |
| Dialogs | Notices, service/current/unavailable, delete, rename preview, Save, Discard, run details, Pause/Resume/Stop previews, Start preview, request result, favourite capacity |

## Recorded results

**Fact:** All 55 frames at each native viewport were visually inspected, including
final reinspection of corrected stage identity, overrun copy, fastest cooling,
and Manual modal bounds. This is author visual verification, not operator approval.
No remaining clipping, overlapping text, off-panel controls, or graph-bound errors
were observed in this bounded matrix. Larger editor/detail panels retain free
space deliberately; their actions remain in predictable fixed positions.

| Check | Actual result | Local log |
| --- | --- | --- |
| UI lab 800 x 480 | 10/10 CTests; 55 captures; zero geometry errors | `build/graphite-review/final/800x480/tests.log` |
| UI lab 800 x 640 | 10/10 CTests; 55 captures; zero geometry errors | `build/graphite-review/final/800x640/tests.log` |
| UI lab 1024 x 600 | 10/10 CTests; 55 captures; zero geometry errors | `build/graphite-review/final/1024x600/tests.log` |
| Shared host matrix | Debug 10/10, Release 10/10, MSVC desktop 10/10, MSVC core 8/8 | `build/graphite-review/host-verification.log` |
| Zephyr foundation | Workflow exit 0; qemu_x86 build passed; Twister 3/3 cases on 1/1 configuration; GCC analysis completed | `build/graphite-review/zephyr-final.log` |
| Python tooling and generated index | 40/40 Python tests; canonical generation and --check passed; git diff --check passed | `build/graphite-review/tooling-verification.log` |

Representative actual LVGL images: [Running](../images/theme/graphite-running-800x640.png),
[Home](../images/theme/graphite-home-800x640.png), and
[Program editor](../images/theme/graphite-editor-800x640.png).
The supplied SVG remains the design reference; these PNGs are actual renders.

## Defects corrected

- Header controls and simulation identity now have reserved space. Columns
  account for gaps, and all layout containers stay above the fixed footer.
- Graphs use final layout dimensions, measured tick widths, rectangular grids,
  stage-boundary nodes, and fixed domains that expand for negative temperatures
  and overruns without contracting other bounds. Measured gaps remain gaps.
- Furnace geometry is drawn together on a dedicated owner-context canvas with
  proportional roof, side, door, window, handle, and static heater detail.
  Short layouts put sensor readouts beside the drawing.
- Program summaries, stage counts, durations, maxima, and preview graphs use
  the selected bounded fixture. Stage cards paginate instead of clipping.
- Editor controls retain their fixed footer and local draft through Cancel;
  confirmed Discard leaves the editor. Keyboard and background navigation
  cannot escape an open editor or modal.
- Home/Running/Device observations update in place. A private snapshot avoids
  aliasing mutable simulator state. Stale snapshots cannot retain current-value
  claims; invalid door/fan/service data remains unavailable.
- Modal lifetime survives telemetry. Stop remains outside the modal input
  overlay and takes priority over a pending low-priority local action.
- Start displays explicit request-preview feedback; controller-observed active
  Program state controls entry into Running. Manual remains a separate mode.
- Deferred callbacks are cancelled before destruction. Graph arrays, action
  slots, fixture counts, and per-frame work remain bounded. Existing LVGL
  fatal-allocation limitations in ADR-0008 still apply.

Two GCC analyzer warnings remain in upstream Zephyr libc `malloc.c` (lines
153 and 249); none were emitted from repository-owned sources. Zephyr also
prints its existing test-random-generator configuration warning. The earlier
PowerShell-redirection invocation reported shell exit 1 despite successful
subcommands; a rerun with direct subprocess exit recording confirmed workflow
exit 0. The final log is `build/graphite-review/zephyr-final.log`.

Author review covered authority, bounded work, object lifetime, and evidence
consistency. Independent reviewer agents, physical touch testing, hardware
execution, and production protocol/acceptance testing were not run in this pass.


## 2026-09-09 local program-edit follow-up

**Fact:** The 800 x 480 MSVC UI-lab build was regenerated after the local
program-edit refinements. The capture executable produced all 55 frames with
zero geometry errors, and the current `host-msvc-ui-lab-debug` CTest preset
passed 10/10. Author inspection covered `home-empty`, `home-full`,
`program-detail`, populated first/final stage pages, and the editor.

**Fact:** Inspection confirmed the transparent/divided header, coloured dotted
status pill, unfilled Notifications control, empty quick-launch prompt, red
delete action, compact favourite/rename controls beside the title, amber View
stages action, 3 x 3 direct-edit stage grid, and editor ordinal/navigation.
The final-stage capture was corrected from an invalid “Page 3 of 2” fixture
selection to the actual “Page 2 of 2”.

**Limit:** This follow-up did not repeat the 800 x 640 or 1024 x 600 review
matrix, physical touch testing, or production/controller verification. The
star remains an asterisk fallback because the bundled LVGL symbols contain no
star, and stage-name keyboard entry remains deferred by request.
## Verification method and limits

`desktop.ui_lab_visual_geometry` captures the actual SDL renderer on the LVGL
owner thread, checks labels/buttons/containers against every ancestor, checks
actual text metrics, and checks plotted vertices. It fails on any error.
`desktop.ui_lab_smoke` additionally checks the 640 x 360 structural floor,
real action callbacks, editor containment/Discard, duplicate pending taps,
modal retention, Program lock, Manual navigation, Stop access, measured gaps,
invalid values, domain retention/expansion, new-session reset, and repeated
modal cycles. The 640 x 360 floor is not a visual-usability claim.

**Fact:** The lab sends no controller requests and establishes no acceptance,
persistence, service authorization, safety behavior, or production retry
semantics. Local recent selections are a host preview preference. Rename and
Save remain explicit request previews. Device acknowledgement records and
controller Settings remain unavailable until their contracts exist.

**Open product/hardware questions:** operator visual approval, panel selection,
physical resistive-touch sizing, reduced-motion preference, final Device and
Settings fields, and production controller/service semantics. These are
separate from software verification; no hardware approval is claimed.

## Reproduction

```powershell
.\.venv\Scripts\python.exe tools\review_ui_lab.py
python tools\run_ui_lab.py --scenario idle
.\build\host-msvc-ui-lab-800x640-debug\simulator\ui_lab\Debug\furnace_hmi_ui_lab.exe --scenario running-normal
.\build\host-msvc-ui-lab-1024x600-debug\simulator\ui_lab\Debug\furnace_hmi_ui_lab.exe --scenario running-normal
```

The review tool stops on configure/build/test failure and encodes native BMPs
as PNGs with Pillow. CTest capture itself needs no Pillow. Open one interactive
executable at a time; native presets, not desktop window resizing, establish
viewport coverage.

## 2026-09-10 Running-view refinement

**Fact:** Fresh 800 x 480 captures inspected the Graph, Split, and Furnace
views after the selector, graph, and footer changes. The capture run reported
zero geometry errors and `host-msvc-ui-lab-debug` passed 10/10 CTests. Wider
viewport review was not repeated for this presentation-only follow-up.