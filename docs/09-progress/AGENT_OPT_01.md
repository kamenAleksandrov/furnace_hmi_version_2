# AGENT-OPT-01 native agent configuration

2026-10-26; owner-authorized configuration/documentation only.

## Scope and authority

Add four reusable native roles in each client, consistent model-agnostic policy,
conservative concurrency and validation. Preserve AGENTS.md, accepted ADRs,
canonical skills and REVIEW_POLICY. No product, protocol, GUI, simulator,
firmware, theme or build behavior changes; no phase or maturity advance.

## Work and acceptance

1. Inspect both repositories, installed clients and official formats (done).
2. Add role definitions, settings and thin guidance; preserve six existing Codex
   specialists and domain review gates (done).
3. Validate syntax/discovery and attempt small read-only native delegations;
   report actual inheritance, quota, permissions and unavailable capabilities.
4. Check documentation and generated metadata where inputs are available;
   review diffs for production changes and hand off separate unmerged PRs.

Canonical behavior is in [the shared policy](../agent-delegation.md).
Actual client behavior and limitations are in
[compatibility and validation](../agent-client-compatibility.md), not inferred
from role names. No architecture decision is introduced. Rollback is a Git
revert of configuration/documentation commits, with no runtime state migration.
The firmware-side trace is WI-L01; HMI product work items are unchanged.
