# Touch keyboard layouts and integration

Status: browser and host-only native implementation complete for review,
including the one-layout text keyboard, 2026-09-26.

## Intent and design

Current request: use one text-entry layout with digits at the top and letters
below, plus a separate numeric keypad. Home/Running retain their current layout
in this pass. Larger graph hit targets, current-temperature emphasis and a more
compact Split reading arrangement remain recommendations, not implemented changes.

The graphite keyboard panel covers the content area while retaining the
screen header and observation state. Underlying content/navigation is inert.
Both layouts have a field title, editable temporary value, validation feedback,
Cancel and Apply. Keys are at least 48 px high at the three review sizes.
The host-only native LVGL UI lab mirrors these layouts with bounded local
editing buffers. Embedded/on-device deployment and website publication remain
out of scope for this pass.

### Text keyboard

- One text layout with digits in the top row and QWERTY letters below, with no
  empty key slots (2026-09-26): `1`-`0`; `q`-`p`; `a`-`l` and `-`; Caps
  (1.5 keys wide), `z`-`m`, Delete (1.5 keys wide). The action row provides
  Space, caret movement, Clear, Cancel and Apply. Rows share the panel height, so
  keys grow above the 48 px minimum instead of leaving an empty band above them.
  There is no symbols page or layout toggle; numeric fields retain their
  separate keypad.
- Opening selects the current value so ordinary replacement needs no clearing.
  Selection replacement, physical typing/paste and caret editing work in the
  temporary field. Escape cancels; Enter applies when valid. Tab stays inside
  the keyboard while it is open.
- Initial program/stage names are nonblank and limited to 32 UTF-16 code units
  in the browser demo. Control characters are rejected; names are trimmed on
  Apply. Production UTF-8 byte/glyph limits and localization must be defined
  before a native port. This is not a claim of complete multilingual keyboard
  or IME support. No auto-correction changes engineering identifiers.
- Program rename affects only the browser catalogue. Stage rename affects only
  the current unsaved stage session until Save all changes.

### Numeric keypad

```text
7    8    9    Delete
4    5    6    Clear
1    2    3    +/-
0    .    <    >
Cancel        Apply
```

The keypad panel is 460 px wide, centred over the dimmed page, and its five rows
share the full panel height, giving roughly square-ish keys (about 104 x 56-62 px)
instead of wide 48 px slabs. Keys unavailable for the field's range or step
(sign, decimal) are dimmed rather than highlighted in both presentations.

The sign key toggles a leading minus and the decimal key inserts a decimal
point. These are numeric input controls, not an expression calculator. Fields
that cannot be negative disable sign; integer fields disable decimal. Physical
typing is checked by the same grammar/range/step validation. Empty input, lone
sign/decimal, exponent notation, repeated decimal points, nonfinite values,
out-of-range and off-step values cannot be applied. Intermediate invalid text
is retained for correction, not silently converted to zero or clamped.

The temporary numeric buffer is bounded to 24 code units. Units remain in the
field title, not inside the numeric value. Bound/step hints use the caller's
metadata. Current limits are explicitly demo bounds, not controller policy.

## Transactions and ownership

Settings values commit to the unsaved Settings form on Apply; Save preferences
is still separate. Stage values commit to the unsaved all-stage edit session;
Save all remains separate. Cancel changes neither destination. Rebuilding or
changing the originating view invalidates the keyboard without applying it.
Focus returns to the opening control when it still exists after cancellation.
Callbacks must never target discarded UI objects in a future native port.

Keyboards cannot open during an active demo run. This preserves Running
containment and Stop accessibility; the keyboard is not an alarm overlay or
an operational control. In production, HMI syntax checks never replace
controller validation. REQ-FUN-002, 010, 013, 014, 017; REQ-NFR-004, 006;
ARCH-INV-001, 005, 007, 008 and ADR-0001/0005 remain governing constraints.
No accepted architecture or protocol behavior changes.

## Implementation and later native plan

1. Browser layout and contextual routing in
   `docs/images/theme/hmi-web-demo.html` (complete).
2. Review full text, symbols, numeric and restricted integer layouts. External
   Show letter keyboard / Show numeric keypad controls permit standalone inspection
   without changing a real field (complete for author browser review).
3. Host-only native LVGL adapter in
   `simulator/ui_lab/src/ui_lab_explorer.c` selects the keyboard from field
   metadata, uses bounded editing buffers and caret behavior, maps existing
   LVGL fonts/symbols, and keeps one GUI owner (complete for host review).
   Transactional Save/Discard boundaries and numeric normalization remain
   explicit.
4. Verify native allocation/lifetime, physical touch, localized label widths,
   invalid/pasted values, repeated edits, focus, dismissal and restart behavior
   before claiming embedded support. Embedded board integration remains later
   work.

## Original verification record (superseded 2026-09-26)

Chrome browser review passed 15 layouts at 800 x 480, 800 x 640 and 1024 x 600:
letters, numbers, symbols, signed numeric and restricted integer entry.
Additional assertions passed selection replacement, Delete, length/range/
increment validation, Apply/Cancel, program/stage integration and invalidation
on render. The native MSVC UI-lab build and complete CTest suite pass at all
three review sizes, including keyboard geometry, program/stage name entry,
page switching, Shift state and numeric Apply behavior. The author inspected
actual browser captures; physical touchscreen and embedded-controller tests
remain pending.

Representative browser images:
- [Text keyboard](../images/theme/text-keyboard-800x480.png)
- [Numeric keypad](../images/theme/numeric-keypad-800x480.png)

## Documentation and rollback

Update index/status/session records with browser and host-only native evidence
and regenerate the owned codebase index. Run syntax, link and whitespace
checks. Rollback removes only the keyboard presentation adapters and their
local name-edit behavior; no persistent data/schema or controller
configuration is migrated. Embedded deployment, website publication and
broader keyboard/localization functionality remain later work.

Final regression checks passed: 90 Device views at three sizes, twelve restart
branches, dirty Settings navigation, local links, extracted JavaScript syntax,
host-only native UI-lab CTest at 800 x 480, 800 x 640 and 1024 x 600, index
generation/check and whitespace checks (existing CRLF notices only).

## 2026-09-26 layout refinement

The active text keyboard is the single digits-plus-letters layout described
above; the former symbol-page screenshots and cycling checks are historical.
Browser smoke verifies the digit row, absence of a symbols toggle, Caps entry,
and that text input remains open and intact while waiting. Native geometry and
the shared UI-lab smoke verify the corresponding LVGL path.
