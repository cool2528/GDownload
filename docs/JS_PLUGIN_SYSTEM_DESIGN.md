# GDownload JS 插件系统设计

> 文档版本：v2.1
> 
> 更新日期：2026-07-18
> 
> 配套文档：[JS 插件系统实施文档](JS_PLUGIN_SYSTEM_IMPLEMENTATION_PLAN.md)

本文说明 GDownload 为什么要将网盘插件从 C++ 原生模块迁移到 JavaScript，以及新插件系统的运行方式、接口约定、安全边界和迁移计划。

这次改造的重点并不只是更换脚本语言。更重要的是让插件能够跨平台分发、独立更新，并在明确的权限范围内运行。迁移完成后，应用只保留 JS 插件通道，不再兼容旧的原生插件格式。

---

## 1. 为什么要重做插件系统

### 1.1 现状与问题

当前插件以 C++ 共享库（`.dll`、`.so` 或 `.dylib`）的形式运行：

- 插件接口为 `INetDiskDownloadPlugin`，定义在 `src/Module/plugin/PluginManager/IDownload_Plugin.h`。
- `DownloadPluginManager` 扫描应用目录，再通过 `PluginLoader`（`LoadLibrary`/`dlopen`）加载插件并进行 SHA-256 校验。
- 仓库中目前只有一个实际实现：负责解析百度网盘分享链接的 `Baidu_Plugin`。

这套方案可以工作，但不适合继续扩展网盘插件生态：

| 问题     | 说明                                              |
| ------ | ----------------------------------------------- |
| 开发门槛高  | 开发者需要准备 C++ 工具链、CMake 和 vcpkg 环境，普通脚本作者很难参与     |
| 分发成本高  | Windows、macOS 和 Linux 都要分别构建和发布二进制包             |
| 更新不够及时 | 网盘接口经常变化，而二进制插件通常只能跟随应用版本更新                     |
| 权限过大   | 原生插件与主程序运行在同一进程，能够访问完整的进程权限，不适合直接运行未经充分审核的第三方代码 |

### 1.2 方案结论

网盘解析插件的大部分工作是发送 HTTP 请求、处理 JSON 和计算签名，耗时主要来自网络 I/O。与原生代码相比，JavaScript 在这个场景中的性能差异不会成为主要瓶颈，却能明显降低开发和发布成本。

因此，新系统采用以下方案：

1. 迁移完成后移除原生插件加载通道，包括 `PluginLoader`、C ABI 导出（`CreatePlugin`/`DestroyPlugin`）、共享库扫描和原有的 SHA-256 白名单机制。
2. 将 `Baidu_Plugin` 重写为首个内置 JS 插件。现有 `BaiduPcsApi` 中“解析分享链接、转存文件、获取直链、清理临时文件”的流程作为迁移基准。
3. 内置插件和社区插件使用同一种包格式、同一套 SDK 和同一条加载路径。
4. 保留 `INetDiskDownloadPlugin` 作为程序内部接口，由 `JsPluginHost` 负责适配，从而尽量减少 `NetWorkDiskManager` 等上层模块的改动。

### 1.3 设计目标

新系统需要达到以下目标：

1. 插件脚本无需编译，同一份代码可以在所有支持的平台上运行。
2. 插件开发只要求 JavaScript 基础，不再强制开发者搭建完整的 C++ 编译环境。
3. 插件可以独立于应用版本更新，以便及时跟进第三方网盘接口的变化。
4. 插件只能使用宿主明确提供的 API，网络、存储和交互能力均由 manifest 声明并由宿主校验。
5. 插件包格式和元数据能够直接用于后续的插件市场、版本检查和签名验证。

### 1.4 本期不包含的内容

- JS 插件不负责实际的数据传输，下载任务仍统一交给 aria2c。
- 不引入 Node.js 或 V8 这类完整运行时，以免显著增加安装体积和攻击面。
- 不提供旧 `.dll` 插件的兼容层。当前只有官方百度插件需要迁移，没有第三方二进制插件的兼容需求。

