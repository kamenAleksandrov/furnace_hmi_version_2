---
name: ui-parity
description: Implement and verify shared user-visible HMI behavior in both the browser demo and native LVGL lab, with an explicit demo-only simulator exception.
---

# UI parity

Use this skill whenever a change affects a user-visible HMI screen, route,
navigation rule, control, state presentation, keyboard, limit display, or
interaction in the web demo or native LVGL lab.

## Required outcome

The browser demo and native LVGL lab are paired implementations of the same
operator workflow. A UI change is incomplete until both interfaces implement
the behavior and both relevant verification paths pass. Keep layout-specific
adaptations where the viewport or widget system requires them, but preserve
the same labels, states, transitions, containment rules, and user-visible
meaning.

## Workflow

1. Read the governing HMI plan/concept and inspect both existing renderers
   before editing either one.
2. Write down the shared behavior: entry route, idle/running/cooling states,
   accepted/rejected/pending actions, limits, Back behavior, and any deliberate
   viewport differences.
3. Implement the web and LVGL changes in the same feature slice. Keep
   controller truth and safety ownership outside the HMI; browser and host
   fixtures may simulate responses but must not imply production authority.
4. Add or update interaction checks in both paths. For native work, include
   MSVC build, CTest semantic smoke, and geometry/capture checks at the review
   sizes. For web work, run the repository browser smoke and inspect the
   affected viewport states.
5. Compare the actual screens and transitions, then report any remaining
   parity gap as a concrete follow-up rather than calling the slice complete.

## Allowed exception

A change may remain simulator-only when it is strictly a demo fixture or
visualization and does not change an HMI screen, route, navigation rule,
control, keyboard, limit, or user-visible state. Document the exception in the
progress record and keep it out of production authority/protocol claims.

## Repository anchors

- Browser source: docs/images/theme/hmi-web-demo.html
- Native source: simulator/ui_lab/src/ui_lab_explorer.c
- Native lab entry/tests: simulator/ui_lab/src/main.c
- Shared operator concepts: docs/03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md
- Implementation plan: docs/09-progress/OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md
