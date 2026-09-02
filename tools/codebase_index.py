#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

"""Generate the deterministic Furnace HMI V2 codebase index.

The indexer consumes source-control metadata and build-system outputs. It does
not parse C or C++ source text. All emitted paths use repository-relative POSIX
notation so the result can be compared across machines.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path, PurePosixPath
from typing import Any, Iterable, Sequence


SCHEMA_ID = "furnace-hmi-codebase-index"
SCHEMA_VERSION = "1.0.1"
GENERATOR_VERSION = "1.0.2"
GENERATOR_PATH = "tools/codebase_index.py"
SCHEMA_PATH = "tools/codebase-index.schema.json"
CANONICAL_MARKDOWN_OUTPUT = "docs/_generated/CODEBASE_INDEX.md"
CANONICAL_BUILD_DIRECTORIES = (
    Path("build/host-debug"),
    Path("build/zephyr-qemu_x86"),
)

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".s", ".asm"}
HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".inc"}
DOC_SUFFIXES = {".md", ".rst", ".txt", ".adoc"}
BOARD_SUFFIXES = {".dts", ".dtsi", ".overlay", ".defconfig"}

LANGUAGES = {
    ".c": "C",
    ".cc": "C++",
    ".cpp": "C++",
    ".cxx": "C++",
    ".s": "Assembly",
    ".asm": "Assembly",
    ".h": "C/C++ header",
    ".hh": "C++ header",
    ".hpp": "C++ header",
    ".hxx": "C++ header",
}

MARKDOWN_LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
WIKI_LINK_RE = re.compile(r"\[\[([^\]]+)\]\]")


def _run_git(repo_root: Path, arguments: Sequence[str]) -> subprocess.CompletedProcess[bytes]:
    command = [
        "git",
        "-c",
        f"safe.directory={repo_root.as_posix()}",
        "-C",
        str(repo_root),
        *arguments,
    ]
    try:
        return subprocess.run(
            command,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except OSError as error:
        return subprocess.CompletedProcess(
            command,
            127,
            stdout=b"",
            stderr=str(error).encode("utf-8", errors="replace"),
        )


def _decode_nul_paths(output: bytes) -> list[str]:
    return sorted(
        {
            item.decode("utf-8", errors="surrogateescape").replace("\\", "/")
            for item in output.split(b"\0")
            if item
        }
    )


def _path_within_repo(path: Path, repo_root: Path) -> str | None:
    try:
        resolved = path.resolve(strict=False)
        relative = resolved.relative_to(repo_root)
    except (OSError, ValueError):
        return None
    return relative.as_posix()


def _project_source_path(path: Path, repo_root: Path) -> str | None:
    """Return a repository-owned source path, excluding build/dependency trees."""

    relative = _path_within_repo(path, repo_root)
    if relative is None:
        return None
    first_part = PurePosixPath(relative).parts[0] if relative else ""
    if first_part in {".deps", "build", "out", ".west", ".venv"} or first_part.startswith(
        ".venv-"
    ):
        return None
    return relative


def _safe_input_label(path: Path, repo_root: Path) -> str:
    relative = _path_within_repo(path, repo_root)
    if relative is not None:
        return relative
    return f"external:{path.name or 'input'}"


def _diagnostic(code: str, severity: str, message: str) -> dict[str, str]:
    return {"code": code, "severity": severity, "message": message}


def enumerate_repository_files(
    repo_root: Path, diagnostics: list[dict[str, str]]
) -> tuple[list[str], set[str]]:
    """Return non-ignored repository files and the subset tracked by Git."""

    tracked_result = _run_git(repo_root, ["ls-files", "-z", "--cached"])
    candidate_result = _run_git(
        repo_root, ["ls-files", "-z", "--cached", "--others", "--exclude-standard"]
    )
    if tracked_result.returncode == 0 and candidate_result.returncode == 0:
        tracked = set(_decode_nul_paths(tracked_result.stdout))
        candidates = [
            path
            for path in _decode_nul_paths(candidate_result.stdout)
            if (repo_root / Path(path)).is_file() or (repo_root / Path(path)).is_symlink()
        ]
        return candidates, tracked

    diagnostics.append(
        _diagnostic(
            "git-unavailable",
            "warning",
            "Git file enumeration failed; a filesystem fallback was used.",
        )
    )
    excluded_directories = {
        ".deps",
        ".git",
        ".west",
        ".venv",
        "build",
        "out",
        "__pycache__",
    }
    files: list[str] = []
    for current_root, directory_names, file_names in os.walk(repo_root):
        directory_names[:] = sorted(
            name for name in directory_names if name not in excluded_directories
        )
        current = Path(current_root)
        for file_name in sorted(file_names):
            relative = _path_within_repo(current / file_name, repo_root)
            if relative is not None:
                files.append(relative)
    return sorted(set(files)), set()


def is_generated_path(path: str) -> bool:
    pure = PurePosixPath(path)
    name = pure.name.lower()
    if path == "docs/_generated/README.md":
        return False
    return (
        path.startswith("build/")
        or path.startswith("out/")
        or path.startswith("docs/_generated/")
        or "/generated/" in f"/{path.lower()}/"
        or name in {"compile_commands.json", "codebase-index.json"}
        or ".generated." in name
    )


def classify_file(path: str) -> str:
    pure = PurePosixPath(path)
    suffix = pure.suffix.lower()
    lower_parts = tuple(part.lower() for part in pure.parts)
    lower_name = pure.name.lower()

    if lower_parts and (
        lower_parts[0] == "tests" or lower_parts[:2] == ("tools", "tests")
    ):
        return "test"
    if lower_parts and lower_parts[0] == "tools":
        return "tool"
    if lower_parts and lower_parts[0] in {".agents", ".codex"}:
        return "agent-configuration"
    if "boards" in lower_parts or suffix in BOARD_SUFFIXES or lower_name == "board.yml":
        return "board"
    if suffix in SOURCE_SUFFIXES:
        return "source"
    if suffix in HEADER_SUFFIXES:
        return "header"
    if lower_name == "cmakelists.txt" or suffix == ".cmake":
        return "build"
    if lower_name in {"west.yml", "kconfig", "prj.conf", "module.yml"}:
        return "build"
    if (
        (lower_parts and lower_parts[0] == "docs")
        or suffix in DOC_SUFFIXES
        or lower_name in {"readme", "readme.md", "license", "license.md"}
    ):
        return "documentation"
    if suffix in {".json", ".toml", ".yaml", ".yml", ".conf", ".ini"}:
        return "configuration"
    return "other"


def module_for_path(path: str) -> str:
    parts = PurePosixPath(path).parts
    if not parts:
        return "."
    if parts[0] == ".agents" and len(parts) >= 3 and parts[1] == "skills":
        return "/".join(parts[:3])
    if parts[0] == ".codex" and len(parts) >= 2:
        return "/".join(parts[:2])
    if parts[0] == "app" and len(parts) >= 3 and parts[1] == "hmi":
        return "/".join(parts[:3])
    if parts[0] == "app" and len(parts) >= 2:
        return "/".join(parts[:2])
    if parts[0] == "simulator" and len(parts) >= 2:
        return "/".join(parts[:2])
    if parts[0] in {"tests", "docs", "boards"} and len(parts) >= 2:
        if PurePosixPath(parts[1]).suffix:
            return parts[0]
        return "/".join(parts[:2])
    if parts[0] in {"tools", "scripts"}:
        return parts[0]
    if len(parts) == 1:
        return "."
    return parts[0]


def make_file_entries(paths: Iterable[str], tracked: set[str]) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for path in sorted(set(paths)):
        generated = is_generated_path(path)
        entry: dict[str, Any] = {
            "generated": generated,
            "git_state": "tracked" if path in tracked else "untracked",
            "kind": classify_file(path),
            "manual": not generated,
            "module": module_for_path(path),
            "path": path,
        }
        language = LANGUAGES.get(PurePosixPath(path).suffix.lower())
        if language is not None:
            entry["language"] = language
        entries.append(entry)
    return entries


def make_modules(files: Sequence[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in files:
        grouped[entry["module"]].append(entry)

    modules: list[dict[str, Any]] = []
    for module_path in sorted(grouped):
        module_files = sorted(grouped[module_path], key=lambda item: item["path"])
        modules.append(
            {
                "file_count": len(module_files),
                "files": [entry["path"] for entry in module_files],
                "generated_file_count": sum(bool(entry["generated"]) for entry in module_files),
                "kinds": sorted({entry["kind"] for entry in module_files}),
                "manual_file_count": sum(bool(entry["manual"]) for entry in module_files),
                "path": module_path,
            }
        )
    return modules


def _resolve_compile_source(entry: dict[str, Any]) -> Path | None:
    file_value = entry.get("file")
    if not isinstance(file_value, str) or not file_value:
        return None
    source = Path(file_value)
    if source.is_absolute():
        return source
    directory = entry.get("directory")
    if isinstance(directory, str) and directory:
        return Path(directory) / source
    return source


def parse_compile_database(
    database_path: Path, repo_root: Path, diagnostics: list[dict[str, str]]
) -> tuple[dict[str, str], list[dict[str, str]]]:
    input_label = _safe_input_label(database_path, repo_root)
    descriptor = {"path": input_label, "status": "loaded"}
    try:
        raw = json.loads(database_path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        descriptor["status"] = "missing"
        diagnostics.append(
            _diagnostic(
                "compile-database-missing",
                "warning",
                f"Compile database '{input_label}' was not found.",
            )
        )
        return descriptor, []
    except (OSError, json.JSONDecodeError):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "compile-database-invalid",
                "error",
                f"Compile database '{input_label}' could not be decoded.",
            )
        )
        return descriptor, []

    if not isinstance(raw, list):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "compile-database-shape",
                "error",
                f"Compile database '{input_label}' is not a JSON array.",
            )
        )
        return descriptor, []

    units: set[tuple[str, str]] = set()
    external_count = 0
    for raw_entry in raw:
        if not isinstance(raw_entry, dict):
            continue
        source = _resolve_compile_source(raw_entry)
        if source is None:
            continue
        relative = _project_source_path(source, repo_root)
        if relative is None:
            external_count += 1
            continue
        language = LANGUAGES.get(PurePosixPath(relative).suffix.lower(), "unknown")
        units.add((relative, language))

    if external_count:
        diagnostics.append(
            _diagnostic(
                "compile-units-external",
                "info",
                f"Compile database '{input_label}' referenced {external_count} non-project source file(s); their paths were omitted.",
            )
        )
    return descriptor, [
        {"database": input_label, "language": language, "source": source}
        for source, language in sorted(units)
    ]


def _find_file_api_index(reply_or_build_path: Path) -> Path | None:
    candidates: list[Path]
    if reply_or_build_path.is_file():
        candidates = [reply_or_build_path]
    else:
        direct_reply = reply_or_build_path / ".cmake" / "api" / "v1" / "reply"
        reply_directory = direct_reply if direct_reply.is_dir() else reply_or_build_path
        candidates = sorted(reply_directory.glob("index-*.json"))
    return candidates[-1] if candidates else None


def _load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def _client_codemodel_responses(reply: dict[str, Any]) -> Iterable[dict[str, Any]]:
    """Yield client-owned responses that explicitly request a codemodel."""

    for key, value in reply.items():
        if not key.startswith("client-") or not isinstance(value, dict):
            continue
        direct = value.get("codemodel-v2")
        if isinstance(direct, dict):
            yield direct
        stateful = value.get("query.json")
        if not isinstance(stateful, dict):
            continue
        requests = stateful.get("requests")
        responses = stateful.get("responses")
        if not isinstance(requests, list) or not isinstance(responses, list):
            continue
        for request, response in zip(requests, responses):
            if (
                isinstance(request, dict)
                and request.get("kind") == "codemodel"
                and isinstance(response, dict)
            ):
                yield response


def _codemodel_selection(index: dict[str, Any]) -> tuple[str | None, bool]:
    """Select a codemodel reference and report an error from that selection."""

    reply = index.get("reply")
    if isinstance(reply, dict):
        codemodel = reply.get("codemodel-v2")
        if isinstance(codemodel, dict):
            if "error" in codemodel:
                return None, True
            if isinstance(codemodel.get("jsonFile"), str):
                return codemodel["jsonFile"], False
    objects = index.get("objects")
    if isinstance(objects, list):
        for item in objects:
            if (
                isinstance(item, dict)
                and item.get("kind") == "codemodel"
                and isinstance(item.get("jsonFile"), str)
            ):
                return item["jsonFile"], False
    if isinstance(reply, dict):
        client_error = False
        for response in _client_codemodel_responses(reply):
            if isinstance(response.get("jsonFile"), str):
                return response["jsonFile"], False
            client_error = client_error or "error" in response
        return None, client_error
    return None, False


def _resolve_codemodel_path(value: str, source_root: Path, build_root: Path) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    source_candidate = source_root / path
    if source_candidate.exists():
        return source_candidate
    return build_root / path


def parse_cmake_file_api(
    reply_or_build_path: Path, repo_root: Path, diagnostics: list[dict[str, str]]
) -> tuple[dict[str, str], list[dict[str, Any]]]:
    input_label = _safe_input_label(reply_or_build_path, repo_root)
    descriptor = {"path": input_label, "status": "loaded"}
    index_path = _find_file_api_index(reply_or_build_path)
    if index_path is None:
        descriptor["status"] = "missing"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-missing",
                "warning",
                f"No CMake File API reply was found for '{input_label}'.",
            )
        )
        return descriptor, []

    try:
        index = _load_json(index_path)
        if not isinstance(index, dict):
            raise ValueError("index is not an object")
    except (OSError, ValueError, json.JSONDecodeError):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-invalid",
                "error",
                f"CMake File API input '{input_label}' could not be decoded.",
            )
        )
        return descriptor, []

    # A failed selected query takes precedence over a stale object fallback,
    # while errors owned by unrelated clients do not invalidate a valid
    # selected codemodel.
    codemodel_file, codemodel_error = _codemodel_selection(index)
    if codemodel_error:
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-query-error",
                "error",
                f"CMake File API input '{input_label}' reports a query error; no codemodel was accepted.",
            )
        )
        return descriptor, []

    try:
        if codemodel_file is None:
            raise ValueError("codemodel reference is absent")
        codemodel = _load_json(index_path.parent / codemodel_file)
        if not isinstance(codemodel, dict):
            raise ValueError("codemodel is not an object")
    except (OSError, ValueError, json.JSONDecodeError):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-invalid",
                "error",
                f"CMake File API input '{input_label}' could not be decoded.",
            )
        )
        return descriptor, []

    paths = codemodel.get("paths", {})
    configurations = codemodel.get("configurations", [])
    if not isinstance(paths, dict) or not isinstance(configurations, list):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-invalid",
                "error",
                f"CMake File API input '{input_label}' has an invalid codemodel shape.",
            )
        )
        return descriptor, []
    source_value = paths.get("source")
    build_value = paths.get("build")
    if source_value is not None and not isinstance(source_value, str):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-invalid",
                "error",
                f"CMake File API input '{input_label}' has an invalid source path.",
            )
        )
        return descriptor, []
    if build_value is not None and not isinstance(build_value, str):
        descriptor["status"] = "invalid"
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-invalid",
                "error",
                f"CMake File API input '{input_label}' has an invalid build path.",
            )
        )
        return descriptor, []
    source_root = Path(source_value) if source_value is not None else repo_root
    build_root = Path(build_value) if build_value is not None else index_path.parents[4]
    targets: list[dict[str, Any]] = []
    for configuration in configurations:
        if not isinstance(configuration, dict):
            continue
        configuration_name = str(configuration.get("name", ""))
        target_references = configuration.get("targets", [])
        if not isinstance(target_references, list):
            diagnostics.append(
                _diagnostic(
                    "cmake-target-invalid",
                    "warning",
                    f"A target list in CMake File API input '{input_label}' has an invalid shape.",
                )
            )
            continue
        for target_reference in target_references:
            if not isinstance(target_reference, dict):
                continue
            json_file = target_reference.get("jsonFile")
            if not isinstance(json_file, str):
                continue
            try:
                target_data = _load_json(index_path.parent / json_file)
            except (OSError, json.JSONDecodeError):
                diagnostics.append(
                    _diagnostic(
                        "cmake-target-invalid",
                        "warning",
                        f"A target record in CMake File API input '{input_label}' could not be decoded.",
                    )
                )
                continue

            if not isinstance(target_data, dict):
                diagnostics.append(
                    _diagnostic(
                        "cmake-target-invalid",
                        "warning",
                        f"A target record in CMake File API input '{input_label}' has an invalid shape.",
                    )
                )
                continue
            source_records = target_data.get("sources", [])
            artifact_records = target_data.get("artifacts", [])
            if not isinstance(source_records, list) or not isinstance(
                artifact_records, list
            ):
                diagnostics.append(
                    _diagnostic(
                        "cmake-target-invalid",
                        "warning",
                        f"A target record in CMake File API input '{input_label}' has invalid source or artifact data.",
                    )
                )
                continue

            target_sources: set[str] = set()
            external_source_count = 0
            for source_record in source_records:
                if not isinstance(source_record, dict) or not isinstance(
                    source_record.get("path"), str
                ):
                    continue
                source_path = _resolve_codemodel_path(
                    source_record["path"], source_root, build_root
                )
                relative = _project_source_path(source_path, repo_root)
                if relative is None:
                    external_source_count += 1
                else:
                    target_sources.add(relative)

            artifacts: set[str] = set()
            for artifact_record in artifact_records:
                if not isinstance(artifact_record, dict) or not isinstance(
                    artifact_record.get("path"), str
                ):
                    continue
                artifact_path = Path(artifact_record["path"])
                if not artifact_path.is_absolute():
                    artifact_path = build_root / artifact_path
                relative = _path_within_repo(artifact_path, repo_root)
                if relative is not None:
                    artifacts.add(relative)

            target: dict[str, Any] = {
                "artifacts": sorted(artifacts),
                "build_system": "cmake-file-api",
                "configuration": configuration_name,
                "external_source_count": external_source_count,
                "input": input_label,
                "name": str(target_data.get("name", target_reference.get("name", ""))),
                "sources": sorted(target_sources),
                "type": str(target_data.get("type", "UNKNOWN")),
            }
            targets.append(target)

    targets.sort(key=lambda item: (item["name"], item["configuration"], item["input"]))
    return descriptor, targets


def parse_cmake_cache_board(
    build_dir: Path, repo_root: Path, diagnostics: list[dict[str, str]]
) -> dict[str, Any] | None:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.is_file():
        return None
    values: dict[str, str] = {}
    try:
        for line in cache_path.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("//") or line.startswith("#") or "=" not in line:
                continue
            key_and_type, value = line.split("=", 1)
            key = key_and_type.split(":", 1)[0]
            if key in {"BOARD", "BOARD_REVISION", "BOARD_DIR"}:
                values[key] = value
    except OSError:
        diagnostics.append(
            _diagnostic(
                "cmake-cache-unreadable",
                "warning",
                f"CMake cache in '{_safe_input_label(build_dir, repo_root)}' could not be read.",
            )
        )
        return None
    if not values.get("BOARD"):
        return None
    board: dict[str, Any] = {
        "build_directory": _safe_input_label(build_dir, repo_root),
        "name": values["BOARD"],
        "source": "CMakeCache.txt",
    }
    if values.get("BOARD_REVISION"):
        board["revision"] = values["BOARD_REVISION"]
    board_directory = values.get("BOARD_DIR")
    if board_directory:
        relative = _path_within_repo(Path(board_directory), repo_root)
        if relative is not None:
            board["definition_directory"] = relative
    return board


def canonical_build_directories(repo_root: Path) -> list[Path]:
    """Return the stable project build inputs used when no override is given."""

    return [repo_root / path for path in CANONICAL_BUILD_DIRECTORIES]


def selected_build_directories(
    repo_root: Path, build_dirs: Sequence[Path]
) -> list[Path]:
    """Resolve explicit build directories or return the canonical defaults."""

    if not build_dirs:
        return canonical_build_directories(repo_root)
    return [path if path.is_absolute() else repo_root / path for path in build_dirs]


def prepare_cmake_file_api_queries(build_dirs: Sequence[Path]) -> list[Path]:
    """Create codemodel-v2 queries before the first CMake configure."""

    queries: list[Path] = []
    for build_dir in build_dirs:
        query = build_dir / ".cmake" / "api" / "v1" / "query" / "codemodel-v2"
        query.parent.mkdir(parents=True, exist_ok=True)
        if not query.exists():
            query.write_text("", encoding="utf-8", newline="\n")
        queries.append(query)
    return queries


def required_build_input_failures(index: dict[str, Any]) -> list[str]:
    """Return deterministic descriptions of missing or invalid required metadata."""

    failures: set[str] = set()
    inputs = index.get("inputs", {})
    if isinstance(inputs, dict):
        for group_name in ("compile_databases", "cmake_file_api"):
            descriptors = inputs.get(group_name, [])
            if not isinstance(descriptors, list):
                failures.add(f"{group_name}: invalid descriptor list")
                continue
            for descriptor in descriptors:
                if not isinstance(descriptor, dict):
                    failures.add(f"{group_name}: invalid descriptor")
                    continue
                status = descriptor.get("status")
                if status != "loaded":
                    failures.add(
                        f"{group_name}: {descriptor.get('path', '<unknown>')} ({status})"
                    )

    diagnostics = index.get("diagnostics", [])
    if isinstance(diagnostics, list):
        for diagnostic in diagnostics:
            if not isinstance(diagnostic, dict):
                continue
            code = diagnostic.get("code")
            if diagnostic.get("severity") == "error" or code == "cmake-target-invalid":
                failures.add(f"diagnostic: {code or '<unknown>'}")
    return sorted(failures)


def documentation_relationships(
    repo_root: Path, files: Sequence[dict[str, Any]]
) -> list[dict[str, str]]:
    known_paths = {entry["path"] for entry in files}
    relationships: set[tuple[str, str, str]] = set()
    for entry in files:
        if entry["kind"] != "documentation" or not entry["path"].lower().endswith(".md"):
            continue
        source_path = repo_root / Path(entry["path"])
        try:
            text = source_path.read_text(encoding="utf-8")
        except OSError:
            continue
        raw_targets = MARKDOWN_LINK_RE.findall(text)
        raw_targets.extend(WIKI_LINK_RE.findall(text))
        for raw_target in raw_targets:
            target = raw_target.split("|", 1)[0].split("#", 1)[0].strip().strip("<>")
            if not target or "://" in target or target.startswith(("mailto:", "#")):
                continue
            target_path = Path(target.replace("\\", "/"))
            if target_path.is_absolute():
                candidate = repo_root / str(target_path).lstrip("/\\")
            else:
                candidate = source_path.parent / target_path
            if candidate.suffix == "":
                candidate = candidate.with_suffix(".md")
            relative = _path_within_repo(candidate, repo_root)
            if relative in known_paths:
                relationships.add((entry["path"], relative, "documents"))
    return [
        {"from": source, "to": target, "type": relationship_type}
        for source, target, relationship_type in sorted(relationships)
    ]


def file_based_boards(files: Sequence[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[str, list[str]] = defaultdict(list)
    for entry in files:
        if entry["kind"] != "board" or not entry["path"].startswith("boards/"):
            continue
        parts = PurePosixPath(entry["path"]).parts
        root = "/".join(parts[:3]) if len(parts) >= 3 else "/".join(parts[:-1])
        grouped[root].append(entry["path"])
    return [
        {
            "definition_directory": root,
            "files": sorted(paths),
            "name": PurePosixPath(root).name,
            "source": "repository",
        }
        for root, paths in sorted(grouped.items())
    ]


def _deduplicate_dicts(items: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    serialized = {json.dumps(item, sort_keys=True, separators=(",", ":")): item for item in items}
    return [serialized[key] for key in sorted(serialized)]


def build_index(
    repo_root: Path,
    *,
    build_dirs: Sequence[Path] = (),
    compile_database_paths: Sequence[Path] = (),
    cmake_reply_paths: Sequence[Path] = (),
) -> dict[str, Any]:
    repo_root = repo_root.resolve(strict=True)
    diagnostics: list[dict[str, str]] = []
    paths, tracked = enumerate_repository_files(repo_root, diagnostics)
    # Include the canonical tracked report before the first generation so the
    # index reaches a fixed point in one run instead of discovering itself on
    # the second run.
    paths = sorted(set(paths) | {CANONICAL_MARKDOWN_OUTPUT})
    files = make_file_entries(paths, tracked)

    database_paths = [
        path if path.is_absolute() else repo_root / path for path in compile_database_paths
    ]
    reply_paths = [path if path.is_absolute() else repo_root / path for path in cmake_reply_paths]

    selected_builds = selected_build_directories(repo_root, build_dirs)
    for build_dir in selected_builds:
        database_paths.append(build_dir / "compile_commands.json")
        reply_paths.append(build_dir)
    cache_builds = selected_builds

    database_paths = sorted(set(database_paths), key=lambda item: item.as_posix())
    reply_paths = sorted(set(reply_paths), key=lambda item: item.as_posix())
    cache_builds = sorted(set(cache_builds), key=lambda item: item.as_posix())

    compile_inputs: list[dict[str, str]] = []
    compile_units: list[dict[str, str]] = []
    for database_path in database_paths:
        descriptor, units = parse_compile_database(database_path, repo_root, diagnostics)
        compile_inputs.append(descriptor)
        compile_units.extend(units)
    if not database_paths:
        diagnostics.append(
            _diagnostic(
                "compile-database-not-provided",
                "warning",
                "No compile database was found; configure a build with CMAKE_EXPORT_COMPILE_COMMANDS enabled.",
            )
        )

    cmake_inputs: list[dict[str, str]] = []
    build_targets: list[dict[str, Any]] = []
    for reply_path in reply_paths:
        descriptor, targets = parse_cmake_file_api(reply_path, repo_root, diagnostics)
        cmake_inputs.append(descriptor)
        build_targets.extend(targets)
    if not reply_paths:
        diagnostics.append(
            _diagnostic(
                "cmake-file-api-not-provided",
                "warning",
                "No CMake File API codemodel was found; prepare a codemodel-v2 query before configuring a build.",
            )
        )

    boards: list[dict[str, Any]] = file_based_boards(files)
    for cache_build in cache_builds:
        board = parse_cmake_cache_board(cache_build, repo_root, diagnostics)
        if board is not None:
            boards.append(board)

    relationships = documentation_relationships(repo_root, files)
    for target in build_targets:
        relationships.extend(
            {"from": f"target:{target['name']}", "to": source, "type": "builds"}
            for source in target["sources"]
        )

    diagnostics.sort(key=lambda item: (item["severity"], item["code"], item["message"]))
    compile_inputs = _deduplicate_dicts(compile_inputs)
    cmake_inputs = _deduplicate_dicts(cmake_inputs)
    compile_units = _deduplicate_dicts(compile_units)
    build_targets = _deduplicate_dicts(build_targets)
    boards = _deduplicate_dicts(boards)
    relationships = _deduplicate_dicts(relationships)

    return {
        "boards": boards,
        "build_targets": build_targets,
        "compile_units": compile_units,
        "diagnostics": diagnostics,
        "files": files,
        "generator": {"name": GENERATOR_PATH, "version": GENERATOR_VERSION},
        "inputs": {
            "cmake_file_api": cmake_inputs,
            "compile_databases": compile_inputs,
        },
        "modules": make_modules(files),
        "relationships": relationships,
        "repository": {
            "file_count": len(files),
            "generated_file_count": sum(bool(entry["generated"]) for entry in files),
            "manual_file_count": sum(bool(entry["manual"]) for entry in files),
            "root": ".",
            "tracked_file_count": sum(entry["git_state"] == "tracked" for entry in files),
            "untracked_file_count": sum(entry["git_state"] == "untracked" for entry in files),
        },
        "schema": {"id": SCHEMA_ID, "path": SCHEMA_PATH, "version": SCHEMA_VERSION},
    }


def render_markdown(index: dict[str, Any]) -> str:
    repository = index["repository"]
    lines = [
        "<!-- Generated by tools/codebase_index.py. Do not edit by hand. -->",
        "# Codebase Index",
        "",
        f"Schema `{index['schema']['version']}`; generator `{index['generator']['version']}`.",
        "",
        "The index contains repository-relative paths only and deliberately omits generation timestamps.",
        "",
        "## Summary",
        "",
        "| Item | Count |",
        "| --- | ---: |",
        f"| Repository files | {repository['file_count']} |",
        f"| Git-tracked files | {repository['tracked_file_count']} |",
        f"| Git-untracked files | {repository['untracked_file_count']} |",
        f"| Manually maintained files | {repository['manual_file_count']} |",
        f"| Generated files | {repository['generated_file_count']} |",
        f"| Modules | {len(index['modules'])} |",
        f"| Build targets | {len(index['build_targets'])} |",
        f"| Compile units | {len(index['compile_units'])} |",
        f"| Boards | {len(index['boards'])} |",
        "",
        "## Inputs",
        "",
        "Repository files come from Git's tracked and non-ignored worktree views at `.`.",
        "",
        "| Build metadata | Kind | Status |",
        "| --- | --- | --- |",
    ]
    for item in index["inputs"]["compile_databases"]:
        lines.append(
            f"| `{item['path']}` | compile database | {item['status']} |"
        )
    for item in index["inputs"]["cmake_file_api"]:
        lines.append(f"| `{item['path']}` | CMake File API | {item['status']} |")

    lines.extend(
        [
            "",
            "## Modules",
            "",
            "| Module | Files | Kinds |",
            "| --- | ---: | --- |",
        ]
    )
    for module in index["modules"]:
        lines.append(
            f"| `{module['path']}` | {module['file_count']} | {', '.join(module['kinds'])} |"
        )

    lines.extend(["", "## Build Targets", ""])
    if index["build_targets"]:
        lines.extend(
            [
                "| Target | Type | Configuration | Repository sources | External sources |",
                "| --- | --- | --- | ---: | ---: |",
            ]
        )
        for target in index["build_targets"]:
            lines.append(
                f"| `{target['name']}` | {target['type']} | `{target['configuration'] or 'default'}` | {len(target['sources'])} | {target['external_source_count']} |"
            )
    else:
        lines.append("No CMake File API targets were available.")

    lines.extend(["", "## Boards", ""])
    if index["boards"]:
        lines.extend(["| Board | Source | Definition/build |", "| --- | --- | --- |"])
        for board in index["boards"]:
            location = board.get("definition_directory", board.get("build_directory", ""))
            lines.append(f"| `{board['name']}` | {board['source']} | `{location}` |")
    else:
        lines.append("No board definitions or configured Zephyr boards were available.")

    lines.extend(["", "## Files", ""])
    lines.extend(["| Path | Kind | Module | Ownership | Git |", "| --- | --- | --- | --- | --- |"])
    for entry in index["files"]:
        ownership = "generated" if entry["generated"] else "manual"
        lines.append(
            f"| `{entry['path']}` | {entry['kind']} | `{entry['module']}` | {ownership} | {entry['git_state']} |"
        )

    lines.extend(["", "## Diagnostics", ""])
    if index["diagnostics"]:
        for item in index["diagnostics"]:
            lines.append(f"- **{item['severity']} / {item['code']}:** {item['message']}")
    else:
        lines.append("No diagnostics.")
    lines.append("")
    return "\n".join(lines)


def _json_text(index: dict[str, Any]) -> str:
    return json.dumps(index, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def _write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(content, encoding="utf-8", newline="\n")
    temporary.replace(path)


def _matches(path: Path, expected: str) -> bool:
    try:
        return path.read_text(encoding="utf-8") == expected
    except FileNotFoundError:
        return False


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--build-dir",
        action="append",
        type=Path,
        default=[],
        help=(
            "Build directory to ingest; one or more values replace the canonical "
            "build/host-debug and build/zephyr-qemu_x86 inputs."
        ),
    )
    parser.add_argument("--compile-commands", action="append", type=Path, default=[])
    parser.add_argument("--cmake-reply", action="append", type=Path, default=[])
    parser.add_argument(
        "--json-output", type=Path, default=Path("build/metadata/codebase-index.json")
    )
    parser.add_argument(
        "--markdown-output", type=Path, default=Path("docs/_generated/CODEBASE_INDEX.md")
    )
    parser.add_argument(
        "--allow-degraded",
        action="store_true",
        help=(
            "Allow generation/check with missing or invalid required build metadata. "
            "The default fails closed; use this only for an explicitly degraded navigation report."
        ),
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check",
        action="store_true",
        help="Do not write outputs; fail when either existing output differs.",
    )
    mode.add_argument(
        "--prepare-cmake-file-api",
        action="store_true",
        help=(
            "Create codemodel-v2 queries in the selected build directories, then exit. "
            "Run this before the first CMake configure."
        ),
    )
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    options = parse_arguments(arguments)
    repo_root = options.repo_root.resolve(strict=True)
    if options.prepare_cmake_file_api:
        build_dirs = selected_build_directories(repo_root, options.build_dir)
        try:
            queries = prepare_cmake_file_api_queries(build_dirs)
        except OSError as error:
            print(f"Could not prepare CMake File API query: {error}", file=sys.stderr)
            return 2
        for query in queries:
            print(_safe_input_label(query, repo_root))
        return 0

    index = build_index(
        repo_root,
        build_dirs=options.build_dir,
        compile_database_paths=options.compile_commands,
        cmake_reply_paths=options.cmake_reply,
    )
    input_failures = required_build_input_failures(index)
    if input_failures and not options.allow_degraded:
        print(
            "Required build metadata is unavailable or invalid: "
            + "; ".join(input_failures)
            + ". Rebuild the selected inputs or pass --allow-degraded explicitly.",
            file=sys.stderr,
        )
        return 2
    json_content = _json_text(index)
    markdown_content = render_markdown(index)
    json_output = options.json_output
    markdown_output = options.markdown_output
    if not json_output.is_absolute():
        json_output = repo_root / json_output
    if not markdown_output.is_absolute():
        markdown_output = repo_root / markdown_output

    if options.check:
        stale: list[str] = []
        if not _matches(json_output, json_content):
            stale.append(_safe_input_label(json_output, repo_root))
        if not _matches(markdown_output, markdown_content):
            stale.append(_safe_input_label(markdown_output, repo_root))
        if stale:
            print("Codebase index is stale: " + ", ".join(stale), file=sys.stderr)
            return 1
        return 0

    _write_text(json_output, json_content)
    _write_text(markdown_output, markdown_content)
    print(_safe_input_label(json_output, repo_root))
    print(_safe_input_label(markdown_output, repo_root))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
