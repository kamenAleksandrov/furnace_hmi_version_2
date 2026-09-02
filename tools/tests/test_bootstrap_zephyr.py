# SPDX-License-Identifier: Apache-2.0
"""Focused regression tests for the repository-local Zephyr bootstrap flow."""

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from unittest import mock

from tools import bootstrap_zephyr


class BootstrapZephyrTests(unittest.TestCase):
    def test_manifest_rejects_changed_lvgl_pin_before_preparation(self) -> None:
        manifest = """\
manifest:
  projects:
    - name: zephyr
      revision: 684c9e8f32e4373a21098559f748f06915f950c9
    - name: lvgl
      revision: 0000000000000000000000000000000000000000
"""
        with tempfile.TemporaryDirectory() as temporary_directory:
            tracked_manifest = Path(temporary_directory) / "west.yml"
            tracked_manifest.write_text(manifest, encoding="utf-8")
            with (
                mock.patch.object(
                    bootstrap_zephyr,
                    "TRACKED_MANIFEST",
                    tracked_manifest,
                ),
                self.assertRaisesRegex(
                    bootstrap_zephyr.BootstrapError,
                    "changed the accepted LVGL pin",
                ),
            ):
                bootstrap_zephyr.manifest_revision()

    def test_inaccessible_required_file_becomes_actionable_bootstrap_error(self) -> None:
        inaccessible = mock.Mock(spec=Path)
        inaccessible.is_file.side_effect = PermissionError("access denied")

        with (
            mock.patch.object(
                bootstrap_zephyr,
                "display_path",
                return_value=".deps/example",
            ),
            self.assertRaisesRegex(
                bootstrap_zephyr.BootstrapError,
                "cannot inspect .deps/example.*access denied",
            ),
        ):
            bootstrap_zephyr.require_file(inaccessible, "Check directory permissions.")

    def test_pinned_checkout_must_also_have_a_clean_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            checkout = Path(temporary_directory)
            (checkout / ".git").mkdir()
            (checkout / ".git" / "HEAD").write_text("ref: refs/heads/main\n", encoding="utf-8")

            with mock.patch.object(
                bootstrap_zephyr,
                "run",
                side_effect=(bootstrap_zephyr.ZEPHYR_REVISION, " M source.c"),
            ):
                self.assertFalse(
                    bootstrap_zephyr.checkout_is_pinned(
                        checkout,
                        bootstrap_zephyr.ZEPHYR_REVISION,
                    )
                )

            with mock.patch.object(
                bootstrap_zephyr,
                "run",
                side_effect=(bootstrap_zephyr.ZEPHYR_REVISION, ""),
            ):
                self.assertTrue(
                    bootstrap_zephyr.checkout_is_pinned(
                        checkout,
                        bootstrap_zephyr.ZEPHYR_REVISION,
                    )
                )

    def test_prepare_creates_workspace_before_binary_tool_validation(self) -> None:
        events: list[str] = []

        def checkout_is_pinned(path: object, revision: str) -> bool:
            del revision
            events.append(f"checkout:{getattr(path, 'name', '<unknown>')}")
            return True

        with (
            mock.patch.object(
                bootstrap_zephyr,
                "manifest_revision",
                side_effect=lambda: events.append("manifest"),
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "synchronize_manifest",
                side_effect=lambda: events.append("synchronize"),
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "initialize_workspace",
                side_effect=lambda: events.append("initialize"),
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "checkout_is_pinned",
                side_effect=checkout_is_pinned,
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "validate_workspace",
                side_effect=lambda: events.append("validate-workspace"),
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "validate_local_runtime",
                side_effect=lambda: events.append("validate-tools"),
            ),
            mock.patch.object(
                bootstrap_zephyr,
                "validate_host_runtime",
                side_effect=lambda: events.append("validate-host"),
            ),
            mock.patch("builtins.print"),
        ):
            bootstrap_zephyr.prepare(all_modules=False, skip_python_packages=True)

        self.assertLess(events.index("synchronize"), events.index("validate-tools"))
        self.assertLess(events.index("initialize"), events.index("validate-tools"))
        self.assertLess(events.index("validate-workspace"), events.index("validate-tools"))

    def test_default_prepare_updates_only_missing_foundation_projects_in_order(self) -> None:
        update_projects: list[str] = []

        def run(command: list[str], **kwargs: object) -> str:
            del kwargs
            if "update" in command:
                update_projects.append(command[-1])
            return ""

        with (
            mock.patch.object(bootstrap_zephyr, "manifest_revision"),
            mock.patch.object(bootstrap_zephyr, "synchronize_manifest"),
            mock.patch.object(bootstrap_zephyr, "initialize_workspace"),
            mock.patch.object(
                bootstrap_zephyr,
                "checkout_is_pinned",
                side_effect=(False, False),
            ),
            mock.patch.object(bootstrap_zephyr, "run", side_effect=run),
            mock.patch.object(bootstrap_zephyr, "validate_workspace"),
            mock.patch.object(bootstrap_zephyr, "validate_local_runtime"),
            mock.patch.object(bootstrap_zephyr, "validate_host_runtime"),
            mock.patch("builtins.print"),
        ):
            bootstrap_zephyr.prepare(all_modules=False, skip_python_packages=True)

        self.assertEqual(update_projects, ["zephyr", "lvgl"])


if __name__ == "__main__":
    unittest.main()
