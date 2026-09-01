# SignPath 免费代码签名接入手册

本文档记录 GDownload 接入 SignPath Foundation 免费开源代码签名服务的完整流程、当前状态与后续操作步骤。

- 仓库：https://github.com/cool2528/GDownload
- 申请入口：https://signpath.org/apply （SignPath Foundation，非营利基金会）
- 签名平台：https://app.signpath.io （SignPath.io，证书私钥托管于其 HSM）

## 1. 两种角色的区别

| 站点 | 角色 | 用途 |
| --- | --- | --- |
| signpath.org | SignPath Foundation | 向开源项目**免费颁发**代码签名证书（证书主体为 "SignPath Foundation"），负责项目审核 |
| signpath.io | SignPath.io 平台 | 证书私钥的 HSM 托管与签名执行；CI 在这里提交签名请求，GitHub 为"受信任构建系统" |

签名后用户在 Windows 属性对话框中看到的发布者为 **SignPath Foundation**。

## 2. 申请条件核对（官方 Terms，signpath.org/terms）

| 条件 | 本项目状态 |
| --- | --- |
| OSI 认可的开源许可证，无商业双许可 | AGPL-3.0（LICENSE.txt） |
| 无恶意软件/潜在有害程序 | 满足（下载工具，无漏洞利用类功能） |
| 无专有闭源组件（System Libraries 除外） | 满足（aria2/libtorrent 等均为开源上游，符合"未签名上游二进制可包含在安装包内"条款） |
| 项目持续维护 | 满足 |
| 项目已以拟签名形态发布 | 满足（tag 自动发版流水线 cli-matrix.yml） |
| 下载页有项目功能描述 | 满足（README + Releases 说明） |
| 团队成员在 SignPath 与 GitHub 均开启 2FA | 申请前需自行确认 GitHub 账号已开启 2FA |
| 用户隐私/系统修改警告/卸载能力 | 满足（安装器含卸载；README 已补隐私声明） |
| 项目主页指定 Code signing policy | 已在 README.md / README_EN.md 增加 "Code signing policy" 一节 |

## 3. 仓库侧改动（已完成，随本分支合入）

| 文件 | 改动 |
| --- | --- |
| `cmake/WindowsVersionResource.rc.in` | 新增：统一版本信息资源模板（ProductName/ProductVersion 等元数据） |
| `cmake/windows_version_resource.cmake` | 新增：`gdl_configure_windows_version_resources()` CMake 函数，按目标类型生成版本资源 |
| `CMakeLists.txt` | 定义统一产品元数据变量；为 GDLCore/Engine/Ed2kEngine/PluginManager 四个自有 DLL 附加版本资源 |
| `src/App/ui/WindowsResourceTemplate.rc.in` | 主程序版本资源改用统一元数据（此前为 "My Company"/"gdownload"） |
| `.github/workflows/cli-matrix.yml` | 两阶段签名步骤（见第 4 节）；`GDLOAD_UPDATE_SIGNER_SPKI_PIN` 接线到构建 |
| `README.md` / `README_EN.md` | 新增 "Code signing policy" 一节（Foundation 硬性要求） |

说明：元数据一致性已按 SignPath 要求对齐——同一构建内所有 PE 文件的 **ProductVersion 完全相同**（= `MAJOR.MINOR.BUILD`，与 Inno Setup 安装包的 `VersionInfoVersion` 一致）；**FileVersion** 字符串在主程序与 DLL 上统一保留 commit 后缀用于问题定位，数值四元组第四位为 0（commit 哈希不是数字）。

## 4. CI 签名流程（两阶段，全部自动）

```
构建 & 安装
  ├─ 阶段 1：自有 PE 文件签名（目标目录 = Inno Setup 打包源目录 build/Release/bin）
  │    gdownload.exe + GDLCore.dll + Engine.dll + Ed2kEngine.dll + PluginManager.dll
  │    打 zip → 上传 artifact → SignPath 签名(windows-binaries 配置)
  │    → Get-AuthenticodeSignature 验签 → 回填 build/Release/bin
  ├─ Inno Setup 打包（含已签名的内部文件）
  └─ 阶段 2：安装包整体签名
       GDownloader_windows_*.exe → 上传 artifact → SignPath 签名(windows-installer 配置)
       → 验签 → 原地覆盖 → 后续发布步骤不变
```

