# GDownload JS 插件系统设计文档

> 版本：v2.0（草案）
> 日期：2026-07-18
> 状态：待评审
> 决策：**JS 插件全面替换 C++ 原生插件体系**，不保留双通道

---

## 1. 背景与目标

### 1.1 现状

当前插件系统基于 C++ 原生共享库（`.dll/.so/.dylib`）：

- 接口：`INetDiskDownloadPlugin`（`src/Module/plugin/PluginManager/IDownload_Plugin.h`）
- 加载：`DownloadPluginManager` 扫描应用目录，通过 `PluginLoader`（LoadLibrary/dlopen）加载，SHA-256 校验
- 唯一实现：`Baidu_Plugin`（百度网盘分享链接解析）

**核心问题：**

| 问题 | 说明 |
|------|------|
| 开发门槛高 | 写插件需要 C++ 工具链 + CMake + vcpkg 环境，社区几乎无法贡献 |
| 分发成本高 | 每个平台单独编译二进制，发版流程重 |
| 更新滞后 | 网盘 API 频繁变动，二进制插件无法热更新 |
| 安全风险 | 原生插件拥有完整进程权限，第三方插件不可信 |

### 1.2 决策：全面转向 JS 插件

经评估，网盘解析类插件的工作负载是 **HTTP 请求 + JSON 解析 + 签名计算**，瓶颈在网络 I/O 而非计算，C++ 的性能优势在此场景没有意义。因此决定：

1. **废弃原生插件通道**：移除 `PluginLoader`、C ABI 导出（`CreatePlugin/DestroyPlugin`）、共享库扫描与 SHA-256 白名单机制
2. **废弃 `Baidu_Plugin` C++ 实现**：用 JS 重写为首个**内置插件**，`BaiduPcsApi` 的逻辑（分享解析 → 转存 → 取直链 → 清理）作为翻译蓝本
3. **所有插件（内置 + 社区）统一为 JS 脚本**：一份代码全平台运行、热更新、沙箱运行
4. **`INetDiskDownloadPlugin` 降级为内部抽象**：不再是插件 ABI，仅作为 `JsPluginHost` 与上层（`NetWorkDiskManager`）之间的 C++ 接口，上层代码零改动

### 1.3 目标

1. **一份脚本，全平台运行** —— 插件不再需要编译
2. **降低贡献门槛** —— 会写 JavaScript 即可写插件
3. **热更新** —— 插件即文本文件，可在线拉取更新；网盘 API 变动当天可修复
4. **沙箱安全** —— 插件只能调用宿主暴露的 SDK API，权限声明式管控（比原生插件的"完全信任"模型更安全）
5. **为插件市场打基础** —— manifest 规范 + Git 注册表机制

### 1.4 非目标（本期不做）

- 不支持 JS 插件直接实现下载传输（下载仍统一交给 aria2c）
- 不引入 Node.js/V8 级别的完整运行时（体积与安全不可控）
- 不做旧 .dll 插件的兼容加载（生态中只有官方百度插件，无历史包袱）

---

## 2. 技术选型

### 2.1 脚本引擎：QuickJS（quickjs-ng 分支）✅ 已决策

| 候选 | 结论 | 理由 |
|------|------|------|
| **quickjs-ng** | ✅ 选用 | QuickJS 的社区活跃维护分支（原版 Bellard 更新缓慢）；C 实现、~700KB、完整 ES2023（含 async/await、Promise、正则、BigInt）、可静态链接、vcpkg port 即为 ng 分支、Gopeed 已验证同类方案 |
| LuaJIT | ❌ | 性能好但生态小，网盘解析参考代码（JxPan 等）多为 JS，移植成本高 |
| V8 | ❌ | ~30MB，构建复杂，杀鸡用牛刀 |
| 嵌入 Python | ❌ | 依赖管理复杂、启动慢；未来可作为 yt-dlp 适配层单独考虑 |

### 2.2 依赖引入方式

- vcpkg 添加 `quickjs`（vcpkg 的该 port 对应 quickjs-ng 分支）
- 若 vcpkg port 版本滞后或跨平台编译有问题，备选方案：以子目录源码形式引入 quickjs-ng（仅数个 .c/.h 文件，无外部依赖）

