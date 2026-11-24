#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run the HMI's operator workflows against the controller over a simulated serial link.

SIMULATION ONLY (Phase 21 M21-5, M21-8). Linux (WSL) only: both programs are native_sim
builds. The controller is the furnace firmware's ``tests/sim/link_pty`` program (with
its scenario channel, firmware PR #53). The HMI side is one of two suites:

- ``client``: ``tests/zephyr/controller_workflows``, which drives the HMI's controller
  link and command mailbox and checks every result;
- ``operator``: ``tests/zephyr/operator_workflows`` (M21-8), which taps the HMI's own
  screens. Its link runs through a relay here, which holds the bytes on the suite's
  request (``--relay_port``: lines ``hold`` and ``release``, each answered ``ok``), so
  the link can be lost and come back while both programs run.

Its temperatures and faults are the controller's simulation, never furnace measurements.

Usage:
  controller_workflow_check.py --controller <link_pty zephyr.exe>
                               --workflows <workflows zephyr.exe> [--suite client|operator]
"""

from __future__ import annotations

import argparse
import os
import re
import select
import subprocess
import sys
import threading
import time
import tty

TIMEOUT_S = 900.0


def make_raw(path: str) -> None:
    """Raw bytes on a pseudo-terminal: no echo, no line editing, no newline translation."""
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
    try:
        tty.setraw(fd)
    finally:
        os.close(fd)


def announced_ports(proc: subprocess.Popen) -> tuple[str, str] | None:
    """The controller's link and scenario pseudo-terminals; None at the deadline."""
    text = b""
    deadline = time.monotonic() + 30.0
    while time.monotonic() < deadline:
        ready, _, _ = select.select([proc.stdout], [], [], 0.2)
        if ready:
            chunk = os.read(proc.stdout.fileno(), 4096)
            if not chunk:
                return None
            text += chunk
            link = re.search(rb"uart_1 connected to pseudotty: (\S+)", text)
            scenario = re.search(rb"uart_2 connected to pseudotty: (\S+)", text)
            if link and scenario:
                return link.group(1).decode(), scenario.group(1).decode()
    return None


class Relay:
    """Copies the link's bytes between the controller and the HMI, or drops them while held.

    The HMI gets a pseudo-terminal of its own; a second one carries the control lines.
    Both slave ends stay open here, so the masters never read EIO between opens.
    """

    def __init__(self, controller_port: str) -> None:
        self.controller = os.open(controller_port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        tty.setraw(self.controller)
        self.hmi_master, self._hmi_slave = os.openpty()
        tty.setraw(self._hmi_slave)
        self.control_master, self._control_slave = os.openpty()
        tty.setraw(self._control_slave)
        self.hmi_port = os.ttyname(self._hmi_slave)
        self.control_port = os.ttyname(self._control_slave)
        self.held = False
        self.dropped = 0
        self.stopping = False
        self._line = b""
        self.thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self.thread.start()

    def stop(self) -> None:
        self.stopping = True
        self.thread.join(timeout=2.0)

    def _control(self, data: bytes) -> None:
        self._line += data
        while b"\n" in self._line:
            line, self._line = self._line.split(b"\n", 1)
            word = line.strip().decode(errors="replace")
            if word in ("hold", "release"):
                self.held = word == "hold"
                print(f"relay: {word}", flush=True)
                os.write(self.control_master, b"ok\n")
            else:
                os.write(self.control_master, b"refused\n")

    def _run(self) -> None:
        pairs = {self.controller: self.hmi_master, self.hmi_master: self.controller}
        while not self.stopping:
            ready, _, _ = select.select([self.controller, self.hmi_master, self.control_master],
                                        [], [], 0.1)
            for fd in ready:
                try:
                    data = os.read(fd, 4096)
                except (BlockingIOError, OSError):
                    continue
                if fd == self.control_master:
                    self._control(data)
                elif self.held:
                    self.dropped += len(data)
                else:
                    os.write(pairs[fd], data)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--controller", required=True)
    parser.add_argument("--workflows", required=True)
    parser.add_argument("--suite", choices=("client", "operator"), default="client")
    args = parser.parse_args()

    controller = subprocess.Popen([args.controller], stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
    controller_output: list[bytes] = []
    output = ""
    returncode = None
    relay = None
    try:
        ports = announced_ports(controller)
        if ports is None:
            print("controller_workflow_check: the controller announced no pseudo-terminals")
            return 1
        # Keep reading the controller's console, so a full pipe never stalls it.
        threading.Thread(target=lambda: controller_output.extend(iter(
            lambda: os.read(controller.stdout.fileno(), 4096), b"")), daemon=True).start()
        for port in ports:
            make_raw(port)
        command = [args.workflows, "--seed-random", f"--scenario_port={ports[1]}"]
        if args.suite == "operator":
            relay = Relay(ports[0])
            relay.start()
            command += [f"--controller_link_port={relay.hmi_port}",
                        f"--relay_port={relay.control_port}"]
        else:
            command += [f"--controller_link_port={ports[0]}"]
        workflows = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   timeout=TIMEOUT_S, check=False)
        output = workflows.stdout.decode(errors="replace")
        returncode = workflows.returncode
    except subprocess.TimeoutExpired as timeout:
        output = (timeout.stdout or b"").decode(errors="replace")
        print("controller_workflow_check: the workflows did not finish")
    finally:
        if relay is not None:
            relay.stop()
        controller.kill()
        controller.wait()
    lines = output.splitlines()
    for index, line in enumerate(lines):
        if line.startswith(("workflow:", " PASS", " FAIL", "    Assertion", "SUITE", "PROJECT")) \
                or "Assertion failed" in line:
            print(line)
            # The assertion's own message follows it.
            if "Assertion failed" in line and index + 1 < len(lines):
                print("   ", lines[index + 1])
    if relay is not None:
        print(f"relay: {relay.dropped} bytes dropped while held")
    passed = returncode == 0 and "PROJECT EXECUTION SUCCESSFUL" in output
    if not passed:
        traces = [line for line in output.splitlines() if line.startswith("hmi-link:")]
        print("last link trace:", traces[-1] if traces else "none")
        print(f"workflows exit code {returncode}; last lines:",
              *[line for line in lines if not line.startswith("hmi-link:")][-8:], sep="\n  ")
        tail = b"".join(controller_output).decode(errors="replace").splitlines()[-5:]
        print("controller output (last lines):", *tail, sep="\n  ")
    print(f"controller_workflow_check: {'PASS' if passed else 'FAIL'}")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
