# ADR-0012: Generated theme and string catalogues shared by both presentations

- Status: Proposed
- Date: 2026-10-05
- Owners: HMI presentation
- Governing requirements: REQ-FUN-013; [STATE_OWNERSHIP](../01-architecture/STATE_OWNERSHIP.md)
  (language, theme and UI preferences are HMI-owned); [ADR-0005](0005-lvgl-single-owner.md);
  [ADR-0008](0008-memory-policy.md) (deferred)

## Context

The product owner requested that themes and languages be switchable and that
the HMI be coded for easy switching, ahead of designing a light theme.
Survey facts (2026-10-05): colours were partly tokens and partly literals in
both presentations; the LVGL palette was compile-time; the LVGL tone table,
hairlines and shadows assumed a dark page; the C string catalogue was one
hand-written English table and the web demo had its own English literals;
the web chose button icons by matching English labels; native Device status
was chosen by comparing colour values. REQ-FUN-013 already requires a
localization boundary.

## Decision drivers

- One source per concern, consumed by both presentations (parity rule).
- A new theme or language is data plus review, not code changes.
- Runtime switching on the target without per-theme assets or extra RAM.
- Mistakes are caught by tools, not by review alone.

## Options considered

- **Hand-maintained palettes and tables per presentation.** No tooling, but
  two copies drift; this is the situation that produced the survey findings.
- **Runtime-loaded theme and language files** (e.g. from storage). Flexible,
  but needs the not yet designed storage adapter, parsing in the GUI path and
  failure handling; premature before ADR-0008.
- **Proposed: data sources compiled in by generators.** JSON sources,
  generated C tables and a generated web copy, checked by `--check` in CTest.

## Decision (proposed)

1. Colours: `app/hmi/presentation/theme/themes.json` (named tokens with RGBA
   values, every theme defines every token, WCAG contrast pairs) generates
   the C module `furnace_hmi_theme` and the web CSS custom properties
   (`tools/build_theme.py`). LVGL resolves colours through the active palette
   at use time; switching rebuilds the visible screen in the GUI context.
2. Text: `app/hmi/presentation/strings/catalogue/<tag>.json` (English defines
   every id; other languages translate subsets and fall back to English)
   generates the C ids and tables and the web copy (`tools/build_strings.py`).
   The generator enforces printf conversion order, `{field}` patterns, the
   one-character decimal separator, and font glyph coverage.
3. Locale conventions (separators, date/time order, keyboard rows) are
   catalogue strings.
4. Theme and language are display preferences in both presentations (preview
   while edited, Save/Discard/Reset like the others); the cards become choices
   when a second option exists. Persistence across restarts waits for the HMI
   storage adapter.
5. Guard rails: `tools/check_theme_language.py` (static) and probe-theme and
   pseudo-language smoke checks in both presentations.

## Consequences

- Memory: one palette is about 100 colour entries (5 bytes each); a language
  is one pointer table plus its text. No duplicated styles, images or fonts.
  Switching costs one screen rebuild.
- Text formatting stays printf-based: a language that needs another argument
  order must rephrase, or the string becomes a `{field}` pattern.
- Fonts: the HMI font is subset to ASCII and a few symbols and upstream
  Tomorrow has no Cyrillic or Greek; non-Latin languages need a font decision
  (open question).
- Data text from outside the HMI (device catalogue text, controller-reported
  names) is not translated by this mechanism; see the follow-ups.
- Reversible: the generators and modules can be replaced without protocol or
  stored-data changes.

## Verification and follow-up

Evidence: [readiness plan](../09-progress/THEME_AND_LANGUAGE_READINESS_PLAN.md)
results (pixel comparisons, CTest, web smoke). Before acceptance: domain and
decision-challenger review; target language list; font strategy for
non-Latin scripts; storage of the preference; device-catalogue text
localisation (ADR-0011 follow-up).

## Review record

Not yet reviewed.
