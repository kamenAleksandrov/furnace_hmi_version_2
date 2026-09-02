# Roadmap

The foundation follows this ordered gate. A later phase may be prepared in
parallel, but it cannot claim completion before its prerequisites and evidence
exist.

| Phase | Outcome | Status on 2026-09-01 |
| --- | --- | --- |
| 1. Repository normalization | Inspect Git, normalize the Obsidian vault, add governance, establish documentation taxonomy | Complete |
| 2. Authoritative documentation | Encode requirements/invariants, V1 lessons, ADRs, questions, roadmap, and status | Complete foundation baseline |
| 3. Agent infrastructure | Add three narrow skills, six read-focused reviewers, `AGENTS.md`, and `PLANS.md`; validate non-overlap | Complete |
| 4. Zephyr 4.4.0 environment | Prepare isolated environment, document tooling, configure VS Code, verify | Complete for Windows `qemu_x86`; production board remains open |
| 5. Minimal Zephyr proof | Build a foundation-only Zephyr target and generate a compilation database | Complete |
| 6. Desktop LVGL foundation | Build and run a project-owned LVGL/SDL2 window and verify input/shareable/debug boundaries | Complete, including an MSVC/PDB `cppvsdbg` source stop |
| 7. Testing foundation | Prove portable host and Zephyr test paths and document exact commands | Complete |
| 8. Indexer V1 | Generate deterministic JSON and Markdown from repository/build metadata | Complete |
| 9. Foundation review | Run required broad reviewers and fix confirmed issues | Complete; no remaining blocker |
| 10. Foundation gate | Record files, environment changes, commands, results, risks, and questions, then stop | Complete; stopped before production work |
| 11. First UI vertical slice | Add a reusable fail-closed state-availability view, keep diagnostics separate, verify it visually/headlessly, and provide a one-command desktop operator workflow | Complete; no furnace semantics or actions introduced |

## Next delivery gate

The next step is an operator-workflow and information-architecture plan backed
by a deterministic semantic simulator. It should select one useful read-only
workflow before adding controls. Only the questions needed for that slice must
be answered: the first operator workflow to model, any V1 screen/field evidence,
and whether the 800 x 480 development viewport may remain the temporary design
target. Production hardware, transport, wire format, and the full question
register can remain deferred until their gates need them.

Any later command/control slice requires explicit domain semantics and review
under [PLANS.md](../../PLANS.md), while preserving the
[authority decision](../07-decisions/0001-authority-boundary.md).
