# Programs presentation refinement

Status: **Proposal for operator review**, 2026-09-14. Scope is the shared HTML
concept and a later native styling pass, not new program content or behavior.

## Evidence and scope

**User mandate:** Review Programs, Program details, and Program stages, retaining
their content and improving style/layout where useful. The user already likes
the existing screens; this is a refinement, not an information-architecture
redesign.

**Facts:** Inspected the current native `programs.bmp`, `program-detail.bmp`, and
`stages.bmp` under `build/host-msvc-ui-lab-debug/simulator/ui_lab/visual-captures/`.
Read `make_program_card`, `build_programs`, `build_program_detail`,
`build_program_stages`, `program_fixtures`, `program_stage`, and graph-domain
logic in `simulator/ui_lab/src/ui_lab_explorer.c` plus the string catalogue.
The browser snapshot contains the same twelve initial program fixtures and
sixty stages, including their existing summaries, estimates and illustrative
graph points. It does not include transient edits in a running native lab.

**Recommendation:** Keep the two-column/six-card library, graph/facts details,
and three-column/up-to-nine-card stages. Reuse the shared graphite shell and
make Programs a working destination beside Device and Settings in the
[existing concept](../images/theme/hmi-web-demo.html).

Governed by the [visual baseline](UX_AND_DOMAIN_BASELINE.md),
[program model](PROGRAM_AND_STARTUP_MODEL.md), REQ-FUN-002, 003, 009, 012, 013,
014, 017; REQ-NFR-004, 006; ARCH-INV-001, 005, 007, 009, 010 in
[requirements](../00-project/REQUIREMENTS.md), and ADR-0001/0005. No new
architecture or protocol decision is proposed; no ADR is needed for these
reversible presentation choices.

## Refinements

| Screen | Existing strength | Proposed styling change |
| --- | --- | --- |
| Programs | Names dominate, six cards per page, direct selection | Use neutral charcoal cards with a fine divider and aligned stage/duration/temperature columns; reserve stronger blue for New program and navigation rather than every summary strip |
| Program details | Complete planned graph, grouped facts, separate Load action | Align graph and fact surfaces; give labels/values a consistent hierarchy; group Back, name, favourite and rename; keep Delete clearly red but less visually dominant than ordinary navigation |
| Program stages | Three-column direct-entry cards and kind colors | Consistent card padding, outlined ordinal badges, quiet boundaries and aligned target baselines; separate the inherited-target caption from its temperature so it cannot crowd the card |
| Shared paging | Explicit Previous / Next | Place the existing page indicator in the action toolbar, releasing its separate row for content; preserve button semantics and disabled endpoints |

Retain 8 px gaps, 6 px main corners, at least 48 px ordinary controls,
48 px action bars, muted supporting text, deep-blue primary navigation and
Heating amber / Hold violet / Cooling teal. Full-card click/tap targets remain
large. Graph-node keyboard/click links and direct stage cards both lead toward
the existing editor. The native graph hit areas must remain usable in the later
port; do not substitute decorative circles for its actual interaction targets.

In the compact stage grid, HOLD still presents **Inherited target (180 C)** or
the appropriate value, but as two aligned typographic pieces. It remains
inherited rather than a newly editable temperature. A badge is the stage's
display ordinal, not a durable stage identity. Long labels can ellipsize only
where the full label remains available at the existing detail/editor destination.

## Content and action parity

| Surface | Preserved content/actions |
| --- | --- |
| Library | Program names; stage count; numeric minutes; maximum temperature; New program; Previous / Next; empty state; card opens details |
| Details | Name; Back; favourite toggle; rename; Delete program; planned graph; stage count; duration; maximum temperature; estimated energy; material; View stages; Load program |
| Stages | Back; Add stage; Previous / Next; ordinal; name; kind; duration; target or inherited target; card opens the existing editor destination |

The browser uses source numeric duration/maximum values, not older unused
formatted-string fixtures. The planned graph starts at the existing 25 C
illustrative point, accumulates the same stage durations/targets, and retains
the existing time/temperature headroom calculation. It adds no measured trace,
live run evidence or new estimate. Energy remains explicitly **Est. energy**.
Graph style changes do not alter the active Running view or trajectory semantics.

Browser favourite state is temporary local illustration. Rename, Delete,
New program, Add stage, editor and Load actions have explicit preview boundaries
where their existing native destination is outside these three proposed pages.
They do not silently perform operations, introduce a new stage-detail route,
turn Load into Start, or redesign the editor. The native action lifecycle,
favourite capacity, save/discard, controller validation and persistence remain
unchanged and must be preserved in a native port.

Device/Settings navigation and dirty Settings guards still work. Program
library full/partial/empty fixtures are external review controls, not new worker
settings. Device data fixtures likewise do not claim real controller state.

## Review judgment

**Recommendation:** Adopt these small layout/contrast changes if the browser
review is preferred, retaining the existing screen structures. Do not add
search, sorting, more program fields, new actions, new authoring policy, or a
different stage workflow as part of this pass. Those have not been requested.

The [implementation plan](../09-progress/PROGRAMS_STYLE_PLAN.md) records the
browser evidence and the scope of a later native change.


## Browser captures

- [Programs at 800 x 480](../images/theme/programs-concept-800x480.png)
- [Program details at 800 x 480](../images/theme/program-detail-concept-800x480.png)
- [Program stages at 800 x 480](../images/theme/program-stages-concept-800x480.png)

These are actual browser renders of the proposal, not native LVGL evidence.


## 2026-09-15 browser interaction extension

The earlier editor/Load preview boundaries in this styling record are now
superseded in the HTML by the [Home/Running/editor demo](../09-progress/HOME_RUNNING_DEMO_PLAN.md).
Stage edits update local browser copies and Load can start a synthetic demo;
this does not change native controller ownership or production semantics.
