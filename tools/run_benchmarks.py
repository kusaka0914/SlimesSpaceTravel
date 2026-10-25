"""Record fixed-input CPU timings and compare only matching benchmark conditions."""

import argparse
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def cpu_description():
    if platform.system() == "Darwin":
        result = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"], text=True, capture_output=True)
        return result.stdout.strip() if result.returncode == 0 else "unavailable"
    if Path("/proc/cpuinfo").is_file():
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                return line.partition(":")[2].strip()
    return platform.processor()


def compiler_version(compiler):
    command = [compiler] if os.name == "nt" else [compiler, "--version"]
    result = subprocess.run(command, text=True, capture_output=True)
    return (result.stdout + result.stderr).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--stage", type=Path, default=ROOT / "assets/data/stage/test.yaml")
    parser.add_argument("--iterations", type=int, default=200)
    parser.add_argument("--samples", type=int, default=11)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--machine-label", help="explicit hardware identity when OS metadata is restricted")
    parser.add_argument("--output", type=Path)
    options = parser.parse_args()
    if options.iterations < 1 or options.samples < 1:
        parser.error("counts must be positive")
    build = options.build.resolve()
    executable = options.executable or build / ("game_benchmarks.exe" if platform.system() == "Windows" else "game_benchmarks")
    output = options.output or build / "benchmarks/current.json"
    stage = options.stage.resolve()
    digest = hashlib.sha256(stage.read_bytes()).hexdigest()
    result = subprocess.run([str(executable), str(stage), str(options.iterations), str(options.samples)],
                            capture_output=True, text=True, check=True, timeout=180)
    rows = []
    for row in csv.DictReader(io.StringIO(result.stdout)):
        rows.append({"name": row["benchmark"], "iterations": int(row["iterations"]), "samples": int(row["samples"]),
                     "median_us": float(row["median_us"]), "p95_us": float(row["p95_us"]), "checksum": int(row["checksum"])})
    if len(rows) != 5 or hashlib.sha256(stage.read_bytes()).hexdigest() != digest:
        raise RuntimeError("benchmark output/input changed unexpectedly")
    configuration = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if "=" in line and ":" in line:
            key, value = line.split("=", 1)
            name = key.split(":", 1)[0]
            if name.startswith(("CMAKE_BUILD_TYPE", "CMAKE_CXX_FLAGS", "CMAKE_CXX_COMPILER", "GAME_ENABLE_",
                                "GAME_TGS_BUILD", "GAME_REVIEW_BUILD", "yaml-cpp_DIR")):
                configuration[name] = value
    conditions = {"system": platform.platform(), "machine": platform.machine(), "stage_sha256": digest,
                  "cpu": cpu_description(), "logical_cpus": os.cpu_count(),
                  "machine_label": options.machine_label,
                  "compiler_version": compiler_version(configuration["CMAKE_CXX_COMPILER"]),
                  "method_sha256": hashlib.sha256((ROOT / "tools/benchmarks/Main.cpp").read_bytes()).hexdigest(),
                  "iterations": options.iterations, "samples": options.samples, "configuration": configuration}
    report = {"conditions": conditions, "results": rows,
              "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "tracked_changes": subprocess.run(["git", "diff", "--quiet", "HEAD", "--"], cwd=ROOT).returncode != 0,
              "scope": "warm in-memory YAML parse/clone/serialization, allocation+Actor lifetime and billboard CPU math; no display, FPS, cold disk load or full scene load"}
    if options.baseline:
        if conditions["cpu"] == "unavailable" and not options.machine_label:
            parser.error("CPU identity is unavailable; provide --machine-label before comparing timings")
        baseline = json.loads(options.baseline.read_text(encoding="utf-8"))
        if baseline["conditions"] != conditions:
            parser.error("baseline conditions differ; refusing a misleading performance comparison")
        previous = {row["name"]: row for row in baseline["results"]}
        report["median_ratio_to_baseline"] = {
            row["name"]: row["median_us"] / previous[row["name"]]["median_us"] for row in rows
        }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(output)
    for row in rows:
        print(f"{row['name']}: median {row['median_us']:.3f} us/op, p95 {row['p95_us']:.3f} us/op")


if __name__ == "__main__":
    main()
