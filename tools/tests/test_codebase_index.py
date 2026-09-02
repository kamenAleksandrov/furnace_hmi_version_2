# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from tools import codebase_index  # noqa: E402


class CodebaseIndexTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.repo = Path(self.temporary_directory.name).resolve()
        self._write(".gitignore", ".deps/\nbuild/\n")
        self._write(".deps/zephyr/kernel.c", "int dependency_probe(void) { return 0; }\n")
        self._write("app/hmi/core.c", "int portable_probe(void) { return 1; }\n")
        self._write("docs/guide.md", "See [other](other.md).\n")
        self._write("docs/other.md", "# Other\n")
        self._write("docs/_generated/old.md", "generated\n")
        self._write("docs/_generated/README.md", "policy\n")
        self._write("tools/helper.py", "pass\n")
        self._git("init", "--quiet")
        self._git(
            "add",
            ".gitignore",
            "app/hmi/core.c",
            "docs/guide.md",
            "docs/other.md",
            "docs/_generated/old.md",
            "docs/_generated/README.md",
        )
        self._create_build_metadata()

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def _write(self, relative_path: str, content: str) -> Path:
        path = self.repo / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
        return path

    def _git(self, *arguments: str) -> None:
        subprocess.run(
            ["git", "-C", str(self.repo), *arguments],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def _create_build_metadata(
        self,
        relative_build: str = "build/host-debug",
        target_name: str = "host_probe",
    ) -> None:
        build = self.repo / relative_build
        reply = build / ".cmake/api/v1/reply"
        reply.mkdir(parents=True, exist_ok=True)
        (build / "compile_commands.json").write_text(
            json.dumps(
                [
                    {
                        "directory": str(build),
                        "file": str(self.repo / "app/hmi/core.c"),
                        "command": "cc -c core.c",
                    },
                    {
                        "directory": str(build),
                        "file": str(self.repo / ".deps/zephyr/kernel.c"),
                        "command": "cc -c kernel.c",
                    },
                ]
            ),
            encoding="utf-8",
        )
        (reply / "index-2026-08-24T00-00-00-0000.json").write_text(
            json.dumps(
                {"reply": {"codemodel-v2": {"jsonFile": "codemodel-v2-test.json"}}}
            ),
            encoding="utf-8",
        )
        (reply / "codemodel-v2-test.json").write_text(
            json.dumps(
                {
                    "paths": {"source": str(self.repo), "build": str(build)},
                    "configurations": [
                        {
                            "name": "Debug",
                            "targets": [
                                {
                                    "name": target_name,
                                    "jsonFile": "target-host-probe.json",
                                }
                            ],
                        }
                    ],
                }
            ),
            encoding="utf-8",
        )
        (reply / "target-host-probe.json").write_text(
            json.dumps(
                {
                    "name": target_name,
                    "type": "EXECUTABLE",
                    "sources": [
                        {"path": "app/hmi/core.c"},
                        {"path": ".deps/zephyr/kernel.c"},
                        {"path": str(Path(__file__).resolve())},
                    ],
                    "artifacts": [{"path": "host_probe.exe"}],
                }
            ),
            encoding="utf-8",
        )
        (build / "CMakeCache.txt").write_text(
            "BOARD:STRING=qemu_x86\nBOARD_REVISION:STRING=1\n",
            encoding="utf-8",
        )

    def _build_index(self) -> dict:
        return codebase_index.build_index(self.repo)

    def test_output_is_deterministic_and_contains_no_absolute_paths(self) -> None:
        first = self._build_index()
        second = self._build_index()

        first_json = json.dumps(first, sort_keys=True)
        self.assertEqual(first_json, json.dumps(second, sort_keys=True))
        self.assertNotIn(str(self.repo), first_json)
        self.assertNotIn(str(Path(__file__).resolve().parent), first_json)
        self.assertNotIn("generated_at", first_json)
        markdown = codebase_index.render_markdown(first)
        self.assertIn("## Inputs", markdown)
        self.assertIn("`build/host-debug/compile_commands.json`", markdown)

    def test_build_outputs_are_ingested_without_parsing_c(self) -> None:
        self._write("host_probe.exe", "source-tree collision\n")
        index = self._build_index()

        self.assertEqual(index["compile_units"][0]["source"], "app/hmi/core.c")
        target = next(item for item in index["build_targets"] if item["name"] == "host_probe")
        self.assertEqual(target["sources"], ["app/hmi/core.c"])
        self.assertEqual(target["artifacts"], ["build/host-debug/host_probe.exe"])
        self.assertEqual(target["external_source_count"], 2)
        self.assertTrue(any(board["name"] == "qemu_x86" for board in index["boards"]))
        self.assertIn(
            {"from": "target:host_probe", "to": "app/hmi/core.c", "type": "builds"},
            index["relationships"],
        )

    def test_manual_generated_and_git_state_are_explicit(self) -> None:
        files = {entry["path"]: entry for entry in self._build_index()["files"]}

        self.assertTrue(files["docs/_generated/old.md"]["generated"])
        self.assertFalse(files["docs/_generated/old.md"]["manual"])
        self.assertEqual(files["docs/_generated/old.md"]["git_state"], "tracked")
        self.assertTrue(files["docs/_generated/README.md"]["manual"])
        self.assertFalse(files["docs/_generated/README.md"]["generated"])
        self.assertTrue(files["tools/helper.py"]["manual"])
        self.assertEqual(files["tools/helper.py"]["git_state"], "untracked")
        self.assertIn(
            {"from": "docs/guide.md", "to": "docs/other.md", "type": "documents"},
            self._build_index()["relationships"],
        )

    def test_tool_test_suites_are_classified_as_tests(self) -> None:
        self.assertEqual(
            codebase_index.classify_file("tools/tests/test_helper.py"),
            "test",
        )

    def test_cli_reaches_fixed_point_and_check_detects_drift(self) -> None:
        script = REPOSITORY_ROOT / "tools/codebase_index.py"
        command = [
            sys.executable,
            str(script),
            "--repo-root",
            str(self.repo),
            "--build-dir",
            "build/host-debug",
        ]
        subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        first_json = (self.repo / "build/metadata/codebase-index.json").read_bytes()
        first_markdown = (self.repo / "docs/_generated/CODEBASE_INDEX.md").read_bytes()

        subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(first_json, (self.repo / "build/metadata/codebase-index.json").read_bytes())
        self.assertEqual(
            first_markdown, (self.repo / "docs/_generated/CODEBASE_INDEX.md").read_bytes()
        )

        check = subprocess.run(
            [*command, "--check"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        self.assertEqual(check.returncode, 0, check.stderr.decode())

        self._write("tools/new_file.py", "pass\n")
        check = subprocess.run(
            [*command, "--check"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        self.assertEqual(check.returncode, 1)

    def test_missing_build_inputs_produce_actionable_diagnostics(self) -> None:
        empty_build = self.repo / "missing-build"
        index = codebase_index.build_index(self.repo, build_dirs=[empty_build])
        codes = {item["code"] for item in index["diagnostics"]}

        self.assertIn("compile-database-missing", codes)
        self.assertIn("cmake-file-api-missing", codes)

    def test_cli_fails_closed_for_missing_build_inputs(self) -> None:
        script = REPOSITORY_ROOT / "tools/codebase_index.py"
        command = [
            sys.executable,
            str(script),
            "--repo-root",
            str(self.repo),
            "--build-dir",
            "missing-build",
        ]

        failed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(failed.returncode, 2)
        self.assertIn(b"Required build metadata", failed.stderr)
        self.assertFalse((self.repo / "build/metadata/codebase-index.json").exists())

        allowed = subprocess.run(
            [*command, "--allow-degraded"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.assertEqual(allowed.returncode, 0, allowed.stderr.decode(errors="replace"))
        checked = subprocess.run(
            [*command, "--allow-degraded", "--check"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.assertEqual(checked.returncode, 0, checked.stderr.decode(errors="replace"))

    def test_canonical_inputs_ignore_transient_builds_and_allow_override(self) -> None:
        self._create_build_metadata("build/scratch-experiment", "scratch_probe")

        canonical = self._build_index()
        self.assertEqual(
            [item["path"] for item in canonical["inputs"]["compile_databases"]],
            [
                "build/host-debug/compile_commands.json",
                "build/zephyr-qemu_x86/compile_commands.json",
            ],
        )
        self.assertEqual(
            [item["name"] for item in canonical["build_targets"]], ["host_probe"]
        )

        overridden = codebase_index.build_index(
            self.repo, build_dirs=[Path("build/scratch-experiment")]
        )
        self.assertEqual(
            [item["path"] for item in overridden["inputs"]["compile_databases"]],
            ["build/scratch-experiment/compile_commands.json"],
        )
        self.assertEqual(
            [item["name"] for item in overridden["build_targets"]], ["scratch_probe"]
        )

    def test_newer_file_api_error_prevents_stale_object_fallback(self) -> None:
        reply = self.repo / "build/host-debug/.cmake/api/v1/reply"
        (reply / "index-2026-08-24T00-00-01-0000.json").write_text(
            json.dumps(
                {
                    "objects": [
                        {
                            "jsonFile": "codemodel-v2-test.json",
                            "kind": "codemodel",
                            "version": {"major": 2, "minor": 9},
                        }
                    ],
                    "reply": {"codemodel-v2": {"error": "synthetic query failure"}},
                }
            ),
            encoding="utf-8",
        )
        diagnostics: list[dict[str, str]] = []

        descriptor, targets = codebase_index.parse_cmake_file_api(
            self.repo / "build/host-debug", self.repo, diagnostics
        )

        self.assertEqual(descriptor["status"], "invalid")
        self.assertEqual(targets, [])
        self.assertIn(
            "cmake-file-api-query-error", {item["code"] for item in diagnostics}
        )

    def test_unrelated_file_api_error_does_not_hide_valid_codemodel(self) -> None:
        reply = self.repo / "build/host-debug/.cmake/api/v1/reply"
        (reply / "index-2026-08-24T00-00-01-0000.json").write_text(
            json.dumps(
                {
                    "reply": {
                        "client-unrelated": {
                            "codemodel-v99": {"error": "synthetic unrelated failure"}
                        },
                        "codemodel-v2": {"jsonFile": "codemodel-v2-test.json"},
                    }
                }
            ),
            encoding="utf-8",
        )
        diagnostics: list[dict[str, str]] = []

        descriptor, targets = codebase_index.parse_cmake_file_api(
            self.repo / "build/host-debug", self.repo, diagnostics
        )

        self.assertEqual(descriptor["status"], "loaded", diagnostics)
        self.assertEqual([item["name"] for item in targets], ["host_probe"])

    def test_malformed_file_api_shapes_produce_diagnostics(self) -> None:
        reply = self.repo / "build/host-debug/.cmake/api/v1/reply"
        cases = [
            ("codemodel-array", "codemodel-v2-test.json", []),
            (
                "paths-array",
                "codemodel-v2-test.json",
                {"configurations": [], "paths": []},
            ),
            ("target-array", "target-host-probe.json", []),
            (
                "sources-null",
                "target-host-probe.json",
                {
                    "artifacts": [],
                    "name": "host_probe",
                    "sources": None,
                    "type": "EXECUTABLE",
                },
            ),
        ]
        for case_name, relative_path, malformed_value in cases:
            with self.subTest(case=case_name):
                self._create_build_metadata()
                (reply / relative_path).write_text(
                    json.dumps(malformed_value), encoding="utf-8"
                )
                diagnostics: list[dict[str, str]] = []

                descriptor, targets = codebase_index.parse_cmake_file_api(
                    self.repo / "build/host-debug", self.repo, diagnostics
                )

                self.assertEqual(targets, [])
                self.assertIn(
                    descriptor["status"], {"loaded", "invalid"}, diagnostics
                )
                self.assertTrue(
                    any(item["severity"] in {"error", "warning"} for item in diagnostics)
                )

    def test_git_executable_absence_uses_filesystem_fallback(self) -> None:
        diagnostics: list[dict[str, str]] = []
        with mock.patch.object(
            codebase_index.subprocess, "run", side_effect=FileNotFoundError("git absent")
        ):
            paths, tracked = codebase_index.enumerate_repository_files(
                self.repo, diagnostics
            )

        self.assertIn("app/hmi/core.c", paths)
        self.assertEqual(tracked, set())
        self.assertIn("git-unavailable", {item["code"] for item in diagnostics})

    @unittest.skipUnless(
        shutil.which("cmake") and shutil.which("ninja"), "CMake/Ninja required"
    )
    def test_prepared_query_produces_codemodel_on_first_configure(self) -> None:
        fresh_build = self.repo / "fresh-cmake"
        queries = codebase_index.prepare_cmake_file_api_queries([fresh_build])
        self.assertEqual(
            queries,
            [fresh_build / ".cmake/api/v1/query/codemodel-v2"],
        )

        configure = subprocess.run(
            [
                "cmake",
                "-S",
                str(REPOSITORY_ROOT),
                "-B",
                str(fresh_build),
                "-G",
                "Ninja",
                "-DFURNACE_HMI_BUILD_DESKTOP=OFF",
                "-DFURNACE_HMI_BUILD_HOST_TESTS=OFF",
                "-DBUILD_TESTING=OFF",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.assertEqual(configure.returncode, 0, configure.stderr.decode(errors="replace"))

        diagnostics: list[dict[str, str]] = []
        descriptor, targets = codebase_index.parse_cmake_file_api(
            fresh_build, REPOSITORY_ROOT, diagnostics
        )
        self.assertEqual(descriptor["status"], "loaded", diagnostics)
        self.assertTrue(any(item["name"] == "furnace_hmi_foundation" for item in targets))

    def test_schema_metadata_matches_generator_contract(self) -> None:
        schema = json.loads(
            (REPOSITORY_ROOT / "tools/codebase-index.schema.json").read_text(encoding="utf-8")
        )
        index = self._build_index()

        self.assertEqual(index["schema"]["id"], codebase_index.SCHEMA_ID)
        self.assertEqual(index["schema"]["version"], codebase_index.SCHEMA_VERSION)
        self.assertEqual(index["generator"]["version"], codebase_index.GENERATOR_VERSION)
        self.assertEqual(set(schema["required"]), set(index))

    def test_generated_index_validates_against_json_schema_when_available(self) -> None:
        try:
            import jsonschema
        except ImportError:
            self.skipTest("jsonschema is an optional test-only validator")

        schema = json.loads(
            (REPOSITORY_ROOT / "tools/codebase-index.schema.json").read_text(encoding="utf-8")
        )
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.Draft202012Validator(schema).validate(self._build_index())


if __name__ == "__main__":
    unittest.main()
