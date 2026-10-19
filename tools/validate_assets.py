"""Validate bundled YAML; retain explicit historical exceptions without accepting new failures."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def validate(validator, directory, baseline, root=ROOT):
    files = sorted(path for path in directory.rglob("*") if path.suffix in {".yaml", ".yml"} and path.is_file())
    if not files:
        raise ValueError("no YAML files found")
    results = []
    used = set()
    for path in files:
        name = path.relative_to(root).as_posix()
        # GitのWindows checkoutによる改行変換だけを許し、YAML本文の変更は検知する。
        digest = hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
        result = subprocess.run([str(validator), str(path)], text=True, capture_output=True, timeout=15)
        status = "valid" if result.returncode == 0 else "failed"
        if name in baseline:
            used.add(name)
            known = baseline[name]
            if digest == known["sha256"] and result.returncode == 1 and known["diagnostic"] in result.stderr:
                status = "known_invalid_unchanged"
            else:
                status = "stale_baseline"
        results.append({"path": name, "sha256": digest, "status": status,
                        "exit_code": result.returncode, "diagnostic": result.stderr.strip()})
    missing = sorted(set(baseline) - used)
    return {"files": results, "hash_policy": "SHA256 of bytes with CRLF normalized to LF", "missing_baseline_files": missing,
            "passed": not missing and all(row["status"] in {"valid", "known_invalid_unchanged"} for row in results)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validator", type=Path, required=True)
    parser.add_argument("--directory", type=Path, default=ROOT / "assets/data")
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "build/assets/report.json")
    options = parser.parse_args()
    baseline = json.loads(options.baseline.read_text(encoding="utf-8"))["known_invalid"] if options.baseline else {}
    report = validate(options.validator.resolve(), options.directory.resolve(), baseline)
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for row in report["files"]:
        if row["status"] != "valid":
            print(f'{row["status"]}: {row["path"]}: {row["diagnostic"]}')
    print(f'Checked {len(report["files"])} YAML files; passed={report["passed"]}')
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