---

## 2. 技术选择

### 2.1 脚本引擎

脚本引擎选用 **quickjs-ng**。它是 QuickJS 的活跃维护分支，同时保持了较小的体积和较简单的嵌入方式。

| 候选             | 结论  | 主要考量                                                                                                                        |
| -------------- | --- | --------------------------------------------------------------------------------------------------------------------------- |
| **quickjs-ng** | 采用  | C 语言实现，体积约 700 KB，可静态链接；支持 ES2023，包括 `async`/`await`、Promise、正则和 BigInt；vcpkg 中的 `quickjs` port 已指向该分支，Gopeed 等同类项目也验证过这一方案 |
| LuaJIT         | 不采用 | 运行性能较好，但网盘解析相关的现有参考实现大多使用 JavaScript，迁移到 Lua 的成本更高                                                                          |
| V8             | 不采用 | 运行时和构建体系都明显重于当前需求，会增加约数十 MB 的体积                                                                                             |
| 嵌入 Python      | 不采用 | 依赖管理和启动成本较高；如果将来需要集成 yt-dlp，可单独设计外部工具适配层                                                                                    |

### 2.2 依赖引入方式

首选方案是在 `vcpkg.json` 中添加 `quickjs` 依赖。该 port 当前对应 quickjs-ng 分支，可以直接接入现有构建流程。

如果 port 版本滞后，或在某个平台上无法稳定构建，则改为将 quickjs-ng 源码作为子目录引入。它只有少量 `.c` 和 `.h` 文件，也没有额外的第三方依赖，因此这一备选方案仍然可控。

---

## 3. 总体架构

对上层业务而言，插件仍表现为 `INetDiskDownloadPlugin`。变化主要发生在 `DownloadPluginManager` 内部：管理器不再加载共享库，而是读取 manifest，并为每个插件创建一个 `JsPluginHost`。

