# GDownload Codebase Exploration Report

- Updated: 2026-07-10
- Baseline commit: `280c5c4`
- Active branch: `develop`

## Project Stage

GDownload is past the prototype stage. The current codebase contains a complete desktop download workflow, a V5 QML interface, settings and update systems, browser-extension integration, packaging for Windows/macOS/Linux, and automated unit, integration, and QML UI tests.

The current development priority is reliability rather than expanding the feature surface. Recent changes focus on aria2 failure handling, asynchronous lifecycle safety, update fallback behavior, and cross-platform build stability.

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
- Release: Windows installer, macOS DMG/universal bundle, Linux AppImage, and update metadata fallback.

## Current Risk Areas

1. Download failures are not yet represented as a complete diagnosis and retry workflow in the UI.
2. Configuration replacement and persistence failure paths need isolated filesystem tests and Windows-safe atomic replacement.
3. aria2 initialization, shutdown, early process exit, RPC timeout, and reconnect behavior need fake-RPC integration coverage.
4. SQLite caches need clearer lifecycle states, error reporting, schema migration, busy timeout, and WAL policy.
5. Update integrity should move from optional hashes to a signed manifest or embedded-key verification model.
6. QML screenshots are produced, but automated pixel/SSIM comparison is not yet part of the quality gate.

## Maintenance Rules

Update this report when module boundaries, major dependencies, build systems, packaging flows, or long-term architectural decisions change. Keep temporary investigation notes out of this file.

Files under `docs/superpowers/` are local-only planning material and must not be tracked or pushed.
