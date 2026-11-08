#!/usr/bin/env python3
"""Regression tests for the production-only Windows release signing gate."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = (ROOT / ".github/workflows/cli-matrix.yml").read_text(encoding="utf-8")
PWSH = shutil.which("pwsh")


class ReleaseSigningPolicyTest(unittest.TestCase):
    def test_build_and_sign_job_share_resolved_gate(self):
        self.assertIn("SIGNPATH_ENABLED: ${{ needs.resolve-windows-signing.outputs.enabled }}", WORKFLOW)
        signing_job = WORKFLOW.split("  sign-windows:\n", 1)[1].split("    steps:\n", 1)[0]
        self.assertIn("needs.resolve-windows-signing.outputs.enabled == 'true'", signing_job)
        self.assertIn("resolve-windows-signing", signing_job)

    def test_unsigned_build_does_not_embed_certificate_pin(self):
        self.assertIn("needs.resolve-windows-signing.outputs.enabled == 'true' && vars.SIGNPATH_SIGNER_SPKI_PIN || ''", WORKFLOW)
        self.assertNotIn("Import-Certificate", WORKFLOW)
        self.assertNotIn("if: vars.SIGNPATH_SIGNING_POLICY_SLUG == 'test-signing'", WORKFLOW)

    def test_manifests_wait_for_signing_or_explicit_skip(self):
        job = WORKFLOW.split("  release-update-manifests:\n", 1)[1]
        self.assertIn("needs: [build, create-release, sign-windows]", job)
        self.assertIn("needs.sign-windows.result == 'success' || needs.sign-windows.result == 'skipped'", job)
        self.assertIn("--private-key-env UPDATE_MANIFEST_ED25519_PRIVATE_KEY", job)

    @unittest.skipUnless(PWSH, "PowerShell is required to exercise the signing gate")
    def test_gate_matrix(self):
        gate_job = WORKFLOW.split("  resolve-windows-signing:\n", 1)[1].split("  create-release:\n", 1)[0]
        script = gate_job.split("        run: |\n", 1)[1]
        script = "\n".join(line[10:] for line in script.splitlines() if line.startswith("          "))
        with tempfile.TemporaryDirectory() as tmp:
            script_path = Path(tmp) / "resolve.ps1"
            script_path.write_text(script, encoding="utf-8")
            base = {"IS_TAG": "true", "SIGNPATH_ENABLED": "false", "SIGNPATH_API_TOKEN": "fixture-token",
                    "SIGNPATH_ORGANIZATION_ID": "fixture-org", "SIGNPATH_SIGNING_POLICY_SLUG": "test-signing",
                    "SIGNPATH_SIGNER_SPKI_PIN": "sha256:" + "a" * 64}
            cases = [
                ("disabled_with_test_policy", {}, False, False),
                ("default_disabled", {"SIGNPATH_ENABLED": ""}, False, False),
                ("test_policy_rejected", {"SIGNPATH_ENABLED": "true"}, True, None),
                ("production_enabled", {"SIGNPATH_ENABLED": "true", "SIGNPATH_SIGNING_POLICY_SLUG": "release-signing"}, False, True),
                ("production_missing_token", {"SIGNPATH_ENABLED": "true", "SIGNPATH_SIGNING_POLICY_SLUG": "release-signing", "SIGNPATH_API_TOKEN": ""}, True, None),
                ("production_missing_pin", {"SIGNPATH_ENABLED": "true", "SIGNPATH_SIGNING_POLICY_SLUG": "release-signing", "SIGNPATH_SIGNER_SPKI_PIN": ""}, True, None),
                ("manual_build_unsigned", {"SIGNPATH_ENABLED": "true", "IS_TAG": "false"}, False, False),
            ]
            for name, overrides, fails, expected in cases:
                with self.subTest(name=name):
                    output = Path(tmp) / (name + ".txt")
                    env = {**os.environ, **base, **overrides, "GITHUB_OUTPUT": str(output)}
                    result = subprocess.run([PWSH, "-NoProfile", "-NonInteractive", "-File", str(script_path)],
                                            env=env, capture_output=True, text=True, encoding="utf-8", errors="replace")
                    self.assertEqual(result.returncode != 0, fails, result.stdout + result.stderr)
                    if expected is not None:
                        self.assertEqual(output.read_text(encoding="utf-8-sig").strip(),
                                         "enabled=" + str(expected).lower())


if __name__ == "__main__":
    unittest.main()