```
┌──────────────────────────────────────────────────────────────┐
│                      GDownload 主程序 (C++)                    │
│                                                              │
│   UI / NetWorkDiskManager / AsyncTaskWorker   （接口保持稳定） │
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

### 3.1 关键设计说明

1. **保留现有 C++ 抽象。** 每个 JS 插件对应一个 `JsPluginHost` 实例，并继续以 `INetDiskDownloadPlugin` 的形式提供给上层。`GetPluginsForUrl`、`GetPluginByName`、验证码回调和消息通知等调用方式保持不变；接口文件中只删除不再需要的 C ABI 定义（`CreatePluginFunc`/`DestroyPluginFunc`）。

2. **内置插件与社区插件使用同一种格式。** 百度网盘等官方插件和社区插件都安装到 `<appdir>/plugins/`，使用相同的 manifest、SDK 和加载流程。内置插件随安装包分发并带有官方签名，后续可以通过应用更新或插件市场进行替换。官方插件同时作为 SDK 的参考实现和回归样例。

3. **每个插件使用独立的 Runtime。** QuickJS 的 `JSRuntime` 不是线程安全的，因此每个插件独占一组 Runtime 和 Context。不同插件之间不共享内存或全局变量。单个插件的调用再通过互斥锁串行化；当前调用主要发生在 `AsyncTaskWorker` 的工作线程中，这把锁用于防止未来出现并发入口。

4. **由桥接层衔接同步 C++ 接口与异步 JS。** `INetDiskDownloadPlugin` 在工作线程中以同步方式调用，而 JS 插件方法返回 Promise。`AwaitPromise` 会持续执行 QuickJS 的 pending jobs，直到 Promise 完成、失败或超时。默认超时时间为 60 秒，并允许配置。HTTP 请求由宿主通过 cpr 执行，JS 侧不需要维护独立的 I/O 事件循环。

5. **URL 匹配走快速路径。** 宿主先使用 manifest 中的 `url_patterns` 做通配符匹配，不必为每次判断都进入 JS 引擎。插件如果需要更精确的规则，可以额外实现 `canHandle()`。

6. **插件元数据以 manifest 为准。** `GetPluginMetadata()` 直接读取 `manifest.json`。即使入口脚本尚未初始化或加载失败，应用仍然可以展示插件名称、版本和错误状态，这也便于后续实现插件市场界面。

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

| 字段                            | 必填  | 说明                                                            |
| ----------------------------- | --- | ------------------------------------------------------------- |
| `manifest_version`            | ✅   | manifest 格式版本，当前为 1                                           |
| `name`                        | ✅   | 唯一标识（kebab-case），对应 `PluginMetadata.name`                     |
| `display_name`                | —   | 面向用户显示的名称                                                     |
| `version`                     | ✅   | 语义化版本，用于市场更新比对                                                |
| `author`                      | —   | 插件作者或维护组织                                                     |
| `description`                 | —   | 插件简介                                                          |
| `homepage`                    | —   | 项目主页或源码仓库地址                                                   |
| `entry`                       | ✅   | 入口脚本相对路径                                                      |
| `type`                        | ✅   | 插件类型，本期仅 `netdisk`；预留 `resolver`、`adapter`                    |
| `url_patterns`                | ✅   | 通配符 URL 匹配规则（宿主侧快速路由），同时填充 `PluginMetadata.supported_domains` |
| `permissions.http`            | ✅   | 允许访问的域名白名单，`gdl.http` 强制校验                                    |
| `permissions.storage`         | —   | 是否允许持久化存储（默认 false）                                           |
| `permissions.verification_ui` | —   | 是否允许弹出验证码交互（默认 false）                                         |
| `min_app_version`             | —   | 最低宿主版本，不满足则拒绝加载                                               |

### 4.3 插件入口约定

入口文件使用 ES Module，并通过默认导出提供插件对象。下面的示例展示了网盘插件需要实现的最小接口：

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

| C++ 接口（INetDiskDownloadPlugin） | JS 插件方法                                       | 备注                 |
| ------------------------------ | --------------------------------------------- | ------------------ |
| `CanHandle(url)`               | manifest `url_patterns` → 可选 `canHandle(url)` | 先走宿主快速匹配，再由插件做精确判断 |
| `ParseUrl(url, user_token)`    | `async parseUrl(url, userToken)`              | 返回 `FileInfo[]`    |
| `EnterDirectory(info)`         | `async enterDirectory(fileInfo)`              | 返回 `FileInfo[]`    |
| `GetDownloadInfo(info)`        | `async getDownloadInfo(fileInfo)`             | 返回 `ParseResult[]` |
| `GetPluginMetadata()`          | —（manifest.json 直读）                           | 不进入 JS             |
| `VerificationCallback`         | `await gdl.ui.requestVerification(...)`       | 见 5.3              |
| `MessageNotifyCallback`        | `gdl.notify(message, level)`                  | 见 5.3              |

---

## 5. 宿主 API（`gdl.*`）

宿主会向每个 JS Context 注入全局对象 `gdl`。这是插件访问网络、存储和界面能力的唯一入口；沙箱中不提供文件系统、进程或 socket 等原生能力。

### 5.1 网络请求：`gdl.http`

`gdl.http` 提供常用的 GET 和 POST 请求，所有方法都返回 `Promise<Response>`。请求参数由宿主统一编码，并在发送前检查 manifest 中声明的域名权限。

```javascript
// 全部返回 Promise<Response>
const getResponse = await gdl.http.get(url, options);
const postResponse = await gdl.http.post(url, options);

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

请求需要满足以下限制：

- 目标域名必须命中 manifest 的 `permissions.http` 白名单，否则请求会抛出异常。
- 宿主基于项目现有的 cpr 依赖执行同步请求，再将结果 resolve 为 Promise。
- 单个插件的并发请求数默认限制为 4，避免插件意外或恶意地占用过多连接。

