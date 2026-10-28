# Native agent delegation policy

AGENT-OPT-01/02/03. The firmware copy is canonical; keep the HMI copy byte-identical
so each repository works independently. This supplements AGENTS.md, approved
requirements, ADRs, skills and review gates; it grants no product or safety approval.

## Decide before every delegation

1. Assess the exact task's complexity, consequences and uncertainty. Delegate
   only when independence, parallelism or context isolation outweighs another
   agent's instructions, context and coordination. Do small tasks directly;
   available concurrency is not a reason to spawn.
2. Check the authenticated client's subscription entitlement, included allowance
   and exposed model/effort choices. A catalogue or documented alias is not proof
   of entitlement. Unknown billing means stop affected inference, not a trial
   against a potentially paid service.
3. Prefer the least resource-intensive sufficiently capable included model and
   suitable modest reasoning for routine work; use greater capability for the
   risks that require it. Select model and effort explicitly where supported,
   using current choices rather than invented IDs or a permanent ranking.
4. If safe selection is unavailable, first do routine work in the main agent.
   Inheritance is a compatibility fallback, not the default reason to spawn
   another high-end instance. Use it only when delegation still has a concrete
   benefit or required independence, and included allowance is confirmed;
   record that reason. Never silently label inheritance as optimization.
5. For intended delegation, inspect the offered project roles; if unavailable,
   report the limitation. Do not invoke an agent merely to discover whether it
   loaded. Apply the assignment contract below, including workspace preflight.
6. Inspect actual returned evidence and integrate as supervisor. Verify the
   actual model/effort when exposed; distinguish requested, actual, inherited
   and unknown values. If scope or required capability grows, stop the bounded
   investigation, report why, and reassess scope/model/review before continuing.

## Match capability to the task

These are task categories, not fixed model tiers or rankings.

| Task | Examples | Selection and review |
| --- | --- | --- |
| Routine | Searches, requirements lookup, simple documentation consistency, log summaries, traceability lookup, straightforward exploration | Prefer an included lightweight choice and modest supported effort; otherwise usually work directly. Check references. |
| Intermediate | Bounded C17/HMI implementation, unit tests, substantial documentation edits, localized debugging, noncritical refactoring | Choose efficient capabilities that can reliably handle the actual state, language and acceptance checks. Strongest-model use is not automatic. |
| High consequence | Heater authorization, actuator inhibition, fault/sensor supervision, concurrency, persistent recovery, uncommanded heating, control safety, protocol authority | Use sufficiently capable reasoning and required independent/domain/human review, focused on changed behavior and its failure paths. Correctness takes priority over conserving allowance. |

A small HMI command change can be high consequence. Lower-capability work is
never the sole authority for safety-sensitive changes. No model approves a
safety allocation, product decision or regulatory conformity.

## Bounded assignment contract

Supply one task, not an invitation to rediscover the repository:

```text
Repository / worktree / branch / exact revision:
Workspace preflight and relevant governing records already checked:
Task and exact question:
Files / functions / requirement and WI IDs / changed diff:
Relevant prior review revision, conclusions and unresolved risks:
Expected concise output and acceptance evidence:
Allowed actions and explicit out-of-scope areas:
Escalate or stop when: missing authority, contradictory evidence, new risk,
  inadequate capability/allowance, or required investigation exceeds scope.
```

Use targeted searches and changed-file diffs. Read adjacent code when needed
to trace the relevant mechanism; if that materially expands the task, return
the evidence and request a revised scope. Never ignore a credible safety issue
because it lies outside the assigned paths. Stop when sufficient evidence
answers the question. Return actionable findings with file/line references,
uncertainties and checked/not-checked boundaries, not full files or raw logs.

The native roles remain narrow: repository-explorer investigates read-only;
implementation-worker implements an approved bounded contract test-first;
verification-analyst assesses evidence and gaps; engineering-reviewer inspects
actual source/tests for identified risks. Canonical skills retain engineering
instructions. HMI specialist roles and REVIEW_POLICY remain required.

## Reuse review evidence without weakening independence

- Record the prior reviewed revision, scope, assumptions, findings and checks.
  Reuse only conclusions still applicable to unchanged behavior. A previous
  review is neither blanket approval nor a reason to skip required reviewers.
- Implement and run targeted tests; independent reviewers inspect actual changed
  source/tests and the affected failure paths, not only a worker's summary.
