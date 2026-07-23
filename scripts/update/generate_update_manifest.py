#!/usr/bin/env python3
"""Create and verify signed GDownload update manifests.

The release private key must be an Ed25519 PKCS#8 PEM value. Environment input
may contain that PEM directly or its base64 encoding. The emitted JSON is
canonicalized exactly as the C++ verifier does: UTF-8, sorted keys, and compact
separators after removing ``signature``.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Any
from urllib.parse import urlparse
from urllib.request import urlopen
import xml.etree.ElementTree as element_tree


ED25519_SPKI_PREFIX = bytes.fromhex("302a300506032b6570032100")
SPARKLE_NAMESPACE = "http://www.andymatuschak.org/xml-namespaces/sparkle"


def canonical_json(payload: dict[str, Any]) -> bytes:
    """Return the byte sequence signed by the native update verifier."""
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":"), sort_keys=True).encode("utf-8")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_openssl(arguments: list[str]) -> subprocess.CompletedProcess[bytes]:
    try:
        return subprocess.run(
            ["openssl", *arguments], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except FileNotFoundError as error:
        raise RuntimeError("OpenSSL is required to sign or verify update metadata") from error
    except subprocess.CalledProcessError as error:
        detail = error.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(f"OpenSSL operation failed: {detail or 'unknown error'}") from error


def private_key_bytes(value: str) -> bytes:
    raw = value.encode("utf-8")
    if b"-----BEGIN" in raw:
        return raw
    try:
        decoded = base64.b64decode(raw, validate=True)
    except ValueError as error:
        raise ValueError("the private key must be PEM text or base64-encoded PEM") from error
    if b"-----BEGIN" not in decoded:
        raise ValueError("the decoded private key is not PEM")
    return decoded


class PrivateKeyFile:
    def __init__(self, arguments: argparse.Namespace) -> None:
        self.path: Path
        self.temporary_path: Path | None = None
        if arguments.private_key_file:
            self.path = Path(arguments.private_key_file)
            if not self.path.is_file():
                raise ValueError("the private key file does not exist")
            return
        value = os.environ.get(arguments.private_key_env, "")
        if not value:
            raise ValueError(f"the private key environment variable {arguments.private_key_env} is empty")
        descriptor, temporary = tempfile.mkstemp(prefix="gdownload-update-key-")
        self.path = Path(temporary)
        self.temporary_path = self.path
        try:
            # os.fchmod 是 POSIX 专有,Windows 的 Python 没有该函数。加守卫使脚本能在
            # Windows 本机运行(开发者生成/派生更新签名密钥时),临时文件由 mkstemp 默认
            # 就限权为 owner-only;Linux/CI 环境仍显式设 0600,行为不变。
            if hasattr(os, "fchmod"):
                os.fchmod(descriptor, 0o600)
            os.write(descriptor, private_key_bytes(value))
        finally:
            os.close(descriptor)

    def __enter__(self) -> Path:
        return self.path

    def __exit__(self, _type: object, _value: object, _traceback: object) -> None:
        if self.temporary_path:
            self.temporary_path.unlink(missing_ok=True)


def public_key_from_private(private_key: Path) -> str:
    result = run_openssl(["pkey", "-in", str(private_key), "-pubout", "-outform", "DER"])
    encoded = result.stdout
    if len(encoded) != len(ED25519_SPKI_PREFIX) + 32 or not encoded.startswith(ED25519_SPKI_PREFIX):
        raise RuntimeError("the private key is not an Ed25519 key")
    return base64.b64encode(encoded[-32:]).decode("ascii")


def public_key_file(public_key_base64: str) -> tempfile.NamedTemporaryFile:
    try:
        raw = base64.b64decode(public_key_base64, validate=True)
    except ValueError as error:
        raise ValueError("the public key is not valid base64") from error
    if len(raw) != 32:
        raise ValueError("the public key must contain 32 Ed25519 bytes")
    output = tempfile.NamedTemporaryFile(prefix="gdownload-update-public-", delete=False)
    output.write(ED25519_SPKI_PREFIX + raw)
    output.flush()
    output.close()
    return output


def sign_message(message: bytes, private_key: Path) -> str:
    with tempfile.NamedTemporaryFile(prefix="gdownload-update-message-", delete=False) as source:
        source.write(message)
        source_path = Path(source.name)
    with tempfile.NamedTemporaryFile(prefix="gdownload-update-signature-", delete=False) as output:
        signature_path = Path(output.name)
    try:
        run_openssl(["pkeyutl", "-sign", "-rawin", "-inkey", str(private_key), "-in", str(source_path), "-out", str(signature_path)])
        signature = signature_path.read_bytes()
    finally:
        source_path.unlink(missing_ok=True)
        signature_path.unlink(missing_ok=True)
    if len(signature) != 64:
        raise RuntimeError("OpenSSL did not produce an Ed25519 signature")
    return base64.b64encode(signature).decode("ascii")


def verify_file(source_path: Path, signature_base64: str, public_key_base64: str) -> None:
    try:
        signature = base64.b64decode(signature_base64, validate=True)
    except ValueError as error:
        raise ValueError("the signature is not valid base64") from error
    if len(signature) != 64:
        raise ValueError("the Ed25519 signature must contain 64 bytes")
    public_key = public_key_file(public_key_base64)
    with tempfile.NamedTemporaryFile(prefix="gdownload-update-signature-", delete=False) as signature_file:
        signature_file.write(signature)
        signature_path = Path(signature_file.name)
    try:
        run_openssl([
            "pkeyutl", "-verify", "-rawin", "-pubin", "-inkey", public_key.name,
            "-keyform", "DER", "-in", str(source_path), "-sigfile", str(signature_path),
        ])
    finally:
        Path(public_key.name).unlink(missing_ok=True)
        signature_path.unlink(missing_ok=True)


def verify_message(message: bytes, signature_base64: str, public_key_base64: str) -> None:
    with tempfile.NamedTemporaryFile(prefix="gdownload-update-message-", delete=False) as source:
        source.write(message)
        source_path = Path(source.name)
    try:
        verify_file(source_path, signature_base64, public_key_base64)
    finally:
        source_path.unlink(missing_ok=True)


def require_https_url(value: str, label: str) -> None:
    parsed = urlparse(value)
    if parsed.scheme != "https" or not parsed.netloc:
        raise ValueError(f"{label} must be an absolute HTTPS URL")


def command_create(arguments: argparse.Namespace) -> None:
    asset = Path(arguments.asset)
    if not asset.is_file():
        raise ValueError("the update asset does not exist")
    if arguments.release_id <= 0:
        raise ValueError("release_id must be positive")
    if arguments.expires_at < arguments.published_at:
        raise ValueError("expires_at must not precede published_at")
    require_https_url(arguments.asset_url, "asset_url")
    notes = Path(arguments.notes_file).read_text(encoding="utf-8") if arguments.notes_file else ""
    payload: dict[str, Any] = {
        "schema_version": 1,
        "release_id": arguments.release_id,
        "version": arguments.version,
        "platform": arguments.platform,
        "published_at": arguments.published_at,
        "expires_at": arguments.expires_at,
        "notes": notes,
        "asset": {
            "name": asset.name,
            "url": arguments.asset_url,
            "size": asset.stat().st_size,
            "sha256": file_sha256(asset),
        },
    }
    with PrivateKeyFile(arguments) as private_key:
        payload["signature"] = sign_message(canonical_json(payload), private_key)
    output = Path(arguments.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def command_verify(arguments: argparse.Namespace) -> None:
    manifest = json.loads(Path(arguments.manifest).read_text(encoding="utf-8"))
    if not isinstance(manifest, dict) or manifest.get("schema_version") != 1:
        raise ValueError("the manifest schema is invalid")
    signature = manifest.pop("signature", None)
    if not isinstance(signature, str):
        raise ValueError("the manifest has no signature")
    verify_message(canonical_json(manifest), signature, arguments.public_key_base64)


def command_public_key(arguments: argparse.Namespace) -> None:
    with PrivateKeyFile(arguments) as private_key:
        print(public_key_from_private(private_key))


def download_to_temporary_file(url: str) -> Path:
    require_https_url(url, "Sparkle enclosure URL")
    with urlopen(url, timeout=60) as response, tempfile.NamedTemporaryFile(prefix="gdownload-sparkle-", delete=False) as output:
        while True:
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            output.write(chunk)
        return Path(output.name)


def command_verify_sparkle(arguments: argparse.Namespace) -> None:
    if arguments.appcast_url:
        appcast_path = download_to_temporary_file(arguments.appcast_url)
    else:
        appcast_path = Path(arguments.appcast)
    try:
        root = element_tree.parse(appcast_path).getroot()
    finally:
        if arguments.appcast_url:
            appcast_path.unlink(missing_ok=True)
    enclosures = root.findall(".//enclosure")
    if not enclosures:
        raise ValueError("the Sparkle appcast has no enclosure")
    signature_key = f"{{{SPARKLE_NAMESPACE}}}edSignature"
    enclosure = next((item for item in enclosures if item.get(signature_key)), None)
    if enclosure is None:
        raise ValueError("the Sparkle appcast has no edSignature")
    signature = enclosure.get(signature_key)
    if not signature:
        raise ValueError("the Sparkle enclosure has an empty edSignature")
    asset_path = Path(arguments.asset) if arguments.asset else download_to_temporary_file(enclosure.get("url", ""))
    try:
        if not asset_path.is_file():
            raise ValueError("the Sparkle enclosure asset does not exist")
        verify_file(asset_path, signature, arguments.public_key_base64)
    finally:
        if not arguments.asset:
            asset_path.unlink(missing_ok=True)


def add_private_key_arguments(parser: argparse.ArgumentParser) -> None:
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--private-key-file")
    group.add_argument("--private-key-env")


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    create = commands.add_parser("create", help="create a signed update manifest")
    create.add_argument("--asset", required=True)
    create.add_argument("--asset-url", required=True)
    create.add_argument("--platform", required=True)
    create.add_argument("--version", required=True)
    create.add_argument("--release-id", required=True, type=int)
    create.add_argument("--published-at", required=True, type=int)
    create.add_argument("--expires-at", required=True, type=int)
    create.add_argument("--notes-file")
    create.add_argument("--output", required=True)
    add_private_key_arguments(create)
    create.set_defaults(handler=command_create)

    verify = commands.add_parser("verify", help="verify a signed update manifest")
    verify.add_argument("--manifest", required=True)
    verify.add_argument("--public-key-base64", required=True)
    verify.set_defaults(handler=command_verify)

    public_key = commands.add_parser("public-key", help="derive a base64 Ed25519 public key")
    add_private_key_arguments(public_key)
    public_key.set_defaults(handler=command_public_key)

    sparkle = commands.add_parser("verify-sparkle-appcast", help="verify a Sparkle enclosure edSignature")
    appcast_group = sparkle.add_mutually_exclusive_group(required=True)
    appcast_group.add_argument("--appcast")
    appcast_group.add_argument("--appcast-url")
    sparkle.add_argument("--asset")
    sparkle.add_argument("--public-key-base64", required=True)
    sparkle.set_defaults(handler=command_verify_sparkle)
    return parser


def main() -> int:
    arguments = make_parser().parse_args()
    try:
        arguments.handler(arguments)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError, element_tree.ParseError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
