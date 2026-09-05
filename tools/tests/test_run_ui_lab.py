# SPDX-License-Identifier: Apache-2.0
"""Regression tests for the host-only semantic UI laboratory workflow."""

from __future__ import annotations

from contextlib import redirect_stderr
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from tools import run_ui_lab


class UiLabWorkflowTests(unittest.TestCase):
    def test_verification_commands_use_the_ui_lab_preset(self) -> None:
        self.assertEqual(
            run_ui_lab.verification_commands(),
            (
                ("cmake", "--preset", "host-msvc-ui-lab-debug"),
                ("cmake", "--build", "--preset", "host-msvc-ui-lab-debug"),
                ("ctest", "--preset", "host-msvc-ui-lab-debug"),
            ),
        )

    def test_check_does_not_launch(self) -> None:
        observed: list[tuple[str, ...]] = []

        def observe(command: tuple[str, ...], *, cwd: Path) -> None:
            observed.append(tuple(command))

        with (
            mock.patch.object(run_ui_lab, "run_command", side_effect=observe),
            mock.patch.object(run_ui_lab, "run_ui_process") as launch,
        ):
            run_ui_lab.execute("check", scenario="running-normal", platform_name="nt")
        self.assertEqual(observed, list(run_ui_lab.verification_commands()))
        launch.assert_not_called()

    def test_run_passes_the_selected_scenario_to_the_executable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "furnace_hmi_ui_lab.exe"
            executable.touch()
            with (
                mock.patch.object(run_ui_lab, "run_command"),
                mock.patch.object(run_ui_lab, "run_ui_process") as launch,
            ):
                run_ui_lab.execute(
                    "run",
                    scenario="fault",
                    executable=executable,
                    platform_name="nt",
                )
        launch.assert_called_once_with(executable, "fault")

    def test_unknown_scenario_fails_before_running_commands(self) -> None:
        with mock.patch.object(run_ui_lab, "run_command") as command:
            with self.assertRaisesRegex(run_ui_lab.UiLabWorkflowError, "unknown scenario"):
                run_ui_lab.execute("check", scenario="not-a-scenario", platform_name="nt")
        command.assert_not_called()

    def test_failed_child_code_is_preserved(self) -> None:
        failure = subprocess.CalledProcessError(17, ("cmake", "--preset", run_ui_lab.PRESET))
        stderr = io.StringIO()
        with (
            mock.patch.object(run_ui_lab.subprocess, "run", side_effect=failure),
            redirect_stderr(stderr),
        ):
            result = run_ui_lab.main(("check",))
        self.assertEqual(result, 17)
        self.assertIn("exit code 17", stderr.getvalue())

    def test_ui_launch_forces_visible_sdl_driver(self) -> None:
        process = mock.Mock()
        process.wait.return_value = 0
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "furnace_hmi_ui_lab.exe"
            executable.touch()
            with (
                mock.patch.dict(run_ui_lab.os.environ, {"SDL_VIDEODRIVER": "dummy"}),
                mock.patch.object(run_ui_lab.subprocess, "Popen", return_value=process) as popen,
            ):
                run_ui_lab.run_ui_process(executable, "paused")
        self.assertEqual(
            popen.call_args.args[0],
            [str(executable), "--scenario", "paused"],
        )
        self.assertEqual(popen.call_args.kwargs["env"]["SDL_VIDEODRIVER"], "windows")
        self.assertFalse(popen.call_args.kwargs["shell"])


if __name__ == "__main__":
    unittest.main()
