# GDownload Codebase Exploration Report

- Updated: 2026-07-11
- Baseline commit: `f0faafc`
- Active branch: `develop`

## Project Stage

GDownload is past the prototype stage. The current codebase contains a complete desktop download workflow, a V5 QML interface, settings and update systems, browser-extension integration, packaging for Windows/macOS/Linux, and automated unit, integration, and QML UI tests.

The current development priority is reliability rather than expanding the feature surface. The current closure covers durable configuration writes, explicit deletion outcomes, SQLite cache lifecycle contracts, aria2 shutdown/failure handling, and signed cross-platform update handoff.

## Main Modules

- `src/Module/GDLCore`: logging, TOML configuration, SQLite caches, filesystem and process helpers, platform abstraction.
- `src/App/engine`: aria2 process management, HTTP/WebSocket JSON-RPC, tracker updates, publish/subscribe infrastructure.
- `src/App/ui`: QML-facing managers, download task models, settings, themes, update UI, and application lifecycle.
- `src/App/ui/Resource/qml`: V5 pages, dialogs, reusable controls, theme-aware download and settings views.
- `tests/unit`: GTest coverage for core helpers, URL processing, defaults, update configuration, and process behavior.
- `tests/integration`: process relaunch integration tests.
- `tests/qml_ui`: offscreen visual tests and QML interaction tests with fake managers.
- `.github/workflows`: release builds and the Windows Debug quality gate.
- `package`, `custom-triplets`, `scripts`: installers, platform triplets, AppImage and macOS packaging helpers.

## Existing Infrastructure

- Download engine: aria2 child process with HTTP/WebSocket JSON-RPC, periodic task refresh, tracker fallback, and ETag caching.
- Persistence: TOML configuration and SQLite download/tracker caches.
- UI system: `GTheme`, `ElementPlusColors`, shared QML cards, dialogs, settings rows, toast notifications, and light/dark themes.
- Testing: CTest registration for unit, process integration, QML visual, and QML interaction tests.
- Release: Windows installer, macOS DMG/universal bundle, Linux AppImage, and per-platform signed update manifests. The release workflow signs Windows installers, signs Linux AppImages, and validates generated manifest signatures before upload.

## Reliability Closure

- Configuration writes use atomic replacement and preserve the previous file on persistence failure.
- Download/task deletion reports a structured result to the QML layer. Record deletion and local-file deletion are separate outcomes, so the UI can show an accurate failure rather than claiming a partial operation succeeded.
- SQLite caches expose operation results, use managed connection lifecycle and WAL/busy-timeout policy, and perform schema changes transactionally.
- aria2 lifecycle management has explicit readiness, shutdown drain, and runtime-failure contracts. Browser-facing availability notifications are queued onto the UI thread.
- Windows updates require a signed Ed25519 manifest, package SHA-256, Authenticode validation, and an SPKI pin. Linux updates use the same signed-manifest/rollback model, a private staged AppImage copy, AppImage signature policy, and an installation lease.
- Update metadata is platform-specific: `latest-windows-x64.json` and `latest-linux-x86_64.json`. A single manifest cannot serve both because the native verifier rejects a platform mismatch.

## Current Risk Areas

1. Download failures are not yet represented as a complete diagnosis and retry workflow in the UI.
2. Linux native build and end-to-end AppImage update execution still need a dedicated Linux CI/runtime test; Windows CTest covers the platform-neutral trust gates only.
3. macOS Sparkle appcast creation and publication remain owned by the hosting release system. This repository can cryptographically validate a hosted `edSignature` when `SPARKLE_APPCAST_URL` is configured, but does not hold the Sparkle private key.
4. `gdownload.uk` must mirror both platform-specific manifest files after a GitHub release. GitHub Release remains the built-in fallback source.
5. QML screenshots are produced, but automated pixel/SSIM comparison is not yet part of the quality gate.

## Maintenance Rules

Update this report when module boundaries, major dependencies, build systems, packaging flows, or long-term architectural decisions change. Keep temporary investigation notes out of this file.

Files under `docs/superpowers/` are local-only planning material and must not be tracked or pushed.
