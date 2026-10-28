# Native agent client compatibility

AGENT-OPT-03; checked 2026-10-28 against installed Windows Codex CLI
0.164.0-alpha.11.1 and Claude Code 2.1.300 and current official documentation.
Earlier AGENT-OPT-01/02 results below remain dated historical evidence, not proof
of current model entitlement. Firmware development/verification stays in WSL;
Windows native-client smoke tests use isolated committed Windows worktrees.

## Claude Code

Project Markdown/YAML roles retain `model: inherit` only as a compatibility
fallback, with no effort pin. Choose current supported aliases and effort on
the native Agent/Task invocation when the account includes them. If safe
selection is unavailable, do a small routine task directly rather than spawn
another inherited high-end session. No permanent family preference is encoded.

The installed client supports per-invocation model aliases and effort. It has
no verified general account-model enumeration tool for the supervisor; inspect
current authenticated choices/entitlement instead of deriving access from docs.
Local/managed settings can override requests; check native traces/task metadata.
Fresh sessions load the project role directory; creating that directory after
session start may require a restart. See [subagents](https://code.claude.com/docs/en/sub-agents)
and [model configuration](https://code.claude.com/docs/en/model-config).

Read-only roles still allow Read/Glob/Grep; the implementation worker also has
writing/shell tools and needs its own assigned worktree. Limits remain two
children and one spawn layer. `DISABLE_EXTRA_USAGE_COMMAND=1` in project settings
hides the purchase/usage-credits command. It does **not** disable server-side
overages or prove a model is included: check subscription auth, allowance and
paid-fallback state before inference; stop if uncertain. No authentication or
account billing setting is changed. See [environment variables](https://code.claude.com/docs/en/env-vars)
and [authentication](https://code.claude.com/docs/en/authentication).

## Codex

Standalone TOML roles retain name, description and developer_instructions;
model/effort are omitted so they cannot override runtime choices. Native spawn
model/effort controls depend on the current client. Use a fresh-context worker
where supported, rather than carrying the full transcript; some full-history
fork modes require inheritance. Do not silently equate role selection with
model selection. For routine fallback, apply the direct-work decision first.

The installed native app-server has read-only account/read,
account/rateLimits/read and model/list methods. Account plan, included allowance
and paid-credit/fallback state must accompany current offered model choices;
a bundled/debug catalogue alone is not entitlement. These are diagnostic native
capabilities, not a new model-discovery service or an automatic routing framework.
See [app-server](https://learn.chatgpt.com/docs/app-server) and
[authentication](https://learn.chatgpt.com/docs/auth).

Native configuration still enables agents and caps concurrent children at two;
no model or reasoning default is added. Custom-agent model settings, if added,
would override runtime resolution, which is why these files leave them unset.
Permissions/provider/MCP remain inherited from the parent; a role cannot create
its own read-only filesystem sandbox. Preserve parent controls and never relax
them to pass a test. See [subagents](https://learn.chatgpt.com/docs/agent-configuration/subagents).

## Efficient harmless validation

One small read-only delegation per client is enough for this policy change.
Use a fresh parent and one project repository-explorer, at a committed revision,
with one policy question, named files, concise output and explicit exclusions.
Choose an included efficient option and appropriate effort at invocation where
supported; otherwise record inheritance and the reason this invocation check
cannot be replaced by doing the task directly. Do not benchmark, exhaust quota,
use paid fallback or impose a hard token ceiling that could truncate review.

Inspect auth/provider and allowance before inference; no API-key inference or
billing/auth changes. A currency budget flag is not proof of subscription-only
execution. Inspect native role/spawn/result/model/effort metadata, compare the
response to the actual assigned lines, and check Git status/production paths.
If unavailable, exhausted or entitlement/paid fallback cannot be established,
record the blocker and finish static checks without trying a paid route.

## AGENT-OPT-03 scope and execution record (2026-10-28)

Configuration/documentation only, supporting WI-L01 with no maturity or phase
advance. Governing records: AGENTS.md, shared delegation policy, standing
AGENT-OPT-02 authorization and HMI REVIEW_POLICY. Inspect clients and dependencies;
update the identical policies and narrow role guidance; validate static controls
and one native delegation per client if eligible; run repository gates; publish
separate PRs without merging. Rollback is a normal Git revert with no runtime
state migration. No new hardware fact or blocker is introduced.

Previous inefficiencies: the owner's reported 133,000-token and combined
298,000-token reviews are observations, not independently audited savings
baselines. Existing policy broadly accepted inheritance; role prompts lacked
specific investigation stop/escalation rules; fresh-context assignment and
review-evidence reuse were underspecified. The new rules address these causes
without claiming that they explain every token in those sessions.

Firmware base: main `ceb993cf3e67`. HMI base: `chore/agent-opt-02`
`f26f7dbe48a8`, dependency [HMI PR 10](https://github.com/kamenAleksandrov/furnace_hmi_version_2/pull/10).
The firmware open Phase 16 PRs 25/26 are unrelated and unchanged. Existing
specialist roles and mandatory review gates remain intact. Only the four common
role prompts change, with no model/effort pins; client permission/tool lists and
the two-child cap are preserved. The supported Claude purchase-command hiding
control is added; it is not an overage enforcement guarantee.

Native validation results and static/gate outcomes will be recorded here before
handoff. Actual model IDs remain in local logs/final reporting, not repo defaults.

## Observed validation (2026-10-26)

- Parsed 4 firmware and 10 HMI Codex TOML roles plus both project configs;
  required fields are present, with no model/effort pins or per-role sandbox key.
  Parsed all 8 Claude frontmatters using the installed workspace PyYAML: valid,
  inherited model, no nested Agent/Task tool. The four general-role definitions
  match between repositories. Shared policy copies compare byte for byte.
- `claude plugin validate .claude/agents --json --strict` returned success in
  both Windows validation worktrees. Its contents list was empty, so this alone
  is not proof of loading. The firmware live CLI initialization roster included
  all four custom names and only permitted read/native delegation tools.
- The initial Claude lookup failed with HTTP 429 `usage_limit_reached`,
  with zero subagents spawned. After the owner confirmed the reset, three
  bounded native CLI sessions completed successfully: one repository-explorer
  in each repository, then sequential read-only checks of implementation-worker,
  verification-analyst and engineering-reviewer in the HMI snapshot. All four
  role types were invoked by their configured names; no generic substitutes.
- Native Claude traces report five children spawned and completed, zero failed,
  maximum depth one and zero nested spawns. Both startup rosters list all four
  custom roles. Every spawn omits model/effort overrides; forwarded child message
  metadata independently matches each parent's actual model, confirming model
  inheritance. Numeric effort values are not exposed, so only omission of an
  effort override is established. Explicit alternative-model selection and
  cheaper routing were not tested or claimed.
- The Claude children used only Read/Grep (the other-role checks used Read only),
  with no permission denials. The worker test assessed readiness only; it did
  not authorize implementation or prove the normally write-capable worker cannot
  write. The parent CLI restricted its tool pool to read/native delegation tools.
  Both Windows test worktrees stayed clean. No quota/permission bypass occurred.
- The supervisor checked the returned SAFE-02/03/04 references at firmware
  requirements.md:47-49 and the HMI authority boundary at AGENTS.md:6-7, then
  checked the role/policy references from the three configuration-only checks.
  The role-review summary noted that its default context-reading instructions
  were wider than the smoke task's assigned files. The supervisor had completed
  repository preflight and governing-record inspection; the checks made no
  engineering decisions, so no project instruction was weakened or changed.
- Successful Claude commands used `claude -p <bounded-prompt>` with
  `--output-format stream-json --verbose --forward-subagent-text`,
  `--no-session-persistence`, `--tools Read,Glob,Grep,Agent`, the matching
  `--allowedTools`, `--permission-mode dontAsk`, `--strict-mcp-config` and an
  empty MCP configuration. The explorer sessions had a client budget cap of
  2 USD each; the three-role session had 3 USD. All exited zero. These were
  small invocation checks, not a benchmark or production verification.
- Initial Codex execution against a WSL UNC path failed on read-only Windows
  sandbox path access; it did not prove role invocation. No permissions were
  relaxed. Separate committed Windows validation worktrees were used next.
- `codex --strict-config -C <validation-worktree> -a never exec -s read-only
  --json <bounded-read-only-prompt>` completed successfully in each repository.
  Firmware returned SAFE-02/03/04 references; the supervisor checked the cited
  requirement lines. HMI returned AGENTS.md and SYSTEM_CONTEXT.md authority
  references; the supervisor checked agreement with Control CPU ownership.
- The HMI native session store independently confirms child role
  repository-explorer, depth one, the same actual model and effort as its parent,
  and a read-only managed filesystem sandbox. No overrides were supplied.
  The firmware test was ephemeral; its CLI reported completion, while no
  persistent child identity was retained. No cheaper routing is claimed.
- Windows validation worktrees remained clean. Diffs against each main base
  contain agent configuration, guidance, planning/evidence and documentation
  only. No product source, build definition, hardware register or skill changed.
- HMI changed-document relative links: 174 links in 13 Markdown files PASS,
  using the existing firmware check_docs validator against the HMI changed set.
  `python3 tools/codebase_index.py --check` in the isolated HMI worktree is
  BLOCKED: canonical host-debug and zephyr-qemu_x86 CMake File API and compile
  databases are absent there. The canonical generated index was preserved;
  no degraded index is substituted. Refresh/check it with valid build inputs
  using the HMI runbook before declaring complete HMI index verification.

The supervising agent also used one inherited-model, read-only audit worker for
existing HMI reviewers/skills. It found the required named review gates, which
were preserved. No engineering-skill conflict was found. The two-repository
request authorizes these HMI configuration edits only; ordinary firmware tasks
still treat the HMI as a read-only reference.

Client logs expose token usage and, in Claude, cost fields. The initial rejected
Claude attempt reported no inference usage; the successful reruns expose scoped
usage/cost fields, not a verified subscription bill. Codex logs have scoped token
measurements without a verified monetary total. Actual model identifiers stay in local client logs,
not repository policy or configuration. No savings estimate is asserted.
Raw logs: Windows `%TEMP%/agent-opt-01-validation/` (local, untracked).
Reset reruns: `claude-fw-reset.jsonl`, `claude-hmi-reset.jsonl`,
`claude-other-roles-reset.jsonl`; structured checks:
`claude-reset-summary.json`. Actual model names remain outside repository files.

## AGENT-OPT-01 changed-file manifest (historical)

Against the task base (`3de05c5`):

```text
.claude/agents/engineering-reviewer.md
.claude/agents/implementation-worker.md
.claude/agents/repository-explorer.md
.claude/agents/verification-analyst.md
.claude/settings.json
.codex/agents/architect.toml
.codex/agents/decision-challenger.toml
.codex/agents/embedded-reviewer.toml
.codex/agents/engineering-reviewer.toml
.codex/agents/error-auditor.toml
.codex/agents/implementation-worker.toml
.codex/agents/protocol-reviewer.toml
.codex/agents/repository-explorer.toml
.codex/agents/software-critic.toml
.codex/agents/verification-analyst.toml
.codex/config.toml
AGENTS.md
CLAUDE.md
docs/09-progress/AGENT_OPT_01.md
docs/09-progress/ROADMAP.md
docs/09-progress/SESSION_LOG.md
docs/09-progress/STATUS.md
docs/INDEX.md
docs/agent-client-compatibility.md
docs/agent-delegation.md
```

## Firmware verification gate

`scripts/verify.sh` all stages PASS on committed clean firmware revision
`264a1b90782a`: env, docs, sources, format, tests, sca. The summary states
`verified 264a1b90782a with 0 uncommitted path(s)`. 768/768 executed test cases
passed; 27/27 build configuration checks passed, seven configurations built
only. GCC analyzer: zero project findings; 12 distinct Zephyr/third-party
findings are informational. Output: west-workspace
`verify-out/agent-opt-01/`. This is host/reference-build evidence, not hardware
verification. Only this result/handoff documentation changes after that run;
the final firmware docs stage is rerun before push. HMI runtime builds are
not claimed.
