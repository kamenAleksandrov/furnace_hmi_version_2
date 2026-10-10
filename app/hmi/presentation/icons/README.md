# HMI icon set

Original line icons for the furnace HMI, shared by the web demo and the LVGL
presentation. `svg/` is the single source of truth; do not copy icon
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
- **LVGL:** `tools/build_lvgl_icons.py` renders every `svg/*.svg` at 16, 18,
  20 and 22 px into 8-bit alpha (A8) images in `generated/hmi_icons.c`, with the
  `furnace_hmi_icon_t` enum and `furnace_hmi_icon_image()` lookup in
  `include/furnace_hmi/hmi_icons.h`. LVGL draws A8 images in their recolour
  colour, so the UI tints them with the button's text colour. Run it after
  adding or changing an icon; `--check` fails when the generated files are
  stale. It needs `resvg-py` and Pillow; builds do not.

**Decision (2026-09-30):** generated A8 images replace the icon-font plan
recorded earlier here. The icons are stroked, so an icon font would first need
a stroke-to-outline conversion toolchain (FontForge or similar).

## Support QR code

`images/support-qr.png` is the product owner's code for `https://telamorph.com/`
(with the logo in the middle). `tools/build_support_qr.py` turns it into one
200 px A8 image (`generated/support_qr.c`, `include/furnace_hmi/support_qr.h`,
built into this library) and a CSS mask in the web demo (between the
`support-qr` markers). Both draw it in the `qr-ink` theme colour on a
`qr-plate` background, so it stays dark on light in every theme. `--verify`
decodes the result (needs OpenCV); `--check` fails when an output is stale.
Replace the PNG and rerun the tool to change the link; keep
`UI_LAB_DEVICE_SUPPORT_VALUE` and the tool's `SUPPORT_URL` in step.

## Status

Used by the HMI web demo (`docs/images/theme/hmi-web-demo.html`) and the native
UI lab (buttons, navigation, rails, keyboard, pager). See the
[LVGL port plan](../../../../docs/09-progress/LVGL_SOFT_TILE_PORT_PLAN.md).
