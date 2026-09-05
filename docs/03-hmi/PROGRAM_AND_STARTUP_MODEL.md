# Program and startup model

This is the reversible HMI/application design baseline for the first UI
implementation. It is not a wire schema, controller implementation, or safety
case. The Control CPU remains the authority for accepted programs, running
processes, limits, timing, alarms, and recovery.

## Startup and reconnect

The HMI must not infer that the furnace or controller is powered off merely
because a transport is unavailable. The presentation distinguishes at least
these states:

1. `UNAVAILABLE`: local UI is initialized, but no usable controller
   relationship exists; operational values and actions fail closed.
2. `CONNECTING`: transport work is asynchronous and the GUI remains responsive.
3. `HANDSHAKING`: protocol/version/capability negotiation is in progress.
4. `SYNCHRONIZING`: the HMI has a controller session identity and is obtaining
   a coherent authoritative snapshot.
5. `CURRENT`: a complete snapshot is available for the identified session.
6. `STALE`: a previous observation is retained only for context and is visibly
   marked stale; it is never treated as current.
7. `INCOMPATIBLE`: the peer or capabilities cannot support this HMI release.

The semantic sequence is:

```text
initialize local UI/preferences/drafts
  -> establish transport
  -> negotiate version and capabilities
  -> identify controller boot/session epoch
  -> invalidate old session-bound cache and pending requests
  -> request one coherent authoritative snapshot
  -> render the controller's idle, Manual, or active-Program state
```

An HMI restart or reconnect never resumes, reissues, or implicitly starts an
old command. If the controller is already running, the HMI reconstructs the
corresponding dashboard from the snapshot. If the controller changes session,
previous observations and pending results are invalidated before new state is
shown.

The coherent snapshot must also expose any controller-defined service condition
needed to explain a proposed-run warning. A prior session's service status is
discarded with the rest of session-bound observations. An HMI acknowledgement
is never restored, replayed, or treated as a local authorization.

## Program and run objects

The HMI may own an unsaved draft, but it does not mint authoritative program
IDs or revisions. A stage's display position is derived from its containing
ordered collection; it is not an identity field. A draft can use a local row
key to keep selection stable while the operator edits or reorders stages.

Conceptual objects (names are provisional):

```text
ProgramDraft {
    local_draft_key
    optional base_program_id, base_revision
    name
    ordered StageDraft[]
    optional saved_ambient_temperature_for_estimation
    dirty
}

StageDraft {
    local_row_key
    optional label
    kind: HEATING | HOLD | COOL
    kind_specific_parameters
}

ProgramSummary {
    controller program_id, latest_revision, name, stage_count
    controller-provided summary
}

AcceptedProgramRevision {
    controller program_id, immutable revision
    name, ordered accepted stages, controller-provided summary
}

ActiveRunSnapshot {
    run/session identity
    accepted program ID/revision or Manual mode
    current stage/phase, timing, run state
    observed values with provenance and validity
}

RunSession {
    immutable historical record; schema and retention remain open
}
```

`ProgramDraft`, `AcceptedProgramRevision`, `ActiveRunSnapshot`, and `RunSession`
are deliberately separate. Live progress never mutates a library definition;
editing a draft during a run cannot alter the accepted revision executing on
the controller.

### Operator-facing program terminology

The UI shall call the entry action **New program** and the edit surface
**Program editor**. It shall not display a local candidate as a library program
or imply that the HMI owns a controller revision. “Draft” remains an internal
engineering term for transient editor data only: it is neither an HMI-persisted
program nor controller acceptance. A future save or stage-change request must
receive Control CPU validation before the authoritative library changes. The
protocol decision remains whether individual field/stage edits receive an
online validation response or the controller validates an atomic Program save.

### Program authoring assistance

`ProgramDraft` means a local, editable, unsaved copy. It is a useful place to
create a new program or change a saved program, but it is neither a saved
program nor evidence that the Control CPU has accepted it. The HMI may use a
current-session capability snapshot to offer range and relationship assistance;
it submits the result for controller validation and shows the authoritative
accepted or rejected outcome.

The stakeholder workflow currently needs three stage kinds:

- `HEATING`: target temperature and positive target delta (degrees Celsius per
  minute) are editable, and duration is also editable. Each edit makes the
  edited value the local driver and recalculates the other rate/duration value
  from the draft estimation context. Initial controls use 1 °C, 0.1 °C/min,
  and 1-minute increments in preparation for keyboard entry. Exact start
  temperature, rounding, limits, and validation remain controller-domain work.
- `HOLD`: duration is editable; the target is inherited from the preceding
  stage and displayed as inherited. A first-stage HOLD is a delayed-start
  preview at saved ambient; the actual controller run begins from its measured
  current temperature.
- `COOL`: target is editable. A controlled cooling rate is optional; no rate
  asks the controller for its fastest permitted cooling. Exact actuator and
  interlock behavior remains controller-owned.

The HMI will not hard-code those bounds as authority. A stage card's visual
order derives from the `ProgramDraft` ordered collection; it is not a durable
stage identity.

## Estimation and graph rules