默认请求会声明支持 gzip/deflate，并由 libcurl 自动解压。如果中间代理破坏了压缩流，导致 `CURLE_BAD_CONTENT_ENCODING`，且插件没有显式设置 `accept_encoding`，宿主会关闭压缩并自动重试一次。插件不需要为这种情况编写额外的兼容逻辑。

#### 5.1.1 Cookie Jar 自动管理

每个插件拥有独立的 Cookie Jar，默认启用，行为遵循浏览器常用的 RFC 6265 规则：

- 响应中的 `Set-Cookie` 会自动解析并存入 Jar，同时遵守 Domain、Path、Expires 和 Max-Age。
- 发起请求时，宿主会自动附带匹配域名和路径、且尚未过期的 Cookie。
- 如果请求显式提供了 `Cookie` 头，宿主会将它与 Jar 合并；同名键以显式值为准。
- 持久 Cookie 会保存到应用数据目录中的 `plugin_cookies/<name>.json`，与 `gdl.storage` 分开管理。应用重启后仍可继续使用，加载时会清理已经过期的 Cookie。
- Jar 只接受和发送 `permissions.http` 白名单内域名的 Cookie。白名单外的 `Set-Cookie` 会被丢弃。
- 将 `options.use_cookie_jar` 设为 `false`，可以只对当前请求关闭 Cookie Jar。

```javascript
// Cookie 管理 API
gdl.http.cookies.list(domain)                    // → [{name, value, domain, path, expires, secure, http_only}]
gdl.http.cookies.set({ name, value, domain, path, expires })   // 手动注入单条
gdl.http.cookies.setFromString(domain, "k1=v1; k2=v2")         // 批量注入（用户粘贴 Cookie 场景）
gdl.http.cookies.remove(domain, name)
gdl.http.cookies.clear(domain)                   // 省略 domain 清空整个 Jar
```

例如，网盘插件可以让用户粘贴登录 Cookie，然后调用 `setFromString` 注入。之后的请求会自动携带这些 Cookie，并根据服务端返回的 `Set-Cookie` 进行更新，插件不需要手工拼接请求头。

### 5.2 数据结构

下面两个对象分别对应 C++ 侧的 `FileInfo` 和 `ParseResult`。桥接层负责在 JS 对象与 C++ 结构体之间进行转换。

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

字段缺失时使用默认值；如果字段类型不匹配，桥接层记录 warning 并跳过该字段。C++ 侧的 `headers` 和 `options` 是 multimap，因此 JS 侧用数组表示同名的多个值，例如 `{ "header-name": ["v1", "v2"] }`。

### 5.3 用户交互：`gdl.ui` / `gdl.notify`

验证码和消息通知都通过宿主回调完成。插件不能直接创建窗口，也不能访问 Qt UI 对象。

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

### 5.4 持久化存储：`gdl.storage`

`gdl.storage` 为每个插件提供隔离的键值存储。使用前必须在 manifest 中声明 `permissions.storage`，单个插件的总容量上限为 1 MB。

```javascript
// 需要 permissions.storage；按插件 name 隔离命名空间
gdl.storage.set("cookie", "xxx");        // 值仅支持 string，复杂对象自行 JSON.stringify
const v = gdl.storage.get("cookie");     // 不存在返回 null
gdl.storage.remove("cookie");
```

宿主会将数据保存到应用数据目录，可以使用每插件一个 JSON 文件或 SQLite 表实现。插件之间的命名空间互不相通。

### 5.5 加密与通用工具：`gdl.crypto` / `gdl.utils`

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

加密函数基于项目现有的 OpenSSL 依赖实现；`sleep` 由宿主调度，并限制最长等待时间为 10 秒。

### 5.6 日志：`gdl.log`

```javascript
gdl.log.debug("...");  gdl.log.info("...");  gdl.log.warn("...");  gdl.log.error("...");
```

