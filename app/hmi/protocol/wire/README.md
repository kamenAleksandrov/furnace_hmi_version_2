# Controller wire codec: a pinned copy

**Do not edit the files under `include/` and `src/`.** They are the controller's
codec (furnace firmware `core/wire` and `core/crc`), copied at the revision named
in [CONTROLLER_SYNC.json](CONTROLLER_SYNC.json), as
[ADR-0009](../../../../docs/07-decisions/0009-protocol-source-sharing.md) decides.
The bytes they produce are defined by the controller's wire format
([ADR-0007](../../../../docs/07-decisions/0007-serialization-and-framing.md)).

| File | Owner |
|---|---|
| `include/`, `src/` | The controller repository: copied, never edited here |
| `CONTROLLER_SYNC.json` | Written by `tools/sync_controller_wire.py --sync` |
| `CMakeLists.txt`, this README | The HMI |

The copied files keep the controller's formatting and SPDX headers (both
repositories are Apache-2.0), and are outside the HMI's formatting and lint
ownership. The controller's golden vectors come with them, as the generated table
in [`tests/conformance/wire`](../../../../tests/conformance/wire), and run in
every HMI build: CTest `host.wire_conformance` (MSVC and GCC) and the Zephyr test
`furnace_hmi.wire.conformance`.

## Checking and syncing

```bash
python tools/sync_controller_wire.py --check                       # the copy matches its manifest
python tools/sync_controller_wire.py --against <furnace-firmware>   # and the controller at that revision
python tools/sync_controller_wire.py --sync <furnace-firmware> --revision <rev>
```

`--against` and `--sync` read the controller with git and run its vector script,
so they need a controller checkout and Python 3 with the controller's tools (in
practice, WSL). CTest runs `--check` as `tools.controller_wire_copy`.

**A change to the codec,** including a fix for a compiler warning, is made in the
controller repository first, then synced here as one reviewed change naming the
controller revision. During the 0.x draft series the HMI and the controller it is
tested against must be at the same synced revision (ADR-0007).
