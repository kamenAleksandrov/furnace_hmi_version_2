#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Prepare or validate the repository-local Zephyr development workspace.

``check`` is read-only and never downloads dependencies. ``prepare`` copies the
tracked manifest into the ignored west workspace, initializes that workspace,
updates the pinned Zephyr project when necessary, installs Zephyr's Python
requirements, and then performs the same validation.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Sequence


ZEPHYR_REVISION = "684c9e8f32e4373a21098559f748f06915f950c9"
LVGL_REVISION = "85aa60d18b3d5e5588d7b247abf90198f07c8a63"
ZEPHYR_SDK_VERSION = "1.0.1"
DTC_VERSION = "1.7.2"
WEST_VERSION = "1.5.0"
SEVEN_ZIP_SHA256 = "56b8cc9f4971cef253644fafe54063ed7fdca551d4dee0f8c6baa81b855acd72"

REPOSITORY = Path(__file__).resolve().parents[1]
WORKSPACE = REPOSITORY / ".deps" / "zephyr-workspace"
MIRROR_MANIFEST = WORKSPACE / "manifest" / "west.yml"
TRACKED_MANIFEST = REPOSITORY / "west.yml"
ZEPHYR = WORKSPACE / "zephyr"
LVGL = WORKSPACE / "modules" / "lib" / "gui" / "lvgl"
SDK = REPOSITORY / ".deps" / f"zephyr-sdk-{ZEPHYR_SDK_VERSION}"
DTC_BIN = REPOSITORY / ".deps" / "tools" / f"dtc-{DTC_VERSION}" / "mingw64" / "bin"
SEVEN_ZIP = REPOSITORY / ".deps" / "tools" / "7zip" / "7z.exe"

if os.name == "nt":
    VENV_PYTHON = REPOSITORY / ".venv" / "Scripts" / "python.exe"
    EXE = ".exe"
else:
    VENV_PYTHON = REPOSITORY / ".venv" / "bin" / "python"
    EXE = ""

GCC = SDK / "gnu" / "x86_64-zephyr-elf" / "bin" / f"x86_64-zephyr-elf-gcc{EXE}"
GDB = SDK / "gnu" / "x86_64-zephyr-elf" / "bin" / f"x86_64-zephyr-elf-gdb{EXE}"
QEMU_I386 = SDK / "hosttools" / "qemu" / f"qemu-system-i386{EXE}"
QEMU_X86_64 = SDK / "hosttools" / "qemu" / f"qemu-system-x86_64{EXE}"
DTC = DTC_BIN / f"dtc{EXE}"


class BootstrapError(RuntimeError):
    """An actionable environment or preparation failure."""


def display_path(path: Path) -> str:
    try:
        return path.relative_to(REPOSITORY).as_posix()
    except ValueError:
        return str(path)


def same_path(first: Path, second: Path) -> bool:
    return os.path.normcase(str(first.resolve())) == os.path.normcase(str(second.resolve()))


def command_text(command: Sequence[str]) -> str:
    return subprocess.list2cmdline(list(command)) if os.name == "nt" else " ".join(command)


def run(
    command: Sequence[str],
    *,
    cwd: Path,
    capture: bool = False,
    git_safe_directory: Path | None = None,
) -> str:
    effective = list(command)
    if git_safe_directory is not None:
        effective[1:1] = ["-c", f"safe.directory={git_safe_directory.resolve()}"]
    print(f"[run] ({display_path(cwd)}) {command_text(effective)}", flush=True)
    try:
        result = subprocess.run(
            effective,
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
        )
    except FileNotFoundError as error:
        raise BootstrapError(f"command not found: {effective[0]}") from error
    except subprocess.CalledProcessError as error:
        detail = (error.stdout or "").strip()
        suffix = f"\n{detail}" if detail else ""
        raise BootstrapError(
            f"command failed with exit code {error.returncode}: {command_text(effective)}{suffix}"
        ) from error
    return (result.stdout or "").strip()


def require_file(path: Path, guidance: str) -> None:
    try:
        is_file = path.is_file()
    except OSError as error:
        raise BootstrapError(
            f"cannot inspect {display_path(path)}: {error}. {guidance}"
        ) from error
    if not is_file:
        raise BootstrapError(f"missing {display_path(path)}. {guidance}")


