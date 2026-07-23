# 阿里云盘插件配置指南

阿里云盘插件用于解析 `alipan.com` / `aliyundrive.com` 的分享链接。**浏览分享内容无需配置**；下载文件需要在插件设置中填写**网页端 refresh_token**。

## refresh_token 是什么？

refresh_token 是阿里云盘网页版的长效登录凭据，插件用它换取访问令牌（access_token）以获取下载地址。**refresh_token 只保存在本机**（应用数据目录的 `plugin_configs.json`），不会上传到任何服务器。

注意：必须是**网页端**（alipan.com）的 refresh_token，App 端或开放平台的 token 不通用。

## 获取步骤

1. 在浏览器（推荐 Chrome / Edge）打开 <https://www.alipan.com> 并登录你的账号。
2. 按 `F12` 打开开发者工具，切换到 **应用（Application）** 标签。
   - Firefox 对应"存储（Storage）"标签。
3. 左侧展开 **Local Storage（本地存储空间）**，选择 `https://www.alipan.com`。
4. 在右侧列表中点击名为 **`token`** 的条目，其值是一段 JSON。
5. 在 JSON 中找到 `refresh_token` 字段，**复制它的值**（引号内的内容，不含引号）。
6. 回到 GDownload：插件市场 → 阿里云盘 → 设置，把值粘贴到 "阿里云盘 refresh_token（网页端）" 输入框，点击**保存**。

## 常见问题

| 现象 | 原因与处理 |
|------|-----------|
| 提示 "check refresh_token" / 设备会话失败 | token 复制错误（多复制了引号或 JSON 其他部分）或已失效，重新获取 |
| 浏览正常但无法下载 | 未配置 refresh_token——浏览免登录，下载必须配置 |
| 一段时间后失效 | 网页端退出登录或修改密码会使 refresh_token 失效，重新获取即可 |

## 安全说明

- refresh_token 等同于你的登录凭据，**不要发给任何人**。
- GDownload 仅在本机保存并使用该值；在插件设置中点击 **Clear** 可随时清除。

## 在线版本

官网图文教程：<https://gdownload.uk/docs/guides/aliyundrive-netdisk.html>
