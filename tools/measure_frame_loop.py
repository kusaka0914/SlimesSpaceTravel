"""Measure the production frame loop with isolated saves; record hardware and build conditions."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

from run_benchmarks import cpu_description


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--configuration", default="Release")
    parser.add_argument("--frames", type=int, default=600)
    parser.add_argument("--scenario", action="append", choices=["one-player", "two-player", "ugc", "cycle"])
    parser.add_argument("--output", type=Path)
    options = parser.parse_args()
    if not 4 <= options.frames <= 216000:
        parser.error("frames must be in 4..216000")
    build = options.build.resolve()
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    configuration = {}
    for line in cache.splitlines():
        if "=" in line and ":" in line and not line.startswith(("//", "#")):
            key, value = line.split("=", 1)
            name = key.split(":", 1)[0]
            if name.startswith(("CMAKE_BUILD_TYPE", "CMAKE_CXX_", "GAME_", "yaml-cpp_DIR")):
                configuration[name] = value
    if options.configuration != "Release" or configuration.get("CMAKE_BUILD_TYPE", "") not in ("", "Release"):
        parser.error("use a Release build for performance measurements")
    if any(configuration.get(name) == "ON" for name in (
            "GAME_ENABLE_COVERAGE", "GAME_ENABLE_RUNTIME_SANITIZERS", "GAME_ENABLE_TEST_SANITIZERS")):
        parser.error("disable coverage and sanitizers for performance measurements")
    executable_name = "game_runtime_tests.exe" if os.name == "nt" else "game_runtime_tests"
    candidates = [build / options.configuration / executable_name, build / executable_name]
    executable = next((path for path in candidates if path.is_file()), None)
    if executable is None:
        parser.error("build with GAME_BUILD_RUNTIME_TESTS=ON first")
    output = (options.output or build / "benchmarks/frame-loop.json").resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    scenarios = []
    environment = dict(os.environ, SDL_AUDIODRIVER="dummy")
    for scenario in options.scenario or ["one-player", "two-player", "ugc"]:
        result = subprocess.run([sys.executable, "-B", str(ROOT / "tools/run_runtime_checks.py"),
                                 "--executable", str(executable), "--working-directory", str(build / "runtime-tests"),
                                 "--", "--frame-loop", str(options.frames), scenario],
                                cwd=build / "runtime-tests", env=environment, text=True, encoding="utf-8",
                                errors="replace", capture_output=True, timeout=options.frames / 20 + 180)
        log = output.parent / f"{output.stem}-{scenario}.log"
        log.write_text(result.stdout + result.stderr, encoding="utf-8")
        result.check_returncode()
        record = {"metrics": [], "log": str(log)}
        for line in result.stdout.splitlines():
            for marker, key in [("FRAME_LOOP_CONTEXT ", "context"), ("FRAME_LOOP_RESULT ", "result")]:
                if line.startswith(marker):
                    record[key] = json.loads(line[len(marker):])
            if line.startswith("FRAME_LOOP_METRIC "):
                record["metrics"].append(json.loads(line[len("FRAME_LOOP_METRIC "):]))
        if record.get("result", {}).get("completed_frames") != options.frames or not record.get("metrics"):
            raise RuntimeError(f"incomplete frame-loop results; see {log}")
        scenarios.append(record)
        print(f"Completed {scenario}: {options.frames} frames", flush=True)
    report = {
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "tracked_changes": subprocess.run(["git", "diff", "--quiet", "HEAD", "--"], cwd=ROOT).returncode != 0,
        "system": platform.platform(), "cpu": cpu_description(), "configuration": configuration,
        "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "method_sha256": hashlib.sha256((ROOT / "tests/RuntimeFrameLoopTests.cpp").read_bytes()).hexdigest(),
        "isolation_runner_sha256": hashlib.sha256((ROOT / "tools/run_runtime_checks.py").read_bytes()).hexdigest(),
        "scenarios": scenarios,
        "limitations": [
            "Visible debug-stage window with isolated saves and dummy audio; stationary players, no physical input.",
            "Frame intervals include VSync/software cap and scenario transitions; frame work excludes software cap.",
            "GPU value is the latest asynchronous viewport sample, not a distribution or full-frame GPU timing.",
            "cycle exercises repeated reload/join/UGC transitions; it is not comparable to a fixed scene.",
            "Results apply only to the recorded machine, framebuffer size and build; no FPS pass threshold is assumed.",
        ],
    }
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
