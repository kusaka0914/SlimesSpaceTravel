"""Collect LLVM coverage for first-party production sources; unexecuted code remains uncovered."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def find_tool(name, explicit):
    if explicit:
        return explicit
    tool = shutil.which(name)
    if tool:
        return tool
    if shutil.which("xcrun"):
        return subprocess.check_output(["xcrun", "--find", name], text=True).strip()
    raise RuntimeError(f"cannot find {name}; specify its path explicitly")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--configuration")
    parser.add_argument("--run-tests", action="store_true")
    parser.add_argument("--llvm-profdata")
    parser.add_argument("--llvm-cov")
    options = parser.parse_args()
    build = options.build.resolve()
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    if "GAME_ENABLE_COVERAGE:BOOL=ON" not in cache:
        parser.error("configure with -DGAME_ENABLE_COVERAGE=ON first")
    directory = build / "coverage"
    if options.run_tests:
        for path in directory.glob("*.profraw"):
            path.unlink()
        command = ["ctest", "--test-dir", str(build), "--output-on-failure"]
        if options.configuration:
            command.extend(["-C", options.configuration])
        subprocess.run(command, check=True)
    raw = sorted(directory.glob("*.profraw"))
    if not raw:
        parser.error("no profiles; run CTest or pass --run-tests")
    manifests = sorted(directory.glob("objects-*.txt"))
    if options.configuration:
        manifests = [directory / f"objects-{options.configuration}.txt"]
    if len(manifests) != 1:
        parser.error("select one build configuration with --configuration")
    objects = [line for line in manifests[0].read_text(encoding="utf-8").splitlines() if line]
    for path in objects:
        if not Path(path).is_file():
            parser.error(f"missing instrumented executable: {path}")
    profile = directory / "merged.profdata"
    subprocess.run([find_tool("llvm-profdata", options.llvm_profdata), "merge", "-sparse", *map(str, raw),
                    "-o", str(profile)], check=True)
    command = [find_tool("llvm-cov", options.llvm_cov), "export", objects[0], "-summary-only",
               f"-instr-profile={profile}"]
    command.extend(f"-object={path}" for path in objects[1:])
    result = subprocess.run(command, text=True, capture_output=True, check=True)
    exported = json.loads(result.stdout)
    files = {}
    for data in exported["data"]:
        for file in data["files"]:
            path = Path(file["filename"]).resolve()
            if not path.is_relative_to(ROOT):
                continue
            relative = path.relative_to(ROOT)
            if (not relative.as_posix().startswith("src/") and
                    relative.as_posix() != "tools/editor_restart_helper/Main.cpp"):
                continue
            if "thirdParty" in relative.parts:
                continue
            name = relative.as_posix()
            if name in files and files[name] != file["summary"]:
                raise RuntimeError(f"conflicting coverage summaries for {name}")
            files[name] = file["summary"]
    totals = {}
    for metric in ["functions", "lines", "branches"]:
        count = sum(file.get(metric, {}).get("count", 0) for file in files.values())
        covered = sum(file.get(metric, {}).get("covered", 0) for file in files.values())
        totals[metric] = {"count": count, "covered": covered,
                          "percent": 100.0 * covered / count if count else None}
    report = {"scope": "instrumented first-party src and production restart helper; excludes tests/quality tools/thirdParty/system headers",
              "objects": objects, "profiles": len(raw), "totals": totals, "files": dict(sorted(files.items())),
              "llvm_diagnostics": result.stderr.strip(),
              "limitations": "source/branch coverage describes executed tests; it does not prove correct behavior, performance or absence of leaks"}
    (directory / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(totals, indent=2))
    if result.stderr:
        print(result.stderr.strip())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