- After fixing confirmed findings, rerun relevant regressions and request a
  bounded final review of corrections/new risks where justified. Do not restart
  a full architecture/phase review for a localized fix with no new such risk.
- Give mandatory specialists distinct questions. Overlapping additional reviews
  need a clear benefit. Expand review when changed interfaces, concurrency,
  protective behavior or invalidated assumptions introduce new risks.
- Qualified human engineering review and all existing safety/release gates
  remain mandatory; token conservation never waives them.

## Context and verification economy

Prefer fresh sessions for independent phases and fresh-context workers where
supported; pass committed plans and concise handoffs instead of old transcripts.
Do not copy successful build logs or whole documentation trees into every task.
Let local scripts perform mechanical checks and pass machine-readable summaries,
failing cases and exact evidence paths to the model. Load more only to resolve
an identified uncertainty. Avoid arbitrary hard token ceilings that could cut
necessary safety review; escalating an unexpectedly broad investigation is the
control. Do not run large multi-agent benchmarks for configuration validation.

Preserve repository verification gates. Reuse successful evidence for unchanged
scope; repeat a full gate only for a relevant change, failure, uncertainty or
an explicit repository gate (including the firmware pre-push gate), not because
another agent finished. Run one full gate per worktree at a time.

## Standing authorization and Git workflow (AGENT-OPT-02)

The owner authorizes autonomous native delegation, branch pushes and PR creation
for authorized development/planning tasks, without asking again. For each task:
inspect current state, latest main, open PRs and dependencies; use a named branch
and isolated worktree; implement the approved scope with tests; verify/review;
update canonical documentation, work-item evidence/maturity and dashboard only
where justified; commit all intended changes; inspect the final diff against the
correct base; push and open a separate PR for each affected repository.

Stack on unmerged dependencies. Open a draft with explicit blockers while
important issues remain; report failures and never imply readiness. Never merge,
force-push, rewrite shared history, delete unreviewed work or invent product,
architecture or safety decisions without explicit owner authorization.

## Subscription-only usage (AGENT-OPT-02/03)

Autonomous delegation is authorized; autonomous spending is prohibited.

- Use only the current authenticated subscription's included model allowance.
  Choose more capable included models when correctness requires them.
- No API-key/pay-as-you-go inference, purchased or extra-usage credits, paid
  overages, paid third-party providers or paid cloud services. Never change
  authentication, billing/account settings, buy credits or enable overages.
- At allowance exhaustion, stop affected inference and report it; do not switch
  providers or billing routes or keep retrying through paid fallback.
- If entitlement or paid-fallback status cannot be confidently established,
  finish unaffected local/static work and record the blocker. Catalogue entries,
  pricing estimates, a dollar budget flag or a role name do not establish it.
- Measure and report usage only when exposed reliably, with its scope; cached
  tokens, subscription allowance and monetary spend are different quantities.
  Do not invent savings or treat client list-price estimates as a subscription bill.

## Resource, worktree and permission boundaries

Keep at most two concurrent subagents and no nested delegation. Preserve one
agent per worktree; parallel tasks get separate worktrees beside the repository
under its workspace topology. The supervisor integrates; never race edits or
run duplicate implementations without a reason. Read-only smoke checks use a
stable snapshot while the supervisor waits; no concurrent writer on their inputs.

Prompts describe behavior; supported sandbox, permissions and tool allowlists
enforce access. Role labels do not create filesystem restrictions. Never relax
permissions to pass a test. Keep execution/verification local, with no mandatory
cloud CI. No heater energization, unauthorized flashing/physical tests, protective
bypass, unapproved architecture changes or unsupported maturity advancement.
Existing hardware blockers, phase sequencing, skills and safety rules still apply.

## Compact phase handoff

Report scope/work items and functionality, actual verification/benchmarks,
corrected defects, remaining risks/uncertainties/hardware dependencies, branch,
final commit, PR URL/status and unmerged dependencies, roadmap/maturity changes.
For delegation add tasks/roles, actual model/effort if exposed, explicit versus
inherited/unknown selection, reason for an inheritance exception, significant
findings, supervisor verification and failed/unavailable delegation. If none
was beneficial, say so. Keep raw logs out of the handoff; link exact evidence.

Client controls and observed limitations: [agent-client-compatibility.md](agent-client-compatibility.md).
