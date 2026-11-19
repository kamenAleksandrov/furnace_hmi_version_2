@AGENTS.md

# Claude Code compatibility

Use native project agents in `.claude/agents/` under the shared
[delegation policy](docs/agent-delegation.md). Read the relevant canonical
`.agents/skills/<name>/SKILL.md` when required; keep engineering rules there.
Spawn only within the delegation ceiling: use a role's pinned model/effort or
pass an explicit alias and effort that is the same or weaker than the session;
the project `PreToolUse` hook denies anything else. Do small and
high-consequence tasks directly. Keep project permission controls and stop at
allowance exhaustion; no paid fallback.
