# GDownload Project Memory

## Current Direction

- Date: 2026-07-11
- Baseline: `develop` at `f0faafc`
- Target: v1.0.9 stability iteration
- Strategy: reliability first, then user-facing failure recovery, then deeper persistence and lifecycle hardening.

## Ordered Workstreams

1. Engineering quality closure
   - Make CTest presets work without manually adding `-C Debug`.
   - Replace core placeholder assertions with real isolated tests.
   - Run Windows Debug tests for pushes and pull requests.
   - Upload QML UI screenshots and manifest from CI.

2. Download failure and safe-operation workflow
   - Separate failed and completed tasks.
   - Expose error details and retry actions.
   - Require an explicit choice before deleting downloaded files.
   - Improve new-task validation and submission feedback.

3. Persistence and lifecycle hardening
   - Completed: atomic configuration replacement, aria2 lifecycle contracts, SQLite migration/WAL/busy-timeout/error reporting, and signed update verification.

## Important Commands

```powershell
cmake --preset windows-msvc-user
cmake --build --preset windows-debug-user -j
ctest --preset windows-debug-test-user
ctest --preset windows-debug-test-user -R UpdateManifestScriptTests --output-on-failure
```

The Windows generator is multi-configuration. Test presets and convenience targets must provide the Debug configuration themselves so callers do not need to remember `-C Debug`.

## Repository Rules

- User-facing QML text uses English source strings wrapped in `qsTr()`.
- Do not modify `.ts/.qm` files or run translation generation commands during feature work.
- After code changes, run `cmake --build --preset windows-debug-user -j`.
- Keep `docs/superpowers/` local-only. Do not track, commit, or push files from that directory.
- Do not push implementation changes unless the user explicitly requests it.

## Reliability Closure (2026-07-11)

- Configuration persistence uses atomic file replacement. All migrated config writers surface failures instead of replacing a valid file with a partially written one.
- Deletion paths return structured record/file outcomes. The QML layer reports partial failure truthfully and does not infer that a local file was deleted just because the download record was removed.
- SQLite history and tracker caches use `CacheResult`, managed connections, transactional schema migration, WAL, and a busy timeout. The migration contract is covered by isolated CTest targets.
- aria2 startup, shutdown, and runtime failures are modeled as lifecycle states. Polling drains before shutdown, and UI availability updates are queued onto the Qt UI thread.
- Update trust is fail-closed. Signed JSON manifests use an Ed25519 public key compiled from CI input. Windows additionally requires SHA-256, Authenticode, and an SPKI pin. Linux stages a private AppImage copy, checks it before handoff, persists the rollback floor under a lease, and restores that floor if launch fails.
- The vendored AppImage updater owns its worker thread and stops/joins it during destruction. The outer Linux updater also handles cancellation racing the native worker startup.

## Release Trust Operations

Tag releases require these GitHub secrets:

- `UPDATE_MANIFEST_ED25519_PRIVATE_KEY`: PEM or base64-encoded PEM Ed25519 PKCS#8 private key.
- `WINDOWS_SIGNING_CERT_PFX_BASE64` and `WINDOWS_SIGNING_CERT_PASSWORD`: Authenticode signing certificate.
- `UPDATE_WINDOWS_SIGNER_SPKI_SHA256`: `sha256:` followed by the signer's 64-character hexadecimal SPKI digest.
- `APPIMAGE_GPG_PRIVATE_KEY` and `APPIMAGE_GPG_FINGERPRINT`: the AppImage release-signing key and its exact fingerprint.

`UPDATE_MANIFEST_ED25519_PUBLIC_KEY` is a GitHub repository variable, not a source value. The release workflow derives the public key from the private key and refuses the release if the variable does not match.

The workflow generates and verifies `latest-windows-x64.json` and `latest-linux-x86_64.json` after signed artifacts have been uploaded. `gdownload.uk` must mirror those two assets; clients fall back to GitHub Release URLs when the primary host is unavailable.

Sparkle uses the public key embedded in `GDownloadOsxBundleInfo.plist.in`. This repository has no Sparkle signing private key, so it cannot manufacture a valid macOS appcast. Set the optional `SPARKLE_APPCAST_URL` repository variable to make the release workflow download and verify the hosted Sparkle `edSignature` against that public key.

## Latest Engineering Findings

- Before the quality-gate work, `ctest --preset windows-debug-test-user` registered 12 tests but ran none because the preset lacked a configuration.
- The generated commit version macro was unquoted, so using it as a string produced an MSVC compile error.
- The production logger path of initialize, write, flush, and shutdown is covered in an isolated test process. Shutdown/reinitialization behavior is not covered by the current test suite.

## Download Failure And Safe Actions

- Failed tasks remain in the stopped-task model and expose `errorCode` and `errorMessage`; no separate failure page is introduced.
- Retry is available only for failed URI tasks that retain their original download link. It reuses the original URL and derives aria2 `dir`/`out` from the saved path.
- A retry keeps the old stopped/history record when adding the replacement task fails. Cleanup failures after a successful add are logged because the new download must not be rolled back.
- Bulk removal defaults to deleting task records only. Removing downloaded files requires an explicit checkbox choice in the confirmation dialog.
- New-task submission trims lines, ignores blank entries, rejects an empty effective task list, and reports successful submission through the existing toast manager.
- Error messages are persisted through the existing history `error_message` column. The aria2 error code is runtime-only because the current schema has no dedicated column.