---

## 3. 总体架构

```
┌──────────────────────────────────────────────────────────────┐
│                      GDownload 主程序 (C++)                    │
│                                                              │
│   UI / NetWorkDiskManager / AsyncTaskWorker    （零改动）      │
│                        │                                     │
│                        ▼                                     │
│   ┌──────────────────────────────────────────────────┐       │
│   │   DownloadPluginManager（重构：仅 JS 通道）          │       │
│   │                                                  │       │
│   │   plugins/ 目录扫描 → manifest 校验 → JsPluginHost │       │
│   │                        │                          │       │
│   │                        ▼                          │       │
│   │      INetDiskDownloadPlugin（内部抽象，接口不变）    │       │
│   └──────────────────────────────────────────────────┘       │
│                        │                                     │
│                        ▼                                     │
│   Aria2cDownloadManager ──JSON-RPC──▶ aria2c 进程              │
│                                                              │
│   ┌──────────────────────────────────────────────────┐       │
│   │   JsPluginHost 内部（每个插件一个实例）              │       │
│   │   ┌──────────────┐    ┌─────────────────────┐    │       │
│   │   │ JsRuntime    │    │ Host SDK (gdl.*)     │    │       │
│   │   │ (QuickJS     │◀──▶│ http/crypto/storage  │    │       │
│   │   │  封装)       │    │ /log/ui/utils        │    │       │
│   │   └──────────────┘    └─────────────────────┘    │       │
│   └──────────────────────────────────────────────────┘       │
└──────────────────────────────────────────────────────────────┘
```

### 3.1 关键设计决策

1. **JsPluginHost 实现 `INetDiskDownloadPlugin`**
   每个 JS 插件对应一个 `JsPluginHost` 实例，对上层表现为普通插件。`GetPluginsForUrl`、`GetPluginByName`、验证码回调、消息通知等上层机制全部不变。接口文件中的 C ABI 部分（`CreatePluginFunc/DestroyPluginFunc`）删除。

2. **内置插件与社区插件同构**
   百度网盘等官方插件与社区插件采用完全相同的格式与加载路径，唯一区别：
   - 内置插件随安装包分发（安装到 `<appdir>/plugins/`），带官方签名
   - 内置插件的更新走应用更新或市场推送，均可热替换
   这保证官方插件永远是社区插件的活模板，SDK 能力以官方插件先行验证。

3. **每插件独立 JSRuntime**
   QuickJS 的 `JSRuntime` 非线程安全。每个插件独占一个 Runtime + Context，插件之间完全隔离（内存、全局变量互不可见）。单插件内部用互斥锁串行化调用（现有调用本来就发生在 `AsyncTaskWorker` 单工作线程，锁只是兜底）。

4. **同步接口桥接异步 JS**
   `INetDiskDownloadPlugin` 的方法是同步阻塞语义（在工作线程被调用）。JS 插件方法是 `async` 函数返回 Promise。桥接层实现 `AwaitPromise`：循环执行 QuickJS pending jobs 直至 Promise settle 或超时（默认 60s，可配）。HTTP 请求在宿主侧用 cpr 同步执行，不需要在 JS 侧实现事件循环 I/O。

5. **CanHandle 快速路径**
   URL 匹配优先用 manifest 中的 `url_patterns`（宿主侧通配符匹配），不进入 JS 引擎，避免频繁调用的性能损耗。插件可选实现 `canHandle()` 做二次精确判断。

6. **元数据来自 manifest，而非 JS 调用**
   `GetPluginMetadata()` 直接读 manifest.json，插件未加载/加载失败时也能展示信息（为插件市场 UI 服务）。

---

## 4. 插件包规范

### 4.1 目录结构

```
<appdir>/plugins/               ← 唯一插件目录
  baidu-netdisk/                ← 内置插件（随安装包分发）
    manifest.json
    main.js
    lib/
      pcs_api.js
  quark-netdisk/                ← 市场安装的社区插件
    manifest.json
    main.js
```

### 4.2 manifest.json 规范

