# Review policy

Reviews supplement builds and tests; they do not replace evidence. Review
agents remain read-only, report findings to the parent/decision owner, and do
not make repository edits or final decisions.

## Select reviews by consequence

- Small, reversible work needs the relevant checks and author review; do not
  invoke every specialist by default.
- Architecture, protocol, or embedded changes use the relevant domain reviewer:
  `architect`, `protocol-reviewer`, or `embedded-reviewer`.
- Before accepting a consequential ADR, use the relevant domain reviewer and
  `decision-challenger`.
- After significant implementation, use the relevant domain reviewer,
  `software-critic`, and `error-auditor`.
- Architecture-baseline gates use broader review across the affected domains.

## Domain and adversarial roles

Domain reviewers check the governing requirements, accepted ADRs, and relevant
technical constraints. Adversarial reviewers have narrower purposes:

- `decision-challenger` tests assumptions, evidence, alternatives,
  reversibility, and long-term cost before an important decision is accepted.
- `software-critic` looks for correctness and maintainability risks, hidden
  coupling/state, blocking, ownership failures, and missing verification after
  substantial implementation.
- `error-auditor` reports concrete defensible defects, contradictions, broken
  configuration, failing checks, and unsupported completion claims.

Every finding identifies evidence and distinguishes an error or requirement
conflict from a risk, trade-off, or preference. The parent/decision owner
reconciles conflicting findings, fixes or explicitly resolves concrete errors,
records consequential decisions, and reports checks that were not run.
