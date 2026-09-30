# Engineering documentation index

This directory is both the engineering documentation root and an Obsidian
vault. Markdown links are repository-relative so the documents also work in a
browser and ordinary editors.

## Start here

- [Operator workflow plan](09-progress/OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md), [screen concepts](03-hmi/OPERATOR_WORKFLOW_SCREEN_CONCEPTS.md), and [controller integration backlog](09-progress/CONTROLLER_INTEGRATION_BACKLOG.md)
- [Project charter](00-project/CHARTER.md)
- [Glossary](00-project/GLOSSARY.md)
- [Requirements](00-project/REQUIREMENTS.md)
- [Developer command runbook](05-platform/DEVELOPER_RUNBOOK.md)
- [Review policy](06-testing/REVIEW_POLICY.md)
- [Current status](09-progress/STATUS.md)
- [Host-only UI laboratory plan](09-progress/UI_LAB_VERTICAL_SLICE_PLAN.md)
- [Graphite UI all-page completion plan](09-progress/GRAPHITE_UI_COMPLETION_PLAN.md)
- [Graphite UI native verification](09-progress/GRAPHITE_UI_VERIFICATION.md)
- [Open questions](09-progress/OPEN_QUESTIONS.md)
- [Settings screen proposal](03-hmi/SETTINGS_SCREEN_PROPOSAL.md) and [implementation plan](09-progress/SETTINGS_IMPLEMENTATION_PLAN.md)
- [Device screen proposal](03-hmi/DEVICE_SCREEN_PROPOSAL.md) and [implementation plan](09-progress/DEVICE_IMPLEMENTATION_PLAN.md)
- [Programs style proposal](03-hmi/PROGRAMS_STYLE_PROPOSAL.md) and [implementation plan](09-progress/PROGRAMS_STYLE_PLAN.md)
- [Home, Running and editor demo plan](09-progress/HOME_RUNNING_DEMO_PLAN.md)
- [Running readings refinement](09-progress/RUNNING_READINGS_REFINEMENT_PLAN.md)
- [Touch keyboard layouts and plan](09-progress/TOUCH_KEYBOARD_PLAN.md)
- [Decision records](07-decisions/README.md)

- [HMI web demo and website publishing](09-progress/HMI_WEB_DEMO.md)
- [Client web demo agent build plan](09-progress/WEB_DEMO_CLIENT_BUILD_PLAN.md)

## Taxonomy

| Area | Purpose | Authoritative entry point |
| --- | --- | --- |
| `00-project` | Scope, terminology, requirements, constraints, V1 evidence | [Charter](00-project/CHARTER.md) |
| `01-architecture` | System context, authority boundaries, state ownership | [System context](01-architecture/SYSTEM_CONTEXT.md) |
| `02-protocol` | Protocol requirements and lifecycle principles; no wire schema yet | [Protocol requirements](02-protocol/PROTOCOL_REQUIREMENTS.md) |
| `03-hmi` | Portable HMI architecture, operator experience, and program/startup model | [HMI architecture](03-hmi/HMI_ARCHITECTURE.md), [program and startup model](03-hmi/PROGRAM_AND_STARTUP_MODEL.md) |
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
