#!/usr/bin/env python3
"""Local web dashboard for the ANSA S09 validation pipeline.

Dependency-free: Python standard library only. The dashboard never invents test
results; it runs the repository's existing scripts/run_all_tests.sh and exposes
its live stdout to the browser.
"""
from __future__ import annotations

import json
import os
import re
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
HOST = os.environ.get("ANSA_DASHBOARD_HOST", "127.0.0.1")
PORT = int(os.environ.get("ANSA_DASHBOARD_PORT", "8765"))

state = {
    "running": False,
    "started_at": None,
    "finished_at": None,
    "returncode": None,
    "status": "idle",
    "phase": "Ready",
    "output": "",
    "pid": None,
}
lock = threading.Lock()

PHASES = [
    ("Configuring CMake", re.compile(r"cmake.*-B build", re.I)),
    ("Building host targets", re.compile(r"cmake --build|Building C object|Built target", re.I)),
    ("Running CTest", re.compile(r"Start [0-9]+:|Test #[0-9]+|100% tests passed|tests passed", re.I)),
    ("Running Python peripheral tests", re.compile(r"pytest|passed.* in .*s", re.I)),
    ("Checking peripheral socket bridge", re.compile(r"bridge|socket", re.I)),
    ("Building QEMU firmware", re.compile(r"make -C backends/qemu-cortex-m/demo|Cross-compiled QEMU", re.I)),
]


def update_phase(line: str) -> None:
    for phase, pattern in reversed(PHASES):
        if pattern.search(line):
            state["phase"] = phase
            return


def run_pipeline() -> None:
    with lock:
        state.update({
            "running": True,
            "started_at": time.time(),
            "finished_at": None,
            "returncode": None,
            "status": "running",
            "phase": "Starting validation",
            "output": "",
            "pid": None,
        })

    script = ROOT / "scripts" / "run_all_tests.sh"
    if not script.exists():
        with lock:
            state.update({"running": False, "status": "failed", "returncode": 127,
                          "finished_at": time.time(), "phase": "Pipeline script missing",
                          "output": f"ERROR: {script} not found\n"})
        return

    try:
        proc = subprocess.Popen(
            ["bash", str(script)], cwd=ROOT,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1, universal_newlines=True,
            start_new_session=True,
        )
        with lock:
            state["pid"] = proc.pid
        for line in proc.stdout:
            with lock:
                state["output"] += line
                update_phase(line)
        rc = proc.wait()
        with lock:
            state["running"] = False
            state["finished_at"] = time.time()
            state["returncode"] = rc
            state["status"] = "passed" if rc == 0 else "failed"
            state["phase"] = "Validation complete" if rc == 0 else "Validation failed"
            state["pid"] = None
    except Exception as exc:
        with lock:
            state.update({"running": False, "status": "failed", "returncode": 1,
                          "finished_at": time.time(), "phase": "Dashboard execution error",
                          "output": state["output"] + f"\nERROR: {exc}\n", "pid": None})


def snapshot() -> dict:
    with lock:
        s = dict(state)
        s["elapsed_s"] = ((s["finished_at"] or time.time()) - s["started_at"]) if s["started_at"] else 0
        s["tests"] = parse_metrics(s["output"])
        return s


def parse_metrics(output: str) -> dict:
    m = {"ctest_passed": None, "ctest_total": None, "pytest_passed": None,
         "bridge_pass": None, "qemu_build": None}
    x = re.search(r"(\d+) tests passed, 0 tests failed out of (\d+)", output, re.I)
    if x:
        m["ctest_passed"], m["ctest_total"] = map(int, x.groups())
    else:
        x = re.search(r"(\d+)% tests passed, 0 tests failed out of (\d+)", output, re.I)
        if x:
            m["ctest_passed"] = int(round(int(x.group(1)) * int(x.group(2)) / 100))
            m["ctest_total"] = int(x.group(2))
    x = re.search(r"(\d+) passed(?:,| in |$)", output)
    if x:
        m["pytest_passed"] = int(x.group(1))
    m["bridge_pass"] = "PASS: complete host regression + peripheral socket bridge" in output
    m["qemu_build"] = "Cross-compiled QEMU demo successfully." in output
    return m


class Handler(BaseHTTPRequestHandler):
    def _send(self, code: int, content_type: str, body: bytes) -> None:
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/":
            body = (ROOT / "dashboard" / "index.html").read_bytes()
            self._send(200, "text/html; charset=utf-8", body)
        elif path in ("/app.js", "/style.css"):
            p = ROOT / "dashboard" / path.lstrip("/")
            if not p.exists():
                self._send(404, "text/plain; charset=utf-8", b"Not found")
            else:
                ctype = "text/javascript; charset=utf-8" if path.endswith(".js") else "text/css; charset=utf-8"
                self._send(200, ctype, p.read_bytes())
        elif path == "/api/status":
            self._send(200, "application/json; charset=utf-8", json.dumps(snapshot()).encode())
        else:
            self._send(404, "text/plain; charset=utf-8", b"Not found")

    def do_POST(self):
        path = urlparse(self.path).path
        if path != "/api/run":
            self._send(404, "text/plain; charset=utf-8", b"Not found")
            return
        with lock:
            if state["running"]:
                self._send(409, "application/json", json.dumps({"error": "Validation is already running"}).encode())
                return
        threading.Thread(target=run_pipeline, daemon=True).start()
        self._send(202, "application/json", b'{"started":true}')

    def log_message(self, fmt, *args):
        print(f"[dashboard] {self.address_string()} - {fmt % args}")


def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"ANSA S09 Validation Dashboard")
    print(f"Project root: {ROOT}")
    print(f"Listening: http://{HOST}:{PORT}")
    print("Use Ctrl+C to stop the dashboard server.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nDashboard stopped.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
