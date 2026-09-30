# Programs styling implementation plan

Status: **Host-only native styling complete; operator review pending**, 2026-09-15.

## Outcome and boundaries

Refine Programs, Program details and Program stages according to the
[style proposal](../03-hmi/PROGRAMS_STYLE_PROPOSAL.md). Preserve current content,
action destinations, counts, values, stage identities and controller boundaries.
Do not redesign editing, change program semantics or introduce production I/O.
The proposal lists facts, recommendations and governing requirement/ADR IDs.

## Affected files and ownership

- `docs/images/theme/hmi-web-demo.html`: shared browser navigation,
  presentation and snapshot of existing lab content. Source snapshot is
  illustrative; it is not a second authoritative program catalogue.
- `simulator/ui_lab/src/ui_lab_explorer.c`: native library card styling, toolbar
  composition, detail facts and stage-card layout.
  Preserve independently implemented native Settings/Device work and every
  existing action binding, lifetime rule and controller-observation guard.
- String catalogue only if an existing label cannot be reused; no new domain
  text or fields are part of the proposed scope.
- Existing host capture/geometry/interaction tooling for the later native pass.
  All LVGL manipulation remains on the designated GUI execution context.

## Milestones and acceptance criteria

1. **Review current native evidence.** Inspect all three screens and read their
   fixture/action sources. Done: current native source and automated capture
   evidence were reviewed; structure was retained and refinements were applied
   rather than replacing content.
2. **Browser prototype.** Add library/details/stages routes to the shared HTML,
   preserving values and explicit preview boundaries. Complete when browser
   checks cover both library pages, partial/empty libraries, all initial program
   details/stage grids, navigation and Settings/Device regression at 800 x 480,
   800 x 640, and 1024 x 600, with actual captures visually inspected.
3. **Operator style review.** Review the three captures/interactive pages. A
   presentation preference is not an architecture conflict. Record any requested
   refinements without introducing content changes incidentally.
4. **Native styling port.** Apply the approved layout/contrast changes
   only to the three existing builders. Preserve six-card and nine-stage paging,
   graph-node/editor routing, all control meanings and execution-owner/lifetime
   contracts. Done when native captures and relevant existing UI-lab checks
   pass at the three sizes. Complete for the host-only lab; image-viewer
   inspection remains unavailable in the current environment.
5. **Close native evidence.** Update the native verification record, status,
   session history, links and generated index with verified results. Complete;
   physical touch inspection remains separate from author desktop review.

## Verification and failure cases

- Compare browser names, material, energy, duration, maximum, stage order/kind/
  target/duration, count and graph points against the current native source.
  Source changes later require deliberate snapshot refresh, not invented data.
- First/last/partial/empty libraries; longest names; initial maximum eight-stage
  fixture; nine-stage layout stress without adding it to normal fixture content.
- No clipping, text collision or controls outside content at the three sizes;
  stable bottom navigation; two and three columns retained respectively.
- Correct library-to-details-to-stages/back routes and page restoration;
  card and graph-node editor previews; favourite toggling; delete cancellation;
  Load does not imply Start; renamed/edited real programs remain outside this
  browser concept's scope.
- Dirty Settings Stay/Discard/Save when moving to Programs; return to Device
  and Settings restores their category rail and correct active destination.
- Native port: existing stage-editor save/discard, lifetime/navigation guards,
  graph geometry, current/stale/unavailable handling and program-run containment
  remain in existing verification. No low-priority layout work delays Stop.

## Documentation and rollback

Update the styling proposal/plan, native implementation and navigation/status
records during the browser and native passes. Keep accepted requirements and
ADRs unchanged. Regenerate owned metadata with `tools/codebase_index.py`; run
`--check`, link and whitespace checks. Existing unrelated working-tree edits
must be preserved.

No storage schema or compatibility migration is involved. Revert only this
presentation diff if the style is not adopted; preserve program data, native
action behavior and independent Device/Settings changes.

## Verification record

**Fact:** Chrome browser checks passed 87 Programs views across 800 x 480,
800 x 640 and 1024 x 600: full/partial/empty library pages and all twelve
program details/stage grids. Additional checks passed nine-stage grid stress,
back/page restoration, favourite toggling, graph/card editor previews, Delete
cancellation, the Load-before-Start boundary, dirty Settings navigation and
Device category-rail restoration.

Actual screenshots of all three pages at 800 x 480, the eight-stage fixture,
taller details and wider stages were visually inspected. The first tall capture
revealed stretched SVG labels/markers; the plot now scales while text and
markers retain physical proportions. Fresh verification passed round 10 px
markers at every tested detail viewport, and the corrected tall capture was
visually inspected. The observer is disconnected when the view is rebuilt.

The existing Device/Settings browser review also passed 90 Device views and
its twelve restart assessment/result regression branches after Programs was
added. The final graph-only fix was followed by the full Programs browser
rerun. Review drivers and captures are under `build/programs-concept-review.cjs`,
`build/device-concept-review.cjs` and their matching capture directories.
Representative final Programs screenshots are retained in `docs/images/theme/`.

A usage-limit approval rejection interrupted the first attempt at final graph
verification; the operator requested continuation and the resumed command
passed. No verification success was inferred from the rejected command.

**Native implementation fact:** The three existing native builders now carry
the approved styling pass. Library cards have neutral aligned summaries;
library and stage page indicators share the action toolbar; detail facts use
aligned two-column rows beside a padded graph; Delete is a restrained
red-outline action; and stage cards use outlined ordinal badges plus a
separate inherited-target caption. Existing six-card/nine-stage paging,
full-card targets, graph-node routing, editor routing, Load-before-Start
meaning, fixture values and local preview boundaries were preserved.

Native MSVC UI-lab builds and full CTest presets passed 10/10 at 800 x 480,
800 x 640 and 1024 x 600. The visual-geometry test reported zero errors for
the generated native captures at each size. The local image viewer could not
open the generated BMP files in this environment, so automated geometry and
workflow evidence is recorded; manual native image inspection remains
pending. No firmware build, controller operation or physical-panel
verification was performed.

**Final documentation checks:** Proposal/plan/concept link targets, extracted
JavaScript syntax, owning index generation and `--check`, and `git diff --check`
passed. Git emitted existing CRLF-to-LF notices only.
