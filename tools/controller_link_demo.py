#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run the controller and the Zephyr HMI as two programs over a simulated serial link.

SIMULATION ONLY (Phase 21 M21-3). Linux (WSL) only: both programs are native_sim
builds. The controller is the furnace firmware's ``tests/sim/link_pty`` program,
built at the revision in ``app/hmi/protocol/wire/CONTROLLER_SYNC.json``: the
controller runtime with its simulated plant. Its temperatures are the controller's
simulation, never furnace measurements.

The relay holds the HMI's end of the link, a pseudo-terminal linked from
``/tmp/furnace-hmi-controller-link`` (the HMI's default device), and copies bytes to
and from the controller's pseudo-terminal. A controller restart therefore leaves the
HMI's device in place. The relay also reads the frames it copies, to check what the
HMI reports against what the controller sent, and injects the scenario's faults.

Scenarios, in order:
  1. connect: handshake, synchronization, CURRENT with the controller's own data;
  2. cut: the cable cut for 4 s (STALE), then restored (CURRENT, same session);
  3. corrupt: garbage and a damaged frame in the stream (dropped, still CURRENT);
  4. controller restart: a new controller process (new session, CURRENT again);
  5. HMI restart: a new HMI process (a new client session, CURRENT again).

Usage:
  controller_link_demo.py --controller <link_pty zephyr.exe> --hmi <HMI zephyr.exe>
                          [--controller-checkout <furnace firmware>] [--window]
"""

from __future__ import annotations

import argparse
import json
import os
import re
import select
import signal
import subprocess
import sys
import time
import tty
import zlib
from dataclasses import dataclass, field
from pathlib import Path

LINK_PATH = "/tmp/furnace-hmi-controller-link"
REPO = Path(__file__).resolve().parent.parent
SYNC = REPO / "app" / "hmi" / "protocol" / "wire" / "CONTROLLER_SYNC.json"

STATUS = 0x8003
FAULT_LIST = 0x8043
CLIENT_READ_MESSAGES = {0x0001: "HELLO", 0x0003: "HEARTBEAT", 0x0004: "SNAPSHOT_REQUEST",
                        0x0005: "CAPABILITIES_REQUEST"}
PTY_LINE = re.compile(rb"(\S+) connected to pseudotty: (/dev/pts/\d+)")
TRACE_LINE = re.compile(r"hmi-link: (.*)")


def cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    while i < len(data):
        code = data[i]
        if code == 0:
            raise ValueError("zero in COBS data")
        out += data[i + 1:i + code]
        i += code
        if code != 0xFF and i < len(data):
            out.append(0)
    return bytes(out)


class FrameTap:
    """Reads the frames in one direction of the stream, apart from both programs."""

    def __init__(self) -> None:
        self.buffer = bytearray()

    def feed(self, data: bytes):
        self.buffer += data
        while 0 in self.buffer:
            at = self.buffer.index(0)
            raw = bytes(self.buffer[:at])
            del self.buffer[:at + 1]
            try:
                frame = cobs_decode(raw)
            except ValueError:
                continue
            if len(frame) < 10 or zlib.crc32(frame[:-4]) != int.from_bytes(frame[-4:], "little"):
                continue
            yield int.from_bytes(frame[2:4], "little"), frame[6:-4], (frame[0], frame[1])


@dataclass
class ControllerStatus:
    session: str
    sequence: int
    temperature: int
    validity: int
    run_state: int
    inhibited: int
    latched: int
    unacknowledged: int
    at: float
    process: int


@dataclass
class Relay:
    controller: subprocess.Popen | None = None
    controller_fd: int | None = None
    hmi: subprocess.Popen | None = None
    master_fd: int | None = None
    connected: bool = True
    hmi_trace: dict = field(default_factory=dict)
    hmi_lines: int = 0
    statuses: list = field(default_factory=list)
    client_messages: dict = field(default_factory=dict)
    forbidden: list = field(default_factory=list)
    to_hmi_tap: FrameTap = field(default_factory=FrameTap)
    to_controller_tap: FrameTap = field(default_factory=FrameTap)
    out_buffers: dict = field(default_factory=dict)
    controller_ready: bool = False
    controller_starts: int = 0
    fault_total: int | None = None

    # -- processes ------------------------------------------------------------------------

    def start_controller(self, binary: str) -> None:
        # The simulated controller's entropy (sim/app_io) starts from the same state in every
        # process, so each boot draws the same session_id: a restart then tests the HMI's
        # repeated-ID rule (a snapshot older than the status held).
        self.controller = subprocess.Popen([binary], stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT)
        os.set_blocking(self.controller.stdout.fileno(), False)
        self.controller_ready = False
        self.controller_fd = None
        self.controller_starts += 1
        self.out_buffers[self.controller.stdout.fileno()] = bytearray()
        deadline = time.monotonic() + 15.0
        while self.controller_fd is None or not self.controller_ready:
            if time.monotonic() > deadline:
                raise RuntimeError("the controller did not announce its link")
            self.pump(0.1)

    def stop_controller(self) -> None:
        if self.controller is not None:
            self.controller.send_signal(signal.SIGTERM)
            self.controller.wait(timeout=10)
            self.out_buffers.pop(self.controller.stdout.fileno(), None)
            self.controller = None
        if self.controller_fd is not None:
            os.close(self.controller_fd)
            self.controller_fd = None

    def start_hmi(self, binary: str, window: bool) -> None:
        env = dict(os.environ)
        if not window:
            env["SDL_VIDEODRIVER"] = "offscreen"
        self.hmi = subprocess.Popen([binary, "--seed-random"], stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, env=env)
        os.set_blocking(self.hmi.stdout.fileno(), False)
        self.out_buffers[self.hmi.stdout.fileno()] = bytearray()
        self.hmi_trace = {}

    def stop_hmi(self) -> None:
        if self.hmi is not None:
            self.hmi.send_signal(signal.SIGTERM)
            self.hmi.wait(timeout=10)
            self.out_buffers.pop(self.hmi.stdout.fileno(), None)
            self.hmi = None

    # -- the relay ------------------------------------------------------------------------

    def open_hmi_end(self) -> None:
        self.master_fd, slave_fd = os.openpty()
        tty.setraw(slave_fd)
        tty.setraw(self.master_fd)
        os.set_blocking(self.master_fd, False)
        if os.path.lexists(LINK_PATH):
            os.unlink(LINK_PATH)
        os.symlink(os.ttyname(slave_fd), LINK_PATH)
        self.slave_fd = slave_fd  # kept open, so the HMI's device outlives an HMI restart

    def on_controller_line(self, line: bytes) -> None:
        match = PTY_LINE.search(line)
        if match and b"1" in match.group(1):
            fd = os.open(match.group(2), os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            tty.setraw(fd)
            self.controller_fd = fd
        if b"sim link: ready" in line:
            self.controller_ready = True

    def on_hmi_line(self, line: bytes) -> None:
        match = TRACE_LINE.search(line.decode("utf-8", "replace"))
        if match:
            self.hmi_trace = dict(item.split("=", 1) for item in match.group(1).split())
            self.hmi_lines += 1

    def pump(self, seconds: float) -> None:
        end = time.monotonic() + seconds
        while True:
            remaining = end - time.monotonic()
            if remaining <= 0:
                return
            fds = [fd for fd in (self.master_fd, self.controller_fd) if fd is not None]
            fds += list(self.out_buffers)
            ready, _, _ = select.select(fds, [], [], min(remaining, 0.05))
            for fd in ready:
                try:
                    data = os.read(fd, 4096)
                except (BlockingIOError, OSError):
                    continue
                if not data:
                    continue
                if fd == self.controller_fd:
                    self.from_controller(data)
                elif fd == self.master_fd:
                    self.from_hmi(data)
                else:
                    self.output(fd, data)

    def output(self, fd: int, data: bytes) -> None:
        buffer = self.out_buffers[fd]
        buffer += data
        while b"\n" in buffer:
            line, _, rest = bytes(buffer).partition(b"\n")
            buffer[:] = rest
            if self.controller is not None and fd == self.controller.stdout.fileno():
                self.on_controller_line(line)
            elif self.hmi is not None and fd == self.hmi.stdout.fileno():
                self.on_hmi_line(line)

    def from_controller(self, data: bytes) -> None:
        for msg_type, payload, _ in self.to_hmi_tap.feed(data):
            if msg_type == STATUS and len(payload) >= 39:
                # Wire format 0.16 STATUS: id128, u32 x3, measured (i32, enum8, u32), enum8
                # operating_mode, array[8] restrictions (u8 count first), group fault_status,
                # group start_status (10 bytes), then run_status starting with run_state.
                faults = 39 + payload[38]
                if len(payload) < faults + 31:
                    continue
                self.statuses.append(ControllerStatus(
                    session=payload[0:4].hex(),
                    sequence=int.from_bytes(payload[16:20], "little"),
                    temperature=int.from_bytes(payload[28:32], "little", signed=True),
                    validity=payload[32], inhibited=payload[faults], latched=payload[faults + 2],
                    unacknowledged=payload[faults + 11], run_state=payload[faults + 30],
                    at=time.monotonic(), process=self.controller_starts))
            elif msg_type == FAULT_LIST and len(payload) >= 10 and payload[9] == 0:
                self.fault_total = payload[8]
        if self.connected and self.master_fd is not None:
            os.write(self.master_fd, data)

    def from_hmi(self, data: bytes) -> None:
        for msg_type, _, _ in self.to_controller_tap.feed(data):
            name = CLIENT_READ_MESSAGES.get(msg_type)
            if name is None:
                self.forbidden.append(hex(msg_type))
            else:
                self.client_messages[name] = self.client_messages.get(name, 0) + 1
        if self.connected and self.controller_fd is not None:
            os.write(self.controller_fd, data)

    def inject_to_hmi(self, data: bytes) -> None:
        os.write(self.master_fd, data)

    def wait_for(self, predicate, seconds: float) -> bool:
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            self.pump(0.1)
            if self.hmi_trace and predicate(self.hmi_trace):
                return True
        return False


def check_controller_revision(checkout: str | None) -> str:
    revision = json.loads(SYNC.read_text(encoding="utf-8"))["controller_revision"]
    if checkout:
        head = subprocess.run(["git", "-C", checkout, "rev-parse", "HEAD"], check=True,
                              capture_output=True, text=True).stdout.strip()
        if head != revision:
            raise SystemExit(f"controller checkout at {head[:7]}, the codec copy at {revision[:7]}")
    return revision


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--controller", required=True)
    parser.add_argument("--hmi", required=True)
    parser.add_argument("--controller-checkout")
    parser.add_argument("--window", action="store_true", help="show the HMI window (WSLg)")
    args = parser.parse_args()

    revision = check_controller_revision(args.controller_checkout)
    print(f"controller revision {revision[:7]} (the HMI's codec copy)")
    relay = Relay()
    results: list[tuple[str, bool, str]] = []

    def record(name: str, ok: bool, detail: str) -> None:
        results.append((name, ok, detail))
        print(f"{'PASS' if ok else 'FAIL'} {name}: {detail}")

    def current(t: dict) -> bool:
        return t.get("link") == "CONNECTED" and t.get("availability") == "CURRENT"

    def matches_controller(t: dict) -> tuple[bool, str]:
        """The HMI's latest values are a STATUS the current controller process sent."""
        sequence = int(t.get("status", "0"))
        sent = [s for s in relay.statuses if s.sequence == sequence and s.session == t["session"]
                and s.process == relay.controller_starts]
        if not sent:
            return False, f"no controller STATUS {sequence} of session {t.get('session')}"
        s = sent[-1]
        ok = (int(t["temp"]) == s.temperature and int(t["validity"]) == s.validity
              and int(t["run_state"]) == s.run_state and int(t["inhibited"]) == s.inhibited
              and int(t["latched"]) == s.latched and int(t["unack"]) == s.unacknowledged)
        return ok, (f"session {s.session}, STATUS {s.sequence}: temperature {s.temperature / 10:.1f}"
                    f" (validity {s.validity}), run state {s.run_state}, heating inhibited "
                    f"{s.inhibited}, latched class {s.latched}, unacknowledged {s.unacknowledged}"
                    f", as the controller sent them")

    try:
        relay.open_hmi_end()
        relay.start_controller(args.controller)
        relay.start_hmi(args.hmi, args.window)

        # 1. connect
        ok = relay.wait_for(current, 20.0)
        t = dict(relay.hmi_trace)
        record("connect", ok and t.get("proto") == "0.16",
               f"link {t.get('link')}, availability {t.get('availability')}, protocol "
               f"{t.get('proto')}, firmware {t.get('fw')}, client {t.get('client')}")
        if ok:
            same, detail = matches_controller(t)
            record("authentic STATUS", same, detail)
            record("fault list as sent",
                   relay.fault_total is not None and t.get("outdated") == "0"
                   and int(t.get("faults", "-1")) == relay.fault_total,
                   f"{t.get('faults')} entries in the HMI, FAULT_LIST total {relay.fault_total} "
                   f"from the controller; outdated {t.get('outdated')}")
        session_before = t.get("session")

        # 2. cut
        relay.connected = False
        stale = relay.wait_for(lambda t: t.get("availability") == "STALE", 6.0)
        relay.pump(1.0)
        relay.connected = True
        back = relay.wait_for(current, 10.0)
        t = dict(relay.hmi_trace)
        record("cut and restore", stale and back and t.get("session") == session_before
               and t.get("changes") == "0",
               f"stale during the cut {stale}; current again {back}, same session "
               f"{t.get('session') == session_before}")

        # 3. corrupt
        dropped_before = int(t.get("dropped", "0"))
        relay.inject_to_hmi(bytes([0x13, 0x37, 0x00, 0x05, 0xFF, 0x11, 0x00]) * 20)
        relay.pump(2.0)
        t = dict(relay.hmi_trace)
        record("corruption", current(t) and int(t.get("dropped", "0")) > dropped_before,
               f"dropped {dropped_before} -> {t.get('dropped')}, link {t.get('link')}")

        # 4. controller restart
        relay.stop_controller()
        silent = relay.wait_for(lambda t: t.get("availability") != "CURRENT", 6.0)
        relay.start_controller(args.controller)
        back = relay.wait_for(lambda t: current(t) and t.get("changes") == "1", 20.0)
        t = dict(relay.hmi_trace)
        repeated = t.get("session") == session_before
        record("controller restart", silent and back,
               f"not current while down {silent}; session {t.get('session')} (was "
               f"{session_before}{', repeated: detected by the status regression rule' if repeated else ''}"
               f"), session changes {t.get('changes')}, regressions {t.get('regressions')}, "
               f"current {back}")
        if back:
            same, detail = matches_controller(t)
            record("authentic STATUS after restart", same, detail)

        # 5. HMI restart
        client_before = t.get("client")
        relay.stop_hmi()
        relay.start_hmi(args.hmi, args.window)
        back = relay.wait_for(current, 20.0)
        t = dict(relay.hmi_trace)
        record("HMI restart", back and t.get("client") != client_before,
               f"new client session {t.get('client')} (was {client_before}), current {back}")

        record("only read messages from the HMI", not relay.forbidden,
               f"sent: {relay.client_messages}; other: {relay.forbidden or 'none'}")
        gaps = [b.at - a.at for a, b in zip(relay.statuses, relay.statuses[1:])
                if a.process == b.process]
        record("controller STATUS cadence", bool(gaps) and max(gaps) < 0.5,
               f"{len(relay.statuses)} STATUS frames from {relay.controller_starts} controller "
               f"processes; longest gap within one process {max(gaps) if gaps else 0:.2f} s "
               f"(interval 0.2 s, threshold 0.5 s) through the cut, the corruption and the HMI "
               f"restart. This is the controller's link service; its control timing is the "
               f"controller's own verification, not repeated here")
    finally:
        relay.stop_hmi()
        relay.stop_controller()
        if os.path.lexists(LINK_PATH):
            os.unlink(LINK_PATH)

    failed = [name for name, ok, _ in results if not ok]
    print(f"{len(results) - len(failed)} of {len(results)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
