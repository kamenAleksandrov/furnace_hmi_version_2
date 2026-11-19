# Native agent delegation policy

AGENT-OPT-01/02/03/04. The firmware copy is canonical; keep the HMI copy
byte-identical so each repository works independently (also
`.agents/delegation/`). This supplements AGENTS.md, approved requirements, ADRs,
skills and review gates; it grants no product or safety approval.

## Delegation ceiling (AGENT-OPT-04)

The owner picks the session model and its highest effort. Delegation goes only
down from there: a subagent uses the **same or a weaker model**, at an effort no
higher than the session's. A strictly weaker model may use up to `high` whatever
the session effort, because its tokens cost far less than the session model's;
`xhigh` and `max` are not delegated (no evidence of benefit, risk of long
loops). Fable is never a delegation target; the owner selects it explicitly for
a session.

- **Order** (weakest first, owner-maintained in `.agents/delegation/ceiling.json`):
  Claude `haiku < sonnet < opus < fable`; Codex `gpt-6-luna < gpt-6.1-terra <
  gpt-6.2-sol < gpt-6.2-astra`. When the owner adopts newer versions, update the
  file; Claude aliases follow the latest version automatically.
- **Session ceiling:** Claude reads the actual session model from the session
  transcript (else from the model a `SessionStart` hook recorded) and the
  effort from the hook input. When either cannot be read,
  the default ceiling applies: Opus (latest) at medium for Claude, gpt-6.2-astra
  at medium for Codex. The owner raises the effort in the session when needed.
- **No implicit inheritance:** a spawn must name its model, or use a role that
  pins one. Forks (which copy the whole conversation) are not used.
- **Enforcement:** Claude: a `PreToolUse` hook on the Agent tool denies spawns
  outside the ceiling and fails closed (`delegation_ceiling.py hook`). Codex has
  no spawn hook: each role pins its model/effort (a role file wins over spawn
  values) and unpinned spawns default to the weakest model at low effort. Do not
  spawn a Codex role pinned above the current session model.
  `delegation_ceiling.py check` (firmware `verify.sh docs`) keeps the pins,
  defaults and hook consistent.

