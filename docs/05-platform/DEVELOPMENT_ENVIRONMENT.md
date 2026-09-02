# Development environment

This document is the authoritative setup and verification record for the
foundation toolchain. Paths below are repository-relative; ignored dependency
and build directories are never architectural inputs.

For daily copy/paste build, run, debug, test, analysis, and index commands, use
the [developer runbook](DEVELOPER_RUNBOOK.md).

## Accepted and tested versions

| Component | Foundation value | Notes |
| --- | --- | --- |
| Zephyr | 4.4.0, commit `684c9e8f32e4373a21098559f748f06915f950c9` | Exact pin in root `west.yml`; changes require an ADR |
| Zephyr SDK | 1.0.1 | Version required by Zephyr 4.4.0 `SDK_VERSION` |
| Zephyr GNU x86 toolchain | GCC 14.3.0, GDB 16.2 | Foundation `qemu_x86` proof only; not a production-board selection |
| C standard | C17 | Zephyr and host compile databases contain `-std=c17`; host CMake disables C extensions |
| west | 1.5.0 | Installed in the project virtual environment |
| Python | 3.13.3, project virtual environment | Zephyr requires Python 3.12 or newer; upstream recommends 3.12 specifically on Windows, so 3.13.3 is a tested local deviation rather than the portable recommendation |
| CMake / Ninja | 4.2.1 / 1.13.2 | Root project requires CMake 3.28 or newer |
| Host C compiler | MSYS2 GCC 15.2.0 and MSVC 19.44 | Both build the desktop proof; GCC remains the canonical compiler-database input |
| Visual Studio | 2022 Community 17.14 | Supplies the verified MSVC/PDB native-debug path |
| LVGL | commit `85aa60d18b3d5e5588d7b247abf90198f07c8a63` | Exact revision used by Zephyr 4.4.0 |
| SDL2 | 2.32.10 | Tested with MSYS2 MinGW and the official VC development archive; not a production dependency pin |
| QEMU | 10.0.2 | Supplied by Zephyr SDK 1.0.1 host tools |
| Device Tree Compiler | 1.7.2 | Checksum-verified project-local MSYS2 binary package |
| VS Code | 1.132.0 | Project recommendations and tasks are under `.vscode/` |

All project-owned production and foundation sources are C. The pinned LVGL
desktop CMake integration enables a C++ compiler (and ASM outside MSVC)
internally even though this proof has no project C++ sources; those are host
build prerequisites, not a change to the production-language decision.

