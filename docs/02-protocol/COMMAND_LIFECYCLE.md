# Command lifecycle principles

A transport action is not an operational result. Later message semantics must
use only the lifecycle distinctions each operation needs, while never
collapsing materially different states.

## Conceptual checkpoints

| Checkpoint | What is known | What is not yet known |
| --- | --- | --- |
| Submitted locally | HMI created a request intent | Whether any bytes left or the controller saw it |
| Transmitted | Transport accepted bytes | Whether a valid frame arrived or was decoded |
| Decoded | Receiver accepted framing/representation | Whether the operation is allowed |
| Queued | Receiver scheduled work | Whether the request will be accepted or applied |
| Accepted or rejected | Controller made its authoritative decision | For accepted asynchronous work, whether it completed |
| State changed | Authoritative state reflects an effect | Whether a longer operation is complete |
| Completed | Operation-specific completion condition occurred | Nothing beyond the defined contract |

These are semantic checkpoints, not mandated status codes or messages. A cheap
read may combine stages; a long-running or safety-relevant request may need
more explicit outcomes.

## HMI behavior

- Give immediate local feedback, then show pending/progress when the controller
  result legitimately takes longer than the ordinary response target.
- Never label a request accepted because it was queued or sent.
- Treat rejection as a normal authoritative outcome and preserve a stable
  reason suitable for operator presentation.
- Expire or invalidate pending work on an incompatible session change.
- Do not replay an uncertain request automatically on reconnect.

## Start and hot start

Start, hot-start proposal, confirmation, cancel and rejection follow the single
controller interface specification in the furnace repository `docs/interfaces/controller-hmi-interface.md` (0.8.0 draft) (PROTO-012,
[ADR-0013](../07-decisions/0013-explicit-start-and-hot-start.md)). A Start is
never replayed, queued or armed; a confirmation applies only to its own
proposal. Since 0.8.0 a proposal may require a duration acknowledgement whose
accepted duration travels with the confirmation, and a run awaiting a quality
decision accepts `RUN_CONTINUE` or `RUN_STOP`; neither is ever replayed
automatically ([ADR-0019](../07-decisions/0019-duration-decisions-and-runtime-presentation.md)).

## Later design questions

Per operation, define idempotency, duplicate recognition, timeout meaning,
late-response handling, cancellation, completion observation, persistence
boundary, and retry authority. Test these through `TEST-004` before production
use.