```json
{
    "manifest_version": 1,
    "name": "baidu-netdisk",
    "display_name": "Baidu NetDisk",
    "version": "2.0.0",
    "author": "GDownload Official",
    "description": "Parse Baidu netdisk share links",
    "homepage": "https://github.com/xxx/gdownload-plugins",
    "entry": "main.js",
    "type": "netdisk",
    "url_patterns": [
        "*://pan.baidu.com/s/*",
        "*://pan.baidu.com/share/*"
    ],
    "permissions": {
        "http": ["pan.baidu.com", "*.baidupcs.com", "*.baidu.com"],
        "storage": true,
        "verification_ui": true
    },
    "min_app_version": "2.0.0"
}
```

字段说明：

| 字段 | 必填 | 说明 |
|------|------|------|
| `manifest_version` | ✅ | manifest 格式版本，当前为 1 |
| `name` | ✅ | 唯一标识（kebab-case），对应 `PluginMetadata.name` |
| `version` | ✅ | 语义化版本，用于市场更新比对 |
| `entry` | ✅ | 入口脚本相对路径 |
| `type` | ✅ | 插件类型，本期仅 `netdisk`；预留 `resolver`、`adapter` |
| `url_patterns` | ✅ | 通配符 URL 匹配规则（宿主侧快速路由），同时填充 `PluginMetadata.supported_domains` |
| `permissions.http` | ✅ | 允许访问的域名白名单，`gdl.http` 强制校验 |
| `permissions.storage` | — | 是否允许持久化存储（默认 false） |
| `permissions.verification_ui` | — | 是否允许弹出验证码交互（默认 false） |
| `min_app_version` | — | 最低宿主版本，不满足则拒绝加载 |

### 4.3 插件入口约定

入口脚本以 ES Module 形式默认导出插件对象：

```javascript
export default {
    // 可选：manifest url_patterns 命中后二次精确判断
    canHandle(url) {
        return /pan\.baidu\.com\/(s\/|share\/)/.test(url);
    },

    // 对应 ParseUrl：解析分享链接，返回文件列表
    async parseUrl(url, userToken) {
        // 返回 FileInfo[]（结构见 5.2）
    },

    // 对应 EnterDirectory：进入目录
    async enterDirectory(fileInfo) {
        // 返回 FileInfo[]
    },

    // 对应 GetDownloadInfo：解析真实下载地址
    async getDownloadInfo(fileInfo) {
        // 返回 ParseResult[]（结构见 5.2）
    }
};
```

方法与内部 C++ 抽象的映射：

| C++ 接口（INetDiskDownloadPlugin） | JS 插件方法 | 备注 |
|------|------|------|
| `CanHandle(url)` | manifest `url_patterns` → 可选 `canHandle(url)` | 快速路径见 3.1-5 |
| `ParseUrl(url, user_token)` | `async parseUrl(url, userToken)` | 返回 `FileInfo[]` |
| `EnterDirectory(info)` | `async enterDirectory(fileInfo)` | 返回 `FileInfo[]` |
| `GetDownloadInfo(info)` | `async getDownloadInfo(fileInfo)` | 返回 `ParseResult[]` |
| `GetPluginMetadata()` | —（manifest.json 直读） | 不进入 JS |
| `VerificationCallback` | `await gdl.ui.requestVerification(...)` | 见 5.3 |
| `MessageNotifyCallback` | `gdl.notify(message, level)` | 见 5.3 |

---

## 5. Host SDK（`gdl.*` API）规范

宿主向每个 JS Context 注入全局对象 `gdl`，这是插件与外界交互的**唯一通道**。沙箱内不提供文件系统、进程、socket 等任何原生能力。

### 5.1 网络：`gdl.http`

