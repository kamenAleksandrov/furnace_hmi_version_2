#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Check the pinned copy of the controller's wire codec (ADR-0009).

  sync_controller_wire.py --check
      Every copied file matches its hash in CONTROLLER_SYNC.json, and nothing
      copied is missing.
  sync_controller_wire.py --against <furnace-firmware checkout>
      In addition, every copied file equals the controller's file at the
      manifest's revision (read with git, whatever the checkout's HEAD), and the
      generated vector table equals the one the controller's script emits there.
  sync_controller_wire.py --sync <furnace-firmware checkout> --revision <rev>
      Copy the files from that revision and rewrite the manifest. Review the
      diff; a sync is one reviewed change naming the controller revision.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "app" / "hmi" / "protocol" / "wire" / "CONTROLLER_SYNC.json"
GENERATED = "tests/conformance/wire/wire_vectors_gen.c"

# HMI path -> controller path. The generated table comes from the controller's
# scripts/wire_vectors.py --emit-c at the same revision.
FILES = {
    "app/hmi/protocol/wire/include/furnace/crc32.h": "core/crc/include/furnace/crc32.h",
    "app/hmi/protocol/wire/src/crc32.c": "core/crc/src/crc32.c",
    "app/hmi/protocol/wire/include/furnace/wire.h": "core/wire/include/furnace/wire.h",
    "app/hmi/protocol/wire/include/furnace/wire_messages.h": "core/wire/include/furnace/wire_messages.h",
    "app/hmi/protocol/wire/src/wire_frame.c": "core/wire/src/wire_frame.c",
    "app/hmi/protocol/wire/src/wire_messages.c": "core/wire/src/wire_messages.c",
    "tests/conformance/wire/vector_table.h": "tests/core/wire/src/vector_table.h",
    GENERATED: "scripts/wire_vectors.py --emit-c",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def normalized(data: bytes) -> bytes:
    """Line endings never make a difference (a Windows checkout may convert them)."""
    return data.replace(b"\r\n", b"\n")


def load_manifest() -> dict:
    return json.loads(MANIFEST.read_text(encoding="utf-8"))


def controller_file(checkout: Path, revision: str, path: str) -> bytes:
    return subprocess.run(["git", "-C", str(checkout), "show", f"{revision}:{path}"],
                          check=True, capture_output=True).stdout


def controller_table(checkout: Path, revision: str) -> bytes:
    """The vector table the controller's own script emits at that revision."""
    with tempfile.TemporaryDirectory() as tmp:
        tree = Path(tmp) / "tree"
        subprocess.run(["git", "-C", str(checkout), "worktree", "add", "--detach", str(tree),
                        revision], check=True, capture_output=True)
        try:
            out = Path(tmp) / "table.c"
            subprocess.run([sys.executable, str(tree / "scripts" / "wire_vectors.py"),
                            "--emit-c", str(out)], check=True, capture_output=True)
            return out.read_bytes()
        finally:
            subprocess.run(["git", "-C", str(checkout), "worktree", "remove", "--force",
                            str(tree)], check=False, capture_output=True)


def check(manifest: dict) -> list[str]:
    errors = []
    listed = {entry["path"]: entry for entry in manifest["files"]}
    if set(listed) != set(FILES):
        errors.append(f"manifest lists {sorted(listed)}, expected {sorted(FILES)}")
    for path, entry in listed.items():
        local = ROOT / path
        if not local.is_file():
            errors.append(f"{path}: missing")
        elif sha256(normalized(local.read_bytes())) != entry["sha256"]:
            errors.append(f"{path}: differs from the manifest (edited in the HMI?)")
    return errors


def against(manifest: dict, checkout: Path) -> list[str]:
    errors = []
    revision = manifest["controller_revision"]
    for path, source in FILES.items():
        theirs = (controller_table(checkout, revision) if path == GENERATED
                  else controller_file(checkout, revision, source))
        if normalized(theirs) != normalized((ROOT / path).read_bytes()):
            errors.append(f"{path}: differs from the controller's {source} at {revision}")
    return errors


def sync(checkout: Path, revision: str) -> None:
    full = subprocess.run(["git", "-C", str(checkout), "rev-parse", revision], check=True,
                          capture_output=True, text=True).stdout.strip()
    entries = []
    for path, source in FILES.items():
        data = normalized(controller_table(checkout, full) if path == GENERATED
                          else controller_file(checkout, full, source))
        (ROOT / path).parent.mkdir(parents=True, exist_ok=True)
        (ROOT / path).write_bytes(data)
        entries.append({"path": path, "source": source, "sha256": sha256(data)})
    manifest = {
        "controller_repository": "kamenAleksandrov/furnace-firmware-v2",
        "controller_revision": full,
        "wire_format": "docs/interfaces/controller-wire-format.md",
        "note": "Pinned copy (ADR-0009). Do not edit the copied files; change the "
                "controller first, then sync.",
        "files": entries,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true")
    mode.add_argument("--against", type=Path)
    mode.add_argument("--sync", type=Path)
    parser.add_argument("--revision")
    args = parser.parse_args()
    if args.sync is not None:
        if not args.revision:
            parser.error("--sync needs --revision")
        sync(args.sync, args.revision)
        print(f"synced from {args.revision}; review the diff")
        return 0
    manifest = load_manifest()
    errors = check(manifest)
    if args.against is not None and not errors:
        errors = against(manifest, args.against)
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    if errors:
        return 1
    where = f" and equal to the controller at {manifest['controller_revision'][:12]}" \
        if args.against is not None else ""
    print(f"controller wire copy: {len(FILES)} files match the manifest{where}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
