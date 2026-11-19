"""Tests of .agents/delegation/delegation_ceiling.py (AGENT-OPT-04)."""

import io
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / ".agents" / "delegation"))

import delegation_ceiling as dc  # noqa: E402


class DecideTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / ".agents/delegation").mkdir(parents=True)
        shutil.copy(REPO / dc.CONFIG_REL, self.root / dc.CONFIG_REL)
        (self.root / ".claude/agents").mkdir(parents=True)
        (self.root / ".claude/agents/explorer.md").write_text(
            "---\nname: explorer\nmodel: haiku\neffort: low\n---\nbody\n", encoding="utf-8")
        (self.root / ".claude/agents/legacy.md").write_text(
            "---\nname: legacy\nmodel: inherit\n---\nbody\n", encoding="utf-8")
        self.cfg = dc.load_config(self.root)

    def tearDown(self):
        self.tmp.cleanup()

    def transcript(self, model):
        path = self.root / "t.jsonl"
        lines = [{"type": "user", "message": {}},
                 {"type": "assistant", "message": {"model": model}},
                 {"type": "assistant", "isSidechain": True, "message": {"model": "claude-x"}}]
        path.write_text("\n".join(json.dumps(x) for x in lines) + "\n", encoding="utf-8")
        return str(path)

    def decide(self, tool_input, model="claude-opus-5-5", effort="medium"):
        event = {"tool_name": "Agent", "tool_input": tool_input,
                 "transcript_path": self.transcript(model) if model else None}
        if effort:
            event["effort"] = {"level": effort}
        return dc.decide(event, self.root, self.cfg)

    def test_pinned_role_without_parameters_is_allowed(self):
        ok, _, rec = self.decide({"subagent_type": "explorer"})
        self.assertTrue(ok)
        self.assertEqual((rec["model"], rec["effort"], rec["session_model"]),
                         ("haiku", "low", "opus"))

    def test_session_model_comes_from_main_transcript_not_sidechain(self):
        _, _, rec = self.decide({"subagent_type": "explorer"}, model="claude-sonnet-5-5")
        self.assertEqual((rec["session_model"], rec["session_model_source"]),
                         ("sonnet", "transcript"))

    def test_stronger_model_than_session_is_denied(self):
        ok, reason, _ = self.decide({"subagent_type": "explorer", "model": "opus"},
                                    model="claude-sonnet-5-5")
        self.assertFalse(ok)
        self.assertIn("stronger", reason)

    def test_same_model_at_session_effort_is_allowed(self):
        ok, _, _ = self.decide({"subagent_type": "explorer", "model": "opus",
                                "effort": "medium"})
        self.assertTrue(ok)

    def test_same_model_above_session_effort_is_denied(self):
        ok, reason, _ = self.decide({"subagent_type": "explorer", "model": "opus",
                                     "effort": "high"})
        self.assertFalse(ok)
        self.assertIn("exceeds", reason)

    def test_weaker_model_may_go_up_to_high_but_not_xhigh(self):
        self.assertTrue(self.decide({"subagent_type": "explorer", "model": "sonnet",
                                     "effort": "high"})[0])
        self.assertFalse(self.decide({"subagent_type": "explorer", "model": "sonnet",
                                      "effort": "xhigh"})[0])

    def test_weaker_model_may_use_high_under_a_low_effort_session(self):
        self.assertTrue(self.decide({"subagent_type": "explorer", "model": "haiku",
                                     "effort": "high"}, effort="low")[0])
        self.assertFalse(self.decide({"subagent_type": "explorer", "model": "opus",
                                      "effort": "medium"}, effort="low")[0])

    def test_weaker_model_effort_is_capped_at_max_subagent_effort(self):
        ok, _, _ = self.decide({"subagent_type": "explorer", "model": "haiku",
                                "effort": "xhigh"}, effort="high")
        self.assertFalse(ok)

    def test_missing_model_would_inherit_and_is_denied(self):
        for role in ("legacy", "general-purpose"):
            ok, reason, _ = self.decide({"subagent_type": role})
            self.assertFalse(ok, role)
            self.assertIn("inherit", reason)

    def test_excluded_model_is_denied_even_under_itself(self):
        ok, reason, _ = self.decide({"subagent_type": "explorer", "model": "fable",
                                     "effort": "low"}, model="claude-fable-5-2")
        self.assertFalse(ok)
        self.assertIn("never", reason)

    def test_fork_is_denied(self):
        ok, reason, _ = self.decide({"subagent_type": "fork"})
        self.assertFalse(ok)
        self.assertIn("fork", reason)

    def test_other_model_needs_explicit_effort(self):
        (self.root / ".claude/agents/noeffort.md").write_text(
            "---\nname: noeffort\nmodel: haiku\n---\n", encoding="utf-8")
        ok, reason, _ = self.decide({"subagent_type": "noeffort"})
        self.assertFalse(ok)
        self.assertIn("explicit effort", reason)

    def test_session_start_model_is_used_without_transcript(self):
        dc.record_session_model({"session_id": "s1", "model": "claude-haiku-5-5"}, self.root)
        event = {"tool_name": "Agent", "session_id": "s1", "effort": {"level": "low"},
                 "tool_input": {"subagent_type": "explorer", "model": "sonnet"}}
        ok, reason, rec = dc.decide(event, self.root, self.cfg)
        self.assertFalse(ok)
        self.assertIn("stronger", reason)
        self.assertEqual(rec["session_model_source"], "session-start")

    def test_transcript_takes_precedence_over_session_start(self):
        dc.record_session_model({"session_id": "s1", "model": "claude-haiku-5-5"}, self.root)
        event = {"tool_name": "Agent", "session_id": "s1", "effort": {"level": "medium"},
                 "transcript_path": self.transcript("claude-opus-5-5"),
                 "tool_input": {"subagent_type": "explorer", "model": "sonnet"}}
        self.assertTrue(dc.decide(event, self.root, self.cfg)[0])

    def test_defaults_apply_without_transcript_or_effort(self):
        ok, _, rec = self.decide({"subagent_type": "explorer", "model": "opus",
                                  "effort": "high"}, model=None, effort=None)
        self.assertFalse(ok)
        self.assertEqual((rec["session_model"], rec["session_effort"],
                          rec["session_model_source"]), ("opus", "medium", "default"))


