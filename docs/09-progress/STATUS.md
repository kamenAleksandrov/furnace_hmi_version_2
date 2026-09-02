# Project status

Last updated: 2026-09-01

## Current gate

The repository foundation and first desktop-visible UI vertical slice are
complete in the local worktree. The UI starts fail-closed in `UNAVAILABLE`,
withholds furnace values and controls, and preserves the Control CPU authority
boundary. Confirmed UI review findings were fixed and the final architecture
recheck found no remaining blocker. Furnace business logic, production protocol
semantics, controller state machines, and operational screens remain
intentionally absent.

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
| First reusable presentation | `UNAVAILABLE`/`STALE`/`CURRENT` mapping, invalid-input fallback, non-interactive structure, parent lifetime, and minimum viewport verified | [UI slice plan](UI_VERTICAL_SLICE_PLAN.md), `app/hmi/presentation/state_availability/` |
| Host/Zephyr tests | Host Debug/Release/MSVC desktop CTest 7/7, MSVC portable CTest 5/5, Zephyr Ztest 2/2 | [Test strategy](../06-testing/TEST_STRATEGY.md) |
| Static analysis/debug | Zephyr GCC analysis completed; SDK GDB and desktop `cppvsdbg` stopped in project source | [Environment verification record](../05-platform/DEVELOPMENT_ENVIRONMENT.md#verification-record--2026-08-25) |
| Deterministic indexer V1 | Schema, 30 tooling regression cases, fail-closed input validation, generation, and `--check` passed | [Generated policy](../_generated/README.md), [generated index](../_generated/CODEBASE_INDEX.md) |
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