When a program is saved, its ambient temperature is an estimation input only.
The active run starts from the controller's measured current temperature and
continues to update from live measurements. The saved value must never be
displayed as a sensor reading. Units, bounds, persistence owner, and the exact
metadata returned by the controller remain open.

The planned trajectory uses the same accepted stage semantics as the
controller. The visual baseline is a muted solid planned line and a
contrasting solid measured line drawn above it. Invalid or missing measurements create gaps;
the HMI does not draw imaginary data. A static status and data presentation
remains available if a decorative furnace animation is disabled or unavailable.

The graph's initial domain contains the full predicted program, all current
measured history, and deliberate time/temperature headroom. It is retained
while measurements stay inside it and is expanded only when live time or
measured temperature exceeds that domain. An HMI domain expansion is a display
response only; it must not diagnose or command a furnace fault. It uses sharp rectangular
plot bounds, readable grid sections, time and temperature axes, a legend, and
the measured path drawn after the planned path. Future stage transition
annotations and in-plot floating labels are technically feasible once their
controller-owned timing/identity data are defined; they are not yet semantics.

The host-lab program preview may display clickable visual stage nodes. They are
local links to a draft-editor preview and illustrated timing only; they do not
represent controller-accepted stage transition data.

The target-delta term means a temperature change per time period, normally
degrees Celsius per minute. HEATING lets the operator drive the local estimate
from rate or duration and shows the calculated counterpart. Limits, rounding, actual start temperature, and
all execution semantics remain controller-defined.

## Service-period acknowledgement

`ServiceRunCondition` is controller-owned observation, not a local setting. A
future current-session snapshot may contain a service counter, configured due
point, calculated forecast completion, and a gate such as `CLEAR`,
`ACKNOWLEDGEMENT_REQUIRED`, or `BLOCKED`. These fields are deliberately model
semantics, not a wire schema.

Before a proposed Start, the HMI can present the one controller-issued service
acknowledgement modal. On confirmation it later sends one correlated request;
delivery is not acceptance. The Control CPU records an accepted acknowledgement
for later Device/maintenance inspection and may permit ordinary operation
thereafter, subject to its other checks. HMI restart, reconnect, or a local
modal dismissal must not issue, repeat, or suppress the event. Threshold,
re-arm-after-service, role, and retention details remain controller policy.

## Operator information architecture proposal

This is a host-lab presentation proposal, not an operational state machine:

- **Home** presents a static 2.5D furnace overview with observed temperature,
  fan RPM, and door status as floating readouts. A compact quick-launch column
  puts bounded favourite programs first, then recent programs, followed by
  `Choose a program`.
- **Program loading** presents the selected accepted program's summary and
  planned trajectory before it offers a Start request. Starting opens the
  dedicated Program-running view only after later controller acceptance.
- **Running** is separate from Home. It keeps elapsed/remaining time, stage and
  phase, current and target temperature, estimated power, and controller state
  always visible. Its graph/visual area supports full graph, split, and full
  visual presentation. While the current Program-run workflow is active, it
  suppresses ordinary bottom navigation until normal Stop; this is a product
  presentation rule, not a controller permission or safety mechanism. Pause,
  Resume, and normal Stop remain request previews in the lab, not local state
  changes.
- **Programs** uses bounded cards and tap pagination. A program overview
  separates program facts and graph from a stage list. Program and stage entry
  actions open the Program editor; no scrolling or drag-to-edit is required.

Device and Settings retain their existing placeholders in this pass. Service
condition/history belongs under Device/maintenance information, not Program
authoring.

## Interaction and safety presentation

Operator editing is tap/click based; drag-to-edit is not required. Interactive
targets should normally be at least 48 x 48 px. Pause, Resume, and normal Stop
are explicit controller requests with visible pending/result states. Delete,
clear, restart, and reset remain separate operations with separately defined
ownership and permissions. Emergency Stop is outside the HMI safety function.

The main run view should prioritize stage, target/error, rate, demand, timing,
fan/door/interlock state, fault severity, and sensor validity. Raw PID P/I/D
terms are better suited to a role-appropriate service/diagnostics view after
their units, permissions, and controller provenance are defined.

## Verification scenarios

The host-only UI laboratory should exercise these semantic cases without
writing LVGL objects from the simulator:

- disconnected startup is responsive and fail-closed;
- startup during an active Program or Manual run reconstructs from a snapshot;
- link loss marks observations stale and does not replay Start/Save/Delete;
- a controller session change invalidates cache/capabilities/pending results;
- partial or out-of-order snapshots never appear as one current state;
- a restored local draft is visibly unsaved and detects base-revision conflict;
- save/start rejection shows a stable controller reason after pending feedback;
- stage reorder/delete preserves ordered collection identity;
- graphs omit unavailable measured samples and separate estimates from measured
  values.

These scenarios map to `ARCH-INV-001`, `ARCH-INV-003`, `ARCH-INV-004`,
`ARCH-INV-007`, `ARCH-INV-009`, `REQ-FUN-002`, `REQ-FUN-007`, `REQ-FUN-009`,
`REQ-FUN-012`, `REQ-NFR-005`, `REQ-NFR-006`, `REQ-NFR-007`, and `PROTO-002`.
Unresolved details are tracked in [Open questions](../09-progress/OPEN_QUESTIONS.md).