```javascript
// 全部返回 Promise<Response>
const resp = await gdl.http.get(url, options);
const resp = await gdl.http.post(url, options);

// options:
{
    headers: { "Cookie": "...", "User-Agent": "..." },
    params: { key: "value" },          // query 参数
    json: { ... },                     // POST JSON body（自动带 Content-Type: application/json）
    form: { k: "v" },                  // POST application/x-www-form-urlencoded（宿主侧编码）
    multipart: { k: "v" },             // POST multipart/form-data
    body: "raw string",                // POST 原始 body
    timeout: 15000,                    // 毫秒，默认 15000，上限 60000
    follow_redirects: false,           // 默认 true；网盘直链解析常需拿 302 Location
    use_cookie_jar: true,              // 默认 true；见 5.1.1
    accept_encoding: "identity"        // 可选；"" 自动(gzip)、"identity" 明文、"disabled" 关闭解压
}

// body 优先级：json > form > multipart > body（互斥，取先命中者）

// Response:
{
    status: 200,
    headers: { "set-cookie": [...], "location": "..." },   // 键统一小写
    text(): string,
    json(): object                     // 解析失败抛 TypeError
}
```

**安全约束：**
- 请求域名必须命中 manifest `permissions.http` 白名单，否则抛异常
- 宿主侧实现基于 cpr（项目现有依赖），同步执行后 resolve Promise
- 单插件并发请求数限制（默认 4），防滥用

**内容编码容错：** 默认宣告 gzip/deflate 并由 libcurl 自动解压。若因中间代理破坏 gzip 流导致解码失败（`CURLE_BAD_CONTENT_ENCODING`）且插件未显式指定 `accept_encoding`，宿主自动降级为关闭压缩重试一次，插件无需感知。

#### 5.1.1 Cookie Jar 自动管理

每个插件拥有独立的 Cookie Jar，默认启用，语义对齐浏览器标准（RFC 6265）：

- **自动入库**：响应中的 `Set-Cookie` 自动解析入 Jar（尊重 Domain / Path / Expires / Max-Age；会话 Cookie 与持久 Cookie 都支持）
- **自动携带**：发起请求时自动附带匹配域名/路径且未过期的 Cookie
- **显式优先**：请求 `headers` 中显式指定 `Cookie` 时，显式值与 Jar 合并，同名键以显式值为准
- **持久化**：持久 Cookie 自动落盘（应用数据目录 `plugin_cookies/<name>.json`，与 `gdl.storage` 分开存储），重启后仍有效；过期 Cookie 加载时自动清理
- **域名边界**：Jar 只接受/发送 `permissions.http` 白名单内域名的 Cookie，白名单外的 `Set-Cookie` 静默丢弃
- **按请求关闭**：`options.use_cookie_jar: false` 可对单次请求禁用

```javascript
// Cookie 管理 API
gdl.http.cookies.list(domain)                    // → [{name, value, domain, path, expires, secure, http_only}]
gdl.http.cookies.set({ name, value, domain, path, expires })   // 手动注入单条
gdl.http.cookies.setFromString(domain, "k1=v1; k2=v2")         // 批量注入（用户粘贴 Cookie 场景）
gdl.http.cookies.remove(domain, name)
gdl.http.cookies.clear(domain)                   // 省略 domain 清空整个 Jar
```

典型场景：网盘插件引导用户粘贴登录 Cookie → `setFromString` 注入 → 之后所有请求自动携带并跟随 `Set-Cookie` 刷新，插件代码不再手工拼 Cookie 头。

### 5.2 数据结构（与 C++ 结构体对应）

```javascript
// FileInfo —— 对应 INetDiskDownloadPlugin::FileInfo
{
    path: "/share/folder",       // string
    name: "movie.mkv",           // string
    size: 1073741824,            // number（字节）
    is_dir: false,               // boolean
    file_id: "fid_xxx",          // string，插件自定义的定位标识
    create_time: 1710000000,     // number（Unix 秒），可省略
    root_path: "/"               // string，可省略
}

// ParseResult —— 对应 INetDiskDownloadPlugin::ParseResult
{
    real_url: "https://cdn.../file",   // string，必填
    file_name: "movie.mkv",            // string
    file_size: 1073741824,             // number
    headers: {                          // 传给 aria2c 的请求头
        "Cookie": "...",
        "User-Agent": "..."
    },
    options: {                          // aria2c 任务选项
        "split": "4",
        "max-connection-per-server": "4",
        "force-http-range": "true"
    },
    mirrors: ["https://cdn2.../file"]  // 备用地址，可省略
}
```

