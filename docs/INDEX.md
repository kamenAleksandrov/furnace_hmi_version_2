# Engineering documentation index

This directory is both the engineering documentation root and an Obsidian
vault. Markdown links are repository-relative so the documents also work in a
browser and ordinary editors.

## Start here

- [Project charter](00-project/CHARTER.md)
- [Glossary](00-project/GLOSSARY.md)
- [Requirements](00-project/REQUIREMENTS.md)
- [Developer command runbook](05-platform/DEVELOPER_RUNBOOK.md)
- [Review policy](06-testing/REVIEW_POLICY.md)
- [Current status](09-progress/STATUS.md)
- [Open questions](09-progress/OPEN_QUESTIONS.md)
- [Decision records](07-decisions/README.md)

## Taxonomy

| Area | Purpose | Authoritative entry point |
| --- | --- | --- |
| `00-project` | Scope, terminology, requirements, constraints, V1 evidence | [Charter](00-project/CHARTER.md) |
| `01-architecture` | System context, authority boundaries, state ownership | [System context](01-architecture/SYSTEM_CONTEXT.md) |
| `02-protocol` | Protocol requirements and lifecycle principles; no wire schema yet | [Protocol requirements](02-protocol/PROTOCOL_REQUIREMENTS.md) |
| `03-hmi` | Portable HMI architecture and operator-experience baseline | [HMI architecture](03-hmi/HMI_ARCHITECTURE.md) |
| `04-controller` | Software-facing Control CPU contract boundary | [Controller interface boundary](04-controller/CONTROLLER_INTERFACE_BOUNDARY.md) |
| `05-platform` | Zephyr, target, transport, and simulator strategy | [Developer runbook](05-platform/DEVELOPER_RUNBOOK.md), [development environment](05-platform/DEVELOPMENT_ENVIRONMENT.md), and [runtime evaluation](05-platform/RUNTIME_EVALUATION.md) |
| `06-testing` | Verification strategy, review policy, and requirement traceability | [Test strategy](06-testing/TEST_STRATEGY.md) and [review policy](06-testing/REVIEW_POLICY.md) |
| `07-decisions` | Architecture Decision Records (ADRs) | [ADR index](07-decisions/README.md) |
| `08-pseudocode` | Non-normative design sketches when useful | [Usage policy](08-pseudocode/README.md) |
| `09-progress` | Status, roadmap, questions, and session history | [Status](09-progress/STATUS.md) |
| `_generated` | Tool-owned indexes | [Generated-document policy](_generated/README.md) |

## Documentation rules

- Requirements and accepted ADRs are normative. Pseudocode and examples are
  explanatory and cannot override them.
- Record consequential decisions in ADRs; do not leave them only in chat.
- Distinguish accepted decisions from assumptions, proposals, and open items.
- Link to one authoritative statement instead of copying it into many files.
- Update requirements and traceability together when behavior changes.
- Do not hand-edit files that declare themselves generated.
