"""Validate every bundled YAML file without allowing known-invalid exceptions."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def validate(validator, directory, root=ROOT):
    files = sorted(path for path in directory.rglob("*") if path.suffix in {".yaml", ".yml"} and path.is_file())
    if not files:
        raise ValueError("no YAML files found")
    results = []
    for path in files:
        name = path.relative_to(root).as_posix()
        # GitのWindows checkoutでも同じ本文に同じハッシュを記録する。
        digest = hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
        result = subprocess.run([str(validator), str(path)], text=True, capture_output=True, timeout=15)
        status = "valid" if result.returncode == 0 else "failed"
        results.append({"path": name, "sha256": digest, "status": status,
                        "exit_code": result.returncode, "diagnostic": result.stderr.strip()})
    return {"files": results, "hash_policy": "SHA256 of bytes with CRLF normalized to LF",
            "passed": all(row["status"] == "valid" for row in results)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validator", type=Path, required=True)
    parser.add_argument("--directory", type=Path, default=ROOT / "assets/data")
    parser.add_argument("--output", type=Path, default=ROOT / "build/assets/report.json")
    options = parser.parse_args()
    report = validate(options.validator.resolve(), options.directory.resolve())
    options.output.parent.mkdir(parents=True, exist_ok=True)
    options.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for row in report["files"]:
        if row["status"] != "valid":
            print(f'{row["status"]}: {row["path"]}: {row["diagnostic"]}')
    print(f'Checked {len(report["files"])} YAML files; passed={report["passed"]}')
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
