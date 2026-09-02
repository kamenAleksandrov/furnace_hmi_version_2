# Architecture Decision Records

ADRs preserve consequential project decisions and their evidence. An accepted
ADR is normative together with the requirements register; examples and
pseudocode cannot override it.

## Status meanings

- **Proposed**: under active consideration; not authority to implement broadly.
- **Accepted**: current project decision.
- **Deferred**: decision intentionally postponed until named evidence exists.
- **Superseded**: replaced by another ADR; retain the historical record.
- **Rejected**: considered and not selected.

Before accepting a consequential ADR, obtain a relevant domain review and a
decision-challenger review. The parent/decision owner synthesizes evidence and
owns the final edit. Follow the [review policy](../06-testing/REVIEW_POLICY.md)
and use [the template](ADR_TEMPLATE.md).

## Index

| ADR | Status | Decision |
| --- | --- | --- |
| [ADR-0001](0001-authority-boundary.md) | Accepted (mandated) | Control CPU authority and HMI remote-interface boundary |
| [ADR-0002](0002-zephyr-4-4-0.md) | Accepted | Pin the HMI runtime to Zephyr 4.4.0 |
| [ADR-0003](0003-protocol-layering.md) | Accepted principle | Separate semantics, representation, framing, and transport |
| [ADR-0004](0004-desktop-simulator.md) | Accepted | Maintain a project-owned LVGL desktop simulator |
| [ADR-0005](0005-lvgl-single-owner.md) | Accepted | Give LVGL one GUI execution owner |
| [ADR-0006](0006-transport-selection.md) | Proposed | Production UART/RS-422 transport baseline |
| [ADR-0007](0007-serialization-and-framing.md) | Deferred | Select serialization, framing, and integrity |
| [ADR-0008](0008-memory-policy.md) | Deferred | Set bounded memory/allocation policy |
| [ADR-0009](0009-protocol-source-sharing.md) | Deferred | Select HMI/controller protocol source-sharing model |