def checkout_status(path: Path) -> str:
    """Return porcelain status for a dependency checkout, including untracked files."""

    return run(
        [
            "git",
            "-C",
            str(path),
            "status",
            "--porcelain=v1",
            "--untracked-files=all",
        ],
        cwd=REPOSITORY,
        capture=True,
        git_safe_directory=path,
    )


def require_clean_checkout(path: Path, label: str) -> None:
    status = checkout_status(path)
    if status:
        summary = "; ".join(status.splitlines()[:3])
        raise BootstrapError(
            f"{label} checkout at {display_path(path)} has local changes: {summary}. "
            "Restore or intentionally preserve them elsewhere before using this environment."
        )


def manifest_revision() -> str:
    require_file(TRACKED_MANIFEST, "Restore the repository's tracked west.yml file.")
    text = TRACKED_MANIFEST.read_text(encoding="utf-8")

    def require_project_revision(
        project: str,
        label: str,
        expected: str,
        decision: str,
    ) -> str:
        matches = re.findall(
            rf"(?ms)^\s*-\s+name:\s*{re.escape(project)}\s*$.*?"
            r"^\s+revision:\s*([0-9a-fA-F]{40})\s*$",
            text,
        )
        if len(matches) != 1:
            raise BootstrapError(
                f"west.yml must contain exactly one 40-character {label} revision"
            )
        revision = matches[0].lower()
        if revision != expected:
            raise BootstrapError(
                f"west.yml changed the accepted {label} pin. Update {decision} and this "
                "helper together; the helper will not mutate the workspace for an "
                "unreviewed revision."
            )
        return revision

    zephyr_revision = require_project_revision(
        "zephyr", "Zephyr", ZEPHYR_REVISION, "ADR-0002"
    )
    require_project_revision(
        "lvgl", "LVGL", LVGL_REVISION, "the LVGL foundation record"
    )
    return zephyr_revision


def _numeric_version(text: str, label: str) -> tuple[int, ...]:
    match = re.search(r"\b(\d+)\.(\d+)(?:\.(\d+))?", text)
    if match is None:
        raise BootstrapError(f"could not parse {label} version from: {text!r}")
    return tuple(int(part) for part in match.groups(default="0"))


def validate_host_runtime() -> None:
    """Validate the documented Windows host-build prerequisites without writing."""

    guidance = "Install the Windows host prerequisites documented in DEVELOPMENT_ENVIRONMENT.md."
    commands: dict[str, Path] = {}
    for name in ("git", "cmake", "ninja"):
        executable = shutil.which(name)
        if executable is None:
            raise BootstrapError(f"missing host command {name!r}. {guidance}")
        commands[name] = Path(executable)

    cmake_version = run([str(commands["cmake"]), "--version"], cwd=REPOSITORY, capture=True)
    if _numeric_version(cmake_version, "CMake") < (3, 28, 0):
        raise BootstrapError(f"CMake 3.28 or newer is required: {cmake_version.splitlines()[0]}")
    ninja_version = run([str(commands["ninja"]), "--version"], cwd=REPOSITORY, capture=True)
    _numeric_version(ninja_version, "Ninja")
    git_version = run([str(commands["git"]), "--version"], cwd=REPOSITORY, capture=True)
    _numeric_version(git_version, "Git")

    compiler_pair: tuple[str, str] | None = None
    for candidates in (("cc", "c++"), ("gcc", "g++"), ("clang", "clang++"), ("cl", "cl")):
        if all(shutil.which(candidate) for candidate in candidates):
            compiler_pair = candidates
            break
    if compiler_pair is None:
        raise BootstrapError(
            "no usable C/C++ compiler pair was found on PATH. "
            "LVGL's host CMake integration requires both languages. "
            + guidance
        )
    compiler_id = {
        "cl": "MSVC",
        "clang": "Clang",
        "clang-cl": "MSVC",
    }.get(compiler_pair[0], "GNU")

    run(
        [
            str(commands["cmake"]),
            "--find-package",
            "-DNAME=SDL2",
            "-DVERSION=2.32.10",
            f"-DCOMPILER_ID={compiler_id}",
            "-DLANGUAGE=C",
            "-DMODE=EXIST",
        ],
        cwd=REPOSITORY,
        capture=True,
    )

    require_file(
        SEVEN_ZIP,
        "Provision the checksum-verified project-local 7-Zip executable documented in "
        "docs/05-platform/DEVELOPMENT_ENVIRONMENT.md.",
    )
    try:
        seven_zip_hash = hashlib.sha256(SEVEN_ZIP.read_bytes()).hexdigest()
    except OSError as error:
        raise BootstrapError(f"could not read {display_path(SEVEN_ZIP)}: {error}") from error
    if seven_zip_hash != SEVEN_ZIP_SHA256:
        raise BootstrapError(
            f"{display_path(SEVEN_ZIP)} has SHA-256 {seven_zip_hash}, expected "
            f"{SEVEN_ZIP_SHA256}"
        )

    print(f"[ok] Host Git: {git_version.splitlines()[0]}")
    print(f"[ok] Host CMake: {cmake_version.splitlines()[0]}")
    print(f"[ok] Host Ninja: {ninja_version.splitlines()[0]}")
    print(f"[ok] Host compilers on PATH: {compiler_pair[0]}, {compiler_pair[1]}")
    print("[ok] Host SDL2 development package: 2.32.10 or newer")
    print(f"[ok] Project-local 7-Zip SHA-256: {seven_zip_hash}")