桥接层负责 JS 对象 ↔ C++ 结构体的双向转换；字段缺失取默认值，类型不匹配记 warning 日志并跳过该字段。
注意：C++ 侧 `headers`/`options` 为 multimap，JS 侧同名多值用数组表达：`{ "header-name": ["v1", "v2"] }`。

### 5.3 交互：`gdl.ui` / `gdl.notify`

```javascript
// 验证码/人机校验 —— 桥接 VerificationCallback
// 需要 permissions.verification_ui
const userInput = await gdl.ui.requestVerification({
    imageBase64: "...",           // 可选，验证码图片
    message: "Enter the code"     // 提示语
});
// 返回用户输入字符串；用户取消则 reject

// 消息通知 —— 桥接 MessageNotifyCallback
gdl.notify("Transfer quota exceeded", "warning");
// level: "success" | "error" | "warning" | "info" | "debug"（对应 MsgType）
```

### 5.4 存储：`gdl.storage`

```javascript
// 需要 permissions.storage；按插件 name 隔离命名空间
gdl.storage.set("cookie", "xxx");        // 值仅支持 string，复杂对象自行 JSON.stringify
const v = gdl.storage.get("cookie");     // 不存在返回 null
gdl.storage.remove("cookie");
```

宿主侧落地到应用数据目录（每插件一个 JSON 文件或 SQLite 表），大小限额 1MB/插件。

### 5.5 工具：`gdl.crypto` / `gdl.utils`

```javascript
gdl.crypto.md5(str)                       // → hex string
gdl.crypto.sha1(str)
gdl.crypto.sha256(str)
gdl.crypto.hmacSha256(key, data)          // → hex string
gdl.crypto.aesCbcDecrypt(keyHex, ivHex, dataBase64)  // → string，视需要提供

gdl.utils.base64Encode(str) / base64Decode(str)
gdl.utils.urlEncode(str) / urlDecode(str)
gdl.utils.sleep(ms)                       // → Promise，上限 10s
```

基于项目现有 OpenSSL 依赖实现。

### 5.6 日志：`gdl.log`

```javascript
gdl.log.debug("...");  gdl.log.info("...");  gdl.log.warn("...");  gdl.log.error("...");
```

输出到 spdlog，统一前缀 `[js-plugin:{name}]`，便于排查。

---

## 6. 安全与资源控制

| 维度 | 机制 |
|------|------|
| 能力沙箱 | 不注入 `std`/`os` 模块（QuickJS-libc 不链接）；仅暴露 `gdl.*`；无文件/进程/网络原生访问 |
| 网络白名单 | `permissions.http` 域名校验，通配符仅允许子域（`*.baidu.com`），禁止 `*` 全通配 |
| 内存限制 | `JS_SetMemoryLimit`：默认 64MB/Runtime |
| CPU 限制 | `JS_SetInterruptHandler`：单次调用执行超时（默认 60s）强制中断 |
| 栈限制 | `JS_SetMaxStackSize`：默认 1MB |
| 完整性 | 内置/市场插件带官方签名；本地手动安装的插件首次加载时提示用户确认 |
| 隔离性 | 每插件独立 Runtime；插件崩溃（JS 异常）不影响宿主，异常统一捕获转为 `std::nullopt` + notify error |

> 相比旧原生插件"加载即完全信任"的模型，JS 沙箱把插件能力压缩到声明的权限范围内，第三方插件的信任成本大幅降低。旧的 SHA-256 白名单/黑名单机制随原生通道一并移除，由 manifest 签名机制替代。

---

## 7. 加载流程与生命周期

```
应用启动 → MainWindow::InitNetDiskPlugins()
  └─ DownloadPluginManager::LoadPlugins(appdir + "/plugins")   （重构后仅此一条路径）
        ├─ 遍历子目录，读取并校验 manifest.json
        │     ├─ manifest_version / min_app_version 校验
        │     ├─ permissions 解析
        │     └─ 签名校验（内置/市场插件）
        ├─ 构造 JsPluginHost（懒初始化：不立即创建 JSRuntime）
        └─ 注册进插件列表

首次实际调用（ParseUrl 等）
  ├─ 创建 JSRuntime + JSContext，注入 gdl.*
  ├─ 加载并 eval 入口 ES Module，取 default export
  └─ 后续调用复用该 Context

插件更新（市场推送 / 应用更新）
  └─ 卸载实例 → 替换 plugins/<name>/ 目录 → 重新加载（热替换，无需重启）

应用退出 / 插件卸载
  └─ JsPluginHost 析构：JS_FreeContext → JS_FreeRuntime
```

