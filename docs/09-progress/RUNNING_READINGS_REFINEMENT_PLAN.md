# Running readings and presentation parity

Current decision: show curing Elapsed and Remaining together on the same line;
the time-switch control is removed. Cooling elapsed and Est. remaining stay
separate in the Cooling summary. Keep graphite reading tiles for the other
readings.
Earlier time-control alternatives and verification below are historical and
superseded by the 2026-09-26 display refinement.
Power remains a single reading; its estimated provenance is not repeated as a
secondary caption in the presentation.

Scope: native host LVGL UI lab and standalone HTML concept. Preserve existing
keyboard, star, title and graph refinements. No controller, protocol or storage changes.
ADR-0001 remains governing: all operational values come from observed state;
unknown/stale time must not become a fabricated total. Total is the display-only
sum of current nonnegative elapsed and remaining observations, not a plan guarantee.

Milestones:
1. Inspect current native changes and synchronize browser presentation.
2. Show both time readings together without an interactive time control; retain
filled readings and live updates.
3. Check native builds, smoke and geometry at three sizes; browser interactions
at the same sizes. Check unknown and changing observations and no clipping.
4. Record evidence and regenerate documentation indexes.

Rollback: revert this presentation change; no persistent state migration.
Status: complete for the host-only UI lab and browser concept.

## Prior implementation record (superseded 2026-09-26)

- Native time toggles in Graph, Split and Furnace views. Full-graph readings
  retain filled graphite backgrounds; the wider time control has a transparent
  interior, a colored border and no trailing swap glyph. Power is shown without
  a separate Estimate caption.
- Remaining is initially selected; selection survives observation updates and
  view changes within the current UI instance. No preference is persisted.
- The full-graph current/target/power labels now refresh with observed values.
- Existing native and browser refinements were retained: five-point favourite
  star, vertically centered larger detail/load titles, opaque View stages,
  lifted graph axis labels, Delete, active upward Shift arrow, explicit ABC
  return and number/symbol cycling. Name inputs use text; time/temperature
  inputs use numeric keyboards.
- Native build and 10/10 CTest pass at 800x480, 800x640 and 1024x600. Each size
  produces 67 native frames with zero reported geometry errors. New assertions
  cover both toggle directions in all views, selection across updates, refreshed
  current temperature and unavailable remaining time producing an unknown total.
- Chrome: nine running layouts across three sizes, both time states, keyboard
  page cycling/Delete, and no reading-width overflow passed.

## Prior design comparison

| Option | Arrangement | Recommendation |
| --- | --- | --- |
| Filled readings | Transparent outlined time control, three graphite readings | Implemented; clear touch target and grouping |
| Shared strip | One graphite background, blue time region | A quieter alternative if tile gaps feel too strong |
| Time pill | Rounded time control beside plain readings | Lightest appearance, less consistent with existing rectangular controls |

The native time format is hours:minutes; the browser demo retains its minute
format. Total means the current elapsed plus remaining estimate, so it can
change as controller observations change. Unknown totals show `--:--` natively.

[Native remaining](../images/theme/running-time-native-800x480.png),
[native elapsed](../images/theme/running-elapsed-time-native-800x480.png),
[native unknown total](../images/theme/running-time-unknown-time-native-800x480.png),
and [browser elapsed](../images/theme/running-time-browser-800x480.png).

No embedded hardware, controller communication or website deployment was tested.


Full Chrome keyboard regression: 15 layouts at three sizes passed, including
selection replacement, deletion, limits, Apply/Cancel and field integration.
JavaScript syntax, documentation links, generated index check and whitespace
checks passed (existing CRLF normalization notices only).

## Current display verification (2026-09-26)

Native Graph view and browser Running show elapsed and remaining together on
one line, without a toggle or button-like time outline. Cooling retains its
separate elapsed and Est. remaining summary. Unavailable native remaining time
stays --:--. The browser smoke also verifies no horizontal time-reading
overflow.

The top yellow line remains visible with a 48-pixel full-width clickable area.
Native build and all 10 CTest checks pass at 800x480, 800x640 and 1024x600;
browser smoke passes keyboard entry, time layout and light-area assertions.
