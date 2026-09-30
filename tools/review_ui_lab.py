"""Build/test native LVGL review sizes and publish an ignored capture gallery.

Uses the visual-geometry CTest's actual SDL framebuffer captures. No desktop
capture, simulated browser UI, controller connection, or source modification.
Pillow is needed only to encode the native BMP captures as reviewable PNGs.
"""
from __future__ import annotations

import hashlib
import html
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PRESETS = {
    "800x480": "host-msvc-ui-lab-debug",
    "800x640": "host-msvc-ui-lab-800x640-debug",
    "1024x600": "host-msvc-ui-lab-1024x600-debug",
}


def run(command: list[str], log: Path) -> None:
    print(" ".join(command), flush=True)
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        print(log.read_text(encoding="utf-8", errors="replace")[-12000:])
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}")


def main() -> int:
    from PIL import Image
    report = ROOT / "build/graphite-review/final"
    report.mkdir(parents=True, exist_ok=True)
    source_paths = sorted(path for folder in ("simulator/ui_lab", "simulator/semantic_controller", "app/hmi/presentation/strings")
                          for path in (ROOT / folder).rglob("*") if path.is_file())
    source_paths += [ROOT / "simulator/desktop/lv_conf.h", ROOT / "CMakePresets.json"]
    source_hash = hashlib.sha256()
    for path in source_paths:
        source_hash.update(path.relative_to(ROOT).as_posix().encode())
        source_hash.update(path.read_bytes())
    manifest = {"source_sha256": source_hash.hexdigest(), "virtual_tick_seconds": 0,
                "capture_origin": "LVGL owner / SDL app renderer", "viewports": {}}
    sections = []
    for size, preset in PRESETS.items():
        destination = report / size
        destination.mkdir(exist_ok=True)
        run(["cmake", "--preset", preset], destination / "configure.log")
        run(["cmake", "--build", "--preset", preset], destination / "build.log")
        run(["ctest", "--preset", preset, "--verbose"], destination / "tests.log")
        transcript = (destination / "tests.log").read_text(encoding="utf-8", errors="replace")
        results = dict(re.findall(r"GEOMETRY_RESULT (\S+) (\d+)", transcript))
        if not results or any(int(value) for value in results.values()):
            raise RuntimeError(f"Missing or failed geometry evidence: {size}")
        origin = ROOT / "build" / preset / "simulator/ui_lab/visual-captures"
        executable = ROOT / "build" / preset / "simulator/ui_lab/Debug/furnace_hmi_ui_lab.exe"
        frames = []
        for name in results:
            source = origin / f"{name}.bmp"
            with Image.open(source) as bitmap:
                if bitmap.size != tuple(map(int, size.split("x"))):
                    raise RuntimeError(f"Wrong native dimensions: {source}")
                bitmap.save(destination / f"{name}.png")
            frames.append({"name": name, "geometry_errors": 0, "image": f"{size}/{name}.png"})
        manifest["viewports"][size] = {"preset": preset, "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(), "frames": frames}
        sections.append(f"<h2>{size}</h2>" + "".join(f'<figure><a href="{frame["image"]}"><img src="{frame["image"]}" width="400"></a><figcaption>{html.escape(frame["name"])}</figcaption></figure>' for frame in frames))
        print(f"{size}: {len(frames)} native captures; zero geometry errors", flush=True)
    (report / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    (report / "index.html").write_text('<!doctype html><meta charset="utf-8"><title>Native Graphite review</title><style>body{background:#171b1e;color:#f1f3f2;font:16px sans-serif}figure{display:inline-block;margin:8px}img{height:auto}figcaption{padding:8px}</style><h1>Actual LVGL captures</h1><p>Click any image for native resolution. Deterministic simulated data; software evidence, not operator or hardware approval.</p>' + "".join(sections), encoding="utf-8")
    print(report / "index.html")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, ImportError) as error:
        print(error, file=sys.stderr)
        raise SystemExit(1)
