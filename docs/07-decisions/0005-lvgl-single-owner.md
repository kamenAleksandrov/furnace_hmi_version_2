# ADR-0005: Give LVGL one execution owner

- Status: Accepted
- Date: 2026-08-24
- Governing requirements: ARCH-INV-005, REQ-NFR-004, TEST-005

## Context

Arbitrary cross-thread LVGL access creates race and lifetime hazards, while
blocking storage, communication, retries, or large computations in the GUI
context makes interaction and Stop presentation unresponsive.

## Decision

Only the designated GUI execution context may create, read through, or mutate
LVGL objects. Other Zephyr or host contexts exchange plain application data and
intents through explicit bounded mechanisms. The GUI context processes bounded
work and does not synchronously perform unbounded filesystem, controller,
storage, retry, or bulk-processing operations.

## Consequences

- All cross-context UI updates require an ownership-preserving handoff.
- Queue capacity, overflow/coalescing, data lifetime, and shutdown behavior must
  be designed and tested rather than hidden in callbacks.
- Desktop code follows the same rule so host-only races do not normalize an
  invalid embedded design.

## Verification and follow-up

Architecture and code review enforce the boundary. `TEST-005` covers ownership,
bounds, overflow, and lifetime behavior when concurrency scaffolding exists.

## Review record

The single-owner rule is an approved invariant. It may be implemented with
platform-appropriate primitives but not bypassed for convenience.
