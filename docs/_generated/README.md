# Generated documentation

Files in this directory, except this policy, are tool-owned. Do not edit them
manually.

Each generated file must identify its generator and schema/generator version.
Generation must be deterministic for the same tracked inputs, use
repository-relative paths, exclude transient artifacts and secrets, and avoid
machine-specific absolute paths. Run the owning tool and review the resulting
diff whenever source, targets, tests, or documentation relationships change.

`tools/codebase_index.py` uses `build/host-debug` and
`build/zephyr-qemu_x86` as its canonical build inputs. It never searches the
whole build tree, so scratch or standalone builds cannot silently change the
committed index. Pass one or more `--build-dir` arguments to replace those
canonical build directories for an intentional alternate report;
`--compile-commands` and `--cmake-reply` add individual explicit inputs.

Prepare the codemodel-v2 query before the first configuration of a clean build
tree, then explicitly configure both canonical trees and generate the index.
The Zephyr build must already exist; run the canonical build in the developer
runbook before this block.

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

The complete ordered command sequence is also maintained in the
[developer runbook](../05-platform/DEVELOPER_RUNBOOK.md).

The preparation command covers both canonical build directories and is safe to
repeat. It writes queries but cannot create replies by itself. The root CMake
project preserves the query on later configurations. A pristine Zephyr build
removes its query and reply, so repeat the preparation and explicit Zephyr
configuration after any such build.
The generator records missing, invalid, or failed inputs as diagnostics rather
than substituting metadata from another build directory. Generation and
`--check` fail closed when required build inputs are missing or invalid. Pass
`--allow-degraded` only when intentionally producing a navigation-only report;
that opt-in never turns degraded metadata into architecture evidence.

Generated indexes are navigation aids, not substitutes for authoritative
requirements or ADRs.
