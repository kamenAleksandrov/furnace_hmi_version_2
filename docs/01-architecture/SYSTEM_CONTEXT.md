# System context

The product contains two independently executing processors separated by a
communication boundary.

```text
Operator
   |
   v
HMI CPU                                  Control CPU / Furnace Controller
Zephyr + LVGL                            authoritative furnace runtime
presentation                             programs and run sessions
local drafts/preferences/cache  <---->   validation and capabilities
controller client                        timing/control/actuators/faults
HMI watchdog                             safety/interlocks/controller watchdog
```

The HMI sends requests and presents results. The controller decides whether a
request is currently legal, applies accepted operations, and publishes
authoritative state. A successful link transfer therefore cannot itself mean
that a furnace operation was accepted or completed.

The processors fail and restart independently. A running controller process
continues through HMI link loss according to controller policy and safety. A
new or restarted HMI joins the current controller session and reconstructs its
view. A controller reboot creates a new identity boundary and invalidates stale
HMI state; exact controller-side furnace behavior remains open.

## External boundaries

- The operator interacts through display and touch/input adapters.
- HMI target hardware is represented through Zephyr-native board, devicetree,
  Kconfig, driver, and platform mechanisms.
- The expected production physical link is RS-422; the application contract is
  transport-independent and a simulator uses memory/loopback transport.
- Electrical and mechanical implementation is outside repository scope. Only
  software-visible hardware contracts belong here.

See [state ownership](STATE_OWNERSHIP.md),
[controller interface boundary](../04-controller/CONTROLLER_INTERFACE_BOUNDARY.md),
and [protocol layering](../02-protocol/LAYERING.md).
