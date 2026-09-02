# ADR-0009: Select protocol source-sharing model

- Status: Deferred
- Date: 2026-08-24
- Governing requirements: PROTO-004, PROTO-005, TEST-004

## Context

The HMI and Control CPU need compatible protocol semantics and wire behavior,
but may use different repositories, toolchains, release cadences, and resource
constraints. Sharing all C source can reduce duplication yet increase coupling;
independent implementations improve diversity but risk semantic drift.

## Decision to defer

Do not assume that the future controller will directly compile HMI repository
sources, or that it will independently duplicate message definitions. Keep the
approved protocol contract toolchain-neutral and explicit until repository and
release constraints are known.

## Options to evaluate later

- one versioned portable C protocol package consumed by both processors;
- one schema with generated target-specific C artifacts;
- independently implemented codecs verified by shared normative vectors;
- a hybrid with shared schema/vectors and separate transport integration.

## Consequences

No shared native struct or SDK error type may become the de facto contract.
Golden vectors and compatibility/version policy remain necessary under every
option. The eventual choice must address ownership, release pinning, and
cross-repository change review.

## Evidence required to decide

Control CPU repository/toolchain constraints, code-generation policy, build
integration and licensing, release cadence, safety/independence expectations,
memory budgets, and maintenance ownership.

## Review record

Protocol-reviewer, architect, and decision-challenger review are required
before acceptance.
