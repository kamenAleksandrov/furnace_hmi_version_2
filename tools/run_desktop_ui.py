#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build, test, and launch the verified Windows desktop UI workflow."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
from typing import Sequence


REPOSITORY = Path(__file__).resolve().parents[1]
PRESET = "host-msvc-desktop-debug"
DESKTOP_EXECUTABLE = (
    REPOSITORY
    / "build"
    / PRESET
    / "simulator"
    / "desktop"
    / "Debug"
    / "furnace_hmi_desktop.exe"
)


class DesktopUiWorkflowError(RuntimeError):
    """An actionable desktop workflow failure with a process exit code."""

    def __init__(self, message: str, *, exit_code: int = 1) -> None:
        super().__init__(message)
        self.exit_code = exit_code


def command_text(command: Sequence[str]) -> str:
    """Format a command for readable progress and error output."""
    return subprocess.list2cmdline(list(command))


def emit(message: str, *, stream: object | None = None) -> None:
    """Print progress safely even when the Windows console is not UTF-8."""
    destination = sys.stdout if stream is None else stream
    encoding = getattr(destination, "encoding", None) or "utf-8"
    safe_message = message.encode(encoding, errors="backslashreplace").decode(encoding)
    print(safe_message, file=destination, flush=True)


def verification_commands() -> tuple[tuple[str, ...], ...]:
    """Return the canonical ordered configure, build, and test commands."""
    return (
        ("cmake", "--preset", PRESET),
        ("cmake", "--build", "--preset", PRESET),
        ("ctest", "--preset", PRESET),
    )


def run_command(command: Sequence[str], *, cwd: Path) -> None:
    """Run one command with inherited output and fail-fast semantics."""
    emit(f"[run] ({cwd}) {command_text(command)}")
    try:
        subprocess.run(list(command), cwd=cwd, check=True, shell=False)
    except OSError as error:
        raise DesktopUiWorkflowError(
            f"could not start {command[0]}: {error}; see "
            "docs/05-platform/DEVELOPER_RUNBOOK.md"
        ) from error
    except subprocess.CalledProcessError as error:
        raise DesktopUiWorkflowError(
            f"command failed with exit code {error.returncode}: "
            f"{command_text(command)}",
            exit_code=error.returncode,
        ) from error


def run_ui_process(executable: Path) -> None:
    """Launch the visible UI in the foreground and retain process ownership."""
    environment = os.environ.copy()
    environment["SDL_VIDEODRIVER"] = "windows"
    command = (str(executable),)
    emit(f"[run] ({executable.parent}) {command_text(command)}")
    try:
        process = subprocess.Popen(
            list(command),
            cwd=executable.parent,
            env=environment,
            shell=False,
        )
    except OSError as error:
        raise DesktopUiWorkflowError(
            f"could not start the desktop UI: {error}"
        ) from error

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
        raise DesktopUiWorkflowError(
            f"desktop UI exited with code {return_code}",
            exit_code=return_code,
        )


def execute(
    action: str,
    *,
    executable: Path = DESKTOP_EXECUTABLE,
    platform_name: str = os.name,
) -> None:
    """Execute ``run`` or ``check`` for the desktop UI."""
    if platform_name != "nt":
        raise DesktopUiWorkflowError(
            f"the {PRESET} workflow requires Windows and Visual Studio"
        )

    for command in verification_commands():
        run_command(command, cwd=REPOSITORY)

    if action == "run":
        if not executable.is_file():
            raise DesktopUiWorkflowError(
                "desktop UI executable is missing after a successful build"
            )
        run_ui_process(executable)


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(
        description=(
            "Operate the verified MSVC desktop UI. With no action, configure, "
            "build, test, and open the UI."
        )
    )
    argument_parser.add_argument(
        "action",
        choices=("run", "check"),
        default="run",
        nargs="?",
        help=(
            "run: verify then open (default); check: verify without opening"
        ),
    )
    return argument_parser


def main(argv: Sequence[str] | None = None) -> int:
    options = parser().parse_args(argv)
    try:
        execute(options.action)
    except DesktopUiWorkflowError as error:
        emit(f"ERROR: {error}", stream=sys.stderr)
        return error.exit_code
    except KeyboardInterrupt:
        emit("\nDesktop UI workflow interrupted.", stream=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
