# Execution plans

Use a written execution plan for architecture or protocol changes,
cross-module features, concurrency changes, runtime/platform migrations,
significant state or storage changes, and work likely to span sessions. A
trivial, isolated, reversible edit does not need one.

Plans are living records. Put project plans in `docs/09-progress/` or in the
issue/PR system when that is the durable work record. A plan should state:

1. outcome and non-goals;
2. governing requirements, invariants, and ADRs;
3. facts, assumptions, open decisions, and risks;
4. affected modules and ownership boundaries;
5. ordered implementation milestones with observable completion criteria;
6. verification, including failure and recovery cases;
7. documentation and generated-artifact updates;
8. rollback or migration approach where state or compatibility is affected.

Update progress as work proceeds. Record discoveries that change scope, and
close with results, deviations, verification evidence, and known follow-ups.
Do not use a plan to imply that an unresolved architecture decision is already
accepted; create or update an ADR first.
