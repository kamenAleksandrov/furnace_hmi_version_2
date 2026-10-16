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
| CTRL-STAGE-KIND | Control CPU/protocol | Report the current stage's kind (Heating / Hold) with the stage index. The HMI model carries an HMI-only `stage_kind` (unknown by default, then the controller's stage name is shown); no wire schema. |
| CTRL-SETPOINT | Control CPU/protocol | Report the live controller setpoint (the ramping value the controller is following) for Manual and Program runs. The HMI Manual readings show Setpoint and currently display "--" because the observation model has no such field; add it to the model once the protocol defines it. |
| CTRL-LIGHT | Control CPU/protocol/hardware | Request/capability/actual state/rejection/snapshot recovery. No direct HMI actuator access. |
| CTRL-FAN | Control CPU/hardware | Investigate measured RPM/validity for display. No operator RPM regulation requested. |
| CTRL-DOOR | Control CPU/responsible engineers | Assess prevent-start-while-open / do-not-stop-running-on-open proposal against applicable requirements/regulations. Reviewed policy before implementation; no HMI interlock authority. |
| CTRL-ALERTS | Control CPU/protocol | Fault categories, codes, severity and texts behind the HMI alert proposal ([UX baseline](../03-hmi/UX_AND_DOMAIN_BASELINE.md#controller-alerts-proposal-2026-10-01)); whether a fault ends the run; power-interruption reporting after restart; acknowledgement/latching (open "Faults/alarms"). The HMI model carries an HMI-only `fault_kind` (none by default); no wire schema. |
| CTRL-EVENTS | Control CPU/protocol | Identifiable start/end/pause/cooling/fault/service events, time/session context, intent/outcome distinction, deduplication, snapshot recovery independent of full history. |
| CTRL-MAINTENANCE | Control CPU/service/product | Later define clear/confirm/reset/check permissions/interval semantics; confirmed service produces event. Both labs now request **Maintenance done** and simulate the confirmation (new interval, last baseline today, service record, log event); define who may record which task and the audit record. Latest decision: a quality-of-life tracking aid, not warranty-based (may change). |
| CTRL-RUN-STOP | Control CPU/protocol | Operator Stop of a program run: heating ends and automatic cooling follows; report that the run was stopped (the labs track it in the HMI); a close-run request ends a stopped/cooled run; whether a new start is accepted while cooling (the labs accept it). |
| CTRL-START-HOT | Control CPU/protocol | Explicit Start, hot-start stage selection, skipped-stage proposal and bound confirmation per the furnace repository `docs/interfaces/controller-hmi-interface.md` (0.1.0 draft); furnace START-01 to START-10. Answers CTRL-RUN-STOP's question on starting while cooling: only through an explicit Start that the controller evaluates; never automatically. |
| CTRL-PROGRAM-STORE | Control CPU/protocol/storage | Save (new or replace), list, delete and start a stored program; capacity (labs: 12); persistence; rejection reasons. The labs keep programs for the session only. |
| CTRL-EVENT-LOG | Control CPU/protocol | Event log query for the Device log: event types (program start/complete/stop/pause/resume, alarms, maintenance done/due, link, display start, settings), time stamps, one-month window, paging. Builds on CTRL-EVENTS; the labs simulate a month of history. |
| CTRL-RESTART | Control CPU/Veso | Decide whether Restart furnace remains and its permissions/recovery. Deferred; no removal/execution inferred. |

HMI follow-ups: bounded volatile session log capacity/eviction/verbosity, separate
from authoritative Run Sessions/service records; Light geometry through keyboards/
dialogs; explicit Manual/cooling containment decision. Preserve pending/rejected/
stale state and no automatic command replay under ADR-0001/0003.
