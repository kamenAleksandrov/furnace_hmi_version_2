# Boundaries and invariants

The normative invariant text is in the
[requirements register](../00-project/REQUIREMENTS.md#architecture-invariants).
This document explains how to apply it.

## Authority test

For any new data or behavior, ask:

1. Can an incorrect value change furnace operation or safety?
2. Must the value survive HMI loss or restart?
3. Must two operator clients agree on it?
4. Does it describe what the furnace is doing rather than how the HMI shows it?

A *yes* is strong evidence for Control CPU ownership. HMI-local presentation,
navigation, drafts, and preferences remain HMI-owned. If ownership is mixed,
separate the concepts rather than inventing shared authority.

## Failure boundary

`ARCH-INV-002`, `ARCH-INV-003`, and `ARCH-INV-004` work together:

- HMI failure cannot be a safety mechanism or automatically stop a run.
- Link recovery cannot repeat an old request by accident.
- Correct reconstruction cannot require every historical event.

Session identity and an authoritative snapshot are therefore architecture
requirements, not protocol conveniences.

## Execution and history

`ARCH-INV-009` freezes the controller-accepted program revision used by an
active run even if the operator saves a new library revision. `ARCH-INV-010`
protects completed history. Reuse creates a new draft; it never rewrites the
past.

## Concurrency and priority

`ARCH-INV-005` makes LVGL a single-owner resource. Other contexts exchange
plain application data through bounded mechanisms. `ARCH-INV-006` means the
Control CPU must isolate furnace-critical work from program/library and HMI
traffic; the HMI cannot compensate for poor controller scheduling.

## Review gate

A change that appears to violate an invariant stops for resolution. Record the
concrete conflict, consequence, alternatives, and governing ID. A preference
or reversible implementation trade-off does not receive invariant status by
assertion.
