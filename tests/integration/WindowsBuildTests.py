"""Exercise Windows CMake discovery without downloading dependencies or building the game."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    options = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    command = Path(os.environ["SystemRoot"]) / "System32/cmd.exe"
    with tempfile.TemporaryDirectory(prefix="slimes build tools ") as directory:
        temporary = Path(directory)
        program_files = temporary / "Program Files (x86)"
        installation = temporary / "Visual Studio"
        tool = installation / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
        locator = program_files / "Microsoft Visual Studio/Installer/vswhere.exe"
        tool.parent.mkdir(parents=True)
        locator.parent.mkdir(parents=True)
        shutil.copy2(options.probe, tool)
        shutil.copy2(options.probe, locator)
        environment = dict(os.environ)
        for key in ("CMAKE_EXE", "VSWHERE", "VS_PATH"):
            environment.pop(key, None)
        environment.update({"ProgramFiles(x86)": str(program_files),
                            "SLIMES_TEST_VS_PATH": str(installation),
                            "PATH": str(command.parent)})

        def check(expected_code, expected_text):
            result = subprocess.run([str(command), "/d", "/c", "build_windows.bat", "--check-cmake"],
                                    cwd=root, env=environment, stdin=subprocess.DEVNULL,
                                    capture_output=True, text=True, errors="replace", timeout=20)
            actual_output = result.stdout + result.stderr

            has_expected_exit_code = result.returncode == expected_code
            has_expected_output = (
                os.path.normcase(expected_text)
                in os.path.normcase(actual_output)
            )

            if not has_expected_exit_code or not has_expected_output:
                raise AssertionError(
                    f"Expected exit code: {expected_code}\n"
                    f"Actual exit code: {result.returncode}\n"
                    f"Expected text: {expected_text!r}\n"
                    f"Actual output: {actual_output!r}"
                )

        check(0, str(tool))
        on_path = temporary / "path tools"
        on_path.mkdir()
        shutil.copy2(options.probe, on_path / "cmake.exe")
        environment["PATH"] = str(on_path) + os.pathsep + str(command.parent)
        check(0, str(on_path / "cmake.exe"))
        environment["PATH"] = str(command.parent)
        tool.unlink()
        check(1, "CMake was not found")
    print("3/3 Windows CMake discovery cases passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
