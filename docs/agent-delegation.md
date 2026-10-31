# Native agent delegation policy

AGENT-OPT-01, extended by AGENT-OPT-02 (standing authorization and
subscription-only usage). The firmware copy of this file is canonical; the HMI keeps an
identical, reviewed local copy so either repository works independently. Change
both copies together and compare their bytes. This policy supplements each
repository's AGENTS.md, requirements, accepted ADRs, skills and review gates.
It grants no product, hardware, installation or merge approval.

## Decide at each delegation

1. Delegate only when parallelism, independent review or isolating bulky output
   outweighs the extra context, tokens and coordination. Do small tasks directly.
2. Assess complexity, consequence, uncertainty and required capabilities using the
   task categories below; these describe tasks, not permanent model tiers.
3. Inspect the current client's exposed subagent tools and available choices.
   Account-visible choices or a successful invocation are evidence of availability;
   documentation and a bundled catalogue alone are not. Never invent a model ID.
4. Choose the most economical available model reasonably capable of the task,
   and a supported effort appropriate to it. If cost or capability is unknown,
   do not invent a ranking. Use supported inheritance/defaults when discovery or
   per-invocation selection is unavailable. Role selection is not model selection.
5. Complete the repository workspace preflight and pass its result. Give one bounded task: repository/worktree, approved requirement and WI IDs
   where applicable, input files, output/edit scope, acceptance checks, forbidden
   actions, evidence required and stop/escalation conditions. Pass needed context
   explicitly rather than assuming the worker sees the parent's conversation.
6. Inspect actual source, tests and command output; resolve contradictions and
   integrate as supervisor. Escalate incomplete or unreliable work to stronger
   available capabilities or human engineering review; never accept a claim on
   the strength of its model label.

## Match the task to the required capability

| Task | Examples | Selection and review |
| --- | --- | --- |
| Routine | Read-only exploration, requirements search, documentation, straightforward transformations, log summaries, roadmap checks | Prefer efficient lower-cost choices when exposed and suitable; check references and results. |
| Intermediate | Bounded C17 or HMI implementation, unit tests, serialization, localized debugging, noncritical refactoring | Choose capabilities that can reliably handle the language, state and test scope without unnecessary effort. |
| High consequence | Actuator inhibition, sensor faults, overtemperature, concurrency, persistent critical-fault recovery, control safety, protocol authority and arbitration | Use sufficiently capable reasoning, inspect the real source and failure paths, and apply rigorous independent review. Required qualified human review remains mandatory. |

Classify by consequence, not file extension: a small HMI command change may be
high consequence. A lower-capability worker is never the sole authority for a
safety-sensitive change. No agent can approve a safety allocation or conformity.

## Reusable roles

- **repository-explorer:** read-only source, requirement and test investigation;
  return exact file/symbol references, findings and uncertainty; no product decisions.
- **implementation-worker:** implement a bounded approved task with the existing
  test-first workflow; no independent safety or architecture changes.
- **verification-analyst:** inspect logs, benchmark/mutation evidence, traceability,
  negative cases and regression claims; identify weak assertions and missing proof.
- **engineering-reviewer:** read actual source and tests for concurrency, state,
  authority, faults and architecture; classify concrete findings and residual risks.

Keep engineering instructions in their canonical documents and skills. The HMI's
existing domain reviewers and REVIEW_POLICY remain applicable; these four roles
supplement them. Do not replace required reviewers with a generic review.

## Standing authorization (owner, AGENT-OPT-02, 2026-10-27)

The product owner authorizes and encourages autonomous native delegation and
pull requests for authorized development and planning tasks; no further
permission is needed for either. Merging is never included.

- **Delegation:** decide when delegation improves quality, efficiency or context
  management, and choose roles and capability as above. Keep high-consequence
  architecture, safety decisions and final integration with appropriately
  capable review. Before starting, confirm that the client loaded the project
  roles (they are read at session start); if they are unavailable, report it
  instead of silently ignoring this policy.
