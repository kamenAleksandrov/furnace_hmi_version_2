# Developer runbook

This is the copy/paste command reference for operating the repository. Run all
PowerShell commands from the repository root unless a section says otherwise.
The authoritative prerequisite versions, download checksums, and installation
details remain in [Development environment](DEVELOPMENT_ENVIRONMENT.md).

## Daily desktop UI loop

The MSVC build is the recommended Windows path when you want to see and debug
the UI. It produces PDB debug information and uses the verified Visual Studio
debugger integration.

The canonical one-command workflow is:

```powershell
python tools\run_desktop_ui.py
```

With no argument, the script configures, builds, runs the complete MSVC desktop
CTest preset, and then opens the exact executable that passed. It waits while
the window is open; close the window to finish. Ctrl+C terminates and reaps the
owned UI process.

To perform the same verification without opening a window:

```powershell
python tools\run_desktop_ui.py check
```

The equivalent underlying commands are retained for troubleshooting:

```powershell
cmake --preset host-msvc-desktop-debug
cmake --build --preset host-msvc-desktop-debug
ctest --preset host-msvc-desktop-debug
.\build\host-msvc-desktop-debug\simulator\desktop\Debug\furnace_hmi_desktop.exe
```

The final command opens the interactive LVGL window. CTest uses SDL's headless
driver, so passing tests do not leave a visible window open. The default window
shows the fail-closed state-availability UI at an 800 x 480 development
viewport; the layout is tested down to 640 x 360.

This wrapper verifies only the Windows MSVC desktop preset. It does not run a
Zephyr build, Twister, static analysis, the other host compiler presets, or the
codebase index refresh. Use their dedicated sections below when a change
requires that broader evidence.

For a single command that runs the four host compiler/test presets without
opening a window:

```powershell
python tools\run_foundation.py host
```

## Host-only semantic UI laboratory

The UI laboratory is a separate host executable. It links a deterministic
simulated controller to the shared HMI model and LVGL presentation; its sources
and synthetic data are not part of the Zephyr image. The window always carries
an explicit `SIMULATION - NOT DEVICE DATA` marker. This is the recommended
environment for trying layouts with running programs, graphs, statistics,
faults, stale state, and disconnected state before real hardware exists.

The expanded explorer uses a dark neutral theme and carries a distinct machine
state accent across every page (running amber, paused violet, Manual teal,
fault red). Click/tap the four bottom destinations. Programs use short pages
and Details rather than scrolling; the stage draft editor uses explicit +/-
buttons, never drag gestures. Press `H`, `P`, `D`, or `S` for Home, Programs,
Device, or Settings; press `I` for the synthetic service-warning dialog. All
program data, edits, and confirmations are lab previews. In particular,
acknowledging the service dialog does not contact a controller or waive any
maintenance policy.

Build, test, and open the default running-program scenario:

```powershell
python tools\run_ui_lab.py
```

Select another deterministic scenario:

```powershell
python tools\run_ui_lab.py --scenario disconnected
python tools\run_ui_lab.py --scenario idle
python tools\run_ui_lab.py --scenario manual
python tools\run_ui_lab.py --scenario paused
python tools\run_ui_lab.py --scenario fault
python tools\run_ui_lab.py --scenario stale
python tools\run_ui_lab.py --scenario running-overrun
```

Use `check` to build and test without opening a window:

```powershell
python tools\run_ui_lab.py check --scenario running-normal
```

The same check can be selected through the all-in-one workflow helper:

```powershell
python tools\run_foundation.py ui-check --scenario running-normal
```

While the laboratory is open, keys `1` through `8` switch between the same
scenarios. The first dashboard is intentionally read-only; simulated values
exercise presentation and recovery states, not controller safety or process
control. The current stage values and planned trajectory are illustrative until
HEATING/HOLD/COOL semantics are accepted.

In VS Code, open **Run and Debug**, select
`Desktop MSVC: debug UI`, set a source breakpoint in
`simulator/desktop/src/main.c`, and press **F5**. Its pre-launch task configures
and builds the executable automatically.

## First-time environment preparation