class HookTest(unittest.TestCase):
    def run_hook(self, payload):
        out = io.StringIO()
        code = dc.run_hook(io.StringIO(payload), out)
        return code, out.getvalue()

    def test_non_agent_tool_is_ignored(self):
        self.assertEqual(self.run_hook(json.dumps({"tool_name": "Bash"})), (0, ""))

    def test_malformed_input_fails_closed(self):
        code, out = self.run_hook("{not json")
        self.assertEqual(code, 0)
        self.assertEqual(json.loads(out)["hookSpecificOutput"]["permissionDecision"], "deny")


class RepositoryTest(unittest.TestCase):
    def test_repository_configuration_is_consistent(self):
        self.assertEqual(dc.check(REPO), [])


class ReportTest(unittest.TestCase):
    def test_usage_counts_each_message_once_per_model(self):
        with tempfile.TemporaryDirectory() as tmp:
            session = Path(tmp) / "s.jsonl"
            msg = {"type": "assistant", "message": {
                "id": "m1", "model": "claude-opus-5-5",
                "usage": {"input_tokens": 1, "cache_read_input_tokens": 10,
                          "output_tokens": 5}}}
            session.write_text(json.dumps(msg) + "\n" + json.dumps(msg) + "\n",
                               encoding="utf-8")
            sub = Path(tmp) / "s" / "subagents"
            sub.mkdir(parents=True)
            (sub / "agent-a.jsonl").write_text(json.dumps({"type": "assistant", "message": {
                "id": "m2", "model": "claude-haiku-5-5",
                "usage": {"input_tokens": 2, "output_tokens": 3}}}) + "\n", encoding="utf-8")
            (sub / "agent-a.meta.json").write_text(json.dumps(
                {"agentType": "repository-explorer", "model": "haiku", "effort": "low"}),
                encoding="utf-8")
            result = dc.report(session)
        self.assertEqual(result["main"]["claude-opus-5-5"]["messages"], 1)
        self.assertEqual(result["main"]["claude-opus-5-5"]["cache_read"], 10)
        self.assertEqual(result["subagents"][0]["role"], "repository-explorer")
        self.assertEqual(result["subagents"][0]["usage"]["claude-haiku-5-5"]["output"], 3)


if __name__ == "__main__":
    unittest.main()
