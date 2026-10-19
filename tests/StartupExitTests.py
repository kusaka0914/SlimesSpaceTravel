"""Run the production entry point with missing assets and isolated user data."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", required=True)
    options = parser.parse_args()
    probe = Path(options.probe).resolve()
    with tempfile.TemporaryDirectory(prefix="slimes startup ") as directory:
        root = Path(directory)
        executable = root / probe.name
        try:
            os.link(probe, executable)
        except OSError:
            shutil.copy2(probe, executable)
        working = root / "working"
        working.mkdir()
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        if os.name == "nt":
            environment["PATH"] = str(probe.parent) + os.pathsep + environment.get("PATH", "")
        result = subprocess.run([str(executable), "--debug"], cwd=working, env=environment,
                                capture_output=True, timeout=20)
        if result.returncode != 1:
            raise AssertionError(f"initialization failure must exit 1, got {result.returncode}: "
                                 f"{result.stderr.decode(errors='replace')}")
    print("[PASS] Production Main reports initialization failure")


if __name__ == "__main__":
    main()
