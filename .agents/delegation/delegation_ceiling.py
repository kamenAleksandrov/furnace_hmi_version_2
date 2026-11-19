#!/usr/bin/env python3
"""Delegation ceiling for native subagents (AGENT-OPT-04).

Keep this file byte-identical in both furnace repositories.

Modes:

* ``hook`` (default; Claude Code PreToolUse on the Agent tool): reads the hook
  JSON from stdin and denies a spawn whose model is stronger than the session's
  model, whose model is excluded, that names no model (it would inherit the
  session model), whose effort exceeds the allowed level, or that forks the
  conversation. The session model is read from the session transcript, else
  from the model a SessionStart hook recorded for the session (Git-ignored
  ``.claude/delegation-sessions.json``; the client does not always report
  one), else ``default_session_model`` applies. The session effort comes
  from the hook input; otherwise ``default_session_effort`` applies. Each
  decision is appended to ``.claude/delegation-log.jsonl`` (Git-ignored).
* ``check``: static consistency check of the ceiling, the pinned Claude and
  Codex roles, the Codex ``[agents]`` defaults and the Claude hook settings.
* ``report <session.jsonl>``: actual model, effort and token usage of a Claude
  session and its subagents, read from the local transcripts. Tokens are client
  counts, not a subscription bill.

Standard library only (Python 3.8+; ``check`` needs 3.11+ for tomllib).
"""

import json
import os
import sys
from pathlib import Path

CONFIG_REL = Path(".agents/delegation/ceiling.json")
LOG_REL = Path(".claude/delegation-log.jsonl")
SESSIONS_REL = Path(".claude/delegation-sessions.json")
MAX_SESSIONS = 50
AGENT_TOOLS = ("Agent", "Task")
TAIL_BYTES = 512 * 1024


def repo_root(start=None):
    env = os.environ.get("CLAUDE_PROJECT_DIR")
    if start is None and env:
        return Path(env)
    here = Path(start) if start else Path(__file__).resolve().parent
    for p in [here, *here.parents]:
        if (p / CONFIG_REL).is_file():
            return p
    return Path(__file__).resolve().parents[2]


def load_config(root):
    return json.loads((Path(root) / CONFIG_REL).read_text(encoding="utf-8"))


def model_alias(model_id, order):
    """Map a full model ID (claude-opus-5-5) or alias to an alias in ``order``."""
    if not model_id:
        return None
    text = str(model_id).lower()
    for alias in order:
        if alias in text:
            return alias
    return None


