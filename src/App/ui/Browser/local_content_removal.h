#pragma once

#include <QString>
#include <QVariantMap>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <system_error>
#include <thread>

namespace gdl {
	namespace ui {
		namespace browser {

			enum class LocalRemovalStatus {
				kNotFound,
				kRemoved,
				kFailed,
			};

			struct LocalRemovalResult {
				LocalRemovalStatus status{LocalRemovalStatus::kNotFound};
				QString path;
				QString error;
				std::optional<std::uintmax_t> removed_count;
				int error_code{0};
				bool partial_possible{false};
			};

			inline QVariantMap LocalRemovalResultToVariantMap(const LocalRemovalResult& result) {
				QString status = QStringLiteral("notFound");
				if (result.status == LocalRemovalStatus::kRemoved) status = QStringLiteral("removed");
				if (result.status == LocalRemovalStatus::kFailed) status = QStringLiteral("failed");
				return {{QStringLiteral("status"), status},
						{QStringLiteral("path"), result.path},
						{QStringLiteral("error"), result.error},
						{QStringLiteral("errorCode"), result.error_code},
						{QStringLiteral("removedCount"),
						 result.removed_count.has_value() ? QVariant::fromValue<qulonglong>(*result.removed_count)
													  : QVariant()},
						{QStringLiteral("partialPossible"), result.partial_possible}};
			}

			inline std::filesystem::path LocalRemovalFilesystemPath(const QString& path) {
#ifdef Q_OS_WIN
				return std::filesystem::path(path.toStdWString());
#else
				return std::filesystem::path(path.toStdString());
#endif
			}

			// max_attempts: 删除尝试次数上限。默认 3 次带退避重试;批量删除在
			// UI 线程同步执行,传 1 避免每个被占用文件都让界面冻结数百毫秒
			inline LocalRemovalResult RemoveLocalContent(const QString& path, int max_attempts = 3) {
				if (path.isEmpty()) {
					return {.status = LocalRemovalStatus::kNotFound,
							.path = path,
							.removed_count = std::uintmax_t{0}};
				}

				const int attempts = max_attempts < 1 ? 1 : max_attempts;
				std::error_code error_code;
				for (int attempt = 0; attempt < attempts; ++attempt) {
					error_code.clear();
					// remove_all 出错时返回 static_cast<uintmax_t>(-1) 而非部分删除数,
					// 失败尝试的返回值必须丢弃,否则跨尝试累加会无符号回绕
					const std::uintmax_t removed_count =
						std::filesystem::remove_all(LocalRemovalFilesystemPath(path), error_code);
					if (!error_code) {
						return {.status = removed_count == 0 ? LocalRemovalStatus::kNotFound
																 : LocalRemovalStatus::kRemoved,
								.path = path,
								.removed_count = removed_count};
					}

					// Windows 上的播放器、预览窗格或杀毒软件可能只短暂持有文件句柄。
					// 以很短的退避重试，避免把可恢复的共享冲突直接暴露给用户。
					if (attempt + 1 < attempts) {
						std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
					}
				}

				return {.status = LocalRemovalStatus::kFailed,
						.path = path,
						.error = QStringLiteral("%1: %2")
								 .arg(error_code.value())
								 .arg(QString::fromStdString(error_code.message())),
						.error_code = error_code.value(),
						.partial_possible = true};
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
