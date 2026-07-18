# GDownload Plugin Development Guide

[简体中文](./DEVELOPMENT.md) · **English**

This guide walks you through writing a GDownload netdisk-parser plugin from scratch. Plugins are
written in **JavaScript (ES2023)**, run inside an embedded **QuickJS sandbox**, and interact with
the outside world only through the host-injected `gdl.*` SDK.

- Reference implementations: [`baidu-netdisk/`](./baidu-netdisk/) (complete), [`demo-httpbin/`](./demo-httpbin/) (minimal)
- User/installation notes: [README.en.md](./README.en.md)

## Contents

- [How plugins work](#how-plugins-work)
- [Directory layout](#directory-layout)
- [manifest.json specification](#manifestjson-specification)
- [settings — declarative configuration panel](#settings--declarative-configuration-panel-optional)
- [Entry script contract](#entry-script-contract)
- [Data structures](#data-structures)
- [gdl.* SDK reference](#gdl-sdk-reference)
- [Cookie Jar](#cookie-jar)
- [Localization (locales)](#localization-locales)
- [Full example: writing a new netdisk plugin](#full-example-writing-a-new-netdisk-plugin)
- [Local testing](#local-testing)
- [Publishing to the plugin market](#publishing-to-the-plugin-market)
- [Sandbox limits and common pitfalls](#sandbox-limits-and-common-pitfalls)

---

## How plugins work

After the user pastes a share link under "Add task → Netdisk", GDownload will:

1. `GetPluginsForUrl(url)` — match against your manifest's `url_patterns` to find the plugin that
   can handle the URL.
2. Call `parseUrl(url, userToken)` — you return the list of files/folders in the share.
3. When the user browses into a folder, call `enterDirectory(fileInfo)` — you return its contents.
4. When the user picks a file to download, call `getDownloadInfo(fileInfo)` — you return the real
   download URL and request headers; the host hands it to aria2c.

Your plugin methods are all **`async`** (return a Promise); the host awaits them. All HTTP, crypto,
storage, etc. go through `gdl.*`.

---

## Directory layout

```
my-plugin/
  manifest.json        required: metadata + permissions + URL matching
  main.js              required: entry, default-exports the plugin object
  lib/                 optional: split-out ES modules (relative import)
    api.js
```

`import` may only reference **relative paths inside the plugin directory** (`./`, `../`) — no
external or bare module names.

---

## manifest.json specification

```json
{
    "manifest_version": 1,
    "name": "quark-netdisk",
    "display_name": "Quark NetDisk",
    "version": "1.0.0",
    "author": "your-name",
    "description": "Parse Quark netdisk share links",
    "homepage": "https://github.com/you/gdownload-plugin-quark",
    "entry": "main.js",
    "type": "netdisk",
    "url_patterns": ["*://pan.quark.cn*"],
    "permissions": {
        "http": ["pan.quark.cn", "*.quark.cn", "drive-h.quark.cn"],
        "storage": true,
        "verification_ui": true
    },
    "min_app_version": "0.0.0",
    "locales": {
        "zh_CN": { "display_name": "夸克网盘", "description": "解析夸克网盘分享链接" }
    }
}
```

| Field | Required | Description |
| --- | --- | --- |
| `manifest_version` | ✅ | currently fixed at `1` |
| `name` | ✅ | unique id, **kebab-case** (lowercase letters/digits/hyphens) |
| `display_name` | — | display name; defaults to `name` |
| `version` | ✅ | semver `x.y.z`; the market uses it to detect updates |
| `author` | — | author |
| `description` | — | description (default English; use `locales` for i18n) |
| `homepage` | — | homepage URL |
| `entry` | ✅ | entry script relative path, usually `main.js` |
| `type` | ✅ | currently only `"netdisk"` |
| `url_patterns` | ✅ | wildcard match array; `*` matches any chars — see note below |
| `permissions.http` | ✅ | domain whitelist enforced by `gdl.http` |
| `permissions.storage` | — | allow `gdl.storage` (default false) |
| `permissions.verification_ui` | — | allow `gdl.ui.requestVerification` (default false) |
| `min_app_version` | — | minimum host version; load is rejected if unmet |
| `locales` | — | localized strings, see [Localization](#localization-locales) |

### ⚠️ url_patterns must match the "bare host"

When locating a plugin, the host probes with the **netdisk's host root** (e.g.
`https://pan.baidu.com`, no path), uniformly for all task types. So your pattern must match the
bare host.

- ✅ correct: `"*://pan.quark.cn*"` — matches both `https://pan.quark.cn` and `https://pan.quark.cn/s/abc`
- ❌ wrong: `"*://pan.quark.cn/s/*"` — only matches links with `/s/`, never the bare host, so the
  plugin is "not found"

### permissions.http whitelist rules

- Exact match: `"pan.quark.cn"` matches only that host
- Subdomain wildcard: `"*.quark.cn"` matches `a.quark.cn`, `a.b.quark.cn`, but **not** `quark.cn`
- `"*"` (match-all) is **forbidden** (load rejected)
- Requests to non-whitelisted domains make `gdl.http` throw; `Set-Cookie` from non-whitelisted
  domains is silently dropped

---

### settings — declarative configuration panel (optional)

Plugins don't write any UI code; instead you declare configuration fields in the manifest, and the
host renders a config form for it uniformly (entry points: the "Settings" button on the plugin
market card / the "Configure plugin" button on the netdisk-parsing page).

```json
"settings": [
    { "key": "cookie", "type": "textarea", "required": true, "role": "token",
      "label": "Cookie", "hint": "Paste your login cookie",
      "locales": { "zh_CN": { "label": "登录 Cookie", "hint": "粘贴登录后的 Cookie" } } },
    { "key": "use_cdn", "type": "bool", "default": true, "label": "Prefer CDN" },
    { "key": "quality", "type": "select", "default": "high", "options": ["high", "low"], "label": "Quality" }
]
```

| Field | Required | Description |
| --- | --- | --- |
| `key` | ✅ | `[a-z0-9_]+`, unique within the plugin |
| `type` | ✅ | `text` / `password` / `textarea` / `bool` / `select` / `number` |
| `label` | ✅ | default (English) label |
| `hint` | — | input hint |
| `required` | — | the field counts as "configured" only once all `required` fields have a value (default false) |
| `default` | — | default value |
| `role` | — | only `"token"`: this field's value is passed as `parseUrl`'s `userToken`; at most one, and it must be a text-type field |
| `options` | required for select | array of enum values |
| `locales` | — | per-language override of `label`/`hint` |

The JS side reads user config through the read-only `gdl.config.get(key)` (user value > default >
null):

```javascript
const useCdn = gdl.config.get("use_cdn");
```

Configuration is persisted by the host (`plugin_configs.json`); plugins cannot write it — once the
user saves, the next method call sees the new value.

---

## Entry script contract

`main.js` default-exports the plugin object as an ES module:

```javascript
export default {
    // optional: second-pass exact check after url_patterns matched
    canHandle(url) {
        return /pan\.quark\.cn\/s\//.test(url);
    },

    // parse a share link → FileInfo[]
    async parseUrl(url, userToken) {
        // userToken is the netdisk cookie the user configured in settings (may be empty)
        // ...
        return [ /* FileInfo... */ ];
    },

    // enter a directory → FileInfo[]
    async enterDirectory(fileInfo) {
        return [ /* FileInfo... */ ];
    },

    // resolve the real download address → ParseResult[]
    async getDownloadInfo(fileInfo) {
        return [ /* ParseResult... */ ];
    }
};
```

On failure, throw or return `null`/`undefined`; the host catches it, tells the user parsing failed,
and does not crash.

To share state across methods (surl, token, login info): create one instance at module top level
that the method closures share — see the `const api = new BaiduApi()` pattern in
`baidu-netdisk/main.js`.

---

## Data structures

### FileInfo (file/directory)

```javascript
{
    path: "/folder/movie.mkv",  // string, full path
    name: "movie.mkv",          // string, file name
    size: 1073741824,           // number, bytes
    is_dir: false,              // boolean
    file_id: "fid_xxx",         // string, your own locator id (used by enterDirectory/getDownloadInfo)
    create_time: 1710000000,    // number, Unix seconds, optional
    root_path: "/"              // string, optional
}
```

### ParseResult (download info)

```javascript
{
    real_url: "https://cdn.../file",  // string, required, real download URL
    file_name: "movie.mkv",           // string
    file_size: 1073741824,            // number
    headers: { /* see important note below */ },
    options: {},                      // reserved; currently ignored by the host
    mirrors: ["https://cdn2.../file"] // string[], backup URLs; see note below
}
```

#### ⚠️ aria2 options go in `headers`, using aria2 option names

The host passes `ParseResult.headers` **wholesale as aria2 task options** to aria2c (it does not
read the `options` field). So request headers and aria2 options both go in `headers`, keyed by
aria2's option names:

```javascript
headers: {
    // request header: aria2 --header (value is the full header line)
    "header": "Cookie:BDUSS=" + bduss,
    // aria2 --user-agent
    "user-agent": "netdisk;P2SP;...",
    // segmented parallelism (useful when a non-member account is rate-limited)
    "force-http-range": "true",
    "max-connection-per-server": "8",
    "split": "16"
}
```

- The file name (`out`) and save directory (`dir`) are filled in by the host — **don't** set `out`
  yourself.
- When `mirrors` is non-empty, the host uses `mirrors` as the download addresses (replacing
  `real_url`). For a single address, leave `mirrors` empty and use `real_url`; for multiple CDN
  mirrors, put them all in `mirrors`.

#### JS object → C++ multimap multi-value

`headers` is a multimap on the host side (same key, multiple values allowed). Express multi-values
with an **array**:

```javascript
headers: { "header": ["Cookie:a=1", "Referer:https://..."] }
```

---

## gdl.* SDK reference

The host injects a global `gdl` into every plugin — the **only** channel to the outside world.

### gdl.http — network requests

```javascript
const resp = await gdl.http.get(url, options);
const resp = await gdl.http.post(url, options);
```

`options` (all optional):

```javascript
{
    headers: { "Cookie": "...", "User-Agent": "..." },  // request headers
    params: { key: "value" },        // query params
    json: { ... },                   // POST JSON body (auto Content-Type: application/json)
    form: { k: "v" },                // POST application/x-www-form-urlencoded
    multipart: { k: "v" },           // POST multipart/form-data
    body: "raw string",              // POST raw body
    timeout: 15000,                  // ms, default 15000, max 60000
    follow_redirects: false,         // default true; set false to read the 302 Location
    use_cookie_jar: true,            // default true, see Cookie Jar
    accept_encoding: "identity"      // optional: "" auto gzip / "identity" plaintext / "disabled" off
}
```

Body is mutually exclusive; priority: `json > form > multipart > body`.

`Response`:

```javascript
{
    status: 200,                     // number
    headers: { "set-cookie": [...], "location": "..." },  // lowercase keys; multi-values as arrays
    text(): string,                  // response body text
    json(): object                   // parse JSON (throws TypeError on failure)
}
```

- The request domain must be in the `permissions.http` whitelist, or it throws.
- gzip is auto-negotiated and decoded; if a proxy corrupts the gzip stream, the host auto-retries
  with compression off.
- Concurrent requests per plugin are capped (anti-abuse); the response body has a size cap (16MB).

### gdl.http.cookies — cookie management

See [Cookie Jar](#cookie-jar).

```javascript
gdl.http.cookies.list(domain)                  // → [{name, value, domain, path, expires, secure, http_only}]
gdl.http.cookies.set({ name, value, domain, path, expires })
gdl.http.cookies.setFromString(domain, "k1=v1; k2=v2")   // for user-pasted cookies
gdl.http.cookies.remove(domain, name)
gdl.http.cookies.clear(domain)                 // omit domain to clear the whole jar
```

### gdl.crypto — digests/signing (returns lowercase hex)

```javascript
gdl.crypto.md5(str)
gdl.crypto.sha1(str)
gdl.crypto.sha256(str)
gdl.crypto.hmacSha256(key, data)
```

### gdl.utils — encode/decode/sleep

```javascript
gdl.utils.base64Encode(str) / base64Decode(str)
gdl.utils.urlEncode(str) / urlDecode(str)      // percent-encoding; space → %20
await gdl.utils.sleep(ms)                       // max 10s
```

> For `application/x-www-form-urlencoded` semantics (space → `+`), use the `form` option, or
> implement quote_plus yourself (see `baidu-netdisk/lib/util.js`).

### gdl.storage — persistent key-value (requires permissions.storage)

```javascript
gdl.storage.set("cookie", "xxx");   // string values only; JSON.stringify complex objects yourself
const v = gdl.storage.get("cookie"); // returns null if absent
gdl.storage.remove("cookie");
```

Isolated per plugin, 1MB quota, persisted to `plugin_storage/<name>.json`.

### gdl.config — read-only user configuration (backs manifest `settings`)

```javascript
const useCdn = gdl.config.get("use_cdn");  // user value > default > null
```

Fields come from the manifest's [`settings`](#settings--declarative-configuration-panel-optional)
declaration; the host renders the form, persists it, and validates it — plugins can only read it.

### gdl.ui / gdl.notify — interaction and notifications

```javascript
// captcha / human verification (requires permissions.verification_ui)
const code = await gdl.ui.requestVerification({
    imageBase64: "...",           // optional, captcha image
    message: "Enter the code"
});
// returns the user's input; rejects if the user cancels

// message notification
gdl.notify("Quota exceeded", "warning");  // level: success | error | warning | info | debug
```

### gdl.log — logging

```javascript
gdl.log.debug("...");  gdl.log.info("...");  gdl.log.warn("...");  gdl.log.error("...");
```

Goes to the host log with a uniform `[js-plugin:{name}]` prefix for easy troubleshooting.

---

## Cookie Jar

Each plugin has its own cookie jar, enabled by default, following browser semantics (RFC 6265):

- **Auto-store**: response `Set-Cookie` is parsed into the jar (Domain/Path/Expires/Max-Age)
- **Auto-send**: requests carry matching, non-expired cookies for the domain/path
- **Explicit wins**: an explicit `Cookie` in request `headers` merges with the jar; same-name keys
  take the explicit value
- **Persistence**: persistent cookies are stored in `plugin_cookies/<name>.json` and survive restarts
- **Domain boundary**: only cookies for domains in `permissions.http` are accepted/sent
- **Per-request off**: `options.use_cookie_jar: false`

Typical scenario — the user pastes a login cookie:

```javascript
gdl.http.cookies.setFromString("pan.baidu.com", userToken);
// afterwards every request to pan.baidu.com carries it and follows Set-Cookie refreshes
```

> Pitfall: if a site issues a one-time cookie on the first request (e.g. BAIDUID) and the jar
> already has it, the server **won't resend** it. When you need to read it, read from the response
> header first and fall back to `gdl.http.cookies.list()`.

---

## Localization (locales)

A plugin's `display_name`/`description` come from the manifest — the app's UI translations can't
reach them. Provide multiple languages with `locales`; the app picks by the current UI language and
**falls back to the default fields** when a locale is missing:

```json
{
    "display_name": "Quark NetDisk",
    "description": "Parse Quark netdisk share links",
    "locales": {
        "zh_CN": { "display_name": "夸克网盘", "description": "解析夸克网盘分享链接" },
        "zh_TW": { "display_name": "夸克網盤", "description": "解析夸克網盤分享連結" },
        "ja_JP": { "display_name": "Quark ネットディスク", "description": "..." },
        "ko_KR": { "display_name": "Quark 넷디스크", "description": "..." }
    }
}
```

Supported locales match the app: `zh_CN` / `zh_TW` / `ja_JP` / `ko_KR` (extensible later). When
publishing to the market, the plugin entry in `registry.json` should carry the same `locales` so the
"available" list is localized too.

---

## Full example: writing a new netdisk plugin

Skeleton using Quark netdisk (pseudocode; adapt to Quark's real API):

```javascript
// quark-netdisk/main.js
const api = {
    async parseUrl(url, userToken) {
        if (userToken) {
            gdl.http.cookies.setFromString("pan.quark.cn", userToken);
        }
        const shareId = (url.match(/\/s\/(\w+)/) || [])[1];
        if (!shareId) {
            gdl.notify("Invalid share link", "error");
            return null;
        }
        const resp = await gdl.http.post(
            "https://drive-h.quark.cn/1/clouddrive/share/sharepage/detail",
            { json: { pwd_id: shareId, passcode: "" } }
        );
        if (resp.status !== 200) return null;
        const data = resp.json();
        return (data.data.list || []).map(f => ({
            name: f.file_name,
            path: "/" + f.file_name,
            size: f.size || 0,
            is_dir: !!f.dir,
            file_id: f.fid
        }));
    },

    async enterDirectory(fileInfo) {
        const resp = await gdl.http.get(
            "https://drive-h.quark.cn/1/clouddrive/share/sharepage/detail",
            { params: { pdir_fid: fileInfo.file_id } }
        );
        if (resp.status !== 200) return null;
        return (resp.json().data.list || []).map(f => ({
            name: f.file_name, path: fileInfo.path + "/" + f.file_name,
            size: f.size || 0, is_dir: !!f.dir, file_id: f.fid
        }));
    },

    async getDownloadInfo(fileInfo) {
        if (fileInfo.is_dir) {
            const files = await api.enterDirectory(fileInfo);
            const out = [];
            for (const f of files || []) {
                const sub = await api.getDownloadInfo(f);
                if (sub) out.push(...sub);
            }
            return out;
        }
        const resp = await gdl.http.post(
            "https://drive-h.quark.cn/1/clouddrive/file/download",
            { json: { fids: [fileInfo.file_id] } }
        );
        if (resp.status !== 200) return null;
        const item = resp.json().data[0];
        return [{
            real_url: item.download_url,
            file_name: fileInfo.name,
            file_size: fileInfo.size,
            headers: {
                "header": "Cookie:" + gdl.http.cookies.list("pan.quark.cn")
                    .map(c => c.name + "=" + c.value).join("; "),
                "user-agent": "Mozilla/5.0 ..."
            },
            options: {},
            mirrors: []
        }];
    }
};

export default {
    canHandle(url) { return url.includes("pan.quark.cn"); },
    parseUrl: (u, t) => api.parseUrl(u, t),
    enterDirectory: (f) => api.enterDirectory(f),
    getDownloadInfo: (f) => api.getDownloadInfo(f)
};
```

---

## Local testing

### 1. Syntax/module check (no network)

```bash
qjs -m my-plugin/main.js
```

Don't touch `gdl.*` at the top level (it isn't injected by the host, so it would error); only do
import/syntax checks.

### 2. Real-network end-to-end (recommended)

Use the repo's smoke-test host, which walks the full production path:

```bash
# build: cmake --build build --config Release --target js_plugin_smoke
js_plugin_smoke <plugins_dir> <data_dir> <share_url> [user_token]
```

It loads the plugin, routes, calls `ParseUrl`/`EnterDirectory`/`GetDownloadInfo`, and prints the
results; `[js-plugin:my-plugin]`-prefixed logs show each step.

### 3. Verify in the real app

Drop the plugin directory into `<app dir>/plugins/`, restart GDownload, and test via
"Add task → Netdisk".

---

## Publishing to the plugin market

The market uses a **Git-repo-as-registry** ([gdownload-plugin-registry](https://github.com/cool2528/gdownload-plugin-registry)).

1. Prepare your plugin directory (`manifest.json` + `main.js` + `lib/`).
2. Package and sign with the Ed25519 private key (the private key is not committed; the maintainer
   holds it):
   ```bash
   python scripts/package_and_sign.py <plugin_dir> <version> keys/registry_private.pem --out dist
   ```
   This prints a version entry JSON and produces `dist/<name>-v<version>.zip`.
3. Merge the version entry into the plugin's `versions` in `registry.json`, update `latest`, and add
   `locales` to the plugin entry.
4. Upload the zip as a GitHub Release asset (tag = `<name>-v<version>`).
5. Open a PR; CI validates the schema, SHA-256, and Ed25519 signature before merging.

On install the client downloads via multi-source fallback (GitHub → jsDelivr → ghproxy) →
**verifies SHA-256 + signature** → extracts → hot-loads. **Every mirror download must pass
verification before install**, so an untrusted mirror is harmless.

---

## Sandbox limits and common pitfalls

| Limit | Value / note |
| --- | --- |
| Available APIs | `gdl.*` only; no filesystem/process/native socket |
| Network | only `permissions.http` whitelist domains; `*` match-all forbidden |
| Memory | 64MB per runtime |
| Stack | 1MB |
| Execution timeout | 60s per call (`gdl.utils.sleep` not counted) |
| Modules | relative import within the plugin dir only |
| Storage | 1MB per plugin |

**Common pitfalls**:

- **url_patterns don't match**: they must match the bare host (use `*://host*`, not just `/s/*`).
- **Download does nothing / options lost**: aria2 options go in `ParseResult.headers` (not
  `options`), using aria2 option names.
- **Duplicate `out`**: don't set `out` in headers yourself — the host adds it.
- **Can't read a one-time cookie like BAIDUID**: when the jar already has it the server won't
  resend; read from the response first, fall back to the jar.
- **gzip "incorrect header check"**: usually a proxy corrupting the gzip stream; the host already
  auto-retries with compression off, or set `accept_encoding: "disabled"` manually.
- **Unencoded `|` in PCS direct links, etc.**: the host tolerates it (only the pre-query part is
  parsed for the host check); the full URL is sent as-is.
- **Localization not applied**: `locales` must be present in both `manifest.json` and the
  `registry.json` plugin entry.