The version choices are supported by the
[Zephyr 4.4.0 release](https://github.com/zephyrproject-rtos/zephyr/releases/tag/v4.4.0),
[Zephyr getting-started guide](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html),
and [Zephyr SDK installation guide](https://docs.zephyrproject.org/4.4.0/develop/toolchains/zephyr_sdk.html).

## Local layout

The reproducible Windows foundation uses ignored project-local locations:

| Path | Purpose |
| --- | --- |
| `.venv/` | Python environment containing west and Zephyr requirements |
| `.deps/zephyr-workspace/` | isolated west top directory with a synchronized local manifest mirror |
| `.deps/zephyr-workspace/zephyr/` | exact Zephyr checkout |
| `.deps/zephyr-workspace/modules/lib/gui/lvgl/` | exact LVGL checkout used by clean host builds |
| `.deps/zephyr-sdk-1.0.1/` | SDK, x86 GNU toolchain, QEMU, and debug tools |
| `.deps/tools/dtc-1.7.2/` | Windows DTC and its libyaml runtime |
| `.deps/tools/7zip/` | checksum-verified SDK extractor |
| `.deps/tools/sdl2-msvc-2.32.10/` | official SDL2 VC development package for the MSVC/PDB desktop build |
| `.deps/downloads/` | checksum-verified downloaded archives retained for local reproduction |
| `build/` | disposable build, test, compile-database, and machine-index output |

The repository itself cannot be the local west top directory: its parent
contains unrelated projects and must not receive a shared `.west` directory.
For this checkout, the helper synchronizes root `west.yml` into an ignored
manifest mirror and rejects drift. Root `west.yml` remains the only
authoritative manifest and also supports a conventional separate west
workspace topology for a fresh clone.

## Windows host prerequisites

The verified host path uses a 64-bit MSYS2 MINGW64 toolchain. It is deliberately
not presented as a byte-for-byte production toolchain pin; the exact tested
versions are recorded above, and clean configure/build/test is the acceptance
gate. On a new Windows machine, install MSYS2, open its MINGW64 shell, and
install the required host packages:

```bash
pacman -Syu
pacman -S --needed git mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-SDL2 mingw-w64-x86_64-pkgconf
```

Add that MINGW64 `bin` directory to the development shell `PATH`. Install
64-bit Python 3.12 from the official Python distribution for the portable
Windows baseline; Python 3.13.3 remains the verified local deviation. The
second verified host path uses Visual Studio 2022 with the Desktop development
with C++ workload. It builds the same LVGL/SDL proof with MSVC and emits PDBs
for the supported Windows source-debug workflow.

The SDK installer also needs project-local 7-Zip. The exact executable used by
this foundation can be provisioned without a global installation:

```powershell
$zephyr7zipDir = ".deps\tools\7zip"
New-Item -ItemType Directory -Force $zephyr7zipDir
Invoke-WebRequest `
  https://github.com/ip7z/7zip/releases/download/26.02/7zr.exe `
  -OutFile "$zephyr7zipDir\7zr.exe"
$zephyr7zipHash = (Get-FileHash "$zephyr7zipDir\7zr.exe" -Algorithm SHA256).Hash
if ($zephyr7zipHash -ne "56B8CC9F4971CEF253644FAFE54063ED7FDCA551D4DEE0F8C6BAA81B855ACD72") {
  throw "Unexpected 7zr.exe hash: $zephyr7zipHash"
}
Copy-Item "$zephyr7zipDir\7zr.exe" "$zephyr7zipDir\7z.exe"
$env:PATH = "$(Resolve-Path $zephyr7zipDir);$env:PATH"
```

The MSVC desktop preset uses SDL's official VC development archive in an
ignored project-local directory. Provision it as follows:

```powershell
$desktopSdlDownloadDir = ".deps\downloads"
$desktopSdlArchive = "$desktopSdlDownloadDir\SDL2-devel-2.32.10-VC.zip"
$desktopSdlInstallDir = ".deps\tools\sdl2-msvc-2.32.10"
New-Item -ItemType Directory -Force $desktopSdlDownloadDir, $desktopSdlInstallDir
Invoke-WebRequest `
  https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-VC.zip `
  -OutFile $desktopSdlArchive
$desktopSdlHash = (Get-FileHash $desktopSdlArchive -Algorithm SHA256).Hash
if ($desktopSdlHash -ne "AF347939395A58B365846AAEA27391E69F9EC9D4DD650D6AC40802159B418A6E") {
  throw "Unexpected SDL2 VC archive hash: $desktopSdlHash"
}
Expand-Archive -LiteralPath $desktopSdlArchive -DestinationPath $desktopSdlInstallDir
```

The resulting `SDL2-2.32.10/cmake` directory is selected explicitly by the
`host-msvc-desktop-debug` preset; it is not discovered from an ambient package
registry.

`tools/bootstrap_zephyr.py check` verifies Git, CMake 3.28 or newer, Ninja, a
C/C++ compiler pair, SDL2 2.32.10 or newer, and the exact project-local 7-Zip
hash before certifying the complete environment. The subsequent clean CMake
configure remains the definitive compiler/SDL integration check.

## Prepare or verify the environment

From a checkout with Git, CMake, Ninja, Python, and SDL2 available:

```powershell
python tools/bootstrap_zephyr.py prepare
python tools/bootstrap_zephyr.py check
```

`prepare` creates/uses the project virtual environment, synchronizes the
manifest, initializes the local west workspace, updates the exact Zephyr
and LVGL projects needed by the foundation, installs Zephyr's Python
requirements, and then validates. Add
`--all-modules` only when a selected target or feature needs the full imported
module set. `check` is read-only and performs no downloads.

Both commands reject modified or untracked files in the Zephyr and LVGL
checkouts. A pinned `HEAD` with local source changes is not a reproducible
dependency and is never reported as consistent.

On a clean machine, the first `prepare` run establishes the Python and west
workspace before checking binary tools. It may therefore finish with an
actionable missing-SDK or missing-DTC error. Provision those two project-local
dependencies as described below, then rerun `prepare` and `check`. This order
avoids requiring a west workspace before the helper can create one.

The SDK is installed explicitly, not selected from an ambient ESP-IDF or
system toolchain. From `.deps/zephyr-workspace`, the equivalent official
command is:

```powershell
..\..\.venv\Scripts\python.exe -m west sdk install `
  --version 1.0.1 `
  --install-dir ..\zephyr-sdk-1.0.1 `
  --gnu-toolchains x86_64-zephyr-elf
```

On Windows this SDK installation needs the working 7-Zip executable prepared
above. The local foundation used official 7-Zip 26.02 standalone archive
support with the recorded SHA-256.
SDK setup registered its project-local path in the current user's CMake
package registry. This is the only lasting machine/user-level change made by
the foundation setup.

Zephyr SDK 1.0.1 supplies QEMU on Windows but its extracted host tools did not
provide `dtc.exe`. The tested DTC was assembled project-locally from these
MSYS2 packages, without modifying the existing global MSYS2 installation:

- `mingw-w64-x86_64-dtc-1.7.2-3-any.pkg.tar.zst`, SHA-256
  `4f0c0c38201117dcc458743dacd74c3dd18584049da13d966099e02ca20a8a21`;
- `mingw-w64-x86_64-libyaml-0.2.5-2-any.pkg.tar.zst`, SHA-256
  `81cd4032639f5e6ec93803756f3d203d44a2b98a1fc2094f68470dc18098bd9d`.

The [MSYS2 DTC package record](https://packages.msys2.org/packages/mingw-w64-x86_64-dtc)
is the source of the binary URL and checksum. `tools/bootstrap_zephyr.py check`
reports the exact expected local path when either host tool is absent.

The tested project-local extraction can be reproduced from the repository
root with Windows `tar`; verify both hashes before extraction:

```powershell
$zephyrDownloadDir = ".deps\downloads"
$zephyrDtcDir = ".deps\tools\dtc-1.7.2"
New-Item -ItemType Directory -Force $zephyrDownloadDir, $zephyrDtcDir
Invoke-WebRequest `
  https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-dtc-1.7.2-3-any.pkg.tar.zst `
  -OutFile "$zephyrDownloadDir\dtc.pkg.tar.zst"
Invoke-WebRequest `
  https://mirror.msys2.org/mingw/mingw64/mingw-w64-x86_64-libyaml-0.2.5-2-any.pkg.tar.zst `
  -OutFile "$zephyrDownloadDir\libyaml.pkg.tar.zst"
Get-FileHash -Algorithm SHA256 "$zephyrDownloadDir\dtc.pkg.tar.zst"
Get-FileHash -Algorithm SHA256 "$zephyrDownloadDir\libyaml.pkg.tar.zst"
tar -xf "$zephyrDownloadDir\dtc.pkg.tar.zst" -C $zephyrDtcDir
tar -xf "$zephyrDownloadDir\libyaml.pkg.tar.zst" -C $zephyrDtcDir
```

Compare the two reported hashes with the accepted values above; do not extract
or execute packages whose hashes differ.

## Command-line workflow

Prepare CMake metadata before the first clean configuration, then build and
test the host foundation:

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py --prepare-cmake-file-api
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
```

Run the visible desktop proof directly:

```powershell
.\build\host-debug\simulator\desktop\furnace_hmi_desktop.exe
```

Build and test the MSVC/PDB desktop path used by the Windows source debugger:

```powershell
python tools\run_desktop_ui.py
```

The wrapper configures, builds, tests, and opens the verified executable. Use
`python tools\run_desktop_ui.py check` to stop after tests. Its underlying
commands are:

```powershell
cmake --preset host-msvc-desktop-debug
cmake --build --preset host-msvc-desktop-debug
ctest --preset host-msvc-desktop-debug
```

For Zephyr commands, use the VS Code tasks or set the project-local environment
explicitly and run west from its actual top directory:

```powershell
$repoRoot = (Resolve-Path .).Path
$env:ZEPHYR_SDK_INSTALL_DIR = "$repoRoot\.deps\zephyr-sdk-1.0.1"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:QEMU_BIN_PATH = "$repoRoot\.deps\zephyr-sdk-1.0.1\hosttools\qemu"
$env:PATH = "$repoRoot\.deps\tools\dtc-1.7.2\mingw64\bin;$env:PATH"
Push-Location "$repoRoot\.deps\zephyr-workspace"
& "$repoRoot\.venv\Scripts\python.exe" -m west build --pristine=auto `
  --board qemu_x86 --build-dir "$repoRoot\build\zephyr-qemu_x86" `
  "$repoRoot\app\zephyr" -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
& "$repoRoot\.venv\Scripts\python.exe" -m west twister `
  --testsuite-root "$repoRoot\tests\zephyr\foundation" --platform qemu_x86 `
  --outdir "$repoRoot\build\twister" --clobber-output --inline-logs
Pop-Location
```

`QEMU_BIN_PATH` is mandatory for Twister to execute QEMU tests on Windows;
having QEMU only on `PATH` makes the configuration build-only.

Generate and verify the deterministic navigation index after canonical host
and Zephyr builds:

```powershell
.\.venv\Scripts\python.exe tools\codebase_index.py
.\.venv\Scripts\python.exe tools\codebase_index.py --check
```

## VS Code and debugging

Recommended extensions are declared in `.vscode/extensions.json`. The current
strategy uses Microsoft C/C++ IntelliSense with the host and Zephyr compilation
databases, CMake Tools, and the project virtual environment. Tasks cover host
configure/build/test, the MSVC desktop-debug build, Zephyr
build/run/Twister/GCC analysis, and index generation/checking.

Select `Desktop MSVC: debug UI` and press F5 for the desktop
source-debug path. Its pre-launch task builds the exact MSVC preset above and
`cppvsdbg` launches the resulting PDB-backed executable. The foundation proof
set a live breakpoint at `simulator/desktop/src/main.c:242`, stopped in `main`,
and exposed `argc`, `argv`, `display`, `smoke_test`, and `view` in Locals. An
independent Visual Studio native-engine attach reproduced the same active
source line and locals. The temporary breakpoints were removed afterward.

The Zephyr debug launch uses SDK GDB against the generated `qemu_x86`
debugserver. The foundation proof attached GDB 16.2, set a source breakpoint,
and stopped at `app/zephyr/src/main.c:main`. The unrelated global MSYS2 GDB is
not used; it currently fails to start because that installation contains a GDB
linked to a missing `libpython3.13.dll`.

There is no invented production-board debug configuration. Add one only after
a board, probe, runner, and transport are selected and verified.

## Verification record — 2026-08-25

- Host Debug and Release configure/build passed; each CTest preset passed 5/5.
- The SDL smoke test exercised display creation plus SDL pointer and text input
  delivery into LVGL widgets under the dummy video driver.
- A native desktop window opened with the expected title and nonzero Windows
  window handle, then closed.
- Portable-only MSVC Debug passed CTest 4/4. The complete MSVC LVGL/SDL
  desktop preset also configured, built, and passed CTest 5/5.
- Zephyr `qemu_x86` built 135 steps and generated
  `build/zephyr-qemu_x86/compile_commands.json` with `-std=c17`.
- Twister executed one QEMU configuration; both Ztest cases passed.
- Zephyr's GCC analyzer build completed. It emitted two analyzer warnings in
  upstream Zephyr `lib/libc/common/source/stdlib/malloc.c`; it emitted none in
  repository-owned sources. These warnings are recorded rather than presented
  as project fixes.
- The SDK debugger hit `main` through the generated QEMU GDB server.
- VS Code `cppvsdbg` stopped the MSVC/PDB desktop proof at
  `simulator/desktop/src/main.c:242` and displayed source locals. Visual Studio's
  native debugger independently reproduced the stop.
- Indexer unit tests passed 15/15 with the project environment, schema
  validation passed, and deterministic regeneration/check passed before the
  final documentation refresh.
- Bootstrap regression tests passed 5/5, including actionable filesystem
  errors, clean dependency worktrees, early rejection of altered Zephyr/LVGL
  manifest pins, the fresh-clone workspace-before-toolchain invariant, and
  ordered Zephyr/LVGL preparation.

The simulator is a functional host proof, not an RTOS timing simulator. The
first production board, resource budgets, hardware debug probe, display/touch
drivers, and target-specific Zephyr modules remain open.

## UI slice verification — 2026-08-27

- Host Debug, Release, and MSVC desktop builds passed 6/6 CTest cases each;
  portable-only MSVC passed 4/4.
- The default MSVC executable opened the fail-closed state-availability screen
  and a fresh window capture was reviewed. The development viewport was
  800 x 480; the semantic smoke test also verified all object bounds at
  640 x 360.
- The presentation smoke exercised `UNAVAILABLE`, `STALE`, and `CURRENT`, plus
  zero, NULL, negative, and upper-bound invalid input. It also checked
  non-interactive object structure, repeated-create rejection, explicit
  destroy, parent-first deletion, address-reuse safety, and handle clearing.
- A clean Zephyr `qemu_x86` build completed 135 steps with the updated shared
  string catalog. Twister ran one QEMU configuration and passed 2/2 Ztest
  cases.
- Zephyr GCC static analysis completed with the same two upstream libc
  `malloc.c` warnings and none in repository-owned sources.
- The complete Python tooling suite passed 20/20 cases, then the canonical host
  and Zephyr File API replies were refreshed and the deterministic index
  generation/check reached a fixed point.
- The availability LVGL component is not yet wired into an embedded display
  application; this verification covers its portable boundary and desktop
  behavior, not production display hardware.
