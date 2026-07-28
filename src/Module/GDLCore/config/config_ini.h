#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <toml++/toml.h>
#include "globalTypes.h"
#include "singleton.hpp"
namespace gdl {
	namespace config {
		namespace detail {
			template <typename T>
			std::string ToStringValue(const T& value) {
				if constexpr (std::is_same_v<T, std::string>) {
					return value;
				} else if constexpr (std::is_same_v<T, const char*>) {
					return std::string(value);
				} else if constexpr (std::is_same_v<T, bool>) {
					return value ? "true" : "false";
				} else {
					return std::to_string(value);
				}
			}

			inline std::string ToLowerCopy(std::string value) {
				std::transform(value.begin(), value.end(), value.begin(),
							   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				return value;
			}

			template <typename T>
			std::optional<T> FromString(const std::string& value) {
				if constexpr (std::is_same_v<T, std::string>) {
					return value;
				} else if constexpr (std::is_same_v<T, bool>) {
					auto lower = ToLowerCopy(value);
					if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
					if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
					return std::nullopt;
				} else if constexpr (std::is_integral_v<T>) {
					try {
						long long parsed = std::stoll(value);
						return static_cast<T>(parsed);
					} catch (...) {
						return std::nullopt;
					}
				} else if constexpr (std::is_floating_point_v<T>) {
					try {
						double parsed = std::stod(value);
						return static_cast<T>(parsed);
					} catch (...) {
						return std::nullopt;
					}
				} else {
					return std::nullopt;
				}
			}
		}  // namespace detail

		class ApplicationConfig : public Singleton<ApplicationConfig> {
			SINGLETON_DECLARE(ApplicationConfig)
		   public:
			~ApplicationConfig();
			// 写入一个配置项。**不会阻塞调用方做文件 I/O** —— 只改内存并置脏位，真正的落盘由
			// 后台写入线程合并后异步完成（见 config_ini.cxx 的 WriterLoop）。
			//
			// 【为什么必须异步】改动前这里是同步 Save()，而 Save() 走
			// filesystem::AtomicFileReplace：建临时文件 -> 写整份 TOML -> **FlushFileBuffers**
			// (真 fsync，等物理落盘) -> ReplaceFileW。QML 里任何一句
			// `SettingsManager.xxx = ...` 都在 **UI 线程**上直接触发这一整套。
			// 实测后果：拖动窗口时 mainWindow.qml 的 400ms 防抖每次触发都写两次配置，
			// 而下载正在猛写盘时那次 fsync 要排在磁盘队列后面 —— 模拟拖动 5 轮 × 150 次，
			// 4 轮出现多秒冻结，最长 **17.8 秒**；卡死瞬间抓的转储里主线程栈正是
			// ZwWriteFile <- AtomicFileReplace <- config::SetValue <- QQmlPropertyPrivate::write。
			// 这就是用户报的"拖窗口卡"与"(未响应)"。
			//
			// 顺带修掉第二个放大器：旧 Save() 持 mutex_ 的**独占锁**做文件 I/O，写盘期间任何
			// 线程的 Get()(共享锁)都被挡住。现在 I/O 在锁外进行，只在取快照时短暂加锁。
			template <typename T>
			void Put(const std::string& key, const T& value) {
				{
					std::unique_lock lock(mutex_);
					SetValueInternalUnlocked(key, detail::ToStringValue(value));
				}
				ScheduleSave();
			}
			template <typename Type>
			Type Get(const std::string& key) {
				std::shared_lock lock(mutex_);
				auto optional_val = TryGetStringUnlocked(key);
				if (optional_val.has_value()) {
					auto converted = detail::FromString<Type>(*optional_val);
					if (converted.has_value()) return *converted;
				}
				return Type();
			}

			// 尝试获取配置值：存在则返回值，不存在返回 std::nullopt
			template <typename Type>
			std::optional<Type> TryGet(const std::string& key) {
				std::shared_lock lock(mutex_);
				auto optional_val = TryGetStringUnlocked(key);
				if (!optional_val.has_value()) return std::nullopt;
				return detail::FromString<Type>(*optional_val);
			}

			// 按逻辑源名获取有序镜像 URL 列表（第一个为主源，后续为回退镜像）
			std::vector<std::string> GetTrackerServerUrlsByName(const std::string& name) const;
			// 将旧版配置名（如 "ngosang-best-link"）归一化为逻辑源名（如 "ngosang-best"）；
			// 未知名称原样返回
			std::string NormalizeTrackerSourceName(const std::string& name) const;
			// 同步落盘并等待完成。仅用于"必须确保已经写下去"的场合(退出前)。
			// 常规写入走 Put 的异步路径，不要在 UI 线程调用本方法。
			bool FlushNow();

		   private:
			explicit ApplicationConfig();
			bool Load();
			bool Save();
			// 置脏位并唤醒后台写入线程；立即返回。
			void ScheduleSave();
			// 后台写入线程主体：合并一小段时间内的多次写入，在**锁外**执行原子替换。
			void WriterLoop();
			void StopWriter();
			bool EnsureConfigFileExists();
			void SetValueInternalUnlocked(const std::string& key, const std::string& value);
			std::optional<std::string> TryGetStringUnlocked(const std::string& key) const;
			bool TryMigrateFromLegacyIniUnlocked();
			toml::node* FindNodeUnlocked(const std::vector<std::string>& segments);
			const toml::node* FindNodeUnlocked(const std::vector<std::string>& segments) const;
			static std::vector<std::string> SplitKeyPath(const std::string& key);
			std::string ResolveDefaultValue(const std::string& key_path, const std::string& def) const;
			std::optional<std::string> ValidateExistingValue(const std::string& key_path,
															 const std::string& current,
															 const std::string& def) const;

		   private:
			toml::table toml_root_;
			String config_file_path_;
			String legacy_config_file_path_;
			std::shared_mutex mutex_;
			// 逻辑源名 -> 有序镜像 URL 列表（主源在前，回退镜像在后）
			std::map<std::string, std::vector<std::string>> tracker_source_server_;

			// —— 异步落盘 ——
			// writer_mutex_ 只保护下面这三个调度用的标志，**不保护 toml_root_**(那是 mutex_ 的事)。
			// 两把锁不嵌套持有：写入线程先在 writer_mutex_ 下取走脏位，再单独用 mutex_ 取快照，
			// 最后不持任何锁做文件 I/O。
			std::mutex writer_mutex_;
			std::condition_variable writer_cv_;
			bool dirty_ = false;
			bool writer_stop_ = false;
			std::thread writer_thread_;
		};
	}  // namespace config
}  // namespace gdl
