# GDownload 插件开发指南

**简体中文** · [English](./DEVELOPMENT.en.md)

本指南教你从零写一个 GDownload 网盘解析插件。插件用 **JavaScript（ES2023）** 编写，运行在
内嵌的 **QuickJS 沙箱**中，只能通过宿主注入的 `gdl.*` SDK 与外界交互。

- 参考实现：[`baidu-netdisk/`](./baidu-netdisk/)（完整）、[`demo-httpbin/`](./demo-httpbin/)（最小）
- 用户/安装说明见 [README.md](./README.md)

## 目录

- [插件的工作原理](#插件的工作原理)
- [目录结构](#目录结构)
- [manifest.json 规范](#manifestjson-规范)
- [settings —— 声明式配置面板](#settings--声明式配置面板可选)
- [入口脚本约定](#入口脚本约定)
- [数据结构](#数据结构)
- [gdl.* SDK 参考](#gdl-sdk-参考)
- [Cookie Jar](#cookie-jar)
- [多语言（locales）](#多语言locales)
- [完整示例：写一个新网盘插件](#完整示例写一个新网盘插件)
- [本地测试](#本地测试)
- [发布到插件市场](#发布到插件市场)
- [沙箱限制与常见坑](#沙箱限制与常见坑)

---

## 插件的工作原理

用户在"添加任务 → 网盘"里粘贴分享链接后，GDownload 会：

1. `GetPluginsForUrl(url)` —— 用你 manifest 的 `url_patterns` 匹配，找到能处理该 URL 的插件。
2. 调 `parseUrl(url, userToken)` —— 你返回分享内的文件/目录列表。
3. 用户浏览目录时调 `enterDirectory(fileInfo)` —— 你返回该目录内容。
4. 用户选择文件下载时调 `getDownloadInfo(fileInfo)` —— 你返回真实下载地址与请求头，
   宿主交给 aria2c 下载。

你的插件方法都是 **`async`**（返回 Promise）；宿主会等待其完成。所有 HTTP、加密、存储等
操作都通过 `gdl.*` 完成。

---

## 目录结构

```
my-plugin/
  manifest.json        必需：元数据 + 权限 + URL 匹配
  main.js              必需：入口，default export 插件对象
  lib/                 可选：拆分的 ES 模块（相对 import）
    api.js
```

`import` 只允许**插件目录内的相对路径**（`./`、`../`），不能引用外部或裸模块名。

---

## manifest.json 规范

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

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `manifest_version` | ✅ | 当前固定为 `1` |
| `name` | ✅ | 唯一标识，**kebab-case**（小写字母/数字/连字符） |
| `display_name` | — | 界面显示名，缺省用 `name` |
| `version` | ✅ | 语义化版本 `x.y.z`，市场据此判断更新 |
| `author` | — | 作者 |
| `description` | — | 描述（英文默认，配合 `locales` 多语言） |
| `homepage` | — | 主页 URL |
| `entry` | ✅ | 入口脚本相对路径，通常 `main.js` |
| `type` | ✅ | 当前仅 `"netdisk"` |
| `url_patterns` | ✅ | 通配符匹配规则数组，`*` 匹配任意字符，详见下方注意 |
| `permissions.http` | ✅ | 允许访问的域名白名单，`gdl.http` 强制校验 |
| `permissions.storage` | — | 是否允许 `gdl.storage`（默认 false） |
| `permissions.verification_ui` | — | 是否允许 `gdl.ui.requestVerification`（默认 false） |
| `min_app_version` | — | 最低宿主版本，不满足拒绝加载 |
| `locales` | — | 多语言文案，见[多语言](#多语言locales) |

### ⚠️ url_patterns 必须匹配"裸主机名"

宿主在定位插件时用**该网盘的主机根地址**探测（例如 `https://pan.baidu.com`，不带路径），
对所有任务类型统一如此。所以你的 pattern 必须能匹配裸主机名。

- ✅ 正确：`"*://pan.quark.cn*"` —— 同时匹配 `https://pan.quark.cn` 和 `https://pan.quark.cn/s/abc`
- ❌ 错误：`"*://pan.quark.cn/s/*"` —— 只匹配带 `/s/` 的分享链，匹配不到裸主机名，插件会"找不到"

### permissions.http 白名单规则

- 精确匹配：`"pan.quark.cn"` 只匹配该主机
- 子域通配：`"*.quark.cn"` 匹配 `a.quark.cn`、`a.b.quark.cn`，**不**匹配 `quark.cn` 本身
- **禁止** `"*"` 全通配（会被拒绝加载）
- 白名单外的请求域名会让 `gdl.http` 抛异常；白名单外域名的 `Set-Cookie` 被静默丢弃

---

### settings —— 声明式配置面板（可选）

插件不写任何 UI 代码；在 manifest 里声明配置字段，宿主统一渲染配置表单
（入口：插件市场卡片"Settings"按钮 / 网盘解析页"Configure plugin"按钮）。

```json
"settings": [
    { "key": "cookie", "type": "textarea", "required": true, "role": "token",
      "label": "Cookie", "hint": "Paste your login cookie",
      "locales": { "zh_CN": { "label": "登录 Cookie", "hint": "粘贴登录后的 Cookie" } } },
    { "key": "use_cdn", "type": "bool", "default": true, "label": "Prefer CDN" },
    { "key": "quality", "type": "select", "default": "high", "options": ["high", "low"], "label": "Quality" }
]
```

| 属性 | 必填 | 说明 |
| --- | --- | --- |
| `key` | ✅ | `[a-z0-9_]+`，插件内唯一 |
| `type` | ✅ | `text` / `password` / `textarea` / `bool` / `select` / `number` |
| `label` | ✅ | 默认（英文）标签 |
| `hint` | — | 输入提示 |
| `required` | — | required 字段全部有值才算"已配置"（缺省 false） |
| `default` | — | 缺省值 |
| `role` | — | 仅 `"token"`：该字段值作为 `parseUrl` 的 `userToken` 传入；至多一个，且须为文本类字段 |
| `options` | select 必填 | 枚举值数组 |
| `locales` | — | 按语言覆盖 `label`/`hint` |

JS 侧用只读 `gdl.config.get(key)` 读取用户配置（用户值 > default > null）：

```javascript
const useCdn = gdl.config.get("use_cdn");
```

配置由宿主落盘（`plugin_configs.json`），插件不可写；用户保存后下次方法调用即读到新值。

---

## 入口脚本约定

`main.js` 以 ES Module 默认导出插件对象：

```javascript
export default {
    // 可选：url_patterns 命中后的二次精确判断
    canHandle(url) {
        return /pan\.quark\.cn\/s\//.test(url);
    },

    // 解析分享链接 → FileInfo[]
    async parseUrl(url, userToken) {
        // userToken 是用户在设置里配置的该网盘 cookie（可能为空）
        // ...
        return [ /* FileInfo... */ ];
    },

    // 进入目录 → FileInfo[]
    async enterDirectory(fileInfo) {
        return [ /* FileInfo... */ ];
    },

    // 解析真实下载地址 → ParseResult[]
    async getDownloadInfo(fileInfo) {
        return [ /* ParseResult... */ ];
    }
};
```

方法失败时抛异常或返回 `null`/`undefined`；宿主会捕获并向用户提示解析失败，不会崩溃。

跨方法共享状态（如 surl、token、已登录信息）：在模块顶层建一个实例，方法闭包共享它——
参考 `baidu-netdisk/main.js` 里 `const api = new BaiduApi()` 的写法。

---

## 数据结构

### FileInfo（文件/目录）

```javascript
{
    path: "/folder/movie.mkv",  // string，完整路径
    name: "movie.mkv",          // string，文件名
    size: 1073741824,           // number，字节
    is_dir: false,              // boolean
    file_id: "fid_xxx",         // string，你自定义的定位标识（供 enterDirectory/getDownloadInfo 用）
    create_time: 1710000000,    // number，Unix 秒，可省略
    root_path: "/"              // string，可省略
}
```

### ParseResult（下载信息）

```javascript
{
    real_url: "https://cdn.../file",  // string，必填，真实下载地址
    file_name: "movie.mkv",           // string
    file_size: 1073741824,            // number
    headers: { /* 见下方重要说明 */ },
    options: {},                      // 保留字段，当前被宿主忽略
    mirrors: ["https://cdn2.../file"] // string[]，备用地址；见下方说明
}
```

#### ⚠️ aria2 选项要放进 `headers`，用 aria2 选项名

宿主把 `ParseResult.headers` **整体作为 aria2 任务选项**传给 aria2c（不读 `options` 字段）。
所以请求头和 aria2 选项都放 `headers`，键用 aria2 的选项名：

```javascript
headers: {
    // 请求头：aria2 的 --header（值是完整头行）
    "header": "Cookie:BDUSS=" + bduss,
    // aria2 的 --user-agent
    "user-agent": "netdisk;P2SP;...",
    // 分段并发加速（非会员限速时有用）
    "force-http-range": "true",
    "max-connection-per-server": "8",
    "split": "16"
}
```

- 文件名（`out`）与保存目录（`dir`）由宿主自动补，**不要**自己塞 `out`。
- `mirrors` 非空时，宿主用 `mirrors` 作为下载地址（会替代 `real_url`）。单地址时 `mirrors` 留空、
  用 `real_url` 即可；多个 CDN 镜像则把它们全放进 `mirrors`。

#### JS 对象 → C++ multimap 的多值表达

`headers` 在宿主侧是 multimap（允许同名多值）。JS 侧同名多值用**数组**表达：

```javascript
headers: { "header": ["Cookie:a=1", "Referer:https://..."] }
```

---

## gdl.* SDK 参考

宿主向每个插件注入全局 `gdl`，这是与外界交互的**唯一通道**。

### gdl.http —— 网络请求

```javascript
const resp = await gdl.http.get(url, options);
const resp = await gdl.http.post(url, options);
```

`options`（全部可选）：

```javascript
{
    headers: { "Cookie": "...", "User-Agent": "..." },  // 请求头
    params: { key: "value" },        // query 参数
    json: { ... },                   // POST JSON body（自动带 Content-Type: application/json）
    form: { k: "v" },                // POST application/x-www-form-urlencoded
    multipart: { k: "v" },           // POST multipart/form-data
    body: "raw string",              // POST 原始 body
    timeout: 15000,                  // 毫秒，默认 15000，上限 60000
    follow_redirects: false,         // 默认 true；取 302 Location 时设 false
    use_cookie_jar: true,            // 默认 true，见 Cookie Jar
    accept_encoding: "identity"      // 可选："" 自动gzip / "identity" 明文 / "disabled" 关闭解压
}
```

body 互斥，优先级：`json > form > multipart > body`。

`Response`：

```javascript
{
    status: 200,                     // number
    headers: { "set-cookie": [...], "location": "..." },  // 键统一小写；同名多值为数组
    text(): string,                  // 响应体文本
    json(): object                   // 解析 JSON（失败抛 TypeError）
}
```

- 请求域名必须在 `permissions.http` 白名单内，否则抛异常。
- 默认自动协商 gzip 解压；若中间代理破坏 gzip 流，宿主会自动降级重试。
- 单插件并发请求数受限（防滥用）；响应体有大小上限（16MB）。

### gdl.http.cookies —— Cookie 管理

见 [Cookie Jar](#cookie-jar)。

```javascript
gdl.http.cookies.list(domain)                  // → [{name, value, domain, path, expires, secure, http_only}]
gdl.http.cookies.set({ name, value, domain, path, expires })
gdl.http.cookies.setFromString(domain, "k1=v1; k2=v2")   // 用户粘贴 cookie 场景
gdl.http.cookies.remove(domain, name)
gdl.http.cookies.clear(domain)                 // 省略 domain 清空整个 Jar
```

### gdl.crypto —— 摘要/签名（返回十六进制小写）

```javascript
gdl.crypto.md5(str)
gdl.crypto.sha1(str)
gdl.crypto.sha256(str)
gdl.crypto.hmacSha256(key, data)
```

### gdl.utils —— 编解码/延时

```javascript
gdl.utils.base64Encode(str) / base64Decode(str)
gdl.utils.urlEncode(str) / urlDecode(str)      // 百分号编码；空格 → %20
await gdl.utils.sleep(ms)                       // 上限 10s
```

> 需要 `application/x-www-form-urlencoded` 语义（空格 → `+`）时用 `form` 选项，或自己实现 quote_plus
> （参考 `baidu-netdisk/lib/util.js`）。

### gdl.storage —— 持久化键值（需 permissions.storage）

```javascript
gdl.storage.set("cookie", "xxx");   // 值仅支持 string；复杂对象自行 JSON.stringify
const v = gdl.storage.get("cookie"); // 不存在返回 null
gdl.storage.remove("cookie");
```

按插件名隔离，单插件限额 1MB，落盘到 `plugin_storage/<name>.json`。

### gdl.config —— 只读用户配置（对应 manifest `settings`）

```javascript
const useCdn = gdl.config.get("use_cdn");  // 用户值 > default > null
```

字段来自 manifest 的 [`settings`](#settings--声明式配置面板可选) 声明；宿主负责渲染表单、落盘与校验，
插件只读、不可写。

### gdl.ui / gdl.notify —— 交互与通知

```javascript
// 验证码/人机校验（需 permissions.verification_ui）
const code = await gdl.ui.requestVerification({
    imageBase64: "...",           // 可选，验证码图片
    message: "请输入验证码"
});
// 返回用户输入；用户取消则 reject

// 消息通知
gdl.notify("配额不足", "warning");  // level: success | error | warning | info | debug
```

### gdl.log —— 日志

```javascript
gdl.log.debug("...");  gdl.log.info("...");  gdl.log.warn("...");  gdl.log.error("...");
```

输出到宿主日志，统一前缀 `[js-plugin:{name}]`，方便排查。

---

## Cookie Jar

每个插件有独立的 Cookie Jar，默认启用，语义对齐浏览器（RFC 6265）：

- **自动入库**：响应 `Set-Cookie` 自动解析入库（Domain/Path/Expires/Max-Age）
- **自动携带**：请求自动带上匹配域名/路径且未过期的 Cookie
- **显式优先**：请求 `headers` 里显式的 `Cookie` 与 Jar 合并，同名以显式为准
- **持久化**：持久 Cookie 落盘 `plugin_cookies/<name>.json`，重启仍有效
- **域名边界**：只接受/发送 `permissions.http` 白名单内域名的 Cookie
- **按请求关闭**：`options.use_cookie_jar: false`

典型场景——用户粘贴登录 Cookie：

```javascript
gdl.http.cookies.setFromString("pan.baidu.com", userToken);
// 之后所有对 pan.baidu.com 的请求自动携带，并跟随 Set-Cookie 刷新
```

> 坑：如果某网站首个请求会下发一次性 Cookie（如 BAIDUID），而 Jar 里已有它，服务器**不会重发**。
> 需要读取它时，优先从响应头取，取不到再从 `gdl.http.cookies.list()` 兜底。

---

## 多语言（locales）

插件的 `display_name`/`description` 来自 manifest，App 的界面翻译管不到。用 `locales` 提供多语言，
App 按当前界面语言挑选，**缺失回退默认字段**：

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

支持的 locale 与 App 一致：`zh_CN` / `zh_TW` / `ja_JP` / `ko_KR`（未来可扩展）。
发布到市场时，`registry.json` 里的插件条目也应带同样的 `locales`，供"可安装"列表显示本地化。

---

## 完整示例：写一个新网盘插件

以夸克网盘为骨架（伪代码，接口以夸克实际 API 为准）：

```javascript
// quark-netdisk/main.js
const api = {
    async parseUrl(url, userToken) {
        if (userToken) {
            gdl.http.cookies.setFromString("pan.quark.cn", userToken);
        }
        const shareId = (url.match(/\/s\/(\w+)/) || [])[1];
        if (!shareId) {
            gdl.notify("无效的分享链接", "error");
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

## 本地测试

### 1. 语法/模块检查（无需网络）

```bash
qjs -m my-plugin/main.js
```

顶层不要访问 `gdl.*`（宿主未注入时会报错）；只做 import/语法检查。

### 2. 真实网络端到端（推荐）

用仓库的冒烟测试宿主，走完整生产路径：

```bash
# 构建：cmake --build build --config Release --target js_plugin_smoke
js_plugin_smoke <plugins_dir> <data_dir> <share_url> [user_token]
```

它会加载插件、路由、调 `ParseUrl`/`EnterDirectory`/`GetDownloadInfo` 并打印结果，
`[js-plugin:my-plugin]` 前缀的日志能看到每步执行情况。

### 3. 在真实 App 里验证

把插件目录放进 `<应用目录>/plugins/`，重启 GDownload，用"添加任务 → 网盘"实测。

---

## 发布到插件市场

插件市场用 **Git 仓库即注册表**（[gdownload-plugin-registry](https://github.com/cool2528/gdownload-plugin-registry)）。

1. 准备好插件目录（`manifest.json` + `main.js` + `lib/`）。
2. 打包并用 Ed25519 私钥签名（私钥不入库，由维护者持有）：
   ```bash
   python scripts/package_and_sign.py <plugin_dir> <version> keys/registry_private.pem --out dist
   ```
   输出一个 version 条目 JSON，并在 `dist/` 生成 `<name>-v<version>.zip`。
3. 把 version 条目并入 `registry.json` 对应插件的 `versions`，更新 `latest`；插件条目补 `locales`。
4. 把 zip 作为 GitHub Release 附件上传（tag = `<name>-v<version>`）。
5. 提 PR；CI 校验 schema、SHA-256、Ed25519 签名后合并。

客户端安装时：按 `download_urls` 多源回退下载（GitHub → jsDelivr → ghproxy）→ **校验 SHA-256 + 签名**
→ 解压 → 热加载。**任一镜像下载都必须通过校验才安装**，故镜像不可信也无妨。

---

## 沙箱限制与常见坑

| 限制 | 值 / 说明 |
| --- | --- |
| 可用 API | 仅 `gdl.*`；无文件系统/进程/原生 socket |
| 网络 | 仅 `permissions.http` 白名单域名；禁止 `*` 全通配 |
| 内存 | 单运行时 64MB |
| 栈 | 1MB |
| 执行超时 | 单次调用 60s（`gdl.utils.sleep` 不计入） |
| 模块 | 仅插件目录内相对 import |
| 存储 | 每插件 1MB |

**常见坑**：

- **url_patterns 匹配不到**：必须能匹配裸主机名（用 `*://host*`，别只写 `/s/*`）。
- **下载没生效/选项丢失**：aria2 选项要放 `ParseResult.headers`（不是 `options`），用 aria2 选项名。
- **`out` 重复**：别自己在 headers 里塞 `out`，宿主会自动补。
- **BAIDUID 类一次性 Cookie 拿不到**：Jar 已存时服务器不重发，改为响应优先、Jar 兜底读取。
- **gzip 报 "incorrect header check"**：一般是中间代理破坏 gzip，宿主已自动降级重试；也可
  手动 `accept_encoding: "disabled"`。
- **PCS 直链等含未编码 `|`**：宿主已容忍（只解析 query 前部分做 host 校验），完整 URL 原样发送。
- **多语言不生效**：`locales` 要同时在 `manifest.json` 与 `registry.json` 插件条目里配。
