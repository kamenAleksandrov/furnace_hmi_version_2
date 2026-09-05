#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build, test, and launch the host-only semantic UI laboratory."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
from typing import Sequence


REPOSITORY = Path(__file__).resolve().parents[1]
PRESET = "host-msvc-ui-lab-debug"
EXECUTABLE = (
    REPOSITORY
    / "build"
    / PRESET
    / "simulator"
    / "ui_lab"
    / "Debug"
    / "furnace_hmi_ui_lab.exe"
)
SCENARIOS = (
    "disconnected",
    "idle",
    "running-normal",
    "manual",
    "paused",
    "fault",
    "stale",
    "running-overrun",
)


class UiLabWorkflowError(RuntimeError):
    """An actionable UI-lab failure with a process exit code."""

    def __init__(self, message: str, *, exit_code: int = 1) -> None:
        super().__init__(message)
        self.exit_code = exit_code


def command_text(command: Sequence[str]) -> str:
    return subprocess.list2cmdline(list(command))


def emit(message: str, *, stream: object | None = None) -> None:
    destination = sys.stdout if stream is None else stream
    encoding = getattr(destination, "encoding", None) or "utf-8"
    safe_message = message.encode(encoding, errors="backslashreplace").decode(encoding)
    print(safe_message, file=destination, flush=True)


def verification_commands() -> tuple[tuple[str, ...], ...]:
    return (
        ("cmake", "--preset", PRESET),
        ("cmake", "--build", "--preset", PRESET),
        ("ctest", "--preset", PRESET),
    )


def run_command(command: Sequence[str], *, cwd: Path) -> None:
    emit(f"[run] ({cwd}) {command_text(command)}")
    try:
        subprocess.run(list(command), cwd=cwd, check=True, shell=False)
    except OSError as error:
        raise UiLabWorkflowError(f"could not start {command[0]}: {error}") from error
    except subprocess.CalledProcessError as error:
        raise UiLabWorkflowError(
            f"command failed with exit code {error.returncode}: {command_text(command)}",
            exit_code=error.returncode,
        ) from error


def run_ui_process(executable: Path, scenario: str) -> None:
    environment = os.environ.copy()
    environment["SDL_VIDEODRIVER"] = "windows"
    command = (str(executable), "--scenario", scenario)
    emit(f"[run] ({executable.parent}) {command_text(command)}")
    try:
        process = subprocess.Popen(
            list(command),
            cwd=executable.parent,
            env=environment,
            shell=False,
        )
    except OSError as error:
        raise UiLabWorkflowError(f"could not start the UI laboratory: {error}") from error
    try:
        return_code = process.wait()
    except KeyboardInterrupt:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        raise
    if return_code != 0:
        raise UiLabWorkflowError(
            f"UI laboratory exited with code {return_code}",
            exit_code=return_code,
        )


def execute(
    action: str,
    *,
    scenario: str,
    executable: Path = EXECUTABLE,
    platform_name: str = os.name,
) -> None:
    if platform_name != "nt":
        raise UiLabWorkflowError(
            f"the {PRESET} workflow requires Windows and Visual Studio"
        )
    if scenario not in SCENARIOS:
        raise UiLabWorkflowError(f"unknown scenario: {scenario}")
    for command in verification_commands():
        run_command(command, cwd=REPOSITORY)
    if action == "run":
        if not executable.is_file():
            raise UiLabWorkflowError("UI laboratory executable is missing after a successful build")
        run_ui_process(executable, scenario)


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(
        description=(
            "Build and run the host-only semantic UI laboratory. The selected "
            "scenario is synthetic and never included in a Zephyr image."
        )
    )
    argument_parser.add_argument(
        "action",
        choices=("run", "check"),
        default="run",
        nargs="?",
        help="run: verify then open (default); check: verify without opening",
    )
    argument_parser.add_argument("--scenario", choices=SCENARIOS, default="running-normal")
    return argument_parser


def main(argv: Sequence[str] | None = None) -> int:
    options = parser().parse_args(argv)
    try:
        execute(options.action, scenario=options.scenario)
    except UiLabWorkflowError as error:
        emit(f"ERROR: {error}", stream=sys.stderr)
        return error.exit_code
    except KeyboardInterrupt:
        emit("\nUI laboratory workflow interrupted.", stream=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
