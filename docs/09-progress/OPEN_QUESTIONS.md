# Open questions

These unresolved items do not block repository foundation work. They must not
be answered implicitly by placeholders, examples, or simulator behavior.

Reviewed 2026-08-25. The foundation pass did not resolve any item below.

| Topic | Open question | Decision/evidence path |
| --- | --- | --- |
| Stage model | What are the exact RAMP, HOLD, and COOL semantics? | Domain requirements and controller/HMI graph consistency review |
| Manual expiry | What controller transition occurs when a Manual deadline expires? | Controller behavior requirement and ADR if consequential |
| Faults/alarms | How do acknowledgement, latching, history, recovery, severity, and context behave? | Domain/protocol design |
| Run Session | What is the exact immutable schema? | Domain/protocol/storage design |
| Run Session retention | What capacity, retention, export, and eviction policy applies? | Target resources and product requirements |
| RS-422 rate | What final baud rate satisfies latency, throughput, and robustness? | [Transport ADR](../07-decisions/0006-transport-selection.md) measurements |
| UART/peripheral | Which target UART and software-visible transceiver configuration are used? | Production board/devicetree evidence |
| Control CPU reboot | What happens to furnace operation during/after controller reboot? | Controller-side safety/operation requirement |
| HMI MCU | Which MCU/board is the first production target? | Resource/driver/toolchain evaluation |
| Display | Which first production display, interface, and resolution? | Software-visible hardware contract |
| Touch | Which first production touch controller/interface? | Zephyr driver and board evaluation |
| Memory | What are stack, heap/pool, LVGL, graph, protocol, and storage budgets? | [Memory policy ADR](../07-decisions/0008-memory-policy.md) |
| Telemetry | What are final rates, burst behavior, and coalescing rules by signal? | Measured protocol/control requirements |
| Dashboard | Which additional statistics are required and how are they grouped? | UX/product requirements |
| Settings | What is the final setting list, owner, metadata, and permission behavior? | Ownership review and controller capability design |
| Service maintenance | How is selected Service Time derived and what maintenance rules apply? | Controller-owned counter/product requirement |
| Windows Python | Should the reproducible Windows baseline require upstream-recommended Python 3.12 instead of the locally verified 3.13.3 deviation? | Clean-machine environment reproduction/CI evidence |

Protocol serialization/framing and source-sharing also remain deferred in
[ADR-0007](../07-decisions/0007-serialization-and-framing.md) and
[ADR-0009](../07-decisions/0009-protocol-source-sharing.md).