def validate_local_runtime() -> None:
    require_file(
        VENV_PYTHON,
        "Create it with `python -m venv .venv`, then install west in that environment.",
    )
    if not same_path(Path(sys.executable), VENV_PYTHON):
        raise BootstrapError(
            f"run this helper with {display_path(VENV_PYTHON)}, not {sys.executable}"
        )

    sdk_guidance = (
        "Provision the pinned project-local SDK as documented in "
        "docs/05-platform/DEVELOPMENT_ENVIRONMENT.md; do not substitute another version."
    )
    require_file(SDK / "sdk_version", sdk_guidance)
    installed_sdk_version = (SDK / "sdk_version").read_text(encoding="utf-8").strip()
    if installed_sdk_version != ZEPHYR_SDK_VERSION:
        raise BootstrapError(
            f"expected Zephyr SDK {ZEPHYR_SDK_VERSION}, found {installed_sdk_version!r} in "
            f"{display_path(SDK / 'sdk_version')}"
        )
    require_file(GCC, sdk_guidance)
    require_file(GDB, sdk_guidance)
    require_file(QEMU_I386, sdk_guidance)
    require_file(QEMU_X86_64, sdk_guidance)

    dtc_guidance = (
        f"Provision the checksum-verified DTC {DTC_VERSION} Windows bundle as documented in "
        "docs/05-platform/DEVELOPMENT_ENVIRONMENT.md."
    )
    require_file(DTC, dtc_guidance)
    if os.name == "nt":
        require_file(DTC_BIN / "libfdt-1.dll", dtc_guidance)
        require_file(DTC_BIN / "libyaml-0-2.dll", dtc_guidance)

    checks = (
        (GCC, ("--version",), "14.3.0", "Zephyr SDK GCC"),
        (GDB, ("--version",), "16.2", "Zephyr SDK GDB"),
        (QEMU_I386, ("--version",), "10.0.2", "Zephyr SDK QEMU"),
        (DTC, ("--version",), DTC_VERSION, "DTC"),
    )
    for executable, arguments, expected, label in checks:
        output = run([str(executable), *arguments], cwd=REPOSITORY, capture=True)
        if expected not in output:
            raise BootstrapError(
                f"{label} at {display_path(executable)} does not report expected version "
                f"{expected}: {output.splitlines()[0] if output else '<no output>'}"
            )
        print(f"[ok] {label}: {output.splitlines()[0]}")

    west_version = run([str(VENV_PYTHON), "-m", "west", "--version"], cwd=REPOSITORY, capture=True)
    if west_version != f"West version: v{WEST_VERSION}":
        raise BootstrapError(
            f"expected west {WEST_VERSION} in the project venv, found {west_version!r}; "
            "run prepare to restore the pinned developer tool"
        )
    print(f"[ok] {west_version}")


