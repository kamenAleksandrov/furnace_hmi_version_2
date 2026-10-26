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
| [ADR-0010](0010-maintenance-request-workflows.md) | Proposed | Separate maintenance actions and controller-validated restart attempts |
| [ADR-0011](0011-component-service-catalogue.md) | Proposed | Generic configured component service catalogue and read-only worker Device view |
| [ADR-0012](0012-theme-and-language-catalogues.md) | Proposed | Generated theme and string catalogues shared by both presentations |
| [ADR-0013](0013-explicit-start-and-hot-start.md) | Accepted (product-owner mandate); message draft Proposed | Explicit Start only; controller-proposed hot start with bound confirmation |
| [ADR-0014](0014-live-hot-start-confirmation-and-sensor-faults.md) | Accepted (product-owner mandate); message draft Proposed | Live hot-start confirmation without normal timeout; sensor-fault and degraded-status presentation |
| [ADR-0015](0015-hold-qualification-and-treatment-time.md) | Accepted (product-owner mandate); field list Proposed | Hold qualification before treatment timing; elapsed versus credited time presentation |
| [ADR-0016](0016-program-execution-presentation.md) | Accepted (product-owner mandate); field and command lists Proposed | Program execution, ramp holdback, Pause and outcome presentation; recoverable Hold interruption |
| [ADR-0017](0017-fault-classes-and-control-diagnostics-presentation.md) | Accepted (product-owner mandate); field lists Proposed | Latched critical fault and protective trip distinct from pause, transition deviation, control diagnostics in a service view |
| [ADR-0018](0018-fault-manager-and-hold-approach-presentation.md) | Accepted (product-owner mandate); message and field lists Proposed | Fault entries, acknowledgement and recovery requests, inhibition reasons, event history; required-sensor faults critical; Hold approach |
| [ADR-0019](0019-duration-decisions-and-runtime-presentation.md) | Accepted (product-owner mandate); message, field and code lists Proposed | Advisory versus maximum duration, accepted duration, overrun and runtime limit, quality decision with Continue, fault categories, chamber temperature; no roles |
| [ADR-0020](0020-diagnostics-presentation.md) | Accepted (product-owner mandate); operations, fields and placement Proposed | Controller-backed Device log viewer, system health with unknown shown as unknown, controller-validated recording schedules; no accounts |
