# Control CPU interface boundary

This repository does not implement controller control or safety behavior. It
defines what the HMI must rely on from the Control CPU and what it must never
assume authority to replace.

## Controller responsibilities visible to the HMI

The Control CPU owns and, through the future interface, exposes suitable views
of authoritative furnace state, controller identity, active run/revision,
mode/phase, temperature validity, setpoint/demand, readiness, faults,
capabilities, program library, run history, operational settings/counters, and
clock. Exposure does not transfer ownership.

Every meaningful request is authoritatively validated against structure,
required fields, ranges, current machine state and capabilities, temperature
validity, program legality, interlocks, and operational/safety restrictions.
The HMI must handle rejection.

## Priority boundary

The controller must isolate control loops, sensor work, actuator handling,
safety/interlocks, and its watchdog from lower-priority HMI traffic and library
operations. Telemetry, large history reads, and program saves cannot be allowed
to starve furnace-critical work. The eventual controller scheduling and memory
policy is outside HMI implementation authority but is a cross-system contract.

## Restart and availability

HMI disconnection does not automatically stop an active process. The
controller continues under its own logic and safety policy. The HMI detects
loss and reconstructs from current truth when communication returns.

A controller restart is more consequential. The HMI may request it only if the
current controller explicitly advertises permission. The exact furnace
behavior through controller restart is open. A new boot/session always
invalidates incompatible HMI state.

## Settings and counters

For controller-owned settings, the future interface should expose value,
writable/read-only state, range, step/precision, capability state, and a reason
for read-only state where relevant. The controller validates submitted changes
again. Operational counters are appropriately sized monotonic controller
values; an HMI task clock cannot define service time.

## Safety statement

Controller-side watchdog, interlock, actuator, and emergency behavior remain
controller/hardware concerns. The HMI can present and request; no safety
mechanism may rely on correct HMI execution.
