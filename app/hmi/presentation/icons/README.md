# HMI icon set

Original line icons for the furnace HMI, shared by the web demo and (later) the
LVGL presentation. `svg/` is the single source of truth; do not copy icon
markup into other files by hand.

## Drawing rules

- 24 x 24 `viewBox`, keep strokes inside 2..22.
- `fill="none"`, `stroke="currentColor"`, `stroke-width="2"`, round caps and
  joins. A filled variant (for example `star-filled`) sets `fill="currentColor"`
  on its own shapes.
- One colour only. The icon takes its colour from the surrounding text, so the
  theme's amber, red, yellow and steel roles apply without separate files.
- File name is the icon name (`trash.svg` -> `trash`). Lower-case, hyphenated.

## Where the icons are used

- **Web demo:** `tools/build_icon_sprite.py` embeds every `svg/*.svg` as an
  inline sprite between the `icon-sprite` markers in the demo HTML. Run it after
  adding or changing an icon; `--check` fails when the embedded copy is stale.
  The page draws an icon with `<svg class="icon"><use href="#i-NAME"/></svg>`.
- **LVGL (not yet built):** convert the same files into an icon font (one
  private-use code point per icon), then generate the C font with LVGL's
  `lv_font_conv` at the sizes the UI uses. Icons then behave like text: styled
  colour, alignment inside buttons, no per-colour images.

## Status

Used by the HMI web demo (`docs/images/theme/hmi-web-demo.html`). The LVGL
port has not been built yet.