```powershell
python tools/bootstrap_zephyr.py prepare
.\.venv\Scripts\python.exe tools\bootstrap_zephyr.py check
```

On a new Windows host, prefer Python 3.12. The first `prepare` may establish the
virtual environment and west workspace, then stop with an actionable missing
SDK or DTC message. Provision those dependencies using
[Development environment](DEVELOPMENT_ENVIRONMENT.md), then rerun both
commands. Use `prepare --all-modules` only when a selected Zephyr target needs
modules beyond the foundation's pinned Zephyr and LVGL projects.

## Canonical MinGW host build

This is the canonical compiler-database input used by the codebase indexer.
Keep the MSYS2 MINGW64 `bin` directory on `PATH`.

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py --prepare-cmake-file-api
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
.\build\host-debug\simulator\desktop\furnace_hmi_desktop.exe
```

Run the optimized parity build with:

```powershell
cmake --preset host-release
cmake --build --preset host-release
ctest --preset host-release
```

Do not reuse a configured build directory after switching compiler families.
Each preset has its own build directory.

## Other MSVC verification

The portable-core-only preset checks shared C code without building LVGL/SDL:

```powershell
cmake --preset host-msvc-core-debug
cmake --build --preset host-msvc-core-debug
ctest --preset host-msvc-core-debug
```

The complete MSVC desktop preset is shown in the daily loop above.

## Focused host tests

List tests or run one named test from the canonical Debug build:

```powershell
ctest --test-dir build\host-debug -N
ctest --test-dir build\host-debug -R desktop --output-on-failure
ctest --test-dir build\host-debug -R host\.ui_strings_catalog --output-on-failure
```

Run all Python tooling regression tests directly:

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s tools\tests -v
```

The desktop executable also exposes the exact modes used by CTest:

```powershell
.\build\host-debug\simulator\desktop\furnace_hmi_desktop.exe --presentation-smoke-test
.\build\host-debug\simulator\desktop\furnace_hmi_desktop.exe --smoke-test
```

## Zephyr build and run

The simplest supported route is **Terminal > Run Task** in VS Code:

- `zephyr: check local environment`
- `zephyr: build qemu_x86 proof`
- `zephyr: run qemu_x86 proof`
- `zephyr: test foundation on qemu_x86`
- `zephyr: GCC static analysis qemu_x86`

For the command-line equivalent, establish the project-local environment and
run west from the actual west top directory:

```powershell
$repoRoot = (Resolve-Path .).Path
$env:ZEPHYR_SDK_INSTALL_DIR = "$repoRoot\.deps\zephyr-sdk-1.0.1"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:QEMU_BIN_PATH = "$repoRoot\.deps\zephyr-sdk-1.0.1\hosttools\qemu"
$env:PATH = "$repoRoot\.deps\tools\dtc-1.7.2\mingw64\bin;$env:PATH"

Push-Location "$repoRoot\.deps\zephyr-workspace"

& "$repoRoot\.venv\Scripts\python.exe" -m west build --pristine=auto `
  --board qemu_x86 `
  --build-dir "$repoRoot\build\zephyr-qemu_x86" `
  "$repoRoot\app\zephyr" `
  -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

& "$repoRoot\.venv\Scripts\python.exe" -m west build `
  --build-dir "$repoRoot\build\zephyr-qemu_x86" `
  --target run
```

Exit the QEMU console with **Ctrl+A**, then **X**.

Run the Zephyr foundation tests and GCC static analysis from the same pushed
west directory:

```powershell
& "$repoRoot\.venv\Scripts\python.exe" -m west twister `
  --testsuite-root "$repoRoot\tests\zephyr\foundation" `
  --platform qemu_x86 `
  --outdir "$repoRoot\build\twister" `
  --clobber-output `
  --inline-logs

& "$repoRoot\.venv\Scripts\python.exe" -m west build --pristine=always `
  --board qemu_x86 `
  --build-dir "$repoRoot\build\zephyr-qemu_x86-gcc-sca" `
  "$repoRoot\app\zephyr" `
  -- -DZEPHYR_SCA_VARIANT=gcc

