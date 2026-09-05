# Open questions

These unresolved items do not block repository foundation work. They must not
be answered implicitly by placeholders, examples, or simulator behavior.

Reviewed 2026-09-02. The foundation and first visual slices intentionally did
not resolve the operational and hardware decisions below.

| Topic | Open question | Decision/evidence path |
| --- | --- | --- |
| Stage model | What are the exact HEATING, HOLD, and COOL execution semantics, including the initial-HOLD delayed-start rule? | Domain requirements and controller/HMI graph consistency review |
| Program editing validation | The operator requires controller agreement before a program or stage becomes part of the authoritative library. Are stage changes validated online individually, or atomically with a Program save request? | Controller interface and protocol lifecycle design |
| Target delta | The UI meaning is °C change per time period, normally °C/min. HEATING may locally drive the estimate from rate or duration; what controller bounds, rounding, preceding-temperature rule, and accepted calculation behavior apply? | Domain requirements and controller/HMI graph consistency review |
| Run trajectory | Which controller-owned accepted-revision forecast, stage-transition data, and fault/event semantics shall form the fixed initial running-graph domain? | Controller interface and protocol requirements before production graph integration |
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
| Navigation | What belongs in the currently unknown fourth destination, and is Settings permanently a bottom-navigation destination? | UX/product information architecture |
| Motion/accessibility | Is a reduced-motion preference required, and which decorative effects may it disable without reducing state visibility or feedback? | UX/accessibility requirement and target performance evidence |
| PID presentation | Which user-facing control/quality metrics belong on the run screen, and which raw P/I/D diagnostics require a service view or role? | UX, service workflow, and permissions design |
| Sensor loss | Which controller safety transition, alarm lifecycle, and HMI presentation apply when a required sensor becomes invalid? | Hazard analysis, controller safety requirements, and protocol design |
| Program estimate ambient | Confirm units, bounds, storage owner, and controller-returned metadata for the saved ambient input used only for estimation | Domain/protocol design; current intended behavior is recorded in [UX and domain baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md) |
| Settings | What is the final setting list, owner, metadata, and permission behavior? | Ownership review and controller capability design |
| Service maintenance | How are service time, due period, forecast completion, and maintenance rules derived? | Controller-owned counter/product requirement |
| Service-period gate | The user requires one controller-issued acknowledgement per configured service period and later maintenance visibility. What are its threshold, re-arm-after-service rule, role/audit retention, hard-block cases, and condition-change behavior between acknowledgement and Start? | Controller policy, product requirements, and protocol lifecycle design |
| Windows Python | Should the reproducible Windows baseline require upstream-recommended Python 3.12 instead of the locally verified 3.13.3 deviation? | Clean-machine environment reproduction/CI evidence |

Protocol serialization/framing and source-sharing also remain deferred in
[ADR-0007](../07-decisions/0007-serialization-and-framing.md) and
[ADR-0009](../07-decisions/0009-protocol-source-sharing.md).
