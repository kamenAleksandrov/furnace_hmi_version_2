# Device and Settings card pages plan

**Status (2026-10-01):** complete; results below.

## Outcome

Device and Settings sub-pages use rectangular cards, like the Device Overview
runtime tiles, in a grid that fills the page instead of long 48 px rows with
empty space below. Specifically, in both the web demo and the native LVGL lab:

1. A shared **card grid**: three columns (two on some pages), rows stretch to
   fill the height left above the footer. Action cards are tappable on their
   whole surface and carry the soft amber left edge; information cards have no
   edge; Device cards take the status edge (amber due, red fault, green up to
   date, neutral otherwise).
2. **Display & interaction** on one page (no More): a full-width Brightness
   card with a tappable ten-step gauge and -/+; Dim when idle (choice: Off, 1,
   2, 5, 10, 30 min); Reduced motion (On/Off); Default Running view (choice:
   Graph, Split, Furnace); Language, Units and Theme as information cards.
3. **Date & time**: a large clock card that opens a new **Set date & time**
   dialog (day, month, year, hour, minute, and AM/PM in 12-hour format, each
   with +/-; a weekday preview; Cancel / Set date & time), plus Time format and
   Clock source cards.
4. **Connections**: a wide Furnace controller card over Server and Bluetooth.
5. Settings **Maintenance**: four action cards in a 2 x 2 grid.
6. Device **Overview**: the three runtime cards plus Service attention,
   Errors and Last service cards.
7. Device **Maintenance** (finishes the item deferred on 2026-09-23,
   "Notifications becomes Maintenance ... detailed maintenance deferred"): all
   catalogue service tasks as status cards, worst first, six per page.
8. Device **Service history** as cards; **Device info** as six cards with no
   pager. Device log stays a list.

**Non-goals:** the light theme (requested for later), real brightness/dimming
hardware, a production clock contract (timezone, time source, permissions),
maintenance write actions, and changes to the Device log.

## Governing rules

- [ADR-0001](../07-decisions/0001-authority-boundary.md): the Control CPU owns
  the clock, service counters and task state. Set date & time is a request
  whose outcome the controller reports; the HMI never edits counters.
- [Settings proposal](../03-hmi/SETTINGS_SCREEN_PROPOSAL.md): date/time edits
  use a separate Set action, not the preference Save transaction; reduced
  motion suppresses decorative motion only; dimming must not blank the screen
  during operation or alerts.
- [Device proposal](../03-hmi/DEVICE_SCREEN_PROPOSAL.md) and proposed
  [ADR-0011](../07-decisions/0011-component-service-catalogue.md): viewing a
  task records nothing and clears nothing.
- AGENTS.md web/LVGL parity; the UI-parity skill.

## Facts, assumptions, decisions

- **Fact:** the web Device Maintenance tab is a placeholder ("will be connected
  ... later") while the native one lists due tasks from its catalogue; this
  plan closes that parity gap.
- **Decision (product owner, 2026-10-01):** cards instead of long rows; whole
  card tappable; brightness by a tappable gauge; dim delay as a few choices plus
  Off; a Set date & time dialog in both presentations; a light theme later.
- **Assumption (reversible):** the dim choices Off, 1, 2, 5, 10 and 30 min and
  a 10 % brightness step (10-100 %). Default stays 80 % and 5 min.
- **Assumption:** the simulated controller accepts every Set date & time
  request. The web clock runs from the browser clock plus the set offset; the
  native clock is a fixed simulated value that changes only when set.

## Affected modules

| Area | Change |
| --- | --- |
| `docs/images/theme/hmi-web-demo.html` | Card grid CSS and helper, Settings and Device pages, choice and date/time dialogs |
| `simulator/ui_lab/src/ui_lab_explorer.c/.h` | Card grid helper, the same pages and dialogs, simulated clock state |
| `app/hmi/presentation/strings` | New labels, captions, month and weekday names |
| `simulator/ui_lab/src/main.c` | Updated Settings/Device smoke checks and captures |
| `tools/hmi_web_demo_p1_smoke.cjs` | Updated Settings/Device checks |

## Milestones

1. Web: card grid, Settings pages, dialogs, Device pages. Done when the smoke
   test passes and captures at 800x480, 800x640 and 1024x600 show no clipping.
2. Native: the same. Done when MSVC CTest passes at the three sizes and
   captures match the web.
3. Documents: UX baseline, Settings and Device proposals, status, generated
   index.

## Verification

Web smoke; native MSVC CTest at 800x480, 800x640 and 1024x600 with captures
compared against the web. Failure cases: day clamped when the month or year
changes (31 Jan -> Feb), 12-hour midnight/noon, Cancel leaves the clock
unchanged, Discard restores brightness/dim/view, dirty-navigation guard still
fires, stale and unavailable Device pages still fit. Not covered: physical
touch, real backlight, real controller clock.

## Rollback

Revert the change. No stored data, protocol or controller behaviour changes.

## Results (2026-10-01)

All three milestones are done in both presentations.

**Deviations:**
- Paged card tabs (Maintenance, Service history) use two taller rows when
  there are four items or fewer, otherwise three rows; a partly filled last
  page keeps the card size.
- Device info values, task names, record titles and action names use 18 px
  SemiBold and may wrap to two lines; other values are 24 px on one line.
- Device info shows Model, HMI firmware, Controller firmware, Controller link,
  Component catalogue and Support; the identity and firmware values are
  illustrative fixtures (`Curing furnace`, `1.0.0`).
- The Connections Furnace controller card reads Connected, Connection lost
  (link lost or stale) or Unavailable; it is information only.
- Native: the settings numeric editor path (brightness and dim by keypad),
  the More page and their actions were removed; the web keeps its unreachable
  Program preferences code unchanged.

**Verification:** `node tools/hmi_web_demo_p1_smoke.cjs` passed, including new
checks for six Overview cards, Maintenance order and paging, six Info cards,
the gauge steps and -/+, the dim choice, Discard, the 31 Jan -> Feb clamp,
Cancel, Set and the 12-hour AM/PM column. Native MSVC builds of
`host-msvc-ui-lab-debug`, `-800x640-debug` and `-1024x600-debug` have no
project warnings; `ctest -E codebase_index` passed 9/9 for each, including the
new brightness, choice and Set date & time smoke checks and zero geometry
errors for the new captures (`settings-*`, `settings-set-date-time`,
`settings-dim-choice`, `device-overview`, `device-log`, `device-maintenance`,
`device-maintenance-2`, `device-history`, `device-info`). `tools.codebase_index`
still fails locally (MinGW, known). `tools/build_fonts.py --check` passes.
Not run: physical touch, Zephyr target build of the UI lab (host-only code).

**Follow-ups:** light theme; real brightness/dimming on the target; the
controller clock contract (timezone, time source, permissions).
