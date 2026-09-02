# ADR-0001: Control CPU authority boundary

- Status: Accepted (project mandate)
- Date: 2026-08-24
- Governing requirements: ARCH-INV-001 through ARCH-INV-004, ARCH-INV-006 through ARCH-INV-011

## Context

V1 lessons show that HMI-owned or event-reconstructed operational state can
diverge from furnace truth. HMI link loss and restart are expected conditions,
and no HMI failure may weaken furnace safety. Two processors therefore need a
clear authority and recovery boundary.

## Decision

The HMI CPU is a remote operator interface. It owns LVGL, navigation, dialogs,
local drafts and transient input, HMI preferences/cache, communication-state
presentation, its controller client, and its own health.

The Control CPU/Furnace Controller owns furnace truth, program library and
revisions, program and Manual execution, validation, limits, setpoints, timing,
PID, actuators, fan policy, faults, interlocks/protection, operational
settings/counters, authoritative clock, and controller health.

The HMI sends requests and assumes they may be rejected. It can constrain
syntax and offer assistance from a valid current-session capability snapshot,
but it cannot authoritatively approve furnace behavior. An authoritative
snapshot must enable reconstruction after HMI restart or missed events, and
reconnect never automatically replays old commands.

Normal software Stop is high priority but remains distinct from independent
emergency/safety mechanisms.

## Consequences

- The future protocol requires session identity, authoritative state recovery,
  explicit request lifecycle, and cache validity.
- Draft, accepted revision, active execution, and history are distinct states.
- Controller implementation must isolate furnace-critical work from HMI and
  library traffic.
- The HMI remains useful through rich presentation without becoming a safety or
  process authority.

## Verification and follow-up

Trace ownership in [state ownership](../01-architecture/STATE_OWNERSHIP.md) and
test reconnect, rejection, immutable execution/history, and priority behavior
under `TEST-001` through `TEST-006`.

## Review record

This decision is mandated by the approved foundation brief. Later changes
require an explicit conflicting-requirement resolution and a superseding ADR;
an implementation shortcut cannot silently weaken it.
