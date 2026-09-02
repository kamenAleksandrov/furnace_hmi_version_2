# ADR-0004: Maintain a project-owned desktop LVGL simulator

- Status: Accepted
- Date: 2026-08-24
- Governing requirements: REQ-NFR-005, TEST-002 through TEST-008

## Context

Fast UI development and deterministic recovery/fault testing should not depend
on production hardware. An upstream LVGL example can prove APIs but does not
define this application's module boundaries.

## Decision

Maintain a project-owned C/CMake desktop application using LVGL and SDL2 unless
strong evidence supports another host driver. It will reuse the portable
presentation, application, model, and controller-client sources used by Zephyr;
only host adapters and entry points differ.

Develop two complementary simulation levels: a semantic simulated controller
for rapid UI scenarios, then the real approved codec/framer over memory/loopback
transport for communication integration and fault injection.

The foundation proof contains only a generic window/render/input/debug path,
not furnace screens or production protocol semantics.

## Consequences

- Host build and dependency versions require reproducible maintenance.
- Portable modules cannot depend directly on Zephyr or SDL APIs.
- The simulator must not become a second source of controller truth or drift
  from embedded behavior.

## Verification and follow-up

Exercise shared sources in both targets and add deterministic scenario and
loopback tests. Track host setup in the development-environment document.

## Review record

Desktop simulation and SDL2 preference are approved. A host-driver change
requires evidence; production UI/domain behavior remains future work.
