# Project status

Last updated: 2026-09-04

## Current gate

The repository foundation and first desktop-visible UI vertical slice are
complete in the local worktree. The current host-only UI-lab follow-up is
implemented and headlessly verified: Home is a furnace overview with local
favourites/recent quick launch, Programs has two full six-card library pages
plus loading/overview/stage/editor routes, and a dedicated Running page has
graph/split/visual modes with ordinary navigation contained. The latest visual
pass tightens the Program and Stage routes, color-codes stage kinds, keeps the
running graph's complete predicted domain until live data leaves its headroom,
and gives the continuous bottom-navigation control an owner-only active-tab
glow. HEATING, HOLD,
and COOL are presentation/draft assistance only. The lab starts fail-closed in
`UNAVAILABLE`, withholds operational values and controls until current state
exists, and preserves the Control CPU authority boundary. Lab edits and action
buttons are local previews only; furnace business logic, production protocol
semantics, controller state machines, and operational screens remain absent.

## Foundation evidence

| Area | State | Evidence |
| --- | --- | --- |
| Documentation vault and governance | Complete | [Index](../INDEX.md), root `AGENTS.md`, root `PLANS.md` |
| Requirements and architecture | Baseline complete; architecture findings fixed | [Requirements](../00-project/REQUIREMENTS.md), [ADRs](../07-decisions/README.md), [review policy](../06-testing/REVIEW_POLICY.md) |
| Agent skills/reviewers | Three skills and six read-focused reviewer configurations validate | Repository `.agents/` and `.codex/` configuration |
| Zephyr 4.4.0 environment | Exact pin and project-local tools verified | [Development environment](../05-platform/DEVELOPMENT_ENVIRONMENT.md), root `west.yml` |
| Minimal Zephyr proof | `qemu_x86` build and compilation database passed | Foundation application under `app/zephyr/` |
| Desktop LVGL/SDL2 UI | Visible fail-closed availability screen, retained pointer/text diagnostic, and MSVC source-debug path passed | [Simulator strategy](../05-platform/DESKTOP_SIMULATOR_STRATEGY.md), `simulator/desktop/` |
| Desktop operator workflow | One Python command configures, builds, tests, and opens the exact MSVC desktop artifact; check-only mode is available | [Developer runbook](../05-platform/DEVELOPER_RUNBOOK.md), `tools/run_desktop_ui.py` |
| Host-only semantic UI laboratory | Separate MSVC preset and launcher render deterministic disconnected/idle/running/Manual/paused/fault/stale/overrun scenarios; the no-scroll explorer has local favourites/recent quick launch, two-page Program library/overview/stages/Program-editor previews, a dedicated navigation-contained Running view, a solid planned/solid measured full-domain graph, and a service-warning preview; simulator code is guarded from Zephyr | [UI-lab plan](UI_LAB_VERTICAL_SLICE_PLAN.md), `tools/run_ui_lab.py`, `simulator/ui_lab/` |
| First simulated dashboard | Bounded session/revision observation model, provenance/validity fields, planned/measured graph, controller-reported service gate, and host-only presentation | `app/hmi/model/`, `app/hmi/presentation/dashboard/`, `simulator/semantic_controller/` |
| First reusable presentation | `UNAVAILABLE`/`STALE`/`CURRENT` mapping, invalid-input fallback, non-interactive structure, parent lifetime, and minimum viewport verified | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md), `app/hmi/presentation/state_availability/` |
| Host/Zephyr tests | Current Host Debug 10/10 and UI-lab 9/9. The existing Zephyr `qemu_x86` build accepted the updated shared catalog (`ninja: no work to do` after the completed incremental build); prior Twister evidence remains 3/3. | [Test strategy](../06-testing/TEST_STRATEGY.md) |
| Static analysis/debug | Zephyr GCC analysis completed; SDK GDB and desktop `cppvsdbg` stopped in project source | [Environment verification record](../05-platform/DEVELOPMENT_ENVIRONMENT.md#verification-record--2026-08-25) |
| Deterministic indexer V1 | Schema, tooling regression cases, fail-closed input validation, generation, and `--check` passed | [Generated policy](../_generated/README.md), [generated index](../_generated/CODEBASE_INDEX.md) |
| Foundation reviewer gate | Complete; architecture, embedded, indexer, software-critic, error-auditor, and workflow findings resolved | Phase 9 in [roadmap](ROADMAP.md) |
| UI slice reviewer gate | Complete; fail-open default, platform copy, state/link wording, allocation contract, lifetime, viewport, and coverage findings resolved | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md) |

## Accepted baseline

- Control CPU authority and HMI/safety boundary: [ADR-0001](../07-decisions/0001-authority-boundary.md)
- Zephyr 4.4.0 pin: [ADR-0002](../07-decisions/0002-zephyr-4-4-0.md)
- Protocol separation principle: [ADR-0003](../07-decisions/0003-protocol-layering.md)
- Project-owned desktop simulator: [ADR-0004](../07-decisions/0004-desktop-simulator.md)
- Single LVGL execution owner: [ADR-0005](../07-decisions/0005-lvgl-single-owner.md)

## Known limitations

- The unrelated global MinGW GDB is broken because its expected Python runtime
  DLL is absent. It is not part of either verified debugger path: Zephyr uses
  SDK GDB, and the desktop proof uses MSVC PDBs with `cppvsdbg`.
- Python 3.13.3 passed the local Zephyr workflow, but upstream recommends Python
  3.12 specifically for Windows; new Windows setups should prefer 3.12.
- The proof uses `qemu_x86`; it provides no evidence for the still-unselected
  production MCU, board, display, touch device, or resource budgets.
- The reusable availability component is currently exercised by the desktop
  target. Zephyr builds the shared foundation/string catalog, but embedded LVGL
  display wiring is intentionally deferred until a display target is selected.
- The host-only UI laboratory is a visual and semantic HMI test environment;
  its synthetic controller cannot validate Control CPU safety, thermal physics,
  or production protocol behavior. Its program edits, service acknowledgement,
  and run controls are request previews, not persistent/controller behavior.
- The current UI-lab graph and stage values are illustrative. Exact
  HEATING/HOLD/COOL execution limits, rounding, and controller acceptance
  behavior remain open.
- The one-command UI runner is intentionally Windows/MSVC-only and does not
  imply a Zephyr build, an embedded graphical target, or visual correctness.
- Two GCC analyzer warnings remain in upstream Zephyr libc `malloc.c`; no
  analyzer warning was emitted from repository-owned sources.

## Scope guard

No foundation or availability proof should be interpreted as furnace behavior.
Questions are resolved at the decision gate that needs them: early presentation
work may continue with reversible simulated data, while operational controls,
protocol behavior, safety claims, and target integration wait for their
requirements.

See [open questions](OPEN_QUESTIONS.md), [session log](SESSION_LOG.md), and
[traceability](../06-testing/TRACEABILITY.md).
