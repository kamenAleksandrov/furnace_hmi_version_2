# Deferred furnace CPU and protocol integration tasks

Date: 2026-09-23. All deferred; no furnace-firmware changes performed.
Canonical retrieval point for agents asked for furnace-side follow-ups from the
[workflow redesign](OPERATOR_WORKFLOW_IMPLEMENTATION_PLAN.md). Handles are tasks,
not normative requirement IDs. Follow sibling firmware repository instructions
when implementing. Later protocol semantics belong in docs/02-protocol/; this
backlog defines no wire schema. Demo observations remain synthetic.

| Task | Owner | Required outcome/evidence |
| --- | --- | --- |
| CTRL-LIMITS | Control CPU/protocol | Session bounds/steps/applicability; product maxima 180 C, 3 C/min, 48 h. Define minima/rounding; independently validate. |
| CTRL-PROGRAM | Control CPU/protocol | Heating/Hold save validation, curing <=48 h excluding final automatic cooling, no names/material. Define atomicity, first Hold, starting-temperature duration basis, legacy compatibility. |
| CTRL-MANUAL | Control CPU/protocol | Manual operations/deadline/cooling at 48 h/warning timing; define paused-time accounting. Edits/reconnect never reset deadline. Fresh session needs accepted Stop/Start. |
| CTRL-COOLING | Engineers/Control CPU | Endpoint/completion/actuator behavior; Svetlio averages and load/ambient applicability. Validate live ETA/ownership; estimates never determine completion. The HMI model now represents a run phase, cooling elapsed, estimated cooling remaining and a per-sample planned-cooling flag (all unavailable by default); this is HMI representation only and defines no wire schema. |
| CTRL-LIGHT | Control CPU/protocol/hardware | Request/capability/actual state/rejection/snapshot recovery. No direct HMI actuator access. |
| CTRL-FAN | Control CPU/hardware | Investigate measured RPM/validity for display. No operator RPM regulation requested. |
| CTRL-DOOR | Control CPU/responsible engineers | Assess prevent-start-while-open / do-not-stop-running-on-open proposal against applicable requirements/regulations. Reviewed policy before implementation; no HMI interlock authority. |
| CTRL-EVENTS | Control CPU/protocol | Identifiable start/end/pause/cooling/fault/service events, time/session context, intent/outcome distinction, deduplication, snapshot recovery independent of full history. |
| CTRL-MAINTENANCE | Control CPU/service/product | Later define clear/confirm/reset/check permissions/interval semantics; confirmed service produces event. |
| CTRL-RESTART | Control CPU/Veso | Decide whether Restart furnace remains and its permissions/recovery. Deferred; no removal/execution inferred. |

HMI follow-ups: bounded volatile session log capacity/eviction/verbosity, separate
from authoritative Run Sessions/service records; Light geometry through keyboards/
dialogs; explicit Manual/cooling containment decision. Preserve pending/rejected/
stale state and no automatic command replay under ADR-0001/0003.
