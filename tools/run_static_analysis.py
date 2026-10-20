"""Analyze selected production boundaries using clang-tidy or the Clang static analyzer."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shlex
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    "src/actor/MathUtils.cpp", "src/system/GamepadRumbleService.cpp", "tools/editor_restart_helper/Main.cpp",
    "src/utils/ProcessLaunch.cpp", "src/editor/session/EditorBuildRestartService.cpp",
    "src/system/StageProgressSystem.cpp", "src/app/Main.cpp",
    "src/editor/stage/panels/StageAddActorPanel.cpp", "src/system/mesh/TextureLoader.cpp",
    "src/system/mesh/ImageLoader.cpp", "src/gfx/TextureUploadState.cpp", "src/app/GameInputController.cpp",
    "src/actor/player/PlayerParticleEffectController.cpp", "src/editor/stage/StageEditTransaction.cpp",
    "src/system/physics/ActorCollisionResolver.cpp", "src/actor/player/PlayerMovement.cpp",
    "src/actor/enemy/EnemyMovement.cpp", "src/editor/stage/StageActorYamlWriter.cpp",
    "src/system/actor_loader/PlatformStageConfig.cpp", "src/component/PlatformPressureSwitchComponent.cpp",
    "src/component/PlatformLatchedGroupSwitchComponent.cpp", "src/editor/stage/StagePlatformCreateCommands.cpp",
    "src/utils/TextFile.cpp", "src/utils/YamlFile.cpp", "src/utils/AtomicFile.cpp",
    "src/editor/stage/StagePlatformIdentifiers.cpp", "src/editor/stage/UGCPlatformCellService.cpp",
    "src/editor/stage/StageYamlRepository.cpp", "src/system/tutorial/TutorialLibrary.cpp",
    "src/editor/ugc/UGCWorkFileService.cpp",
    "src/gfx/GameRenderTargets.cpp", "src/gameplay/ugc/UGCWorkStorage.cpp", "src/system/UGCModeController.cpp",
    "src/editor/stage/StageActorInspector.cpp", "src/system/ActorLoadSystem.cpp", "src/world/GameWorld.cpp",
    "src/editor/stage/StageNPCInspector.cpp", "src/editor/stage/StageActorPlacementController.cpp",
    "src/editor/ugc/UGCWorkController.cpp", "src/editor/ugc/UGCWorkPanel.cpp", "src/system/UserDataPaths.cpp",
)


def analyze(source, entries, tidy, build, output):
    path = ROOT / source
    candidates = [entry for entry in entries if (Path(entry["directory"]) / entry["file"]).resolve() == path]
    candidates.sort(key=lambda entry: "game_startup_probe" in entry.get("command", ""))
    if not candidates:
        raise ValueError(f"no compilation entry for {source}")
    entry = candidates[0]
    stem = source.replace("/", "__")
    diagnostics = []
    if tidy:
        command = [tidy, str(path), "-p", str(build), "--quiet",
                   f"--export-fixes={output / (stem + '.yaml')}"]
    else:
        arguments = entry.get("arguments") or shlex.split(entry["command"])
        command = []
        skip = False
        for argument in arguments:
            if skip:
                skip = False
            elif argument in ("-o", "-MF", "-MT", "-MQ"):
                skip = True
            elif argument not in ("-c", "-MD", "-MMD", "-MP", "-fcoverage-mapping") and not argument.startswith(
                ("-fprofile-", "-fsanitize=", "-fno-sanitize-")
            ):
                command.append(argument)
        command.extend(["--analyze", "-Xanalyzer", "-analyzer-output=plist", "-o", str(output / (stem + ".plist"))])
    result = subprocess.run(command, cwd=entry["directory"], text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=180)
    (output / (stem + ".log")).write_text(result.stdout, encoding="utf-8")
    plist = output / (stem + ".plist")
    if not tidy and plist.exists():
        with plist.open("rb") as stream:
            report = plistlib.load(stream)
        for diagnostic in report.get("diagnostics", []):
            location = diagnostic["location"]
            file = Path(report["files"][location["file"]]).resolve()
            if file.is_relative_to(ROOT) and "thirdParty" not in file.parts:
                diagnostics.append({"path": str(file.relative_to(ROOT)), "line": location["line"],
                                    "checker": diagnostic["check_name"], "message": diagnostic["description"]})
    return {"path": source, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "exit_code": result.returncode, "diagnostics": diagnostics,
            "log": str(output / (stem + ".log")), "passed": result.returncode == 0 and not diagnostics}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--clang-tidy", default=shutil.which("clang-tidy"))
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--all-production", action="store_true",
                        help="analyze every first-party production translation unit in the compilation database")
    options = parser.parse_args()
    build = options.build.resolve()
    output = (options.output or build / "analysis").resolve()
    output.mkdir(parents=True, exist_ok=True)
    entries = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    sources = SOURCES
    if options.all_production:
        sources = set()
        for entry in entries:
            path = (Path(entry["directory"]) / entry["file"]).resolve()
            if path.is_relative_to(ROOT / "src") and "thirdParty" not in path.parts:
                sources.add(path.relative_to(ROOT).as_posix())
        sources.add("tools/editor_restart_helper/Main.cpp")
        sources = sorted(sources)
    if options.jobs < 1:
        parser.error("--jobs must be positive")
    if not options.clang_tidy:
        compiler = entries[0].get("arguments") or shlex.split(entries[0]["command"])
        version = subprocess.run([compiler[0], "--version"], capture_output=True, text=True, check=True).stdout
        if os.name == "nt" or "clang" not in version.lower():
            parser.error("install clang-tidy or use a Clang compilation database")
    with ThreadPoolExecutor(max_workers=options.jobs) as executor:
        results = list(executor.map(lambda source: analyze(source, entries, options.clang_tidy, build, output), sources))
    report = {"engine": "clang-tidy" if options.clang_tidy else "Clang static analyzer",
              "scope": ("all first-party production translation units" if options.all_production else
                        "selected production boundaries") + "; external-header diagnostics are excluded from the plist gate",
              "results": results, "passed": all(result["passed"] for result in results)}
    (output / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for result in results:
        print(f"{'PASS' if result['passed'] else 'FAIL'} {result['path']}")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