日志最终写入 spdlog，并统一使用 `[js-plugin:{name}]` 前缀，方便从主程序日志中定位具体插件。

---

## 6. 安全与资源控制

插件代码运行在受限的 QuickJS 环境中。宿主不仅控制插件能调用哪些 API，也限制单次调用可以消耗的内存、栈和 CPU 时间。

| 维度     | 机制                                                                   |
| ------ | -------------------------------------------------------------------- |
| 能力沙箱   | 不链接 QuickJS-libc，也不注入 `std`/`os` 模块；只暴露 `gdl.*`，不提供文件、进程或原生网络访问      |
| 网络白名单  | 校验 `permissions.http`；通配符只允许匹配子域（例如 `*.baidu.com`），不允许使用 `*` 全量放行    |
| 内存限制   | `JS_SetMemoryLimit`，默认每个 Runtime 限制为 64 MB                           |
| CPU 限制 | `JS_SetInterruptHandler`，单次调用默认在 60 秒后中断                             |
| 栈限制    | `JS_SetMaxStackSize`，默认限制为 1 MB                                      |
| 完整性    | 内置插件和市场插件使用官方签名；本地手动安装的插件首次加载时提示用户确认                                 |
| 隔离性    | 每个插件使用独立 Runtime。JS 异常会在宿主侧统一捕获，转换为 `std::nullopt` 并发送错误通知，不会直接影响主程序 |

与“加载即完全信任”的原生插件不同，JS 插件只能使用 manifest 声明且宿主实际提供的能力。原有的 SHA-256 白名单/黑名单会随原生通道一起移除，完整性校验改由 manifest 签名机制负责。

---

## 7. 加载流程与生命周期

插件的生命周期分为三个阶段：应用启动时发现并校验插件，首次使用时初始化脚本运行时，更新或退出时释放运行时资源。具体流程如下：

```
应用启动 → MainWindow::InitNetDiskPlugins()
  └─ DownloadPluginManager::LoadPlugins(appdir + "/plugins")   （迁移后唯一的加载路径）
        ├─ 遍历子目录，读取并校验 manifest.json
        │     ├─ manifest_version / min_app_version 校验
        │     ├─ permissions 解析
        │     └─ 签名校验（内置/市场插件）
        ├─ 构造 JsPluginHost（懒初始化：不立即创建 JSRuntime）
        └─ 注册进插件列表

首次实际调用（例如 ParseUrl）
  ├─ 创建 JSRuntime + JSContext，注入 gdl.*
  ├─ 加载并 eval 入口 ES Module，取 default export
  └─ 后续调用复用该 Context

插件更新（市场推送或应用更新）
  └─ 卸载实例 → 替换 plugins/<name>/ 目录 → 重新加载（热替换，无需重启）

应用退出或插件卸载
  └─ JsPluginHost 析构：JS_FreeContext → JS_FreeRuntime
```

采用懒初始化后，即使安装了 20 个插件，应用也只会为实际使用到的插件创建 Runtime，从而避免为闲置插件预先占用内存。

---

## 8. 迁移旧体系并重写百度插件

### 8.1 移除清单

| 移除项                                          | 说明                                                              |
| -------------------------------------------- | --------------------------------------------------------------- |
| `PluginManager/loader/plugin_loader.h/.cxx`  | 删除 `LoadLibrary`/`dlopen` 封装                                    |
| `IDownload_Plugin.h` 中的 C ABI                | 删除 `CreatePluginFunc` / `DestroyPluginFunc` typedef，保留接口类作为内部抽象 |
| `DownloadPluginManager::PluginResourceGuard` | 删除共享库生命周期管理，改由 `JsPluginHost` 管理脚本运行时                           |
| SHA-256 白名单/黑名单机制（`LoadPluginOptions`）       | 改为 manifest 签名校验                                                |
| `Baidu_Plugin/` C++ 模块                       | JS 版本通过回归验证后，删除源码和构建目标                                          |
| 共享库扫描逻辑（`.dll`/`.so`/`.dylib`）               | 改为扫描 `plugins/` 下的 `manifest.json`                              |