def validate_workspace() -> None:
    require_file(
        MIRROR_MANIFEST,
        f"Run `{display_path(VENV_PYTHON)} tools/bootstrap_zephyr.py prepare`.",
    )
    if MIRROR_MANIFEST.read_bytes() != TRACKED_MANIFEST.read_bytes():
        raise BootstrapError(
            f"{display_path(MIRROR_MANIFEST)} differs from tracked west.yml; run prepare to synchronize it"
        )

    west_config = WORKSPACE / ".west" / "config"
    require_file(
        west_config,
        f"Run `{display_path(VENV_PYTHON)} tools/bootstrap_zephyr.py prepare`.",
    )
    config = configparser.ConfigParser()
    config.read(west_config, encoding="utf-8")
    if config.get("manifest", "path", fallback="") != "manifest":
        raise BootstrapError(f"{display_path(west_config)} must set manifest.path = manifest")
    if config.get("manifest", "file", fallback="west.yml") != "west.yml":
        raise BootstrapError(f"{display_path(west_config)} must set manifest.file = west.yml")

    topdir = Path(
        run([str(VENV_PYTHON), "-m", "west", "topdir"], cwd=WORKSPACE, capture=True)
    )
    if not same_path(topdir, WORKSPACE):
        raise BootstrapError(
            f"west topdir is {topdir}, expected project-local {display_path(WORKSPACE)}"
        )
    print(f"[ok] west topdir: {display_path(topdir)}")

    declared_revision = run(
        [str(VENV_PYTHON), "-m", "west", "list", "zephyr", "-f", "{revision}"],
        cwd=WORKSPACE,
        capture=True,
    ).lower()
    if declared_revision != ZEPHYR_REVISION:
        raise BootstrapError(
            f"west resolves Zephyr revision {declared_revision!r}, expected {ZEPHYR_REVISION}"
        )

    declared_lvgl_revision = run(
        [str(VENV_PYTHON), "-m", "west", "list", "lvgl", "-f", "{revision}"],
        cwd=WORKSPACE,
        capture=True,
    ).lower()
    if declared_lvgl_revision != LVGL_REVISION:
        raise BootstrapError(
            f"Zephyr resolves LVGL revision {declared_lvgl_revision!r}, expected {LVGL_REVISION}"
        )

    require_file(
        ZEPHYR / ".git" / "HEAD",
        f"Run `{display_path(VENV_PYTHON)} tools/bootstrap_zephyr.py prepare`.",
    )
    checkout_revision = run(
        ["git", "-C", str(ZEPHYR), "rev-parse", "HEAD"],
        cwd=REPOSITORY,
        capture=True,
        git_safe_directory=ZEPHYR,
    ).lower()
    if checkout_revision != ZEPHYR_REVISION:
        raise BootstrapError(
            f"Zephyr checkout is {checkout_revision}, expected pinned {ZEPHYR_REVISION}; run prepare"
        )
    require_clean_checkout(ZEPHYR, "Zephyr")
    print(f"[ok] Zephyr checkout: {checkout_revision}")

    require_file(
        LVGL / ".git" / "HEAD",
        f"Run `{display_path(VENV_PYTHON)} tools/bootstrap_zephyr.py prepare`.",
    )
    lvgl_checkout_revision = run(
        ["git", "-C", str(LVGL), "rev-parse", "HEAD"],
        cwd=REPOSITORY,
        capture=True,
        git_safe_directory=LVGL,
    ).lower()
    if lvgl_checkout_revision != LVGL_REVISION:
        raise BootstrapError(
            f"LVGL checkout is {lvgl_checkout_revision}, expected pinned {LVGL_REVISION}; run prepare"
        )
    require_clean_checkout(LVGL, "LVGL")
    print(f"[ok] LVGL checkout: {lvgl_checkout_revision}")

    run([str(VENV_PYTHON), "-m", "pip", "check"], cwd=REPOSITORY)
    print("[ok] Python package dependency check")


def synchronize_manifest() -> None:
    MIRROR_MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    content = TRACKED_MANIFEST.read_bytes()
    if MIRROR_MANIFEST.is_file() and MIRROR_MANIFEST.read_bytes() == content:
        print(f"[ok] manifest already synchronized: {display_path(MIRROR_MANIFEST)}")
        return
    temporary = MIRROR_MANIFEST.with_suffix(".yml.tmp")
    temporary.write_bytes(content)
    os.replace(temporary, MIRROR_MANIFEST)
    print(f"[ok] synchronized manifest: {display_path(MIRROR_MANIFEST)}")


def initialize_workspace() -> None:
    config = WORKSPACE / ".west" / "config"
    if config.is_file():
        print(f"[ok] west workspace already initialized: {display_path(WORKSPACE)}")
        return
    WORKSPACE.mkdir(parents=True, exist_ok=True)
    run(
        [str(VENV_PYTHON), "-m", "west", "init", "-l", str(MIRROR_MANIFEST.parent)],
        cwd=WORKSPACE,
    )


