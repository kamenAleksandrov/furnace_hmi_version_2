#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run the HMI's operator workflows against the controller over a simulated serial link.

SIMULATION ONLY (Phase 21 M21-5). Linux (WSL) only: both programs are native_sim
builds. The controller is the furnace firmware's ``tests/sim/link_pty`` program (with
its scenario channel, firmware PR #53); the HMI side is
``tests/zephyr/controller_workflows``, which drives the HMI's controller link and
command mailbox and checks every result. Its temperatures and faults are the
controller's simulation, never furnace measurements.

Usage:
  controller_workflow_check.py --controller <link_pty zephyr.exe>
                               --workflows <controller_workflows zephyr.exe>
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

TIMEOUT_S = 600.0


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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--controller", required=True)
    parser.add_argument("--workflows", required=True)
    args = parser.parse_args()

    controller = subprocess.Popen([args.controller], stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT)
    controller_output: list[bytes] = []
    output = ""
    returncode = None
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
        workflows = subprocess.run(
            [args.workflows, "--seed-random", f"--controller_link_port={ports[0]}",
             f"--scenario_port={ports[1]}"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=TIMEOUT_S, check=False)
        output = workflows.stdout.decode(errors="replace")
        returncode = workflows.returncode
    except subprocess.TimeoutExpired as timeout:
        output = (timeout.stdout or b"").decode(errors="replace")
        print("controller_workflow_check: the workflows did not finish")
    finally:
        controller.kill()
        controller.wait()
    for line in output.splitlines():
        if line.startswith(("workflow:", " PASS", " FAIL", "    Assertion", "SUITE", "PROJECT")) \
                or "Assertion failed" in line:
            print(line)
    passed = returncode == 0 and "PROJECT EXECUTION SUCCESSFUL" in output
    if not passed:
        traces = [line for line in output.splitlines() if line.startswith("hmi-link:")]
        print("last link trace:", traces[-1] if traces else "none")
        tail = b"".join(controller_output).decode(errors="replace").splitlines()[-5:]
        print("controller output (last lines):", *tail, sep="\n  ")
    print(f"controller_workflow_check: {'PASS' if passed else 'FAIL'}")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
