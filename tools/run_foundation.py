#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run reproducible repository verification workflows.

This helper orchestrates existing CMake, west, CTest, and indexer commands. It
does not download dependencies, flash hardware, or open a graphical window
unless the ``ui`` action is selected explicitly.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
from typing import Sequence


REPOSITORY = Path(__file__).resolve().parents[1]
VENV_PYTHON = REPOSITORY / ".venv" / "Scripts" / "python.exe"
ZEPHYR_WORKSPACE = REPOSITORY / ".deps" / "zephyr-workspace"
SDK_DIR = REPOSITORY / ".deps" / "zephyr-sdk-1.0.1"
DTC_BIN = REPOSITORY / ".deps" / "tools" / "dtc-1.7.2" / "mingw64" / "bin"
CANONICAL_ZEPHYR_BUILD = REPOSITORY / "build" / "zephyr-qemu_x86"


class WorkflowError(RuntimeError):
    """An actionable workflow failure."""

    def __init__(self, message: str, *, exit_code: int = 1) -> None:
        super().__init__(message)
        self.exit_code = exit_code


def emit(message: str, *, stream: object | None = None) -> None:
    destination = sys.stdout if stream is None else stream
    encoding = getattr(destination, "encoding", None) or "utf-8"
    safe_message = message.encode(encoding, errors="backslashreplace").decode(encoding)
    print(safe_message, file=destination, flush=True)


def command_text(command: Sequence[str]) -> str:
    return subprocess.list2cmdline(list(command))


def run(
    command: Sequence[str],
    *,
    cwd: Path = REPOSITORY,
    env: dict[str, str] | None = None,
) -> None:
    emit(f"[run] ({cwd}) {command_text(command)}")
    try:
        subprocess.run(list(command), cwd=cwd, env=env, check=True, shell=False)
    except OSError as error:
        raise WorkflowError(f"could not start {command[0]}: {error}") from error
    except subprocess.CalledProcessError as error:
        raise WorkflowError(
            f"command failed with exit code {error.returncode}: {command_text(command)}",
            exit_code=error.returncode,
        ) from error


def project_python() -> str:
    if not VENV_PYTHON.is_file():
        raise WorkflowError(
            f"project virtual environment is missing: {VENV_PYTHON}. "
            "Run `python tools\\bootstrap_zephyr.py prepare` first."
        )
    return str(VENV_PYTHON)


def zephyr_environment() -> dict[str, str]:
    environment = os.environ.copy()
    environment["ZEPHYR_SDK_INSTALL_DIR"] = str(SDK_DIR)
    environment["ZEPHYR_TOOLCHAIN_VARIANT"] = "zephyr"
    environment["ZEPHYR_BASE"] = str(ZEPHYR_WORKSPACE / "zephyr")
    environment["QEMU_BIN_PATH"] = str(SDK_DIR / "hosttools" / "qemu")
    environment["PATH"] = f"{DTC_BIN}{os.pathsep}{environment.get('PATH', '')}"
    return environment


def host_matrix() -> None:
    for preset in (
        "host-debug",
        "host-release",
        "host-msvc-desktop-debug",
        "host-msvc-core-debug",
    ):
        run(("cmake", "--preset", preset))
        run(("cmake", "--build", "--preset", preset, "--parallel", "4"))
        run(("ctest", "--preset", preset, "--output-on-failure"))


def ui_lab(*, action: str, scenario: str) -> None:
    run((project_python(), "tools\\run_ui_lab.py", action, "--scenario", scenario))


def zephyr_verification() -> None:
    python = project_python()
    environment = zephyr_environment()
    run((python, "tools\\bootstrap_zephyr.py", "check"), env=environment)
    if not ZEPHYR_WORKSPACE.is_dir():
        raise WorkflowError(f"west workspace is missing: {ZEPHYR_WORKSPACE}")
    run(
        (
            python,
            "-m",
            "west",
            "build",
            "--pristine=auto",
            "--board",
            "qemu_x86",
            "--build-dir",
            str(CANONICAL_ZEPHYR_BUILD),
            str(REPOSITORY / "app" / "zephyr"),
            "--",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ),
        cwd=ZEPHYR_WORKSPACE,
        env=environment,
    )
    run(
        (
            python,
            "-m",
            "west",
            "twister",
            "--testsuite-root",
            str(REPOSITORY / "tests" / "zephyr" / "foundation"),
            "--platform",
            "qemu_x86",
            "--outdir",
            str(REPOSITORY / "build" / "twister"),
            "--clobber-output",
            "--inline-logs",
        ),
        cwd=ZEPHYR_WORKSPACE,
        env=environment,
    )
    run(
        (
            python,
            "-m",
            "west",
            "build",
            "--pristine=always",
            "--board",
            "qemu_x86",
            "--build-dir",
            str(REPOSITORY / "build" / "zephyr-qemu_x86-gcc-sca"),
            str(REPOSITORY / "app" / "zephyr"),
            "--",
            "-DZEPHYR_SCA_VARIANT=gcc",
        ),
        cwd=ZEPHYR_WORKSPACE,
        env=environment,
    )


def tooling_and_index() -> None:
    python = project_python()
    run((python, "-m", "unittest", "discover", "-s", "tools\\tests", "-v"))
    run((python, "tools\\codebase_index.py", "--prepare-cmake-file-api"))
    run(("cmake", "--preset", "host-debug"))
    run(
        (
            "cmake",
            "-S",
            str(REPOSITORY / "app" / "zephyr"),
            "-B",
            str(CANONICAL_ZEPHYR_BUILD),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ),
        env=zephyr_environment(),
    )
    run((python, "tools\\codebase_index.py"))
    run((python, "tools\\codebase_index.py", "--check"))
    run(("git", "-c", f"safe.directory={REPOSITORY}", "diff", "--check"))


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(
        description=(
            "Run repository verification. No action downloads dependencies or "
            "flashes hardware; only `ui` opens a graphical window."
        )
    )
    argument_parser.add_argument(
        "action",
        choices=("host", "ui-check", "ui", "zephyr", "tooling", "all"),
        nargs="?",
        default="host",
        help="workflow to run (default: host)",
    )
    argument_parser.add_argument(
        "--scenario",
        choices=("disconnected", "idle", "running-normal", "manual", "paused", "fault", "stale"),
        default="running-normal",
        help="UI-lab scenario for `ui` or `ui-check`",
    )
    return argument_parser


def execute(action: str, *, scenario: str) -> None:
    if action == "host":
        host_matrix()
    elif action == "ui-check":
        ui_lab(action="check", scenario=scenario)
    elif action == "ui":
        ui_lab(action="run", scenario=scenario)
    elif action == "zephyr":
        zephyr_verification()
    elif action == "tooling":
        tooling_and_index()
    else:
        host_matrix()
        ui_lab(action="check", scenario="running-normal")
        zephyr_verification()
        tooling_and_index()


def main(argv: Sequence[str] | None = None) -> int:
    options = parser().parse_args(argv)
    try:
        execute(options.action, scenario=options.scenario)
    except WorkflowError as error:
        emit(f"ERROR: {error}", stream=sys.stderr)
        return error.exit_code
    except KeyboardInterrupt:
        emit("\nWorkflow interrupted.", stream=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
