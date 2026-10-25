"""Run game_runtime_tests with a private executable directory for Editor sessions."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import uuid


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--configuration", default="Release")
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--working-directory", type=Path)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    options = parser.parse_args()
    build = options.build.resolve()
    name = "game_runtime_tests.exe" if os.name == "nt" else "game_runtime_tests"
    candidates = [options.executable] if options.executable else [build / options.configuration / name, build / name]
    executable = next((path.resolve() for path in candidates if path.is_file()), None)
    if executable is None or executable.name != name:
        parser.error("build game_runtime_tests first, or specify its executable")
    arguments = options.arguments
    if arguments[:1] == ["--"]:
        arguments = arguments[1:]
    working_directory = (options.working_directory or build / "runtime-tests").resolve()
    if not working_directory.is_dir():
        parser.error("runtime working directory does not exist")

    # セッション保存先は実行ファイル基準なので、バイナリだけを複製する。アセットは既存の作業ディレクトリを使う。
    with tempfile.TemporaryDirectory(prefix="slimes_runtime_binary_") as directory:
        private_directory = Path(directory)
        private_executable = private_directory / name
        shutil.copy2(executable, private_executable)
        token = uuid.uuid4().hex
        (private_directory / ".runtime-test-token").write_text(token, encoding="ascii")
        environment = dict(os.environ, SLIMES_RUNTIME_TEST_TOKEN=token)
        # Windowsの依存DLLは元の配置から探し、セーブ用パスを決める環境変数は変更しない。
        environment["PATH"] = str(executable.parent) + os.pathsep + environment.get("PATH", "")
        child = subprocess.Popen([str(private_executable), *arguments], cwd=working_directory, env=environment)
        print(f"Runtime check PID: {child.pid}", file=sys.stderr, flush=True)
        try:
            return_code = child.wait()
        finally:
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
        return return_code if return_code >= 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
