# ADR-0007: Select serialization, framing, and integrity

- Status: Accepted for the controller's 0.x draft series (product owner, 2026-11-10); reviews recorded below; reopen triggers listed
- Date: 2026-08-24; decided 2026-11-10
- Owners: product owner; the controller repository owns the format
- Governing requirements: PROTO-001 through PROTO-009, TEST-004; PROTO-010 and PROTO-011 have known gaps (below)

## Context

The wire format must be explicit, bounded, versionable, robust to corruption
and duplicates, and independent of native C layout. In 2026-08 the semantics,
sizes and error environment were not defined well enough to choose, so the
decision was deferred. Since then the controller repository has specified and
built a format (furnace firmware Phase 20b, 0.16), with layout tables, golden
test vectors and a codec. Phase 21 needs the HMI to speak it.

## Decision

Use the controller's wire format of the 0.x draft series, as specified in
furnace firmware `docs/interfaces/controller-wire-format.md` at the controller
revision named in the codec manifest (ADR-0009). At acceptance that is 0.16 at
`721f3cb`. It is the only definition of the bytes, owned by the controller
repository:

- each frame carries a protocol major and minor, a 16-bit message type, a
  16-bit payload length (at most 1024), the payload, and a CRC-32/ISO-HDLC over
  all of them; the frame is COBS-encoded and ends with one `0x00`;
- multi-byte integers are little-endian; message layouts are the tables of its
  §7, from which the golden test vectors are generated;
- versions: a draft minor (major 0) must match exactly; a `HELLO` of another
  version is answered `VERSION_UNSUPPORTED`, and other frames of another
  version are dropped and counted, never read as this version;
- duplicates and retries: a request ID per client session, replay of a recorded
  result, otherwise `DUPLICATE_REQUEST` meaning unresolved (the controller
  interface's command envelope); an exhausted request ID needs a new client
  session (`HELLO`).

**Compatibility identity during the draft series.** The controller has amended
0.16 several times without changing the minor while it was unshipped. `HELLO`
therefore catches only a change of minor. Within one minor, compatibility is
the synced controller revision together with the hashes of the copied codec and
vectors (ADR-0009): the HMI and the controller it is tested against must be at
the same synced revision.

The HMI does not define its own format and proposes changes through the
controller's interface process. Raw C structures stay off the wire, and the
codec's message structures do not reach the HMI model (ADR-0003).

## Reopen triggers

This decision is reopened, not silently extended, at:

- the controller's 1.0 freeze;
- a wire change for the controller's Phase 20d messages (program library,
  settings, diagnostics), which needs a lockstep HMI resync (Phase 21 M21-6);
- the production link choice (ADR-0006): a half-duplex 2-wire RS-485 link or
  Modbus would need a different exchange pattern or framing;
- the HMI memory budget (ADR-0008, still deferred);
- a measured link error model that the CRC-32 or the 1024-byte limit does not
  fit. The integrity choice is the controller's and is to be confirmed with the
  electrical engineer once the physical link is chosen.

## Known gaps

- **PROTO-010 (service gate):** 0.16 has none; the controller's interface
  defers it (its Q15).
- **PROTO-011 (capabilities):** `CAPABILITIES` does not carry increments,
  cooling modes or per-field editability.

Both are to be raised as `CFL` entries in the controller's requirements register
and do not block Phase 21's workflows; the HMI keeps its own presentation rules
for them until the controller provides the data.

## Consequences

- The HMI implements the protocol layers above the format (ADR-0003) and does
  not maintain a second definition of the bytes.
- Every controller change to the format is followed by an HMI resync and its
  tests; during the draft series a mismatch can pass `HELLO` and is caught only
  by the revision identity above.

## Evidence at acceptance

| Required in 2026-08 | Status |
|---|---|
| Approved semantic operations and lifecycle | Met for the 0.16 operations: the controller's interface specification |
| Maximum payload classes | Met: 1024 bytes, layouts with stated sizes |
| Compatibility/version policy | Met for the draft series with the identity above; 1.0 open |
| Duplicate/retry model | Met: the command envelope |
| Cross-CPU implementation constraints | Met by ADR-0009 |
| Target memory/flash budgets | **Residual risk:** ADR-0008 deferred; a 1024-byte payload's buffers are sized in M21-3 |
| Measured link characteristics | **Residual risk:** the link is simulated; reopen trigger above |

## Historical: the 2026-08-24 deferral

The deferral and its list of options are kept for the record and are answered
by the decision above. The options were bounded hand-written or generated
representations and framed binary approaches, judged against explicit schemas,
tooling on both CPUs, cost, bounds, evolution, diagnostics and golden vectors;
the integrity mechanism was to follow the physical error model.

## Review record

2026-11-10, with the Phase 21 plan (AI reviewers acting in the HMI's review
roles; the product owner decided):

- **Protocol and architect review:** the format is stated accurately. Findings
  applied: stale "deferred" text in `docs/02-protocol`; evidence items met or
  residual; the PROTO-010 and PROTO-011 gaps; draft-minor resync with 20d; the
  codec's structures kept out of the model.
- **Decision challenger:** findings applied: the status claimed reviews not yet
  recorded; `HELLO` cannot catch changes within an unshipped minor (the
  revision identity above); reopen triggers; the electrical engineer named for
  the integrity choice; historical sections marked.
- **Unresolved:** none blocking; the residual risks are listed above.
