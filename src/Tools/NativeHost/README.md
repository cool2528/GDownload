# GDownload Native Messaging Host

浏览器扩展与 GDownload 桌面端的原生消息桥（`com.gdownload.host`）。

## 职责

- 读取 GDownload 配置文件（`<AppData>/gdownload/gd.toml`）中的 `aria2c.rpc-listen-port`
  与 `aria2c.rpc-secret`，通过握手返回给浏览器扩展，实现**零配置自动配对**。
- 探测 GDownload 是否运行（连接本地 aria2 RPC 端口）。
- 按扩展请求唤起 GDownload 主程序。
- **不做数据转发**：下载任务下发仍由扩展直连 aria2 WebSocket RPC 承载。

## 协议（stdio，与扩展 `browser-extension/src/background/nativeBridge.ts` 对齐）

4 字节小端长度前缀 + UTF-8 JSON。

| 方向 | 消息 |
|------|------|
| 扩展 → host | `{type:"handshake",extVersion}` / `{type:"launch"}` / `{type:"ping"}` |
| host → 扩展 | `{type:"handshakeAck",hostVersion,appRunning,rpcPort,rpcSecret,appVersion}` |
|            | `{type:"launchResult",ok}` / `{type:"pong"}` |

`parseShare` 转发（网盘解析）目前返回 `not_implemented`，待主程序单实例 IPC 接入后补齐。

## 依赖

仅 header-only 的 `nlohmann-json` 与 `toml++`（均在 vcpkg 中）。**刻意不链接 GDLCore**，
避免引入 boost/sqlite/openssl 等重型依赖，保持 host 体积极小、启动极快。

## host manifest 注册（安装期完成，尚未实现）

`com.gdownload.host.{chrome,firefox}.template.json` 是 host manifest 模板，安装器需：

1. 用 host 可执行文件的实际路径替换 `@HOST_EXE_PATH@`。
2. 用**打包扩展的固定 ID** 替换 `@CHROME_EXTENSION_ID@` / `@FIREFOX_EXTENSION_ID@`
   （该 ID 由 T2.2 的固定打包私钥决定，当前未定；见设计文档 §6.3、§9.1）。
3. 将填好的 manifest 写入平台约定位置并在注册表/文件系统注册：
   - Windows：`HKCU\Software\Google\Chrome\NativeMessagingHosts\com.gdownload.host`
     （及 Edge、Firefox 对应键），值为 manifest JSON 路径。
   - macOS：`~/Library/Application Support/Google/Chrome/NativeMessagingHosts/com.gdownload.host.json` 等。
   - Linux：`~/.config/google-chrome/NativeMessagingHosts/` + Firefox `~/.mozilla/native-messaging-hosts/`。

## 待办（后续任务）

- `launch` 的 appRunning 检测目前用 aria2 端口探测，主程序补单实例命名互斥量后可更精确。
- `parseShare` 需主程序侧单实例 IPC 接收并转交通用网盘解析页（复用现有 `NetWorkDiskManager::ParseShareUrl`）。
- macOS/Linux 的 `GetConfigPath` 路径需与各自 `os::GetAppDataDir` 实测对齐；`LaunchApp`/`IsAppRunning` 需补非 Windows 实现。
