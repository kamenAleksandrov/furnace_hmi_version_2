# HMI themes

`themes.json` is the single source of HMI colours for both presentations (the
LVGL UI and the web demo). Design record:
[ADR-0012](../../../../docs/07-decisions/0012-theme-and-language-catalogues.md);
rules: [theme-and-language skill](../../../../.agents/skills/theme-and-language/SKILL.md).

## Source

- `tokens`: every colour role, each with a `doc` saying where it is used.
  Name the role, not the look (`status-due`, not `orange`).
- `themes`: one entry per theme with an `id` (also the web `data-hmi-theme`
  value), a `name_string` catalogue id for its display name, and a value for
  every token: `#rrggbb`, or `rgba(r,g,b,a)` for a colour with its own
  opacity. Shadows, hairlines, glows and scrims are tokens too, so a light
  theme can use dark hairlines and softer shadows.
- `contrast`: text/background token pairs with a minimum WCAG ratio that every
  theme must meet.

## Generated

`tools/build_theme.py` writes (never edit these by hand):

- `include/furnace_hmi/hmi_theme.h`: `furnace_hmi_color_role_t`,
  `furnace_hmi_theme_id_t`, `furnace_hmi_color_t`, `furnace_hmi_theme()`.
- `generated/hmi_theme.c`: the palettes (portable data, no LVGL).
- The `hmi-theme` block of `docs/images/theme/hmi-web-demo.html`: CSS custom
  properties per theme and the theme/token lists for the page script.

```powershell
python tools/build_theme.py           # regenerate
python tools/build_theme.py --check   # fail if stale, invalid or below contrast
```

## Use

- LVGL: `ui_lab_theme_select(id)` selects a theme; colours come from
  `ui_lab_color(role)` / `ui_lab_opa(role)` at use time;
  `furnace_hmi_ui_lab_explorer_refresh_presentation()` restyles what is on
  screen. GUI context only.
- Web: CSS uses `var(--token)`; drawing code uses `themeColor('token')`;
  `setHmiTheme(id)` switches.

## Adding a theme

Add a `themes` entry with every token, run the generator, and review both
presentations at all sizes. The Settings Theme card becomes a choice as soon
as a second theme exists.