def detect_session_model(transcript_path, order):
    """Model of the most recent main-session assistant message, or None."""
    if not transcript_path:
        return None
    try:
        with open(transcript_path, "rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            f.seek(max(0, size - TAIL_BYTES))
            lines = f.read().decode("utf-8", errors="replace").splitlines()
    except OSError:
        return None
    for line in reversed(lines):
        try:
            obj = json.loads(line)
        except ValueError:
            continue
        if obj.get("type") != "assistant" or obj.get("isSidechain"):
            continue
        alias = model_alias((obj.get("message") or {}).get("model"), order)
        if alias:
            return alias
    return None


def _load_sessions(root):
    try:
        data = json.loads((Path(root) / SESSIONS_REL).read_text(encoding="utf-8"))
        return data if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


def record_session_model(event, root):
    """SessionStart: remember the session's model when the client reports it."""
    session_id, model = event.get("session_id"), event.get("model")
    if isinstance(model, dict):
        model = model.get("id") or model.get("display_name")
    if not session_id or not model:
        return
    sessions = _load_sessions(root)
    sessions.pop(session_id, None)
    sessions[session_id] = str(model)
    keep = dict(list(sessions.items())[-MAX_SESSIONS:])
    try:
        (Path(root) / SESSIONS_REL).write_text(json.dumps(keep, indent=1) + "\n",
                                               encoding="utf-8")
    except OSError:
        pass


def recorded_session_model(event, root, order):
    return model_alias(_load_sessions(root).get(event.get("session_id") or ""), order)


def role_pins(root, name):
    """(model, effort) from .claude/agents/<name>.md frontmatter; None if unset."""
    path = Path(root) / ".claude" / "agents" / f"{name}.md"
    if not name or not path.is_file():
        return None, None
    lines = path.read_text(encoding="utf-8").splitlines()
    fields = {}
    if lines and lines[0].strip() == "---":
        for line in lines[1:]:
            if line.strip() == "---":
                break
            key, sep, value = line.partition(":")
            if sep:
                fields[key.strip()] = value.strip()
    model = fields.get("model")
    if model == "inherit":
        model = None
    return model or None, fields.get("effort") or None


def decide(event, root, cfg):
    """Return (allowed, reason, record) for a PreToolUse event."""
    claude = cfg["claude"]
    order = claude["order"]
    efforts = cfg["effort_order"]
    tool_input = event.get("tool_input") or {}
    role = tool_input.get("subagent_type") or "general-purpose"
    record = {"role": role, "description": tool_input.get("description")}

    if role == "fork" and claude.get("deny_fork", True):
        return False, ("fork copies the whole conversation at the session model; spawn a "
                       "fresh project role with an explicit model instead"), record

    session, source = detect_session_model(event.get("transcript_path"), order), "transcript"
    if not session:
        session, source = recorded_session_model(event, root, order), "session-start"
    if not session:
        session, source = claude["default_session_model"], "default"
    record["session_model_source"] = source
    session_effort = ((event.get("effort") or {}).get("level")
                      or os.environ.get("CLAUDE_EFFORT")
                      or claude["default_session_effort"])
    record.update(session_model=session, session_effort=session_effort)

    pin_model, pin_effort = role_pins(root, role)
    model = model_alias(tool_input.get("model"), order) or tool_input.get("model") or pin_model
    effort = tool_input.get("effort") or pin_effort
    if not model:
        return False, (f"no model for '{role}': it would inherit the session model "
                       f"({session}); pass an explicit model (same or weaker)"), record
    if model in claude.get("never_delegate", []):
        return False, f"model '{model}' is never used for delegation", record
    if model not in order or session not in order:
        return False, f"unknown model '{model}' or session model '{session}'", record
    if order.index(model) > order.index(session):
        return False, (f"model '{model}' is stronger than the session model '{session}'; "
                       "use the same or a weaker model"), record
    if not effort:
        if model != session:
            return False, (f"no effort for '{role}' on '{model}': a different model does not "
                           "take the session effort; pass an explicit effort"), record
        effort = session_effort
    record.update(model=model, effort=effort)
    if effort not in efforts or session_effort not in efforts:
        return False, f"unknown effort '{effort}' or session effort '{session_effort}'", record
    limit = efforts.index(session_effort)
    if model != session:
        limit = min(limit + int(claude.get("weaker_model_extra_effort_levels", 0)),
                    efforts.index(claude.get("max_subagent_effort", efforts[-1])))
    if efforts.index(effort) > limit:
        return False, (f"effort '{effort}' exceeds the allowed '{efforts[limit]}' for model "
                       f"'{model}' under session '{session}'/'{session_effort}'"), record
    return True, "within ceiling", record


def log_decision(root, allowed, reason, record):
    try:
        from datetime import datetime, timezone
        entry = dict(record, allowed=allowed, reason=reason,
                     time=datetime.now(timezone.utc).isoformat(timespec="seconds"))
        with open(Path(root) / LOG_REL, "a", encoding="utf-8") as f:
            f.write(json.dumps(entry) + "\n")
    except OSError:
        pass


def deny_json(reason):
    return json.dumps({"hookSpecificOutput": {
        "hookEventName": "PreToolUse",
        "permissionDecision": "deny",
        "permissionDecisionReason": f"Delegation ceiling (docs/agent-delegation.md): {reason}",
    }})


def run_hook(stdin, stdout):
    try:
        event = json.loads(stdin.read() or "{}")
        if event.get("hook_event_name") == "SessionStart":
            record_session_model(event, repo_root())
            return 0
        if event.get("tool_name") not in AGENT_TOOLS:
            return 0
        root = repo_root()
        allowed, reason, record = decide(event, root, load_config(root))
        log_decision(root, allowed, reason, record)
    except Exception as exc:  # fail closed for spawns
        allowed, reason = False, f"ceiling hook error: {exc!r}"
    if not allowed:
        stdout.write(deny_json(reason) + "\n")
    return 0


def check(root):
    """Return a list of consistency errors (empty when consistent)."""
    import tomllib

    root = Path(root)
    errors = []
    cfg = load_config(root)
    efforts = cfg["effort_order"]
    for client in ("claude", "codex"):
        c = cfg[client]
        if c["default_session_model"] not in c["order"]:
            errors.append(f"{client}: default_session_model not in order")
        if c["default_session_effort"] not in efforts:
            errors.append(f"{client}: default_session_effort unknown")

    claude = cfg["claude"]
    top = claude["order"].index(claude["default_session_model"])
    roles = sorted((root / ".claude/agents").glob("*.md"))
    if not roles:
        errors.append(".claude/agents: no roles")
    for path in roles:
        model, effort = role_pins(root, path.stem)
        where = f"{path.relative_to(root)}"
        if model not in claude["order"] or model in claude["never_delegate"]:
            errors.append(f"{where}: model must be pinned to a delegable alias, got {model!r}")
        elif claude["order"].index(model) >= top:
            errors.append(f"{where}: pinned model must be weaker than the default session model")
        if effort not in efforts or efforts.index(effort) > efforts.index(
                claude["default_session_effort"]):
            errors.append(f"{where}: effort must be pinned at or below the default session "
                          f"effort, got {effort!r}")

    settings = json.loads((root / ".claude/settings.json").read_text(encoding="utf-8"))
    env = settings.get("env", {})
    for bad in ("CLAUDE_CODE_EFFORT_LEVEL", "CLAUDE_CODE_SUBAGENT_MODEL_FORCE"):
        if bad in env:
            errors.append(f".claude/settings.json: {bad} would override per-role selection")
    hooks = [h for m in settings.get("hooks", {}).get("PreToolUse", [])
             if "Agent" in m.get("matcher", "") for h in m.get("hooks", [])]
    if not any("delegation_ceiling.py" in h.get("command", "") and
               h.get("onFailure") == "block" for h in hooks):
        errors.append(".claude/settings.json: PreToolUse Agent hook with onFailure=block "
                      "running delegation_ceiling.py is missing")
    starts = [h for m in settings.get("hooks", {}).get("SessionStart", [])
              for h in m.get("hooks", [])]
    if not any("delegation_ceiling.py" in h.get("command", "") for h in starts):
        errors.append(".claude/settings.json: SessionStart hook recording the session model "
                      "is missing")

    codex = cfg["codex"]
    ctop = codex["order"].index(codex["default_session_model"])
    conf = tomllib.loads((root / ".codex/config.toml").read_text(encoding="utf-8"))
    agents = conf.get("agents", {})
    if agents.get("default_subagent_model") != codex["order"][0]:
        errors.append(".codex/config.toml: default_subagent_model must be the weakest model")
    if agents.get("default_subagent_reasoning_effort") != efforts[0]:
        errors.append(".codex/config.toml: default_subagent_reasoning_effort must be "
                      f"'{efforts[0]}'")
    if agents.get("max_concurrent_threads_per_session") != codex[
            "max_concurrent_threads_per_session"]:
        errors.append(".codex/config.toml: max_concurrent_threads_per_session differs")
    croles = sorted((root / ".codex/agents").glob("*.toml"))
    if not croles:
        errors.append(".codex/agents: no roles")
    for path in croles:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
        where = f"{path.relative_to(root)}"
        model, effort = data.get("model"), data.get("model_reasoning_effort")
        if model not in codex["order"] or model in codex["never_delegate"]:
            errors.append(f"{where}: model must be pinned to a listed model, got {model!r}")
        elif codex["order"].index(model) >= ctop:
            errors.append(f"{where}: pinned model must be weaker than the default session model")
        if effort not in efforts or efforts.index(effort) > efforts.index(
                codex["default_session_effort"]):
            errors.append(f"{where}: effort must be pinned at or below the default session "
                          f"effort, got {effort!r}")
    return errors


def _usage(path):
    """Sum token usage per model in one transcript, counting each message once."""
    totals, seen = {}, set()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            try:
                obj = json.loads(line)
            except ValueError:
                continue
            msg = obj.get("message") or {}
            usage = msg.get("usage")
            if obj.get("type") != "assistant" or not usage:
                continue
            key = msg.get("id") or obj.get("uuid")
            if key in seen:
                continue
            seen.add(key)
            t = totals.setdefault(msg.get("model") or "unknown", {
                "messages": 0, "input": 0, "cache_write": 0, "cache_read": 0, "output": 0})
            t["messages"] += 1
            t["input"] += usage.get("input_tokens") or 0
            t["cache_write"] += usage.get("cache_creation_input_tokens") or 0
            t["cache_read"] += usage.get("cache_read_input_tokens") or 0
            t["output"] += usage.get("output_tokens") or 0
    return totals


def report(session_path):
    """Usage summary of a Claude session transcript and its subagents."""
    session_path = Path(session_path)
    result = {"session": str(session_path), "main": _usage(session_path), "subagents": []}
    for sub in sorted((session_path.with_suffix("") / "subagents").glob("*.jsonl")):
        meta_path = sub.with_suffix(".meta.json")
        meta = {}
        if meta_path.is_file():
            meta = json.loads(meta_path.read_text(encoding="utf-8"))
        result["subagents"].append({
            "file": sub.name, "role": meta.get("agentType"),
            "requested_model": meta.get("model"), "effort": meta.get("effort"),
            "usage": _usage(sub)})
    return result


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    mode = argv[0] if argv else "hook"
    if mode == "hook":
        return run_hook(sys.stdin, sys.stdout)
    if mode == "check":
        errors = check(repo_root(argv[1]) if len(argv) > 1 else repo_root(Path.cwd()))
        for e in errors:
            print(f"delegation ceiling: {e}", file=sys.stderr)
        if not errors:
            print("delegation ceiling: roles, Codex defaults and Claude hook consistent")
        return 1 if errors else 0
    if mode == "report" and len(argv) == 2:
        print(json.dumps(report(argv[1]), indent=2))
        return 0
    print("usage: delegation_ceiling.py [hook | check [repo] | report <session.jsonl>]",
          file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