懒初始化的目的：装了 20 个插件但只用 2 个时，不为闲置插件付出 Runtime 内存成本。

---

## 8. 旧体系移除与百度插件重写

### 8.1 移除清单

| 移除项 | 说明 |
|------|------|
| `PluginManager/loader/plugin_loader.h/.cxx` | LoadLibrary/dlopen 封装，整体删除 |
| `IDownload_Plugin.h` 中的 C ABI | `CreatePluginFunc` / `DestroyPluginFunc` typedef 删除；接口类本身保留为内部抽象 |
| `DownloadPluginManager::PluginResourceGuard` | 共享库生命周期管理，由 JsPluginHost 取代 |
| SHA-256 白名单/黑名单机制（`LoadPluginOptions`） | 由 manifest 签名机制取代 |
| `Baidu_Plugin/` 整个 C++ 模块 | 逻辑翻译为 JS 后删除源码与构建目标 |
| 共享库扫描逻辑（`.dll/.so/.dylib` + 文件名含 "Plugin"） | 改为 plugins/ 目录 + manifest.json 扫描 |

### 8.2 百度插件 JS 重写

`BaiduPcsApi`（`Baidu_Plugin/baiduApi/baidu_pcs_api.cxx`）是重写蓝本，核心流程逐一翻译：

| C++ 现有逻辑 | JS 对应实现 |
|------|------|
| cpr HTTP 请求（带 Cookie/UA） | `gdl.http.get/post` |
| boost::url 拼接参数 | `gdl.utils.urlEncode` + 模板字符串 |
| nlohmann::json 解析 | 原生 `JSON.parse` / `resp.json()` |
| OpenSSL 签名计算 | `gdl.crypto.*` |
| 验证码回调 | `await gdl.ui.requestVerification(...)` |
| 转存→取直链→清理 流程 | `async` 函数顺序编排，逻辑不变 |

重写完成的 `plugins/baidu-netdisk/` 同时承担三个角色：
1. JS SDK 的验收用例（覆盖 http/crypto/storage/ui 全部 API）
2. 社区插件开发的官方参考模板
3. 回归基准——重写版必须通过与 C++ 版相同的手工测试用例（分享链解析、目录浏览、单文件/多文件下载、验证码流程）后，才删除 C++ 版源码

---

## 9. 插件市场（Phase 3 概要设计）

### 9.1 注册表

采用 **Git 仓库即注册表**（参考 Gopeed / Claude Code marketplace 模式）：

```
gdownload-plugin-registry/          ← 官方 GitHub 仓库
  registry.json                     ← 索引：所有插件的元数据 + 下载地址 + 签名
  plugins/
    quark-netdisk/  → 指向独立仓库 release 的 zip 包
```

`registry.json` 条目：

```json
{
    "name": "quark-netdisk",
    "version": "1.2.0",
    "description": "...",
    "download_url": "https://github.com/.../releases/download/v1.2.0/quark-netdisk.zip",
    "sha256": "...",
    "signature": "...",
    "min_app_version": "2.0.0",
    "verified": true
}
```

### 9.2 客户端功能

- 设置页新增「插件市场」：浏览 / 搜索 / 安装 / 更新 / 卸载 / 启用禁用
- 安装 = 下载 zip → 校验 sha256 + 签名 → 解压到 `plugins/<name>/` → 热加载
- 支持添加第三方注册表 URL（默认仅官方源，添加第三方源时给出安全警告）
- 更新检查：启动时/手动比对 registry 版本号

### 9.3 国内镜像加速

GitHub 在国内访问不稳定，registry 与插件包下载均需多源回退：

