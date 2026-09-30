# Home, Running and stage-editor browser plan

Status: browser implementation complete for review, 2026-09-15. User requests the same HTML treatment for Home
and Running plus real routing into stage editing. A later website demo is a
future use, not authorization to publish or connect a furnace now.

## Scope and baseline

Facts: native Home, Running and editor captures and current repository status
were reviewed. Home has favourites/recent quick launch, observed readings and
furnace illustration. Running has Graph/Split/Furnace, timings/readings and
Pause/Resume/Stop. Stage editing retains all-stage Save/Discard and field
applicability for Heating/Hold/Cooling.

Recommendation: extend the existing standalone HTML with these compositions
and a bounded, explicitly synthetic browser interaction model. The shared
simulation label remains visible. No network requests, production command
acceptance, safety behavior, persistence or website deployment is implemented.

Governing records: REQ-FUN-002, 003, 007, 009, 017; REQ-NFR-004, 006;
ARCH-INV-001, 003, 005, 007, 009 and ADR-0001/0005 in the existing requirements,
program/startup model and visual baseline. Native LVGL ownership and immutable
controller execution remain unchanged. This browser model is not controller
semantics and is not an ADR accepting a production scheduler.

## Proposed interaction

- Home opens a program through quick launch or Programs. Empty favourites/recent
  categories are omitted. Current/target/remaining, door and fan retain explicit
  unknown/stale presentation. A code-native SVG supplies the furnace illustration.
- Load reviews the selected program before Start demo. A demo run copies the
  program so later local library changes cannot mutate that run. Running hides
  ordinary navigation while active/paused. Stop remains visible; notifications
  open an in-place read-only notice rather than navigating away.
- Demo elapsed time advances only through an external review control. Pause
  freezes advancement. Completed demonstration can return Home; no furnace
  completion guarantee is inferred. Graph is based on the copied plan, with
  clearly synthetic measured points and a stable full-program domain.
- Stage cards/graph nodes open a bounded local edit session. Previous/Next
  retain unsaved edits. Heating allows target and rate/duration assistance;
  Hold inherits its target; Cooling supports controlled rate or fastest mode.
  Unknown fastest-cooling duration is not zero. Such a plan is reviewable but
  cannot start this simple timed demo until its duration is known.
- Save all replaces the local demo copy and recalculates affected summaries;
  outdated energy estimates become unavailable. Discard restores the pre-edit
  program. Numeric validation rejects invalid inputs and inconsistent stage
  direction. Demo bounds/rounding are illustrative, not controller limits.
  Settings preference-policy integration, New/Add authoring, rename/delete,
  service authorization and actual controller validation remain separate work.
- Browser reload resets state. Website hosting, mobile layout, persistent demo
  state, URL/history routing and broader authoring can be designed later.

## Implementation sequence and ownership

1. Inspect references and define scope (done).
2. Extend `docs/images/theme/hmi-web-demo.html`: Home, loading, Running,
   editor, navigation guards, local edit/run state, SVG presentation. Preserve
   prior Settings/Device/program layouts and unrelated native work.
3. Verify current/stale/unavailable Home, favourites/recent, all Running views,
   pause/resume/stop/completion, editor field kinds, invalid input, all-stage
   Save/Discard, immutable run copy, and dirty navigation across three sizes.
4. Inspect real browser captures and correct clipping/text/graph defects.
5. Update documentation/status/index and record actual evidence. Native port
   and website deployment remain future milestones.

## Verification and rollback

Use local browser checks and screenshots at 800 x 480, 800 x 640 and 1024 x 600.
Check runtime DOM errors, geometry, editing/lifetime guards, rejected invalid
values, unknown duration, paused advancement, graph endpoint consistency,
late UI callbacks, and return routes. No real timing/safety claims. Run relevant
Device/Settings regression, script syntax, link and generated metadata checks.
No firmware build is needed for an HTML/document-only change.

Rollback affects only browser presentation/model and linked planning records;
no persistent schema, program library or controller state needs migration.

## Results

The same HTML now opens Home, has working quick-launch/Load/Start demo routing,
three Running views with pause/resume/Stop, external bounded time advancement,
and interactive stage editing from stage cards and graph nodes. Edits return
to the originating Programs/Load context. Graph trace construction is capped
at roughly 121 samples even for long edited durations. Numeric typing updates
validation/calculated counterparts without destroying the active input.

Chrome checks passed 39 rendered Home/Load/Running/editor states at 800 x 480,
800 x 640 and 1024 x 600, plus direct numeric typing/Save, all-stage retention,
invalid-input rejection, inherited targets, discard, unknown fastest cooling,
pause freeze, completion, immutable run copy and dirty Settings navigation.
Actual Home, Graph/Split Running and cooling-editor captures were inspected.
The test driver is `build/home-running-demo-review.cjs`; browser captures are
under `build/home-running-demo-review/`. This is browser evidence only.

Representative captures:
- [Home](../images/theme/home-demo-800x480.png)
- [Running](../images/theme/running-demo-800x480.png)
- [Stage editor](../images/theme/stage-editor-demo-800x480.png)

No native code, firmware, real controller connection or website publication
changed. Manufacturer/thermal/run semantics remain unverified synthetic
examples. This timed demo cannot execute an unknown-duration plan. New/Add,
rename/delete and service operations retain their previous preview boundaries.
Navigation uses in-page view state; deployment-grade URL/history routing,
mobile layout, persistence and public-hosting review remain later work.

Final regression/maintenance checks passed: 90 Device views at three sizes,
twelve restart branches, dirty Settings navigation, local artifact links,
extracted JavaScript syntax, owning index generation/check and diff whitespace.
Git reported existing CRLF-to-LF notices only.