Why: the owner's earlier "no ranking; choose per task" rule let subagents
inherit the session model. Phase 20d-4 workers ran Opus at high effort; one
read about 75 million cached tokens. Published measurements
([claude-model-router](https://github.com/Manuele-T/claude-model-router), small
runs, API prices as a proxy) found that left alone, Opus never delegated; agent-chosen
routing mostly picked Sonnet and often cost more; cheap readers coordinated and
checked by a strong session halved large review costs while finding more; on
small tasks delegation overhead exceeded the savings.

## Roles and defaults

| Role | Pinned (Claude / Codex) | Delegate | Never |
| --- | --- | --- | --- |
| repository-explorer | haiku medium / gpt-6-luna medium | Read-only sweeps: requirement, traceability and test lookups; docs-versus-code checks; module summaries | Decisions or conclusions on safety |
| verification-analyst | haiku medium / gpt-6-luna medium | Twister/verify/benchmark logs, `Verifies:` coverage, mutation survivor lists | Judging whether safety evidence is sufficient |
| implementation-worker | haiku high / gpt-6-luna high | A precise, contained change whose failing tests already exist; documentation edits; scripts | Heater authorization, actuator inhibition, fault/sensor supervision, interlocks, concurrency, persistent recovery, protocol authority |
| engineering-reviewer | sonnet high / gpt-6.2-sol high | A first pass that lists candidate risks for the session to check | The final or required review |
| HMI specialists (Codex) | gpt-6.2-sol high | Their REVIEW_POLICY questions, as a first pass | Replacing the session's own review |

Effort follows the work, not the model: readers at medium (lookups need
coverage more than depth), code and review first passes at high (reasoning
reduces rework the session must check). The published measurements ran at the
client's default effort and did not vary it, so these settings are a
judgement to confirm with `report`, not a measured optimum. A Claude
invocation may pass another model/effort within the ceiling (for example
`low` for a trivial lookup, or one escalation, below). Codex roles keep their pin.

## Decide before every delegation

1. **Delegate when the work is mostly reading:** a read-only sweep of roughly
   10,000 lines or 30 files or more that a script or one search cannot answer.
   Below about 2,000 lines, or when one script/`rg` answers it, work directly. In
   between, work directly unless the reading clearly dominates. (Measured: on a
   ~10k-line project the session alone was cheaper or as cheap; delegation paid
   from ~22k lines and most on 50k+. This repository has ~50k code lines and
   ~21k documentation lines.)
2. **Delegate implementation only** to the implementation-worker under the
   conditions in the table, with an expected change of more than a few lines;
   otherwise implement directly. Tests decide acceptance, then the session reviews
   the diff.
3. **High-consequence work stays with the session**, at the session model: the
   "never" column above, safety-relevant review and any change whose correctness
   tests cannot establish. Under the ceiling a subagent cannot be stronger, so the
   strongest reasoning available is the session itself. Qualified human
   engineering review remains mandatory.
4. **Escalate once:** if a result is wrong, empty or incomplete, retry once with
   the next stronger model still within the ceiling (Claude), then do it in the
   session or ask the owner. No loops.
5. **Check every citation:** cheaper models cited nonexistent lines (up to a
   fifth of Haiku's and a third of Sonnet's citations in the measurements above).
   Confirm each file/line before using a finding; record rejected citations.
6. Subscription entitlement and allowance remain preconditions (below). Inspect
   offered roles; if unavailable, report it. Do not invoke an agent merely to
   discover whether it loaded.

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

Canonical skills retain engineering instructions. HMI specialist roles and
REVIEW_POLICY remain required.

## Reuse review evidence without weakening independence

- Record the prior reviewed revision, scope, assumptions, findings and checks.
  Reuse only conclusions still applicable to unchanged behavior. A previous
  review is neither blanket approval nor a reason to skip required reviewers.
- Implement and run targeted tests; reviewers inspect actual changed
  source/tests and the affected failure paths, not only a worker's summary.
- After fixing confirmed findings, rerun relevant regressions and review the
  corrections/new risks where justified. Do not restart a full architecture/phase
  review for a localized fix with no new such risk.
- Give mandatory specialists distinct questions. Overlapping additional reviews
  need a clear benefit.
- Qualified human engineering review and all existing safety/release gates
  remain mandatory; conserving allowance never waives them.

## Context and verification economy

Prefer fresh-context workers; pass committed plans and concise handoffs instead
of old transcripts. Do not copy successful build logs or whole documentation
trees into every task. Let local scripts perform mechanical checks and pass
machine-readable summaries, failing cases and exact evidence paths to the model.
Avoid arbitrary hard token ceilings that could cut necessary safety review;
escalating an unexpectedly broad investigation is the control.

Preserve repository verification gates. Reuse successful evidence for unchanged
scope; repeat a full gate only for a relevant change, failure, uncertainty or
an explicit repository gate (including the firmware pre-push gate), not because
another agent finished. Run one full gate per worktree at a time.

## Measure, do not estimate

Re-pricing delegated tokens at the session model's rate overstated savings
(predicted 34-85%; measured from 30% more expensive to 75% cheaper). Report
actual per-agent model, effort and token counts:
`python3 .agents/delegation/delegation_ceiling.py report <session.jsonl>`
(Claude transcripts under `~/.claude/projects/`; decisions in the Git-ignored
`.claude/delegation-log.jsonl`). Codex usage comes from its native session
metadata. Token counts are client measurements, not a subscription bill; claim
savings only from comparable measured workflows.

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

- Use only the current authenticated subscription's included model allowance,
  within the delegation ceiling.
- No API-key/pay-as-you-go inference, purchased or extra-usage credits, paid
  overages, paid third-party providers or paid cloud services. Never change
  authentication, billing/account settings, buy credits or enable overages.
- At allowance exhaustion, stop affected inference and report it; do not switch
  providers or billing routes or keep retrying through paid fallback.
- If entitlement or paid-fallback status cannot be confidently established,
  finish unaffected local/static work and record the blocker. Catalogue entries,
  pricing estimates, a dollar budget flag or a role name do not establish it.

## Resource, worktree and permission boundaries

Keep at most four concurrent subagents (owner, AGENT-OPT-04) and no nested
delegation. Parallelism is for read-only sweeps split into disjoint parts, which
cover more ground in less time; it does not reduce total usage, so it never
justifies extra spawns. At most one writer per worktree, and subagents do not
run Twister, `verify.sh` or other long builds in parallel (WSL memory). Parallel
tasks get separate worktrees beside the repository under its workspace topology. The supervisor integrates; never race edits or
run duplicate implementations without a reason. Read-only smoke checks use a
stable snapshot while the supervisor waits; no concurrent writer on their inputs.

Prompts describe behavior; supported sandbox, permissions, tool allowlists and
the ceiling hook enforce it. Role labels do not create filesystem restrictions.
Never relax permissions or the ceiling to pass a test. Keep execution/verification
local, with no mandatory cloud CI. No heater energization, unauthorized
flashing/physical tests, protective bypass, unapproved architecture changes or
unsupported maturity advancement. Existing hardware blockers, phase sequencing,
skills and safety rules still apply.

## Compact phase handoff

Report scope/work items and functionality, actual verification/benchmarks,
corrected defects, remaining risks/uncertainties/hardware dependencies, branch,
final commit, PR URL/status and unmerged dependencies, roadmap/maturity changes.
For delegation add one line per spawn (role, model, effort, reason), hook
denials, rejected citations, measured usage where available, and supervisor
verification. If no delegation was beneficial, say so. Keep raw logs out of the
handoff; link exact evidence.

Client controls and observed limitations: [agent-client-compatibility.md](agent-client-compatibility.md).
