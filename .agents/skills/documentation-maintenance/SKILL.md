---
name: documentation-maintenance
description: Maintain Furnace HMI V2 engineering memory, requirements, ADRs, progress records, indexes, and generated documentation. Use whenever repository behavior or decisions change, documents move, traceability is updated, or generated metadata must be refreshed.
---

# Documentation Maintenance

## Find the canonical owner

1. Start at `docs/INDEX.md` and read `docs/_generated/README.md` before editing generated material.
2. Put project requirements in `docs/00-project/`, system boundaries and ownership in `docs/01-architecture/`, normative interface behavior in `docs/02-protocol/`, HMI design in `docs/03-hmi/`, the controller-facing boundary in `docs/04-controller/`, platform evidence in `docs/05-platform/`, verification strategy in `docs/06-testing/`, and decisions in `docs/07-decisions/`.
3. Keep volatile status, roadmap, questions, and session evidence in `docs/09-progress/`. Link to canonical facts instead of copying them.

## Preserve document integrity

1. Distinguish mandates, accepted decisions, proposals, assumptions, open questions, and generated observations.
2. Use stable IDs only for traceable requirements, invariants, protocol requirements, hazards, and tests. Link ADRs and tests to the IDs they influence or verify.
3. Use ordinary relative Markdown links that work in both Git hosting and Obsidian.
4. Preserve useful shared Obsidian settings, but never track user workspace state.
5. Give every generated file a generator name, schema or generator version, source inputs, and a no-manual-edit warning. Never hand-edit generated claims.
6. Avoid timestamps in deterministic generated artifacts unless time is itself a required input.

## Close each maintenance pass

Run the repository link, metadata, and index validation commands documented in `README.md`. Regenerate derived indexes from their authoritative inputs. Update `STATUS.md`, `ROADMAP.md`, and `SESSION_LOG.md` with verified outcomes rather than planned claims. Record unresolved conflicts in `OPEN_QUESTIONS.md`; do not settle engineering decisions through incidental documentation edits.
