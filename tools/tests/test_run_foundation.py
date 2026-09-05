#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Regression tests for the foundation workflow orchestration."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "run_foundation.py"
SPEC = importlib.util.spec_from_file_location("run_foundation", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class FoundationWorkflowTests(unittest.TestCase):
    def test_default_action_is_host(self) -> None:
        options = MODULE.parser().parse_args([])
        self.assertEqual(options.action, "host")

    def test_ui_scenarios_are_explicitly_bounded(self) -> None:
        options = MODULE.parser().parse_args(["ui-check", "--scenario", "fault"])
        self.assertEqual(options.scenario, "fault")

    def test_zephyr_environment_is_project_local(self) -> None:
        environment = MODULE.zephyr_environment()
        self.assertEqual(environment["ZEPHYR_SDK_INSTALL_DIR"], str(MODULE.SDK_DIR))
        self.assertEqual(environment["ZEPHYR_BASE"], str(MODULE.ZEPHYR_WORKSPACE / "zephyr"))
        self.assertIn("dtc-1.7.2", environment["PATH"])

    def test_action_set_has_no_flash_or_download_action(self) -> None:
        actions = next(action for action in MODULE.parser()._actions if action.dest == "action")
        self.assertNotIn("flash", actions.choices)
        self.assertNotIn("download", actions.choices)


if __name__ == "__main__":
    unittest.main()
