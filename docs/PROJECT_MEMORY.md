# GDownload Project Memory

## Current Direction

- Date: 2026-07-10
- Baseline: `develop` at `280c5c4`
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
   - Make configuration replacement safe on Windows.
   - Add fake aria2 RPC lifecycle tests.
   - Add SQLite migration, WAL, busy timeout, and error reporting.
   - Strengthen update signature verification.

## Important Commands

```powershell
cmake --preset windows-msvc-user
cmake --build --preset windows-debug-user -j
ctest --preset windows-debug-test-user
```

The Windows generator is multi-configuration. Test presets and convenience targets must provide the Debug configuration themselves so callers do not need to remember `-C Debug`.

## Repository Rules

- User-facing QML text uses English source strings wrapped in `qsTr()`.
- Do not modify `.ts/.qm` files or run translation generation commands during feature work.
- After code changes, run `cmake --build --preset windows-debug-user -j`.
- Keep `docs/superpowers/` local-only. Do not track, commit, or push files from that directory.
- Do not push implementation changes unless the user explicitly requests it.

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
