# Native agent client compatibility

AGENT-OPT-01; checked 2026-10-26. Configuration only. Installed Windows clients:
Codex CLI 0.162.0-alpha.17.2; Claude Code 2.1.300. No native Linux client was
found in WSL PATH; firmware development and verification still use WSL.

## Claude Code

Four project agents live in `.claude/agents/` as Markdown with YAML frontmatter.
They use `model: inherit`, with no effort pin. Read-only roles allow only
Read/Glob/Grep; the worker additionally allows local shell/edit/Skill tools and
must be assigned its own writing worktree. The parent applies permissions.
The shared settings cap normal native subagent concurrency at two and spawning
depth at one. These limits do not govern separate sessions or all client modes.

Choose an available per-invocation model and effort only if exposed by the
current Agent/Task tool. This CLI has no verified automatic account model-list
interface for the supervising model. Use the current session choices or inherit;
do not build discovery automation or encode a permanent alias/ranking here.
Project agents are read when a session starts: a session started before
`.claude/agents/` existed in its directory does not offer the roles until it
is restarted (observed 2026-10-27, AGENT-OPT-02). Check the offered agent
types at the start of each task. Overrides from local/managed settings can affect resolution. Check actual usage
or the task display instead of assuming the requested model ran. See the
[official subagent documentation](https://code.claude.com/docs/en/sub-agents)
and [model settings](https://code.claude.com/docs/en/model-config).

## Codex

Four standalone TOML agents live in `.codex/agents/`; name, description and
developer_instructions define roles. Model and reasoning effort are omitted.
`.codex/config.toml` enables agents and caps concurrent subagent threads at two.
The policy prohibits nested delegation. No global model settings are changed.

Use only choices exposed by the active spawn tool. Per-invocation model/effort
selection is client-dependent; a full-history fork may require inheritance.
The installed `codex debug models` refreshes a catalogue, but a catalogue entry
alone does not prove account access or price. If discovery/selection is absent,
inherit the parent/applicable defaults and disclose that fallback.

Custom-agent sandbox_mode does not override the parent in this client: the old
HMI declarations were removed to avoid a false enforcement claim. A read-only
role is behavioral; use a read-only parent session for hard filesystem controls.
See [official subagents](https://learn.chatgpt.com/docs/agent-configuration/subagents)
and [configuration reference](https://learn.chatgpt.com/docs/config-file/config-reference).

## Reproduce a harmless check

Start a fresh session in the intended repository/worktree. Native project config
may depend on client trust; report ignored configuration rather than changing
trust or permissions silently. Do not use bypass-permission flags.

1. Check installed versions: `claude --version`, `codex --version`.
2. Run `claude plugin validate .claude/agents --json --strict` and parse every
   TOML using Python's tomllib. A validator exit alone is not live discovery proof.
3. In Claude, allow only read tools and native Agent/Task for this test. Ask the
   repository-explorer to locate three heating-inhibit invariants (firmware) or
   the controller-authority boundary (HMI), with exact file references. Do not
   pass model/effort overrides. Confirm the initialization roster and actual
   subagent events. If account quota prevents inference, stop and record it.
4. In Codex use a read-only parent (`codex -s read-only`) and request the same
   project role. Confirm the spawn result, role, references and exposed model.
   Missing roles or denied filesystem access are failed tests, not success.
5. Compare Git status and production paths with the baseline. Do not edit
   production source, install clients, run hardware or retry by weakening access.

Client versions and observations are dated evidence, not model pins. Repeat
small checks after a client update; do not run a multi-agent benchmark.

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

## Changed-file manifest

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
