@AGENTS.md

# Claude Code compatibility

Use native project agents in `.claude/agents/` under the shared
[delegation policy](docs/agent-delegation.md). Read the relevant canonical
`.agents/skills/<name>/SKILL.md` when required; keep engineering rules there.
Choose model/effort at invocation only when the current client exposes suitable
available choices; otherwise inherit. Keep the project permission controls.
