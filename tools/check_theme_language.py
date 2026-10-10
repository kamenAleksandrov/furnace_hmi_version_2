"""Static guard rails for theme and language readiness.

Fails when HMI code writes a colour value instead of a theme token, or when the
LVGL explorer writes visible text instead of a catalogue string. The runtime
checks (probe theme and pseudo language in both presentations' smoke tests)
catch what a static scan cannot; this scan gives fast feedback first.

Checked:
  simulator/ui_lab/src/ui_lab_explorer.c, ui_lab_theme.c
      colour literals: lv_color_hex(0x...), lv_color_white/black/make(), lv_palette_*
      visible text: string literals with words (a capitalised word or a space)
  docs/images/theme/hmi-web-demo.html, HMI parts only (base style, soft-tile
  style up to the demo-page section, HMI script up to the demo-page section)
      colour literals: #rgb/#rrggbb/#rrggbbaa values, rgb()/rgba(), hsl()
      (alpha masks such as mask:linear-gradient(#000 0 0) are allowed)

A line that must keep a literal carries the marker "theme-lint: allow" or
"i18n-lint: allow" with a reason. Generated blocks are skipped.

Usage:
    python tools/check_theme_language.py
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
C_FILES = [ROOT / "simulator" / "ui_lab" / "src" / "ui_lab_explorer.c",
           ROOT / "simulator" / "ui_lab" / "src" / "ui_lab_theme.c"]
WEB_PAGE = ROOT / "docs" / "images" / "theme" / "hmi-web-demo.html"

C_COLOUR = re.compile(r"lv_color_hex\(\s*0[xX]|lv_color_(white|black|make)\s*\(|lv_palette_")
C_STRING = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
CSS_COLOUR = re.compile(r"(?<![\w&-])#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{3,4})\b|\b(?:rgba?|hsla?)\(")
JS_COLOUR = re.compile(r"['\"`]#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{3,4})['\"`]|\b(?:rgba?|hsla?)\(")
WORDS = re.compile(r"[A-Z][a-z]+|[A-Za-z]{2,} [A-Za-z]")


def strip_c_comments(text: str) -> str:
    """Blank out comments, keeping line structure and string literals."""
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append(re.sub(r"[^\n]", " ", text[i:end]))
            i = end
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
            out.append(" " * (end - i))
            i = end
        elif text[i] == '"':
            match = re.match(r'"(?:[^"\\\n]|\\.)*"', text[i:])
            literal = match.group(0) if match else text[i]
            out.append(literal)
            i += len(literal)
        elif text[i] == "'":
            match = re.match(r"'(?:[^'\\\n]|\\.)*'", text[i:])
            literal = match.group(0) if match else text[i]
            out.append(literal)
            i += len(literal)
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def check_c(path: Path, problems: list[str]) -> None:
    raw = path.read_text(encoding="utf-8").split("\n")
    code = strip_c_comments("\n".join(raw)).split("\n")
    in_static_assert = False
    for number, (line, original) in enumerate(zip(code, raw), start=1):
        if "theme-lint: allow" in original or "i18n-lint: allow" in original:
            continue
        if C_COLOUR.search(line):
            problems.append(f"{path.relative_to(ROOT)}:{number}: colour literal; use a theme role")
        # Build-time messages (_Static_assert, possibly over several lines) are not UI text.
        if "_Static_assert" in line:
            in_static_assert = True
        if in_static_assert:
            in_static_assert = ");" not in line
            continue
        if line.strip().startswith("#include"):
            continue
        for literal in C_STRING.findall(line):
            visible = re.sub(r"%[-+ #0-9.]*[a-zA-Z]+", "", literal)
            if WORDS.search(visible):
                problems.append(f"{path.relative_to(ROOT)}:{number}: visible text \"{literal[:40]}\"; "
                                "use a catalogue string")


def web_regions(lines: list[str]) -> list[tuple[int, int, str]]:
    def index(predicate, start=0):
        return next(i for i in range(start, len(lines)) if predicate(lines[i]))
    base = index(lambda l: l == "<style>")
    base_end = index(lambda l: l == "</style>", base)
    soft = index(lambda l: l.startswith("<style id='soft-tiles-theme'>"))
    soft_end = index(lambda l: "Demo page (outside the HMI rectangle): Telamorph" in l, soft)
    script = index(lambda l: l == "<script>")
    script_end = index(lambda l: l.startswith("// Demo page (outside the HMI rectangle)"), script)
    return [(base, base_end, "css"), (soft, soft_end, "css"), (script, script_end, "js")]


def check_web(problems: list[str]) -> None:
    lines = WEB_PAGE.read_text(encoding="utf-8").split("\n")
    generated = False
    for start, end, kind in web_regions(lines):
        for number in range(start + 1, end):
            line = lines[number]
            if ":begin (generated" in line:
                generated = True
            if generated:
                generated = ":end" not in line
                continue
            if "theme-lint: allow" in line:
                continue
            for match in (CSS_COLOUR if kind == "css" else JS_COLOUR).finditer(line):
                if kind == "css" and re.search(r"mask[^;{}]*$", line[:match.start()]):
                    continue
                problems.append(f"{WEB_PAGE.relative_to(ROOT)}:{number + 1}: colour literal "
                                f"\"{match.group(0)}\"; use a theme token")


def main() -> int:
    problems: list[str] = []
    for path in C_FILES:
        check_c(path, problems)
    check_web(problems)
    for problem in problems:
        print(problem, file=sys.stderr)
    if problems:
        print(f"{len(problems)} theme/language problems; see .agents/skills/theme-and-language/SKILL.md",
              file=sys.stderr)
        return 1
    print("theme/language lint: no colour literals or untranslated LVGL text in HMI code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