Pop-Location
```

`QEMU_BIN_PATH` is required for Twister to execute the QEMU test rather than
only build it. The current GCC analysis has two recorded warnings in upstream
Zephyr libc and none in repository-owned sources.

For Zephyr source debugging, select `Zephyr qemu_x86: break at main` in VS Code
and press **F5**. It builds, starts the generated QEMU debug server, attaches
the SDK GDB, and stops in the real Zephyr application `main`.

## Codebase index

The canonical index requires both `build/host-debug` and
`build/zephyr-qemu_x86` metadata. Release or MSVC builds do not replace them.
Run the canonical host and Zephyr build sections above first. Then prepare the
queries and explicitly reconfigure both build trees; preparing a query alone
does not create a CMake File API reply.

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py --prepare-cmake-file-api
cmake --preset host-debug

$repoRoot = (Resolve-Path .).Path
$env:ZEPHYR_SDK_INSTALL_DIR = "$repoRoot\.deps\zephyr-sdk-1.0.1"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:PATH = "$repoRoot\.deps\tools\dtc-1.7.2\mingw64\bin;$env:PATH"
cmake -S "$repoRoot\app\zephyr" -B "$repoRoot\build\zephyr-qemu_x86"

.\.venv\Scripts\python.exe tools\codebase_index.py
.\.venv\Scripts\python.exe tools\codebase_index.py --check
```

Run this metadata block after any pristine Zephyr build, because pristine mode
removes the query and reply with the rest of that disposable build tree.

Generated outputs are:

- `build/metadata/codebase-index.json`
- `docs/_generated/CODEBASE_INDEX.md`

Never edit the generated Markdown by hand.

## Useful discovery commands

```powershell
cmake --list-presets=all
ctest --test-dir build\host-debug -N
git status --short
```

VS Code task names and launch configurations are versioned in
`.vscode/tasks.json` and `.vscode/launch.json`. If a command here and a task
ever disagree, treat that as documentation drift and fix both in the same
change.

## Full local foundation check

The complete non-graphical sequence can be delegated to one project-local
script. It performs host verification, the UI-lab smoke check, the Zephyr
qemu_x86 build/Twister/GCC analysis, Python tooling tests, index regeneration,
and the generated-index/whitespace checks. It never downloads dependencies or
flashes hardware:

```powershell
python tools\run_foundation.py all
```

The individual actions are useful when iterating:

```powershell
python tools\run_foundation.py host
python tools\run_foundation.py ui-check --scenario fault
python tools\run_foundation.py zephyr
python tools\run_foundation.py tooling
python tools\run_foundation.py ui --scenario running-normal
```

The final `ui` action is the only one that opens a visible window and waits for
you to close it. The script requires the project `.venv` for UI-lab, Zephyr,
and tooling actions; prepare it first with `python tools\bootstrap_zephyr.py
prepare` on a new checkout.

This is the practical pre-review sequence after a cross-cutting change:

```powershell
.\.venv\Scripts\python.exe tools\bootstrap_zephyr.py check

cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug

cmake --preset host-release
cmake --build --preset host-release
ctest --preset host-release

cmake --preset host-msvc-desktop-debug
cmake --build --preset host-msvc-desktop-debug
ctest --preset host-msvc-desktop-debug

.\.venv\Scripts\python.exe -m unittest discover -s tools\tests -v
.\.venv\Scripts\python.exe tools\codebase_index.py --prepare-cmake-file-api
cmake --preset host-debug
cmake -S "$repoRoot\app\zephyr" -B "$repoRoot\build\zephyr-qemu_x86"
.\.venv\Scripts\python.exe tools\codebase_index.py
.\.venv\Scripts\python.exe tools\codebase_index.py --check
git diff --check
git status --short
```

Before the full block, establish `$repoRoot` and the Zephyr environment using
the Zephyr section above, then run its build, Twister, and static-analysis
commands whenever shared target-facing sources or Zephyr configuration change.
The final explicit CMake reconfiguration is required for fresh File API
metadata. Report any check you did not run; do not infer a pass from an old
build directory.
