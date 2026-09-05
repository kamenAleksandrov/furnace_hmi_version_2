# V1 lessons

These observations are stakeholder-reported lessons supplied for V2 planning;
the legacy source has not been audited in this repository. They are design
evidence, not a specification to reproduce V1.

| Lesson | V2 consequence |
| --- | --- |
| HMI responsibility can creep into control authority. | Make ownership explicit and review crossings against `ARCH-INV-001`. |
| Locally reconstructed run state can diverge from controller truth. | Recover from an authoritative snapshot, not only event history. |
| A dropped lifecycle event can desynchronize the UI. | Model command lifecycle and resynchronization deliberately. |
| Storage work can block Stop or the UI. | Separate priorities and bound GUI work; do not put blocking I/O on the critical interaction path. |
| Queued is not accepted. | Distinguish transport, decode, queue, acceptance, state change, and completion. |
| Manual mode modeled as a fake long program obscures its lifecycle. | Treat Manual as a first-class controller mode. |
| Inferred stage intent can disagree with operator intent. | Move toward explicit HEATING/HOLD/COOL stage types; semantics remain open. |
| Screen layout is a poor source of operational limits. | Obtain current-session capability metadata and validate authoritatively on the controller. |
| HMI task time is not an authoritative service counter. | Keep monotonic operational counters on the Control CPU. |
| Estimates can be mistaken for measurements. | Preserve engineering-value provenance and label estimates explicitly. |
| Reconnect without a recovery model leaves stale state and commands. | Identify the controller session, invalidate stale data, fetch truth, and never auto-replay. |
| Native C layout is not a protocol. | Define a versioned serialized representation with explicit types, units, and integrity behavior later. |

These lessons motivate the [requirements](REQUIREMENTS.md) and
[architecture invariants](../01-architecture/BOUNDARIES_AND_INVARIANTS.md).
