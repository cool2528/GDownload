#!/usr/bin/env python3
"""Black-box tests for the release manifest generator."""

from __future__ import annotations

import base64
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "scripts" / "update" / "generate_update_manifest.py"
OPENSSL = shutil.which("openssl")


@unittest.skipUnless(OPENSSL, "OpenSSL is required for manifest signing tests")
class UpdateManifestGeneratorTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.key = self.root / "release-key.pem"
        subprocess.run([OPENSSL, "genpkey", "-algorithm", "ED25519", "-out", str(self.key)], check=True)
        self.asset = self.root / "GDownload-setup.exe"
        self.asset.write_bytes(b"release asset bytes")
        self.notes = self.root / "notes.txt"
        self.notes.write_text("Release notes\n", encoding="utf-8")

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def run_generator(self, *arguments: str, check: bool = True) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(GENERATOR), *arguments],
            text=True,
            capture_output=True,
            check=check,
        )

    def public_key(self) -> str:
        return self.run_generator("public-key", "--private-key-file", str(self.key)).stdout.strip()

    def test_creates_and_verifies_canonical_manifest(self) -> None:
        manifest = self.root / "latest-windows-x64.json"
        self.run_generator(
            "create",
            "--asset", str(self.asset),
            "--asset-url", "https://github.com/cool2528/GDownload/releases/download/v1/GDownload-setup.exe",
            "--platform", "windows-x64",
            "--version", "1.0.9",
            "--release-id", "42",
            "--published-at", "1700000000",
            "--expires-at", "1800000000",
            "--notes-file", str(self.notes),
            "--output", str(manifest),
            "--private-key-file", str(self.key),
        )
        self.run_generator("verify", "--manifest", str(manifest), "--public-key-base64", self.public_key())
        content = manifest.read_text(encoding="utf-8")
        manifest.write_text(content.replace("windows-x64", "linux-x86_64"), encoding="utf-8")
        result = self.run_generator(
            "verify", "--manifest", str(manifest), "--public-key-base64", self.public_key(), check=False
        )
        self.assertNotEqual(result.returncode, 0)

    def test_verifies_sparkle_ed_signature(self) -> None:
        signature_path = self.root / "asset.signature"
        subprocess.run(
            [OPENSSL, "pkeyutl", "-sign", "-rawin", "-inkey", str(self.key), "-in", str(self.asset), "-out", str(signature_path)],
            check=True,
        )
        signature = base64.b64encode(signature_path.read_bytes()).decode("ascii")
        appcast = self.root / "appcast.xml"
        appcast.write_text(
            "<rss xmlns:sparkle=\"http://www.andymatuschak.org/xml-namespaces/sparkle\">"
            "<channel><item><enclosure url=\"https://example.test/GDownload.dmg\" "
            f"sparkle:edSignature=\"{signature}\" /></item></channel></rss>",
            encoding="utf-8",
        )
        self.run_generator(
            "verify-sparkle-appcast",
            "--appcast", str(appcast),
            "--asset", str(self.asset),
            "--public-key-base64", self.public_key(),
        )
        self.asset.write_bytes(b"tampered asset")
        result = self.run_generator(
            "verify-sparkle-appcast",
            "--appcast", str(appcast),
            "--asset", str(self.asset),
            "--public-key-base64", self.public_key(),
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
