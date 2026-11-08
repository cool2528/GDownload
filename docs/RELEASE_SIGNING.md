# 发版签名配置 (Release Signing)

GDownload 采用**免费签名方案**,不购买付费代码签名证书:

- ✅ **ed25519 更新清单签名** —— 保护自动更新不被篡改(免费自生成)
- ✅ **AppImage GPG 签名** —— Linux 包完整性校验(免费自生成)
- **Windows Authenticode 代码签名暂未启用** —— SignPath 生产证书仍待签发，本次发布未签名安装包；`SIGNPATH_ENABLED=false`，测试证书不用于发布。生产证书就绪后的接入流程见 [SIGNPATH_SETUP.md](SIGNPATH_SETUP.md)。
- ❌ **不做 macOS 代码签名** —— 需要付费 Apple Developer 账号,不在免费方案内

更新链的安全由两层保证:ed25519 签名的更新清单(校验版本号 / 下载地址 / SHA-256)+ 下载包的 SHA-256 完整性校验;SignPath 接入后,Windows 更新包还可选启用 Authenticode 信任链 + SPKI 指纹锁定(见 SIGNPATH_SETUP.md 第 8 节)。

## 一、需要配置的 Secrets 与 Variables

GitHub repo → Settings → Secrets and variables → Actions

| 名称 | 类型 | 用途 |
|------|------|------|
| `UPDATE_MANIFEST_ED25519_PRIVATE_KEY` | **Secret** | 更新清单 ed25519 签名私钥 |
| `UPDATE_MANIFEST_ED25519_PUBLIC_KEY` | **Variable** | 对应公钥(CI 校验其与私钥匹配) |
| `APPIMAGE_GPG_PRIVATE_KEY` | **Secret** | Linux AppImage GPG 私钥 |
| `APPIMAGE_GPG_FINGERPRINT` | **Secret** | GPG 指纹(40 位 hex) |

> 注意 `UPDATE_MANIFEST_ED25519_PUBLIC_KEY` 是 **Variable** 不是 Secret(公钥无需保密,且 CI 需明文读取校验)。

## 二、生成命令

### ① ed25519(1 Secret + 1 Variable)

```bash
# 1) 生成 PKCS#8 PEM 私钥
openssl genpkey -algorithm ed25519 -out gdownload_update_key.pem

# 2) 派生 base64 公钥(脚本已兼容 Windows / Linux / macOS)
export UPDATE_MANIFEST_ED25519_PRIVATE_KEY="$(cat gdownload_update_key.pem)"
python scripts/update/generate_update_manifest.py public-key \
  --private-key-env UPDATE_MANIFEST_ED25519_PRIVATE_KEY
```

- Secret `UPDATE_MANIFEST_ED25519_PRIVATE_KEY` = `gdownload_update_key.pem` 的完整内容(含 `-----BEGIN/END PRIVATE KEY-----`)
- Variable `UPDATE_MANIFEST_ED25519_PUBLIC_KEY` = 上面第 2 步命令输出的 base64 字符串

### ② AppImage GPG(2 Secrets)

```bash
# 1) 生成密钥(选 ECC/Curve25519 或 RSA 4096,填 name/email;CI 用建议不设 passphrase)
gpg --full-generate-key

# 2) 记下 40 位 fingerprint
gpg --list-secret-keys --keyid-format=long

# 3) 导出私钥块
gpg --armor --export-secret-keys <FINGERPRINT>
```

- Secret `APPIMAGE_GPG_PRIVATE_KEY` = 第 3 步导出的私钥块(含 `-----BEGIN/END PGP PRIVATE KEY BLOCK-----`)
- Secret `APPIMAGE_GPG_FINGERPRINT` = 第 2 步的 40 位 fingerprint

## 三、发版流程

1. 确认上述 **4 项**已配置(3 Secret + 1 Variable)
2. 更新 `CHANGELOG.md`(其内容会作为 GitHub Release notes)
3. push 到默认分支
4. 打 tag 触发发版:
   ```bash
   git tag v2.0.0
   git push origin v2.0.0
   ```
5. CI(`.github/workflows/cli-matrix.yml`)自动完成:
   - `validate-release-inputs` 校验签名输入齐全
   - 打包 macOS(DMG)/ Linux(AppImage)/ Windows(exe)
   - ed25519 签名更新清单 + AppImage GPG detached 签名
   - 创建 GitHub Release 并上传产物

## 四、安全须知

- 私钥(`.pem` / GPG 私钥)是敏感信息,配置进 GitHub 后请**离线妥善保管**,切勿提交进仓库
- 若私钥泄露,需重新生成、更新 secrets/variable,并同步更新已发布客户端所信任的公钥
- 版本号由 git tag 驱动(`cmake/get_version.cmake` 解析 `v<major>.<minor>.<build>`)

## 五、Windows 生产证书签发前的发布策略

- `SIGNPATH_ENABLED` 未设置或为 `false` 时，Windows 构建直接打包未签名安装包，不提交 SignPath 请求，也不导入测试证书。
- 未签名构建的 `GDOWNLOAD_UPDATE_SIGNER_SPKI_PIN` 固定为空；更新完整性继续由 Ed25519 清单和 SHA-256 校验保证。
- 生产证书签发后配置 `SIGNPATH_API_TOKEN`、`SIGNPATH_ORGANIZATION_ID`、`SIGNPATH_SIGNING_POLICY_SLUG=release-signing` 和生产证书的 `SIGNPATH_SIGNER_SPKI_PIN`，最后显式设 `SIGNPATH_ENABLED=true`。
- `resolve-windows-signing` 是构建与签名作业共用的唯一开关；输入不全时启用签名会报错。更新清单作业等待签名作业成功或明确跳过后才下载产物。
- macOS 公证仍需要 Apple Developer 账号；当前策略不因本次 Windows 调整而改变。