- `registry.json` 中每个插件的下载地址扩展为 `download_urls` 数组（主源 + 镜像源）
- 内置镜像源链：GitHub Releases（主）→ jsDelivr（`cdn.jsdelivr.net/gh/...`）→ ghproxy 类代理前缀
- registry 索引本身同样多源：GitHub Raw → jsDelivr → 自建 CDN（可选）
- 客户端策略：按顺序尝试，单源超时 10s 自动切换；成功源记忆并优先复用；设置页允许用户自定义镜像前缀
- 完整性保障：无论从哪个镜像下载，都必须通过 sha256 + 签名校验后才安装（镜像不可信也无妨）

### 9.4 未来扩展的插件类型（预留）

| type | 场景 | 说明 |
|------|------|------|
| `netdisk` | 网盘解析 | 本期实现 |
| `resolver` | 通用链接解析 | 视频页→直链（配合外部工具） |
| `adapter` | 外部工具适配 | 声明式封装 yt-dlp/BBDown/lux 等 CLI（manifest 描述命令行与输出解析规则） |

### 9.5 已确认决策（2026-07-18 评审）

| 决策项 | 结论 |
|------|------|
| 脚本引擎 | quickjs-ng（vcpkg port） |
| 内置插件混淆 | 不混淆，内置插件即官方开源模板 |
| 签名算法 | Ed25519，复用 release 签名密钥基础设施 |
| Cookie Jar | Phase 1 完整实现（见 5.1.1），含持久化与管理 API |
| 国内镜像 | Phase 4 实现多源回退（见 9.3） |

---

## 10. 风险与对策

| 风险 | 影响 | 对策 |
|------|------|------|
| QuickJS vcpkg port 跨平台编译问题 | 阻塞集成 | 备选：源码子目录引入，QuickJS 无外部依赖，编译极简单 |
| 同步桥接死锁（JS 内 await 一个永不 settle 的 Promise） | 工作线程卡死 | AwaitPromise 强制超时 + InterruptHandler 兜底 |
| JS 版百度插件行为回退（重写引入 bug） | 唯一现有功能受损 | 8.2 的回归基准：JS 版通过全部现有测试用例后才删 C++ 版；开发期两版并存于分支 |
| 网盘 API 变动导致插件失效 | 功能不可用 | 这正是 JS 热更新要解决的问题；市场推送更新 |
| 恶意插件（第三方源） | 用户 Cookie/Token 泄露 | http 域名白名单 + 权限声明 + 官方源签名审核 + 第三方源警告 |
| `AwaitPromise` 期间宿主回调重入（验证码 UI） | 死锁/竞态 | 验证码回调本身已是跨线程异步模式（现有 `VerificationCallbackParam` 机制），JS 侧 Promise 挂起等待，不阻塞 UI 线程 |
| multimap 语义在 JS 对象上的表达 | 头部丢失 | 约定数组值表达多值（见 5.2），转换层做好双向映射 |

---

## 11. 与现有代码的接触面清单

| 现有代码 | 改动 |
|------|------|
| `IDownload_Plugin.h` | 保留接口类（内部抽象）；删除 C ABI typedef |
| `plugin_manager.h/.cxx` | **重构**：删除共享库加载路径，改为 manifest 扫描 + JsPluginHost 构造；`GetPluginsForUrl` / `GetPluginByName` 对外签名不变 |
| `PluginManager/loader/` | **删除** |
| `Baidu_Plugin/` | JS 重写通过回归后**删除**（含 CMake 目标） |
| `mainwindow.cxx::InitNetDiskPlugins` | 加载路径参数改为 `plugins/` 子目录 |
| `NetWork_Disk_magager.cxx` | **不改** |
| `vcpkg.json` | 新增 `quickjs` 依赖 |
| `src/Module/plugin/` | 新增 `JsPluginRuntime/` 子模块（详见实施文档） |
| 安装包脚本（NSIS/DMG/AppImage） | 打包 `plugins/baidu-netdisk/`（JS 内置插件） |

---

*文档更新：2026-07-18 v2.1 —— 评审决策落地：quickjs-ng、Ed25519 签名、Cookie Jar 完整规范（5.1.1）、市场国内镜像方案（9.3）；v2.0 按"全面替换"决策重写；v1.0 双通道方案作废*