def checkout_is_pinned(path: Path, revision: str) -> bool:
    if not (path / ".git" / "HEAD").is_file():
        return False
    try:
        checkout_revision = run(
            ["git", "-C", str(path), "rev-parse", "HEAD"],
            cwd=REPOSITORY,
            capture=True,
            git_safe_directory=path,
        )
        status = checkout_status(path)
    except BootstrapError:
        return False
    return checkout_revision.lower() == revision and not status


def prepare(*, all_modules: bool, skip_python_packages: bool) -> None:
    manifest_revision()
    synchronize_manifest()
    initialize_workspace()

    if all_modules:
        run([str(VENV_PYTHON), "-m", "west", "update"], cwd=WORKSPACE)
    else:
        if checkout_is_pinned(ZEPHYR, ZEPHYR_REVISION):
            print("[ok] pinned Zephyr checkout already present; update download skipped")
        else:
            run([str(VENV_PYTHON), "-m", "west", "update", "zephyr"], cwd=WORKSPACE)

        # The desktop proof is a foundation deliverable, so its exact LVGL
        # dependency is part of the default preparation rather than requiring
        # a full download of every Zephyr module.
        if checkout_is_pinned(LVGL, LVGL_REVISION):
            print("[ok] pinned LVGL checkout already present; update download skipped")
        else:
            run([str(VENV_PYTHON), "-m", "west", "update", "lvgl"], cwd=WORKSPACE)

    if skip_python_packages:
        print("[skip] Zephyr Python package installation")
    else:
        run(
            [str(VENV_PYTHON), "-m", "west", "packages", "pip", "--install"],
            cwd=WORKSPACE,
        )
    validate_workspace()
    # Validate binary tools only after the source/Python workspace exists.  On
    # a fresh checkout this ordering deliberately leaves a usable west tree
    # from which the documented SDK installation command can be run, even when
    # the final check reports that the SDK or DTC still needs provisioning.
    validate_local_runtime()
    validate_host_runtime()


def west_is_pinned() -> bool:
    if not VENV_PYTHON.is_file():
        return False
    try:
        output = run(
            [str(VENV_PYTHON), "-m", "west", "--version"],
            cwd=REPOSITORY,
            capture=True,
        )
    except BootstrapError:
        return False
    return output == f"West version: v{WEST_VERSION}"


def enter_project_venv(mode: str) -> int | None:
    """Create/repair the venv for prepare, then re-execute under it if needed."""
    if same_path(Path(sys.executable), VENV_PYTHON):
        if mode == "prepare" and not west_is_pinned():
            run(
                [str(VENV_PYTHON), "-m", "pip", "install", f"west=={WEST_VERSION}"],
                cwd=REPOSITORY,
            )
        return None

    if mode == "prepare":
        if not VENV_PYTHON.is_file():
            run([sys.executable, "-m", "venv", str(REPOSITORY / ".venv")], cwd=REPOSITORY)
        if not west_is_pinned():
            run(
                [str(VENV_PYTHON), "-m", "pip", "install", f"west=={WEST_VERSION}"],
                cwd=REPOSITORY,
            )
    else:
        require_file(
            VENV_PYTHON,
            "Check mode is read-only; run prepare once to create the project environment.",
        )

    command = [str(VENV_PYTHON), str(Path(__file__).resolve()), *sys.argv[1:]]
    print(f"[run] ({display_path(REPOSITORY)}) {command_text(command)}", flush=True)
    return subprocess.call(command, cwd=REPOSITORY)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="mode", required=True)
    subparsers.add_parser("check", help="validate without writing or downloading")
    prepare_parser = subparsers.add_parser("prepare", help="prepare and then validate")
    prepare_parser.add_argument(
        "--all-modules",
        action="store_true",
        help="update every project imported by Zephyr instead of only the Zephyr repository",
    )
    prepare_parser.add_argument(
        "--skip-python-packages",
        action="store_true",
        help="do not invoke west packages pip --install (useful for an already validated venv)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        manifest_revision()
        reexecuted = enter_project_venv(args.mode)
        if reexecuted is not None:
            return reexecuted
        if args.mode == "prepare":
            prepare(
                all_modules=args.all_modules,
                skip_python_packages=args.skip_python_packages,
            )
        else:
            validate_host_runtime()
            validate_local_runtime()
            validate_workspace()
    except BootstrapError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    except OSError as error:
        print(f"ERROR: filesystem operation failed: {error}", file=sys.stderr)
        return 1
    print(f"[ok] Zephyr {ZEPHYR_REVISION} development environment is consistent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
