---
name: architecture-planning
description: Plan or review Furnace HMI V2 system boundaries, ownership, layering, concurrency, portability, and architecture decisions. Use for architecture proposals, ADR work, cross-module changes, or any request that could weaken the HMI and Control CPU authority boundary.
---

# Architecture Planning

## Start from recorded engineering memory

1. Read `docs/00-project/REQUIREMENTS.md`, `docs/01-architecture/BOUNDARIES_AND_INVARIANTS.md`, and `docs/01-architecture/STATE_OWNERSHIP.md`.
2. Read the relevant ADRs in `docs/07-decisions/` and the current questions in `docs/09-progress/OPEN_QUESTIONS.md`.
3. Classify each input as a project mandate, accepted decision, proposal, assumption, or open question. Do not silently promote one class into another.
4. Cite the requirement, invariant, or ADR IDs that constrain the work.

## Produce a defensible design

1. State the problem, scope, constraints, and excluded behavior.
2. Preserve these non-negotiable boundaries:
   - The Control CPU is the sole authority for furnace truth, validation, timing, control, actuators, interlocks, faults, and safety.
   - The HMI expresses operator intent, presents controller-observed state, and remains non-authoritative.
   - Loss, restart, delay, or corruption of the HMI must not make the furnace unsafe.
   - Only the designated GUI context may manipulate LVGL objects.
3. Separate reusable application concerns from Zephyr, board, display, touch, storage, and transport adapters. Use Zephyr-native device and configuration mechanisms at the platform boundary.
4. Compare realistic alternatives. For each, record consequences, evidence, unresolved inputs, and the verification needed to retire uncertainty.
5. Define bounded ownership, concurrency, resource, error, stale-state, restart, and test behavior before implementation.
6. Prefer the smallest architecture that preserves known future variation. Do not create abstractions for variation that has no evidence.

## Challenge conflicts precisely

When a request conflicts with an invariant or accepted decision, do not implement it quietly. Identify whether it is an error, risk, tradeoff, or preference; explain the consequence; cite the governing record; offer viable alternatives; and require an explicit decision for any boundary change.

## Record the outcome

Update the single canonical document for the decision. Create an ADR when alternatives and consequences matter, link affected requirements and tests, and update `docs/09-progress/STATUS.md` or `ROADMAP.md` only with verified current state. Do not add production furnace logic while performing architecture planning.
