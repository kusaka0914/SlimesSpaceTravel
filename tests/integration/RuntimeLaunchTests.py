"""Check copied-binary loading and unsafe debug launch rejection without opening a display."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--working-directory", type=Path, required=True)
    options = parser.parse_args()
    executable = options.executable.resolve()
    directory = executable.parent
    if directory.name in {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"}:
        directory = directory.parent
    shared_session = directory / "debug_editor_session.yaml"
    before = shared_session.read_bytes() if shared_session.exists() else None
    environment = dict(os.environ)
    environment.pop("SLIMES_RUNTIME_TEST_TOKEN", None)
    cases = [
        ([str(executable), "--frame-loop", "4", "ugc"], 1, "launch debug Runtime checks through"),
        ([sys.executable, "-B", str(ROOT / "tools/run_runtime_checks.py"), "--executable", str(executable),
          "--working-directory", str(options.working_directory.resolve()), "--", "--frame-loop", "0", "ugc"],
         2, "Expected 4..216000 frames"),
    ]
    for command, expected_code, expected_message in cases:
        result = subprocess.run(command, cwd=options.working_directory, env=environment, capture_output=True,
                                text=True, encoding="utf-8", errors="replace", timeout=20)
        if result.returncode != expected_code or expected_message not in result.stdout + result.stderr:
            raise RuntimeError(f"Runtime launch returned {result.returncode}: {result.stdout}{result.stderr}")
    after = shared_session.read_bytes() if shared_session.exists() else None
    if after != before:
        raise RuntimeError("Runtime launch checks changed the shared Editor session")
    print("2/2 Runtime launch isolation checks passed; shared Editor session unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
