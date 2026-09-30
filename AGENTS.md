# Repository instructions

Before substantial work, read [the documentation index](docs/INDEX.md),
[current status](docs/09-progress/STATUS.md), and any relevant accepted ADRs.

- Preserve the authority boundary: the HMI CPU is a remote operator interface;
  the Control CPU owns furnace truth, operation, validation, and safety.
- State whether a claim is a fact, assumption, recommendation, or decision.
- Never silently change accepted architecture or protocol behavior. Use an ADR
  for consequential decisions and update the authoritative documents.
- Do not manually edit generated documentation. Use its owning generator.
- Only the designated GUI execution context may manipulate LVGL objects.
- Keep hardware selection and wiring in Zephyr devicetree, Kconfig, board
  configuration, drivers, and adapters rather than portable application code.
- Review actual verification output and report checks that were not run.
- Do not implement a request that clearly violates a requirement, invariant,
  ADR, C/Zephyr/LVGL correctness, concurrency/lifetime safety, protocol
  correctness, or recoverability. Identify the concrete issue, consequence,
  and governing evidence; propose viable alternatives; require resolution.
- Distinguish concrete errors and requirement conflicts from risks,
  trade-offs, and preferences. Do not block valid work over taste.

Substantial work follows [PLANS.md](PLANS.md). Detailed procedures belong in
the relevant documentation or repository skill, not in this file.

## Web/LVGL UI parity

When changing a user-visible web demo or LVGL UI behavior, implement and verify
the corresponding behavior in both interfaces in the same work slice. Treat
the web demo and host LVGL lab as two presentations of one operator workflow;
do not close a UI task with only one implementation. The only exception is a
demo-only simulator fixture or visualization that does not represent an HMI
screen, navigation rule, control, or user-visible behavior. Use the
[UI-parity skill](.agents/skills/ui-parity/SKILL.md) for the detailed workflow.
