"""Run the packet decoder and ESPHome calibration tests without sensor hardware."""

import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / ".test-build"
BUILD.mkdir(exist_ok=True)
env = os.environ.copy()
env["ZIG_GLOBAL_CACHE_DIR"] = str(BUILD / "zig-global-cache")
env["ZIG_LOCAL_CACHE_DIR"] = str(BUILD / "zig-cache")
env["PYTEST_DISABLE_PLUGIN_AUTOLOAD"] = "1"
executable = BUILD / ("test_packet.exe" if os.name == "nt" else "test_packet")


def run(arguments):
    subprocess.run(arguments, cwd=ROOT, env=env, check=True)


run([
    sys.executable, "-m", "ziglang", "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
    "-I", str(ROOT), str(ROOT / "tests/test_packet.cpp"), "-o", str(executable),
])
run([str(executable), str(ROOT / "tests/fixtures/hw3-sw1.txt")])
run([sys.executable, "-m", "pytest", "tests", "-q", "-p", "no:cacheprovider"])
