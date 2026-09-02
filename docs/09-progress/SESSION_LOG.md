# Session log

## 2026-08-25 — repository foundation execution

### Scope and result

Executed the approved engineering-foundation pass while excluding furnace
business logic, real protocol semantics, controller commands/state machines,
and production furnace screens. The implementation exists in the local
worktree and has not been committed by this session.

### Repository and documentation

- Normalized the existing Obsidian vault to `docs/`, preserved shared settings,
  excluded user workspace state, and removed the default welcome document.
- Added root navigation/governance, line-ending/editor policy, ignore policy,
  requirements and invariant registers, V1 lessons, nine ADRs, platform and
  testing strategies, traceability, progress records, and generated-file
  policy.
- Added three narrow repository skills and six read-focused reviewer
  configurations. Added a concise policy that reserves broad adversarial review
  for consequential gates.
- Kept unresolved furnace semantics, physical transport details, serialization,
  memory policy, and protocol sharing decisions explicitly open.

### Build, simulator, tests, and tooling

- Pinned Zephyr 4.4.0 to commit
  `684c9e8f32e4373a21098559f748f06915f950c9` and LVGL to the exact revision
  selected by that release.
- Added a portable C17 compile/link probe, foundation-only Zephyr application,
  Ztest suite, host tests, and project-owned LVGL/SDL2 desktop proof.
- Verified a native desktop window and nonzero native handle. The headless smoke
  path pushes real SDL pointer and text events and asserts their delivery into
  LVGL widgets.
- Added project-local VS Code settings, compilation-database configurations,
  build/test/static-analysis tasks, a real Zephyr QEMU debug launch, and an
  MSVC/PDB desktop `cppvsdbg` launch.
- Added an isolated west/bootstrap verifier and documented exact tool versions,
  paths, package hashes, CLI workflows, and the fresh-clone preparation order.
- Added deterministic Indexer V1 using Git, compiler databases, and CMake File
  API metadata; it emits schema-validated JSON plus generated Markdown without
  timestamps or absolute paths.

### Verification evidence

- Zephyr environment check passed: west 1.5.0, exact Zephyr checkout, SDK 1.0.1,
  x86 GCC 14.3.0, GDB 16.2, SDK QEMU 10.0.2, DTC 1.7.2, and `pip check` clean.
- Fresh Host Debug and Release configure/builds passed with warnings treated as
  errors; both CTest presets passed 5/5.
- Portable MSVC Debug configure/build passed; CTest 4/4 passed.
- The complete MSVC LVGL/SDL desktop preset configured and built against the
  checksum-verified official SDL2 VC 2.32.10 package; CTest 5/5 passed.
- Zephyr `qemu_x86` built 135 steps and emitted a C17 compilation database.
- Twister ran one QEMU configuration and both Ztest cases passed.
- Zephyr GCC analyzer build passed. Its two warnings were confined to upstream
  Zephyr libc `malloc.c`; repository-owned sources emitted none.
- SDK GDB attached to the generated QEMU debug server and stopped at the real
  application `main` source breakpoint.
- VS Code `cppvsdbg` stopped at `simulator/desktop/src/main.c:242` in `main`
  and displayed `argc`, `argv`, `display`, `smoke_test`, and `view`. A separate
  Visual Studio native-engine attach reproduced the active line and locals.
- Indexer tests passed 15/15; schema validation, fail-closed required-input
  behavior, deterministic generation, and `--check` passed.
- Bootstrap reproducibility and error-path regression tests passed 5/5.

Exact reproduction commands and the tested-version table live in the
[development-environment record](../05-platform/DEVELOPMENT_ENVIRONMENT.md).

### Review findings addressed

- Architecture review corrected ambiguous HMI authority wording, separated all
  visible proof strings from screen logic, removed an invented ABI implication,
  restored standalone CMake composition, and aligned traceability.
- Embedded review corrected west task working directories, explicit SDK/QEMU/DTC
  environment propagation, Twister execution, debugger configuration, and
  missing environment documentation.
- Indexer adversarial review removed dependence on arbitrary transient build
  trees, made the File API query reproducible, hardened stale/error reply
  handling and schema constraints, and added deterministic fallback coverage.
- Dependency review made the LVGL checkout pin reject tracked, modified, or
  untracked drift, extended the same clean-checkout invariant to the bootstrap
  for both Zephyr and LVGL, and recorded the tested SDL2 floor.
- Final adversarial findings made Indexer V1 fail closed on missing or invalid
  canonical build metadata, classified tool tests correctly, translated
  bootstrap I/O failures into actionable errors, added host-prerequisite
  preflight coverage, and closed the desktop source-debug requirement.
- The final software-critic, error-auditor, and embedded-workflow audits found
  no remaining blocker. Their two low hardening observations were also closed:
  the manual Twister command now clobbers stale output, and bootstrap rejects
  an altered LVGL manifest pin before it mutates the workspace.

### Machine and user-level changes

- Created ignored project-local `.venv/`, `.deps/`, and `build/` content.
- Downloaded and checksum-verified the official SDL2 VC 2.32.10 development
  archive under `.deps/downloads/` and extracted it under `.deps/tools/` for
  the MSVC/PDB desktop preset.
