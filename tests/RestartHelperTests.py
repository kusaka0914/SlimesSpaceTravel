"""Exercise the helper CLI with isolated build directories and a harmless launch probe."""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest


class RestartHelperTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="slimes restart ")
        cls.root = Path(cls.directory.name)
        source = cls.root / "source"
        source.mkdir()
        (source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.13)\nproject(Probe NONE)\nadd_custom_target(game)\n",
            encoding="utf-8",
        )
        cls.build = cls.root / "build with 'quotes'"
        subprocess.run([CMAKE, "-S", str(source), "-B", str(cls.build)], check=True, capture_output=True)
        gone = subprocess.Popen([sys.executable, "-c", "pass"])
        gone.wait()
        cls.parent_pid = str(gone.pid)

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def setUp(self):
        self.session = self.root / "session 'quoted'.yaml"
        self.marker = Path(str(self.session) + ".launched")
        self.marker.unlink(missing_ok=True)
        self.log = self.root / "build log.txt"

    def arguments(self, pid=None, directory=None, executable=None):
        return [
            "--parent-pid", self.parent_pid if pid is None else pid,
            "--build-directory", str(directory or self.build),
            "--configuration", "Debug",
            "--game-executable", str(executable or PROBE),
            "--session-file", str(self.session),
            "--build-log", str(self.log),
        ]

    def run_helper(self, arguments, timeout=15):
        return subprocess.run([HELPER, *arguments], cwd=self.root, capture_output=True, timeout=timeout)

    def launched_arguments(self):
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if self.marker.exists():
                text = self.marker.read_text(encoding="utf-8")
                if text.endswith("complete\n"):
                    return text.splitlines()
            time.sleep(0.01)
        self.fail("launch probe did not complete")

    def test_rejects_invalid_pid_without_waiting_or_launching(self):
        pid_limit = (1 << (32 if os.name == "nt" else 31)) - 1
        for pid in ["0", "-1", "+12", "12tail", " 12", "12 ", "", str(pid_limit + 1), str(1 << 80)]:
            with self.subTest(pid=pid):
                try:
                    result = self.run_helper(self.arguments(pid=pid), timeout=2)
                except subprocess.TimeoutExpired:
                    self.fail(f"invalid PID {pid!r} reached the parent-wait loop")
                self.assertEqual(2, result.returncode, result.stderr.decode(errors="replace"))

    def test_rejects_missing_unknown_and_duplicate_options(self):
        for arguments in [self.arguments()[:-1], self.arguments() + ["--unknown", "value"],
                          self.arguments() + ["--parent-pid", self.parent_pid]]:
            with self.subTest(arguments=arguments):
                self.assertEqual(2, self.run_helper(arguments).returncode)

    def test_missing_executable_is_reported_by_parent(self):
        result = self.run_helper(self.arguments(executable=self.root / "missing game"))
        self.assertEqual(3, result.returncode)
        self.assertIn("Failed to relaunch", self.log.read_text(encoding="utf-8"))

    def test_missing_working_directory_is_reported_by_parent(self):
        result = self.run_helper(self.arguments(directory=self.root / "missing directory"))
        self.assertEqual(3, result.returncode)
        self.assertFalse(self.marker.exists())

    def test_success_preserves_arguments_and_working_directory(self):
        result = self.run_helper(self.arguments())
        self.assertEqual(0, result.returncode)
        lines = self.launched_arguments()
        self.assertEqual(self.build.resolve(), Path(lines[0]).resolve())
        self.assertEqual(["--debug", "--restore-editor-session", str(self.session), "complete"], lines[1:])

    def test_build_failure_restarts_existing_executable_with_log(self):
        directory = self.root / "not configured"
        directory.mkdir(exist_ok=True)
        result = self.run_helper(self.arguments(directory=directory))
        self.assertEqual(1, result.returncode)
        self.assertEqual(["--debug", "--restore-editor-session", str(self.session),
                          "--editor-restart-error-log", str(self.log), "complete"], self.launched_arguments()[1:])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--helper", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--cmake", required=True)
    options, remaining = parser.parse_known_args()
    HELPER = str(Path(options.helper).resolve())
    PROBE = str(Path(options.probe).resolve())
    CMAKE = options.cmake
    unittest.main(argv=[sys.argv[0], *remaining])