开关机制：只有当仓库同时配置了 `SIGNPATH_API_TOKEN`（secret）与 `SIGNPATH_ORGANIZATION_ID`、`SIGNPATH_SIGNING_POLICY_SLUG`（vars）时签名步骤才生效；缺失时输出提示并空转，发版行为与接入前完全一致。因此本改动可先行合入，不影响现有发版。

第三方二进制（aria2c.exe、Qt/SSL 运行时 DLL 等）**不在签名范围**：Foundation 条款只允许签自有构建产物，上游未签名文件允许包含在安装包内（已安装文件由阶段 2 的安装包签名整体覆盖保护）。

每次发版需要在 SignPath 后台（app.signpath.io → Signing Requests）由 Approver **人工点击批准两次**（阶段 1、阶段 2 各一次），这是 Foundation 的强制政策。不批准则 CI 等待直至超时失败。

## 5. 申请与配置步骤

### 步骤 A：注册 SignPath.io 并创建组织（人工，约 5 分钟）

1. 打开 https://app.signpath.io ，用 GitHub 账号登录（GitHub 需已开启 2FA）。
2. 创建一个 Organization（名称建议 `gdownload` 或个人名，仅组织管理用，与基金会审核无冲突）。
3. 进入组织设置页，记录 **Organization ID**（GUID 格式）。

### 步骤 B：提交 Foundation 申请（人工，约 5 分钟）

打开 https://signpath.org/apply 填写（HubSpot 表单，字段以实际页面为准）：

- **Project name**: GDownload
- **Website / Download page**: https://github.com/cool2528/GDownload/releases
- **Source repository**: https://github.com/cool2528/GDownload
- **License**: GNU Affero General Public License v3 (AGPL-3.0)
- **Description**:
  > A modern cross-platform download manager built with C++20 and Qt 6, supporting HTTP/HTTPS/FTP/BitTorrent/Metalink, a plugin architecture, and cross-platform auto-update. Windows installers are built and published automatically via GitHub Actions, but they are currently unsigned, so users face SmartScreen warnings.
- **Why do you need code signing / Anything else**:
  > To provide users a verifiable link between the GitHub repository and the published Windows binaries, and to eliminate unsigned-installer warnings. The repository already implements a Code signing policy page, per-file version metadata, and a two-stage signing workflow (inner binaries + installer) ready for SignPath integration.
- **联系邮箱**: panquick@qq.com

提交后等待基金会审核（无官方时限，社区经验数天至数周）。期间**仓库侧改造可先行合并**，不影响现有发版。

### 步骤 C：审核通过后的平台配置（审核邮件会附带证书与说明）

1. **添加受信任构建系统**：组织设置 → Trusted Build Systems → 选择预置的 `GitHub.com`；按提示安装 **SignPath GitHub App** 并授权访问 `cool2528/GDownload`。
2. **导入证书**：组织 → Certificates → Add（按审核邮件指引，选 Foundation 提供的证书）。
3. **创建项目**：组织 → Projects → Add project：
   - Name: `GDownload`，Slug 必须为 `gdownload`（CI 中已写死）。
4. **创建签名策略**：项目 → Signing Policies，建议先建一条 `test-signing`（不要求批准或测试用），正式发布策略 slug 需与仓库 var 一致（建议 `release-signing`）。
5. **创建 artifact 配置**：项目 → Artifact Configurations，导入以下两份（见第 6 节）。
6. **创建 API Token**：右上角用户菜单 → API Tokens → Add，Scope 选 **Submit signing requests**，绑定项目与策略。生成后立即写入仓库 secret `SIGNPATH_API_TOKEN`（只显示一次）。

### 步骤 D：仓库 secret/vars 配置

在 GitHub 仓库 Settings → Secrets and variables → Actions 配置：

| 类型 | 名称 | 值 |
| --- | --- | --- |
| Secret | `SIGNPATH_API_TOKEN` | 步骤 C.6 生成的 token |
| Variable | `SIGNPATH_ORGANIZATION_ID` | 组织 ID（GUID） |
| Variable | `SIGNPATH_SIGNING_POLICY_SLUG` | 正式签名策略 slug（如 `release-signing`） |
| Variable | `SIGNPATH_SIGNER_SPKI_PIN`（可选，推荐） | `sha256:<64位十六进制>`——签名证书公钥的 SPKI SHA-256，启用更新器 Authenticode 校验（计算方法见第 8 节） |

配置完成后的下一次 tag 发版将自动走两阶段签名。

## 6. Artifact 配置（在 SignPath 平台导入）

### windows-binaries（阶段 1：zip 包内的自有 PE 文件）

