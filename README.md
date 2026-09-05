# Furnace HMI V2

Furnace HMI V2 is the software foundation for a Zephyr- and LVGL-based remote
operator interface. The HMI CPU presents controller-observed state and edits
local drafts or operator intent; the Control CPU (Furnace Controller) remains
authoritative for furnace operation, validation, timing, actuators, faults,
safety, and persisted operational data.

This repository is currently in its engineering-foundation phase. It contains
architecture, requirements, build/test scaffolding, a desktop UI proof, and
tooling. It intentionally contains no furnace business logic or production
protocol implementation.

Start with:

- [Engineering documentation](docs/INDEX.md)
- [Current status](docs/09-progress/STATUS.md)
- [Requirements](docs/00-project/REQUIREMENTS.md)
- [Architecture invariants](docs/01-architecture/BOUNDARIES_AND_INVARIANTS.md)
- [Development environment](docs/05-platform/DEVELOPMENT_ENVIRONMENT.md)
- [Developer command runbook](docs/05-platform/DEVELOPER_RUNBOOK.md)
- [Contributor and agent rules](AGENTS.md)

Zephyr is pinned to 4.4.0. A version change is an architecture decision, not a
routine dependency refresh.

## Foundation quick start

Prepare or verify the isolated Zephyr environment:

```powershell
python tools/bootstrap_zephyr.py prepare
python tools/bootstrap_zephyr.py check
```

Build and test the host LVGL/SDL proof:

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py --prepare-cmake-file-api
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
```

On Windows, the verified source-debug path uses the project-local SDL2 VC
package documented in the environment guide:

```powershell
python tools\run_desktop_ui.py
```

That command configures, builds, tests, and opens the desktop UI. The equivalent
individual commands are:

```powershell
cmake --preset host-msvc-desktop-debug
cmake --build --preset host-msvc-desktop-debug
ctest --preset host-msvc-desktop-debug
```

Then select `Desktop MSVC: debug UI` in VS Code and press F5.

For experimental screens and synthetic furnace observations, use the separate
host-only UI laboratory. It cannot be included in a Zephyr image:

```powershell
python tools\run_ui_lab.py
python tools\run_ui_lab.py --scenario fault
python tools\run_ui_lab.py --scenario running-overrun
python tools\run_ui_lab.py check --scenario running-normal
```

The lab window is marked `SIMULATION - NOT DEVICE DATA`; keys `1` through `8`
switch scenarios while it is open. See the [UI-lab vertical-slice plan](docs/09-progress/UI_LAB_VERTICAL_SLICE_PLAN.md)
and [developer runbook](docs/05-platform/DEVELOPER_RUNBOOK.md). For the full
non-graphical verification sequence, run `python tools\run_foundation.py all`.
for its boundary and current non-goals.

Build and test Zephyr through the project tasks in `.vscode/tasks.json`, or
use the exact environment and west commands in the
[development-environment guide](docs/05-platform/DEVELOPMENT_ENVIRONMENT.md).
After both canonical builds, regenerate and verify navigation metadata:

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py
.\.venv\Scripts\python.exe tools\codebase_index.py --check
```