- Installed no ESP-IDF runtime dependency and made no change to global MSYS2.
- Zephyr SDK setup registered the project-local SDK in the current user's CMake
  package registry; this is the only lasting user-level configuration change.
- A redundant temporary standalone QEMU download was uninstalled and removed
  after the SDK-supplied QEMU was verified; it is recoverable by downloading it
  again and no QEMU process remains.

### Known limitations and decisions

- The global MinGW GDB cannot start because its expected Python runtime DLL is
  absent. It is outside the verified workflows: Zephyr uses SDK GDB and the
  desktop proof uses MSVC PDBs with `cppvsdbg`.
- Python 3.13.3 is a verified local deviation from Zephyr's Windows-specific
  recommendation to use 3.12.
- No consequential architecture decision beyond the user-approved baseline was
  introduced, so `decision-challenger` was not invoked to reopen those mandates.
- No furnace-domain or wire-format choice was inferred from a proof or example.

## 2026-08-27 — first UI vertical slice

### Scope and result

- Added a canonical [developer runbook](../05-platform/DEVELOPER_RUNBOOK.md)
  for setup, visible UI, debugging, host/Zephyr tests, analysis, and index
  maintenance.
- Planned and completed the first reusable presentation slice without adding
  furnace-domain values, commands, protocol behavior, or target drivers.
- The desktop executable now opens a read-only state-availability screen by
  default. The original SDL pointer/text proof remains available only through
  its diagnostic mode.

### Authority and lifecycle behavior

- `UNAVAILABLE` is the zero/default state. NULL and invalid inputs also fail
  closed, withholding furnace values and controls.
- Presentation copy distinguishes observed-state availability from transport
  connectivity. All visible text remains in the English string catalog.
- Desktop-only identity is owned by simulator wiring, while the reusable LVGL
  component contains no SDL or Zephyr APIs.
- The view owns a fixed object tree, rejects recreation until destroy, clears
  its stable handle on parent/root deletion, and cannot delete an unrelated
  object after allocator address reuse.
- Direct constructor NULL results clean partial construction. LVGL-internal
  allocation/style OOM remains a documented fatal assertion policy pending
  the production memory ADR and target budgets.

### Verification and review

- Host Debug, Release, and MSVC desktop CTest passed 6/6; portable MSVC passed
  4/4. The screen was visually captured from the exact MSVC build.
- Clean Zephyr `qemu_x86` build passed 135 steps, Twister passed 2/2 QEMU Ztest
  cases, and GCC analysis emitted no repository-owned warning.
- Table-driven presentation checks cover all valid/fallback states,
  non-interactive structure, lifecycle, and the 640 x 360 minimum viewport.
- Architecture, software, and error reviewers identified and drove fixes for
  fail-open initialization, platform leakage, state/link wording, incomplete
  state coverage, viewport overclaim, allocation contract, and ABA lifetime
  risk. Final architecture recheck found no remaining blocker.
- Progress records, traceability, command policy, and the deterministic codebase
  index were refreshed from the final source/build snapshot.

### Deferred decisions

- Product hardware, production viewport, physical transport, protocol/wire
  format, reconnect/session rules, furnace fields, modes, alarms, navigation,
  and control workflows remain open.
- These questions are intentionally answered just in time. The next UI plan
  needs only a first operator workflow, any available V1 evidence, and a choice
  to retain or replace the temporary 800 x 480 development viewport.

## 2026-09-01 — one-command desktop UI workflow

### Scope and result

- Added `tools/run_desktop_ui.py` so a developer can run one command from any
  working directory to configure, build, test, and open the Windows MSVC
  desktop UI.
- The default `run` action always verifies before launch. The `check` action
  performs the same configure/build/CTest sequence without opening a window.
- No stale-artifact-only launch mode is exposed. The wrapper performs no
  downloads and does not claim to run Zephyr, embedded LVGL, hardware, or
  furnace behavior.

### Failure and process behavior

- Commands use argv lists without a shell, run from the discovered repository
  root, stop at the first failure, and preserve the failing child exit code.
- Interactive launch forces SDL's visible Windows driver, uses the executable
  directory for runtime DLL discovery, waits in the foreground, and terminates
  and reaps the owned process on Ctrl+C.
- Missing tools, missing artifacts, access/spawn errors, non-Windows hosts, and
  nonzero UI exits produce concise actionable errors.

### Verification

- Ten focused wrapper tests cover command order, check/run separation, exact
  exit-code propagation, paths with spaces and non-ASCII text, visible SDL
  environment, generic spawn errors, missing output, non-Windows rejection,
  and interruption ownership.
- The wrapper's real `check` action configured and built the MSVC desktop target
  and passed its complete 7/7 CTest preset.
- Host Debug and Release also passed 7/7, portable MSVC passed 5/5, the full
  Python tooling suite passed 30/30, and deterministic index generation/check
  passed after the final documentation refresh.
- Zephyr was not rerun because no target-facing source, target configuration,
  or accepted Zephyr dependency changed; the previously verified 2/2 QEMU
  Ztest evidence remains current for that scope.