### 8.2 百度插件 JS 重写

`BaiduPcsApi`（`Baidu_Plugin/baiduApi/baidu_pcs_api.cxx`）作为迁移基准。重写时保持业务流程不变，只替换实现所依赖的 API：

| C++ 现有逻辑                 | JS 对应实现                                 |
| ------------------------ | --------------------------------------- |
| cpr HTTP 请求（带 Cookie/UA） | `gdl.http.get/post`                     |
| boost::url 拼接参数          | `gdl.utils.urlEncode` + 模板字符串           |
| nlohmann::json 解析        | 原生 `JSON.parse` / `resp.json()`         |
| OpenSSL 签名计算             | `gdl.crypto.*`                          |
| 验证码回调                    | `await gdl.ui.requestVerification(...)` |
| 转存→取直链→清理 流程             | `async` 函数顺序编排，逻辑不变                     |

完成后的 `plugins/baidu-netdisk/` 同时承担三项职责：

1. 作为 JS SDK 的验收样例，覆盖 `http`、`crypto`、`storage` 和 `ui` 等主要 API。
2. 作为社区插件开发时可以直接参考的官方模板。
3. 作为回归基准。JS 版本必须先通过与 C++ 版本相同的手工用例（分享链接解析、目录浏览、单文件和多文件下载、验证码流程），再删除 C++ 版本源码。

---

## 9. 插件市场（Phase 4 概要）

### 9.1 注册表

插件市场采用“Git 仓库即注册表”的方式。官方维护一个 GitHub 仓库，仓库中的索引文件记录插件元数据、下载地址和签名信息：

```
gdownload-plugin-registry/          ← 官方 GitHub 仓库
  registry.json                     ← 索引：所有插件的元数据 + 下载地址 + 签名
  plugins/
    quark-netdisk/  → 指向独立仓库 release 的 zip 包
```

`registry.json` 中每个插件可以包含多个版本。每个版本都带有校验信息和一组下载地址，客户端选择版本后再按顺序尝试这些地址：

```json
{
    "name": "quark-netdisk",
    "description": "...",
    "latest": "1.2.0",
    "verified": true,
    "versions": [
        {
            "version": "1.2.0",
            "size": 12345,
            "sha256": "...",
            "signature": "...",
            "download_urls": [
                "https://github.com/.../releases/download/v1.2.0/quark-netdisk.zip",
                "https://cdn.jsdelivr.net/gh/.../quark-netdisk-v1.2.0.zip"
            ],
            "min_app_version": "2.0.0"
        }
    ]
}
```

### 9.2 客户端功能

客户端需要提供以下能力：

- 在设置页浏览、搜索、安装、更新、卸载插件，并控制插件启用状态。
- 安装插件时依次下载 zip、校验 SHA-256 和签名、解压到 `plugins/<name>/`，然后热加载插件。
- 允许用户添加第三方注册表 URL。默认只使用官方源，启用第三方源前显示安全提示。
- 在应用启动时或用户手动触发时，比较本地版本与注册表中的版本。

### 9.3 国内镜像加速

考虑到 GitHub 在部分网络环境下访问不稳定，注册表和插件包都需要支持多源回退：

- `registry.json` 中每个插件的下载地址使用 `download_urls` 数组，同时记录主源和镜像源。
- 默认源顺序为 GitHub Releases、jsDelivr（`cdn.jsdelivr.net/gh/...`）以及可选的自建 CDN 或代理。
- 注册表索引本身也支持从 GitHub Raw、jsDelivr 和自建 CDN 获取。
- 客户端按顺序尝试各个源，单个源超时 10 秒后切换；成功的源可以记录下来，并在下次请求时优先使用。设置页允许用户配置自定义镜像前缀。
- 无论文件来自哪个镜像，安装前都必须通过 SHA-256 和签名校验。镜像只负责传输，不改变信任边界。

