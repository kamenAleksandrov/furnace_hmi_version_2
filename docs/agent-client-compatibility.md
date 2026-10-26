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
Overrides from local/managed settings can affect resolution. Check actual usage
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

## Validation evidence

Validation is in progress. Final observed results and exact command summaries
will be recorded here before handoff. Raw client logs remain local outside Git;
they may contain account-specific model names and session metadata.
