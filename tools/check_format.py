"""Check formatting on changed C++ lines without rewriting the historical code base."""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SUFFIXES = {".h", ".hpp", ".cpp", ".mm"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True, help="existing Git revision to compare against")
    parser.add_argument("--clang-format", default=shutil.which("clang-format"))
    parser.add_argument("--fix", action="store_true")
    options = parser.parse_args()
    if not options.clang_format:
        parser.error("clang-format 18 is required")
    version = subprocess.check_output([options.clang_format, "--version"], text=True)
    if not re.search(r"version 18\.", version):
        parser.error("use clang-format 18 locally and in CI")
    changed = subprocess.check_output(
        ["git", "diff", "--name-only", "--diff-filter=ACMR", "-z", options.base, "--"], cwd=ROOT
    ).decode().split("\0")
    untracked = subprocess.check_output(
        ["git", "ls-files", "--others", "--exclude-standard", "-z"], cwd=ROOT
    ).decode().split("\0")
    failures = []
    checked = 0
    for name in sorted(set(changed + untracked)):
        path = ROOT / name
        if path.suffix not in SUFFIXES or "thirdParty" in path.parts or not path.is_file():
            continue
        diff = subprocess.check_output(
            ["git", "diff", "--unified=0", options.base, "--", name], cwd=ROOT, text=True
        )
        ranges = []
        for match in re.finditer(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", diff, re.MULTILINE):
            start, count = int(match[1]), int(match[2] or 1)
            if count:
                ranges.append(f"--lines={start}:{start + count - 1}")
        if name in untracked:
            ranges = []  # A new file is checked in full.
        elif not ranges:
            continue
        checked += 1
        formatted = subprocess.check_output([options.clang_format, *ranges, str(path)])
        if formatted != path.read_bytes():
            if options.fix:
                path.write_bytes(formatted)
            else:
                failures.append(name)
    print(f"Checked {checked} C++ files against {options.base}; {len(failures)} formatting failures")
    for name in failures:
        print(name)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