- **Branches and pull requests:** for each authorized task:
  1. check the repository state, latest `main`, active branches and unmerged
     dependencies;
  2. work on a named feature branch in its own worktree;
  3. implement with tests, verify and review;
  4. update requirements, documentation, work-item maturity, the progress
     dashboard and status where applicable;
  5. commit every intended change;
  6. check the final diff against the correct target branch, with no unrelated
     changes;
  7. push the branch and open a pull request when the work is ready for review.

  A phase built on an unmerged phase is a stacked pull request against that
  branch, never against an outdated baseline. Open a **draft** pull request,
  naming the blockers, while important issues remain; report failing tests or
  safety-critical defects prominently and never claim readiness. Changes to
  both repositories get separate branches and pull requests.
- **Never without explicit owner authorization:** merging a pull request,
  force-pushing or rewriting shared history, deleting branches or worktrees
  with unreviewed work, inventing product, architecture or safety decisions,
  changing approved scope or settled requirements, energizing or operating
  equipment, flashing unapproved hardware, bypassing protection or unapproved
  hardware experiments.
- **Final phase report:** implemented functionality and work items; tests,
  benchmarks and verification; defects found and corrected; remaining risks,
  uncertainties and hardware dependencies; branch, final commit and pull request
  URL with draft or ready status; dependencies on other unmerged pull requests;
  roadmap progress and maturity changes; and the delegation summary of the
  compact phase handoff below.

## Subscription-only usage (owner, AGENT-OPT-02, 2026-10-27)

Autonomous delegation is authorized; autonomous spending is prohibited. These
rules bind the supervising agent and every subagent.

1. Use only models available through the currently authenticated client's
   existing subscription entitlement; confirm availability where possible and
   never use a model merely because a catalogue lists it.
2. Prefer the least resource-intensive suitable model. Inheriting the parent's
   model is acceptable while it stays within the included allowance.
   Correctness comes first: use a stronger included model where safety-critical
   work requires it.
3. No API-key billing, pay-as-you-go inference, purchased or extra-usage
   credits, paid third-party model providers or paid cloud services. Never
   change the authentication method, enable paid overages, buy credits or
   switch billing accounts.
4. If billing status or entitlement is uncertain, do not invoke that model or
   service. If the subscription limit is reached, stop the affected activity
   and report it; never switch to paid usage. Any task that would need payment
   requires the owner's explicit approval first.
5. Report the models actually used and whether they were selected explicitly
   or inherited.

## Resource, worktree and permission boundaries

Use at most two concurrent subagents and no nested delegation. Preserve the
repository one-agent-per-worktree rule: give parallel tasks separate worktrees
beside the repository, following its workspace topology. The supervisor
integrates. Never race edits, repeat implementations without a reason, or run
duplicate full verification. Read-only smoke checks use a stable snapshot while
the supervisor waits; do not run a writer against their inputs.

Agent prompts describe behavior; filesystem permissions, sandbox and tool
allowlists enforce access. A role called read-only does not itself enforce it.
Use supported parent sandbox/permission controls for hard restrictions. Do not
relax permissions to make a smoke test pass; report denied or unavailable tools.

Keep execution and verification local; no mandatory cloud CI. Do not flash,
energize, physically test, bypass protection, change approved architecture,
advance work-item maturity without evidence, or merge PRs without the required
owner authorization and procedures. Existing safety, Git and verification rules
apply to every worker. This task authorizes agent configuration only; development
phase sequencing and hardware blockers stay unchanged.

## Compact phase handoff

Report delegated tasks and roles, actual model if exposed, explicit selection
versus inheritance/default (or unknown), significant corrections, supervisor
verification and failed/unavailable delegation. Report tokens or cost only from
reliable client measurements with their scope; no inferred savings. If the actual
model is hidden, say so. If nobody delegated, say that briefly.

Client-specific behavior, commands and observed evidence belong in
[agent-client-compatibility.md](agent-client-compatibility.md).