```xml
<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
  <parameters>
    <parameter name="version" required="true" />
  </parameters>
  <zip-file>
    <pe-file-set product-name="GDownload" product-version="${version}">
      <include path="*.exe" />
      <include path="*.dll" />
      <for-each>
        <authenticode-sign />
      </for-each>
    </pe-file-set>
  </zip-file>
</artifact-configuration>
```

说明：`<pe-file-set>` + `<for-each>` 对 zip 内全部 exe/dll 逐一签名；元素上的 `product-name`/`product-version` 属性是 **file metadata restrictions**（官方语法），每次签名时强制校验文件元数据——ProductName 必须为 `GDownload`、ProductVersion 必须与 CI 传入的 `version` 参数（tag 去掉 v 前缀）一致。这正是 Foundation "Set all product name/product version attributes and enforce using file metadata restrictions" 要求的落地。

### windows-installer（阶段 2：安装包本体）

```xml
<artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
  <parameters>
    <parameter name="version" required="true" />
  </parameters>
  <pe-file product-name="GDownload" product-version="${version}">
    <authenticode-sign />
  </pe-file>
</artifact-configuration>
```

Inno Setup 安装包的 PE 元数据由 `.iss` 的 `VersionInfoVersion={#MyAppVersion}` 等生成（ProductName 取 AppName = GDownload），与 `${version}` 校验一致。

阶段 1 与阶段 2 各产生一条签名请求；使用测试策略（test-signing）时无需批准，正式发布策略必须由 Approver 在 app.signpath.io 后台各批准一次。

## 7. 首次签名验证

1. 推一个测试 tag（如 `v9.9.9-test`）触发 `CLI-All-Platforms`。
2. 观察 Windows job：`Resolve SignPath signing inputs` 应输出 enabled；两条 `Submit ... signing request` 步骤会等待批准。
3. 登录 app.signpath.io → Signing Requests，分别批准两条请求（检查 CI 来源、artifact 摘要）。
4. CI 取回签名产物并自动验签（`Get-AuthenticodeSignature` Status 必须为 Valid，否则 job 失败）。
5. 在 GitHub Release 下载安装包，右键 → 属性 → 数字签名：发布者应为 **SignPath Foundation**；安装后检查 `gdownload.exe` 与自有 DLL 均已签名。
6. 验证完成后删除测试 tag 与对应 Release。

## 8. 计算签名者 SPKI SHA-256（可选步骤 D 的 pin）

对任一已签名产物（PowerShell）：

```powershell
$cert = (Get-AuthenticodeSignature .\GDownloader_windows_x.y.z.exe).SignerCertificate
$spki = $cert.PublicKey.EncodedKeyValue.RawData
$sha = [System.Security.Cryptography.SHA256]::Create()
-join ($sha.ComputeHash($spki) | ForEach-Object { $_.ToString('x2') })
# 写入仓库 var SIGNPATH_SIGNER_SPKI_PIN: sha256:<上面的十六进制串>
```

启用后，应用内更新下载的安装包会执行完整 Authenticode 信任链校验并锁定该指纹（`src/Module/GDLCore/update/platform_package_verifier.cxx`，构建时经 `GDLOAD`/`GDDOWNLOAD_UPDATE_SIGNER_SPKI_PIN` 注入），pin 不匹配则拒绝更新——把"更新清单签名"升级为"清单签名 + 发布者签名"双重校验。

## 9. 常见问题

- **CI 卡在 Submit signing request**：等待 Approver 批准（默认超时 20 分钟）。批准入口：app.signpath.io → Signing Requests。
- **签名请求被拒 / artifact configuration error**：核对 zip 结构与 artifact 配置的 path 是否匹配（阶段 1 的 zip 内是平铺的 5 个文件）。
- **验证步骤报 Signature verification failed**：产物可能被中途篡改或签名服务异常，先在 SignPath 后台核对请求详情再重试。
- **签名后 SmartScreen 仍提示**：新证书需要积累信誉（SmartScreen 信誉与签名者历史安装量相关），Foundation 证书签名正确性不受影响；一段时间后警告消失。
- **macOS/Linux**：Foundation 免费计划仅覆盖 Windows Authenticode。macOS 签名需自有 Apple Developer 账号（现有 Sparkle 流程不变），Linux AppImage 已有 GPG 签名。
- **每次发版要批准两次太麻烦**：这是 Foundation 的强制政策（"Every release needs manual approval for signing"），不可绕过。
