#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "PluginManager_export.h"
#include "../JsPluginRuntime/plugin_manifest.h"

namespace gdl {
	namespace market {

		// 插件相对于本地的安装状态
		enum class InstallState {
			Available,		 // 未安装
			Installed,		 // 已安装且为最新
			UpdateAvailable	 // 已安装但有新版本
		};

		// registry.json 中一个版本条目
		struct PluginVersion {
			std::string version;
			std::string min_app_version;
			std::uint64_t size{0};
			std::string sha256;
			std::string signature;				  // 对 zip 字节的 Ed25519 签名（base64）
			std::vector<std::string> download_urls;	 // 多源，按序回退
		};

		// registry.json 中一个插件
		struct RegistryPlugin {
			std::string name;
			std::string display_name;
			std::string description;
			std::string author;
			std::string homepage;
			std::string type;
			std::vector<std::string> url_patterns;
			bool verified{false};
			std::string latest;
			std::vector<PluginVersion> versions;
			// 本地化文案：locale -> {display_name, description}
			std::map<std::string, plugin::js::LocaleStrings> locales;

			const PluginVersion* FindVersion(const std::string& version) const;
			const PluginVersion* LatestVersion() const;
		};

		// 市场项：registry 元数据 + 本地状态
		struct MarketItem {
			RegistryPlugin meta;
			std::string installed_version;	// 空表示未安装
			bool enabled{true};
			InstallState state{InstallState::Available};
		};

		using ProgressCallback = std::function<void(int percent, const std::string& stage)>;

		// 插件市场核心服务（Qt-free）：注册表拉取、下载、校验、解压、安装/卸载、启用禁用
		// 线程模型：非线程安全，由上层（QObject 包装的 worker 线程）串行调用
		class PluginManager_API PluginMarketService {
		   public:
			// plugins_dir: <appdir>/plugins；data_dir: 应用数据目录（存 plugin_state.json）
			PluginMarketService(std::filesystem::path plugins_dir, std::filesystem::path data_dir);

			// 设置当前界面语言（如 zh_CN），用于本地化插件名称/描述；空表示默认（英文）
			void SetLocale(const std::string& locale) { locale_ = locale; }

			// 从多源拉取并解析 registry.json（按序回退，单源超时切换）
			bool FetchRegistry(const std::vector<std::string>& registry_urls, std::string& error_out);

			// 结合本地已安装插件与禁用列表，计算每个 registry 项的状态
			std::vector<MarketItem> ComputeItems() const;

			// 安装/更新指定插件到指定版本：下载(多源)->SHA-256->Ed25519->解压->落盘
			bool InstallPlugin(const std::string& name, const std::string& version,
							   const ProgressCallback& progress, std::string& error_out);

			// 卸载：删除 plugins/<name>/ 目录
			bool UninstallPlugin(const std::string& name, std::string& error_out);

			// 启用/禁用：维护 data_dir/plugin_state.json 的 disabled 列表
			bool SetEnabled(const std::string& name, bool enabled, std::string& error_out);

			// 当前禁用的插件名集合（供 DownloadPluginManager 加载时跳过）
			std::vector<std::string> DisabledPlugins() const;

			const std::vector<RegistryPlugin>& registry() const { return registry_; }

		   private:
			// 扫描 plugins/*/manifest.json，返回已装插件的完整 manifest
			std::vector<plugin::js::PluginManifest> ScanInstalled() const;
			// 下载 URL 到内存（多源回退）；失败返回 nullopt
			std::optional<std::string> DownloadWithFallback(const std::vector<std::string>& urls,
														   const ProgressCallback& progress,
														   std::string& error_out) const;
			// 原子解压 zip 字节到 plugins/<name>/（先解到临时目录再替换）
			bool ExtractZip(const std::string& zip_bytes, const std::string& name, std::string& error_out) const;

		   private:
			std::filesystem::path plugins_dir_;
			std::filesystem::path data_dir_;
			std::vector<RegistryPlugin> registry_;
			std::string locale_;  // 当前界面语言
		};

		// ---- 校验原语（也供测试直接调用）----
		// 计算内存字节的 SHA-256 小写十六进制
		PluginManager_API std::string Sha256Hex(const std::string& bytes);
		// 用内置 Ed25519 公钥验证签名（base64）对 bytes 是否有效
		PluginManager_API bool VerifyEd25519(const std::string& bytes, const std::string& signature_base64);

	}  // namespace market
}  // namespace gdl
