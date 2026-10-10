# HMI string catalogue

All visible HMI text, for both presentations. Design record:
[ADR-0012](../../../../docs/07-decisions/0012-theme-and-language-catalogues.md);
rules: [theme-and-language skill](../../../../.agents/skills/theme-and-language/SKILL.md).

## Source

`catalogue/<tag>.json`, one file per language:

- `language`: `tag` (BCP 47, matches the file name) and `name` (the language's
  own name, shown in the Language choice).
- `strings`: id to text. `en.json` defines every id and is the fallback; other
  languages translate any subset.
- `notes` (optional): context for translators.

Text syntax: C printf conversions (`%s`, `%u`, `%ld`, `%02u`, `%%`); an
`<inttypes.h>` conversion is written `%<PRIi32>`. Strings whose id ends in
`_PATTERN` use `{field}`s that a language may reorder (dates, times).

## Generated

`tools/build_strings.py` writes (never edit these by hand):

- `include/furnace_hmi/ui_string_ids.h`: `furnace_hmi_ui_string_id_t`,
  `furnace_hmi_ui_language_t`.
- `generated/ui_strings_catalogue.c`: the tables.
- The `hmi-strings` block of `docs/images/theme/hmi-web-demo.html`
  (`HMI_LANGUAGES`, `HMI_STRINGS`).

It fails when a translation changes the printf conversions or their order,
drops or invents `{fields}`, uses characters the HMI font cannot draw, or
leaves an id unknown or empty.

```powershell
python tools/build_strings.py           # regenerate
python tools/build_strings.py --check   # fail if stale or invalid
```

## Use

- C: `furnace_hmi_ui_string(id)` (hand-written lookup in `src/ui_strings.c`),
  `furnace_hmi_ui_set_language()`, `furnace_hmi_ui_language_name()`;
  `furnace_hmi_ui_strings_override()` for tests and pseudo-localisation.
  GUI context only.
- Web: `t(id, ...args)`, `tp(id, {field})`, ids passed to `button()`/`tell()`;
  `setHmiLanguage(tag)`.

## Adding a language

Add `catalogue/<tag>.json`, run the generator, then review both presentations
at all sizes (text may be longer). Characters outside the HMI font need the
font subset extended first (`tools/build_fonts.py`). The Settings Language
card becomes a choice as soon as a second language exists.
