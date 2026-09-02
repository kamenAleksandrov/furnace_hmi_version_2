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