### 9.4 未来扩展的插件类型（预留）

| type       | 场景     | 说明                                                   |
| ---------- | ------ | ---------------------------------------------------- |
| `netdisk`  | 网盘解析   | 本期实现                                                 |
| `resolver` | 通用链接解析 | 视频页→直链（配合外部工具）                                       |
| `adapter`  | 外部工具适配 | 声明式封装 yt-dlp/BBDown/lux 等 CLI（manifest 描述命令行与输出解析规则） |

### 9.5 当前已确认的决策（截至 2026-07-18）

| 决策项        | 结论                                   |
| ---------- | ------------------------------------ |
| 脚本引擎       | quickjs-ng（vcpkg port）               |
| 内置插件混淆     | 不混淆，内置插件即官方开源模板                      |
| 签名算法       | Ed25519，复用 release 签名密钥基础设施          |
| Cookie Jar | 在 Phase 1 完整实现（见 5.1.1），包括持久化和管理 API |
| 国内镜像       | 在 Phase 4 实现多源回退（见 9.3）              |

---

## 10. 风险与对策

| 风险                              | 影响                   | 对策                                                                  |
| ------------------------------- | -------------------- | ------------------------------------------------------------------- |
| QuickJS vcpkg port 跨平台编译问题      | 集成被阻塞                | 优先切换到源码子目录方案；QuickJS 没有额外的第三方依赖，备选构建路径相对简单                          |
| 同步桥接死锁（JS 中等待永不完成的 Promise）     | 工作线程卡住               | `AwaitPromise` 设置强制超时，并由 `InterruptHandler` 作为最后一道保护                |
| JS 版百度插件出现行为回退                  | 现有网盘功能受影响            | 以 8.2 节的回归用例为准，全部通过后再删除 C++ 版本；开发阶段允许两版并行对照                         |
| 网盘 API 发生变化                     | 插件暂时无法解析链接           | 通过插件市场发布修复版本，避免等待主程序发版                                              |
| 第三方源提供恶意插件                      | 用户 Cookie 或 Token 泄露 | 域名白名单、权限声明、官方源签名审核，以及安装第三方源插件时的安全提示                                 |
| `AwaitPromise` 期间宿主回调重入（验证码 UI） | 死锁或竞态                | 沿用现有 `VerificationCallbackParam` 的跨线程异步机制；JS Promise 等待结果时不阻塞 UI 线程 |
| multimap 在 JS 对象中的表达不明确         | 请求头或任务选项丢失           | 统一使用数组表示同名多值，并在转换层进行双向校验（见 5.2）                                     |

---

## 11. 对现有代码的影响

| 现有代码                                 | 改动                                                                                          |
| ------------------------------------ | ------------------------------------------------------------------------------------------- |
| `IDownload_Plugin.h`                 | 保留接口类作为内部抽象，删除 C ABI typedef                                                                |
| `plugin_manager.h/.cxx`              | 删除共享库加载路径，改为扫描 manifest 并构造 `JsPluginHost`；`GetPluginsForUrl` 和 `GetPluginByName` 的对外签名保持不变 |
| `PluginManager/loader/`              | 删除整个加载器目录                                                                                   |
| `Baidu_Plugin/`                      | JS 版本通过回归验证后删除源码和 CMake 目标                                                                  |
| `mainwindow.cxx::InitNetDiskPlugins` | 将插件目录改为 `plugins/` 子目录                                                                      |
| `NetWork_Disk_magager.cxx`           | 不需要改动                                                                                       |
| `vcpkg.json`                         | 新增 `quickjs` 依赖                                                                             |
| `src/Module/plugin/`                 | 新增 `JsPluginRuntime/` 子模块，具体任务见实施文档                                                         |
| 安装包脚本（NSIS/DMG/AppImage）             | 将 `plugins/baidu-netdisk/` 作为内置 JS 插件打包                                                     |


