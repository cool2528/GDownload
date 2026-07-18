# GDownload Plugins

[简体中文](./README.md) · **English**

This directory holds GDownload's **built-in JS plugins**. GDownload plugins are written in
JavaScript and run inside an embedded QuickJS sandbox — one script works on every platform,
is hot-updatable, and can only reach the outside world through the host-provided `gdl.*` SDK.

> Want to write your own plugin? See [DEVELOPMENT.en.md](./DEVELOPMENT.en.md) (full guide).

## Contents

| Topic | Link |
| --- | --- |
| What plugins are and how to use them | this file |
| Building a plugin from scratch | [DEVELOPMENT.en.md](./DEVELOPMENT.en.md) |
| `gdl.*` SDK API reference | [DEVELOPMENT.en.md#gdl-sdk-reference](./DEVELOPMENT.en.md#gdl-sdk-reference) |
| Publishing to the plugin market | [DEVELOPMENT.en.md#publishing-to-the-plugin-market](./DEVELOPMENT.en.md#publishing-to-the-plugin-market) |
| Reference implementations | [`baidu-netdisk/`](./baidu-netdisk/) · [`demo-httpbin/`](./demo-httpbin/) |

## What a plugin looks like

Each plugin is a subdirectory under `plugins/`:

```
plugins/
  baidu-netdisk/           ← a plugin
    manifest.json          ← metadata + permissions + URL matching (required)
    main.js                ← entry (default-exports the plugin object)
    lib/                   ← optional: the plugin's own ES modules
      baidu_api.js
      ...
```

## Users: how to use plugins

### Install from the plugin market (recommended)

1. Open GDownload → **Preferences → Plugin Market**
2. Browse the available plugins and click **Install**
3. Installation: downloads from the official source → **verifies SHA-256 + Ed25519 signature**
   → extracts into `plugins/` → hot-loads

The market supports install / update / uninstall / enable-disable / search. Every plugin is
signature-verified; third-party sources show a security warning.

### Manual install

Drop a plugin directory (containing `manifest.json`) into the app's `plugins/` folder and
restart GDownload to load it.

### Where plugins load from

At startup GDownload scans `<app dir>/plugins/` for subdirectories that contain a `manifest.json`
and loads them. Plugins "disabled" from the market (recorded in `plugin_state.json` under the app
data dir) are skipped.

Plugin data lives in the app data directory:
- `plugin_cookies/<name>.json` — the plugin's own cookie jar
- `plugin_storage/<name>.json` — the plugin's own key-value storage

## Built-in plugins

| Plugin | Description |
| --- | --- |
| [`baidu-netdisk/`](./baidu-netdisk/) | Baidu netdisk share-link parser (reference implementation, exercises the whole SDK) |
| [`demo-httpbin/`](./demo-httpbin/) | SDK acceptance demo (development only, not shipped in installers) |

## Quick check (developers)

Use the bundled smoke-test host `js_plugin_smoke` (build with
`cmake --build build --target js_plugin_smoke`, then find it at
`build/.../bin/.../js_plugin_smoke.exe`) to drive a plugin over the real network:

```bash
js_plugin_smoke <plugins_dir> <data_dir> <url> [user_token]
```

It walks the full production path: `LoadJsPlugins → GetPluginsForUrl → ParseUrl / EnterDirectory /
GetDownloadInfo`.

You can also validate script syntax and module loading with QuickJS itself (`qjs`):

```bash
qjs -m your-plugin/main.js   # syntax / import check (don't touch gdl.* at top level)
```

## Security model

Plugins run sandboxed:
- Only `gdl.*` is exposed — **no** filesystem, process, or native network access
- Network requests are restricted to the `manifest.permissions.http` domain whitelist
- 64MB memory, 1MB stack, 60s per-call execution timeout
- Each plugin gets its own JS runtime; a plugin crash cannot take down the host
- Market-distributed plugins carry an Ed25519 signature and are only installed after verification

See [DEVELOPMENT.en.md](./DEVELOPMENT.en.md) for details.
