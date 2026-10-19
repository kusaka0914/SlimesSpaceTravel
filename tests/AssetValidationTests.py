"""Check malformed data without editing any packaged assets."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.validate_assets import validate


class AssetValidationTests(unittest.TestCase):
    def validate(self, text, expected_code, message=""):
        with tempfile.TemporaryDirectory(prefix="slimes schema ") as directory:
            path = Path(directory) / "stage.yaml"
            path.write_text(text, encoding="utf-8")
            result = subprocess.run([VALIDATOR, str(path)], capture_output=True, text=True, timeout=10)
            self.assertEqual(expected_code, result.returncode, result.stderr)
            self.assertIn(message, result.stderr)

    def test_unknown_fields_and_legacy_platform_ids_remain_valid(self):
        self.validate("planets: [{}]\nplatforms: [{components: {pressureSwitch: {targets: [legacy_platforms_1]}}}, {}]\n"
                      "future: [freeform, data]\n", 0)

    def test_duplicate_keys_are_rejected(self):
        self.validate("planets: []\nplanets: [{}]\n", 1, "duplicate mapping key")

    def test_invalid_root_and_syntax_are_rejected(self):
        for text in ["[not, a, mapping]", "planets: [", "---\na: 1\n---\nb: 2"]:
            with self.subTest(text=text):
                self.validate(text, 1)

    def test_nonfinite_and_wrong_dimension_are_rejected(self):
        self.validate("planets: [{center: [0, 1]}]\n", 1, "invalid vector dimension")
        for value in [".nan", ".inf", "-.inf"]:
            with self.subTest(value=value):
                self.validate(f"planets: [{{center: [0, {value}, 0]}}]\n", 1, "non-finite vector")

    def test_invalid_planet_and_switch_references_are_rejected(self):
        self.validate("planets: []\n", 1, "at least one planet")
        self.validate("planets: [{}]\nNPCs: [{pos: [1, 2]}]\n", 1, "invalid vector dimension")
        self.validate("planets: [{}]\nstar: [{currentPlanetNum: 2}]\n", 1, "missing planet reference")
        self.validate("planets: [{}]\nplayers: [{currentPlanetNum: 2}]\n", 1, "missing planet reference")
        self.validate("planets: [{}]\nplatforms: [{platformId: same}, {platformId: same}]\n", 1, "platform ID")
        self.validate("planets: [{}]\nplatforms: [{components: {pressureSwitch: {targets: [missing]}}}]\n",
                      1, "missing platform target")

    def test_cyclic_alias_does_not_recurse_indefinitely(self):
        self.validate("node: &cycle {child: *cycle}\n", 1, "cyclic YAML alias")

    def test_known_invalid_baseline_requires_identical_content_and_diagnostic(self):
        with tempfile.TemporaryDirectory(prefix="slimes asset baseline ") as directory:
            root = Path(directory)
            path = root / "stage.yaml"
            path.write_text("key: 1\nkey: 2\n", encoding="utf-8")
            baseline = {"stage.yaml": {"sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                                       "diagnostic": "duplicate mapping key: key"}}
            known = validate(Path(VALIDATOR), root, baseline, root)
            self.assertTrue(known["passed"])
            self.assertEqual("known_invalid_unchanged", known["files"][0]["status"])
            path.write_bytes(b"key: 1\r\nkey: 2\r\n")
            self.assertTrue(validate(Path(VALIDATOR), root, baseline, root)["passed"])
            baseline["stage.yaml"]["diagnostic"] = "different error"
            self.assertFalse(validate(Path(VALIDATOR), root, baseline, root)["passed"])
            path.write_text("key: 1\nkey: 3\n", encoding="utf-8")
            self.assertFalse(validate(Path(VALIDATOR), root, baseline, root)["passed"])

    def test_baseline_does_not_accept_new_invalid_files_or_stale_exceptions(self):
        with tempfile.TemporaryDirectory(prefix="slimes asset baseline ") as directory:
            root = Path(directory)
            (root / "valid.yaml").write_text("key: value\n", encoding="utf-8")
            self.assertTrue(validate(Path(VALIDATOR), root, {}, root)["passed"])
            self.assertFalse(validate(Path(VALIDATOR), root, {"missing.yaml": {}}, root)["passed"])
            (root / "invalid.yaml").write_text("key: 1\nkey: 2\n", encoding="utf-8")
            self.assertFalse(validate(Path(VALIDATOR), root, {}, root)["passed"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--validator", required=True)
    options, remaining = parser.parse_known_args()
    VALIDATOR = str(Path(options.validator).resolve())
    unittest.main(argv=[sys.argv[0], *remaining])
