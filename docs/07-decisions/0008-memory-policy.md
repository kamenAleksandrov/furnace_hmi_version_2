# ADR-0008: Define bounded memory and allocation policy

- Status: Deferred
- Date: 2026-08-24
- Governing requirements: REQ-NFR-002 through REQ-NFR-005, ARCH-INV-005, ARCH-INV-006

## Context

LVGL, graph/history data, program drafts, protocol buffering, and multiple MCU
families create different memory pressures. The first production target,
display resolution/color depth, RAM/external memory, payload limits, queue
topology, and failure policy are still open. A universal ban or unrestricted
dynamic allocation would both be premature.

## Decision to defer

Do not set a production-wide allocation policy or fixed budgets during
foundation work. Require explicit ownership, lifetime, bounds, failure handling,
and execution-context rules for every new buffer or allocation. No code may
assume external memory or silently perform unbounded work in the GUI path.

## Options to evaluate later

- compile-time/static storage for fixed critical paths;
- bounded pools/slabs for variable concurrent objects;
- controlled initialization-time allocation;
- target-appropriate LVGL allocators and external-memory placement;
- bounded streaming/windowing for graph, program, and history data.

## Consequences

Foundation interfaces must expose bounds and allocation failure rather than
hide them. Concrete optimization waits for measured budgets, avoiding false
portability based on one development machine.

## Evidence required to decide

Production target memory map, LVGL/display buffer plan, worst-case protocol and
queue sizes, graph/history requirements, stack measurements, fragmentation
evidence, and fault/recovery requirements.

## Foundation implementation note — 2026-08-27

The first availability view owns a fixed, known LVGL object tree with an
explicit GUI-context lifetime. Direct object-constructor NULL results are
checked and a partial tree is removed. Pinned LVGL has internal allocation and
style paths that do not return recoverable errors; its current assertion
configuration treats those failures as fatal. The component documents that
limit instead of claiming recoverable out-of-memory behavior.

This is a transparent foundation constraint, not the production memory-policy
decision. Before target UI work can claim bounded recovery, this ADR still
needs the selected target memory map, LVGL allocator/configuration, measured
worst case, and an explicit fatal-versus-recovery policy.

## Review record

Embedded-reviewer, software-critic, and decision-challenger review are required
before acceptance.
