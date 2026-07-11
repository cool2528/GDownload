#pragma once

#include <QString>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <system_error>

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

			inline std::filesystem::path LocalRemovalFilesystemPath(const QString& path) {
#ifdef Q_OS_WIN
				return std::filesystem::path(path.toStdWString());
#else
				return std::filesystem::path(path.toStdString());
#endif
			}

			inline LocalRemovalResult RemoveLocalContent(const QString& path) {
				std::error_code error_code;
				const std::uintmax_t removed_count =
					std::filesystem::remove_all(LocalRemovalFilesystemPath(path), error_code);
				if (error_code) {
					return {.status = LocalRemovalStatus::kFailed,
							.path = path,
							.error = QStringLiteral("%1: %2")
									 .arg(error_code.value())
									 .arg(QString::fromStdString(error_code.message())),
							.error_code = error_code.value(),
							.partial_possible = true};
				}

				return {.status = removed_count == 0 ? LocalRemovalStatus::kNotFound
													 : LocalRemovalStatus::kRemoved,
						.path = path,
						.removed_count = removed_count};
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
