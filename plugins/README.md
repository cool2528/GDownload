# GDownload 插件

本目录存放 GDownload 的**内置 JS 插件**。GDownload 的插件用 JavaScript 编写，在内嵌的
QuickJS 沙箱中运行——一份脚本全平台通用、可热更新、只能调用宿主暴露的 `gdl.*` SDK。

> 想写自己的插件？请看 [DEVELOPMENT.md](./DEVELOPMENT.md)（完整开发指南）。

## 目录

| 内容 | 链接 |
| --- | --- |
| 插件是什么、怎么用 | 本文件 |
| 从零开发一个插件 | [DEVELOPMENT.md](./DEVELOPMENT.md) |
| `gdl.*` SDK API 参考 | [DEVELOPMENT.md#gdl-sdk-参考](./DEVELOPMENT.md#gdl-sdk-参考) |
| 发布到插件市场 | [DEVELOPMENT.md#发布到插件市场](./DEVELOPMENT.md#发布到插件市场) |
| 参考实现 | [`baidu-netdisk/`](./baidu-netdisk/) · [`demo-httpbin/`](./demo-httpbin/) |

## 一个插件长什么样

每个插件是 `plugins/` 下的一个子目录：

```
plugins/
  baidu-netdisk/           ← 一个插件
    manifest.json          ← 元数据 + 权限 + URL 匹配规则（必需）
    main.js                ← 入口（默认导出插件对象）
    lib/                   ← 可选：插件自身的 ES 模块
      baidu_api.js
      ...
```

## 用户：怎么用插件

### 从插件市场安装（推荐）

1. 打开 GDownload → **偏好设置 → 插件市场**
2. 浏览可用插件，点 **安装**
3. 安装会：从官方源下载 → **校验 SHA-256 + Ed25519 签名** → 解压到 `plugins/` → 热加载

插件市场支持：安装 / 更新 / 卸载 / 启用禁用 / 搜索。所有插件都经过签名校验，第三方源会有安全提示。

### 手动安装

把插件目录（含 `manifest.json`）直接放进应用目录的 `plugins/` 下，重启 GDownload 即可加载。

### 插件从哪里加载

启动时 GDownload 扫描 `<应用目录>/plugins/` 下**含 `manifest.json`** 的子目录并加载。
被市场"禁用"的插件（记录在应用数据目录的 `plugin_state.json`）会跳过。

插件数据（Cookie、存储）落在应用数据目录：
- `plugin_cookies/<name>.json` —— 每插件独立的 Cookie Jar
- `plugin_storage/<name>.json` —— 每插件独立的键值存储

## 内置插件

| 插件 | 说明 |
| --- | --- |
| [`baidu-netdisk/`](./baidu-netdisk/) | 百度网盘分享链接解析（参考实现，覆盖全部 SDK 能力） |
| [`demo-httpbin/`](./demo-httpbin/) | SDK 验收示例插件（仅供开发验证，不随安装包分发） |

## 快速验证（开发者）

用仓库自带的冒烟测试宿主 `js_plugin_smoke`（`cmake --build build --target js_plugin_smoke` 后于
`build/.../bin/.../js_plugin_smoke.exe`）在真实网络下驱动插件：

```bash
js_plugin_smoke <plugins_dir> <data_dir> <url> [user_token]
```

它走完整生产路径：`LoadJsPlugins → GetPluginsForUrl → ParseUrl / EnterDirectory / GetDownloadInfo`。

也可用 QuickJS 本体（`qjs`）单独校验脚本语法与模块加载：

```bash
qjs -m your-plugin/main.js   # 语法/import 检查（顶层不要访问 gdl.*）
```

## 安全模型

插件运行在沙箱中：
- 只暴露 `gdl.*`，**没有**文件系统、进程、原生网络访问
- 网络请求受 `manifest.permissions.http` 域名白名单限制
- 内存 64MB、栈 1MB、单次调用执行超时 60s
- 每个插件独立的 JS 运行时，插件崩溃不影响宿主
- 市场分发的插件带 Ed25519 签名，校验通过才安装

详见 [DEVELOPMENT.md](./DEVELOPMENT.md)。
