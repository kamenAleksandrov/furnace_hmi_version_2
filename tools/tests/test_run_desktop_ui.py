# SPDX-License-Identifier: Apache-2.0
"""Regression tests for the one-command Windows desktop UI workflow."""

from __future__ import annotations

from contextlib import redirect_stderr
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from tools import run_desktop_ui


class DesktopUiWorkflowTests(unittest.TestCase):
    def test_verification_commands_match_the_canonical_preset_sequence(self) -> None:
        self.assertEqual(
            run_desktop_ui.verification_commands(),
            (
                ("cmake", "--preset", "host-msvc-desktop-debug"),
                (
                    "cmake",
                    "--build",
                    "--preset",
                    "host-msvc-desktop-debug",
                ),
                ("ctest", "--preset", "host-msvc-desktop-debug"),
            ),
        )

    def test_check_runs_verification_without_launching(self) -> None:
        observed: list[tuple[tuple[str, ...], Path]] = []

        def observe(command: tuple[str, ...], *, cwd: Path) -> None:
            observed.append((tuple(command), cwd))

        with mock.patch.object(run_desktop_ui, "run_command", side_effect=observe):
            run_desktop_ui.execute("check", platform_name="nt")

        self.assertEqual(
            [command for command, _ in observed],
            list(run_desktop_ui.verification_commands()),
        )
        self.assertTrue(all(cwd == run_desktop_ui.REPOSITORY for _, cwd in observed))

    def test_run_verifies_then_launches_the_built_executable(self) -> None:
        observed: list[tuple[tuple[str, ...], Path]] = []

        def observe(command: tuple[str, ...], *, cwd: Path) -> None:
            observed.append((tuple(command), cwd))

        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = Path(temporary_directory) / "furnace_hmi_desktop.exe"
            executable.touch()
            with (
                mock.patch.object(run_desktop_ui, "run_command", side_effect=observe),
                mock.patch.object(run_desktop_ui, "run_ui_process") as launch,
            ):
                run_desktop_ui.execute(
                    "run",
                    executable=executable,
                    platform_name="nt",
                )

        self.assertEqual(
            [command for command, _ in observed],
            list(run_desktop_ui.verification_commands()),
        )
        launch.assert_called_once_with(executable)

    def test_run_fails_cleanly_when_the_built_executable_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            missing = Path(temporary_directory) / "missing.exe"
            with (
                mock.patch.object(run_desktop_ui, "run_command"),
                self.assertRaisesRegex(
                    run_desktop_ui.DesktopUiWorkflowError,
                    "missing after a successful build",
                ),
            ):
                run_desktop_ui.execute("run", executable=missing, platform_name="nt")

    def test_msvc_workflow_rejects_non_windows_hosts(self) -> None:
        with self.assertRaisesRegex(
            run_desktop_ui.DesktopUiWorkflowError,
            "requires Windows",
        ):
            run_desktop_ui.execute("check", platform_name="posix")

    def test_failed_child_exit_code_is_preserved_and_stops_the_sequence(self) -> None:
        failure = subprocess.CalledProcessError(
            8,
            ("cmake", "--preset", run_desktop_ui.PRESET),
        )
        stderr = io.StringIO()
        with (
            mock.patch.object(
                run_desktop_ui.subprocess,
                "run",
                side_effect=failure,
            ) as run,
            redirect_stderr(stderr),
        ):
            result = run_desktop_ui.main(("check",))

        self.assertEqual(result, 8)
        run.assert_called_once()
        self.assertIn("exit code 8", stderr.getvalue())

    def test_generic_spawn_error_is_reported_without_a_traceback(self) -> None:
        stderr = io.StringIO()
        with (
            mock.patch.object(
                run_desktop_ui.subprocess,
                "run",
                side_effect=PermissionError("access denied"),
            ),
            redirect_stderr(stderr),
        ):
            result = run_desktop_ui.main(("check",))

        self.assertEqual(result, 1)
        self.assertIn("access denied", stderr.getvalue())

    def test_ui_launch_uses_visible_windows_driver_and_exact_path(self) -> None:
        process = mock.Mock()
        process.wait.return_value = 0
        with tempfile.TemporaryDirectory() as temporary_directory:
            executable = (
                Path(temporary_directory)
                / "UI build & тест"
                / "furnace_hmi_desktop.exe"
            )
            executable.parent.mkdir()
            executable.touch()
            with (
                mock.patch.dict(
                    run_desktop_ui.os.environ,
                    {"SDL_VIDEODRIVER": "dummy"},
                ),
                mock.patch.object(
                    run_desktop_ui.subprocess,
                    "Popen",
                    return_value=process,
                ) as popen,
            ):
                run_desktop_ui.run_ui_process(executable)

        command = popen.call_args.args[0]
        options = popen.call_args.kwargs
        self.assertEqual(command, [str(executable)])
        self.assertEqual(options["cwd"], executable.parent)
        self.assertEqual(options["env"]["SDL_VIDEODRIVER"], "windows")
        self.assertFalse(options["shell"])

    def test_ui_nonzero_exit_code_is_preserved(self) -> None:
        process = mock.Mock()
        process.wait.return_value = 23
        with (
            mock.patch.object(
                run_desktop_ui.subprocess,
                "Popen",
                return_value=process,
            ),
            self.assertRaisesRegex(
                run_desktop_ui.DesktopUiWorkflowError,
                "exited with code 23",
            ) as raised,
        ):
            run_desktop_ui.run_ui_process(run_desktop_ui.DESKTOP_EXECUTABLE)

        self.assertEqual(raised.exception.exit_code, 23)

    def test_interrupt_terminates_and_reaps_the_ui_process(self) -> None:
        process = mock.Mock()
        process.wait.side_effect = (KeyboardInterrupt(), 0)
        with (
            mock.patch.object(
                run_desktop_ui.subprocess,
                "Popen",
                return_value=process,
            ),
            self.assertRaises(KeyboardInterrupt),
        ):
            run_desktop_ui.run_ui_process(run_desktop_ui.DESKTOP_EXECUTABLE)

        process.terminate.assert_called_once_with()
        self.assertEqual(
            process.wait.call_args_list,
            [mock.call(), mock.call(timeout=5)],
        )


if __name__ == "__main__":
    unittest.main()
