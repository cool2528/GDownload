#pragma once

#include <QDir>
#include <QFileInfo>
#include <QVariantList>
#include <QVariantMap>
#include <ctime>
#include <nlohmann/json.hpp>
#include <optional>

#include "download_task_model.h"
#include "download_url_utils.h"

namespace gdl {
	namespace ui {
		namespace browser {

			struct RetryTaskRequest {
				QVariantList urls;
				QVariantMap options;
			};

			inline DownloadTaskInfo DownloadTaskInfoFromAria2Object(const nlohmann::json& object) {
				DownloadTaskInfo task_info;
				if (!object.is_object() || !object.contains("gid") || !object["gid"].is_string()) {
					return task_info;
				}

				auto parse_ll = [&object](const char* key) -> std::int64_t {
					if (object.contains(key) && object[key].is_string()) {
						try {
							return std::stoll(object[key].get<std::string>());
						} catch (...) {
						}
					}
					return 0;
				};
				auto parse_string = [&object](const char* key) -> QString {
					if (object.contains(key) && object[key].is_string()) {
						return QString::fromStdString(object[key].get<std::string>());
					}
					return {};
				};

				const std::string status = object.value("status", std::string());
				const QString task_id = QString::fromStdString(object["gid"].get<std::string>());
				const std::int64_t completed = parse_ll("completedLength");
				const std::int64_t connections = parse_ll("connections");
				const std::int64_t speed = parse_ll("downloadSpeed");
				const std::int64_t total_length = parse_ll("totalLength");

				QString first_path;
				QString download_url;
				int file_count = 0;
				if (object.contains("files") && object["files"].is_array()) {
					const auto& files = object["files"];
					file_count = static_cast<int>(files.size());
					for (const auto& file : files) {
						if (first_path.isEmpty() && file.contains("path") && file["path"].is_string()) {
							first_path = QString::fromStdString(file["path"].get<std::string>());
						}
						if (download_url.isEmpty() && file.contains("uris") && file["uris"].is_array()) {
							for (const auto& uri : file["uris"]) {
								if (uri.contains("uri") && uri["uri"].is_string()) {
									download_url = QString::fromStdString(uri["uri"].get<std::string>());
									break;
								}
							}
						}
					}
				}

				QString torrent_name;
				if (object.contains("bittorrent") && object["bittorrent"].is_object()) {
					const auto& bt = object["bittorrent"];
					if (bt.contains("info") && bt["info"].is_object() && bt["info"].contains("name") &&
						bt["info"]["name"].is_string()) {
						torrent_name = QString::fromStdString(bt["info"]["name"].get<std::string>());
					}
				}
				const QString task_dir = parse_string("dir");

				QString file_name;
				QString save_path;
				if (file_count > 1) {
					if (!torrent_name.isEmpty() && !task_dir.isEmpty()) {
						save_path = task_dir + "/" + torrent_name;
						file_name = torrent_name;
					} else if (!task_dir.isEmpty()) {
						save_path = task_dir;
						file_name = QFileInfo(first_path).fileName();
					} else {
						save_path = first_path;
						file_name = QFileInfo(first_path).fileName();
					}
				} else {
					save_path = first_path;
					file_name = QFileInfo(first_path).fileName();
				}

				if (save_path.isEmpty()) {
					file_name = SuggestDownloadFileNameFromUrl(download_url);
					if (file_name.isEmpty()) {
						file_name = QStringLiteral("download-%1").arg(task_id);
					}
					save_path = task_dir.isEmpty() ? file_name : QDir(task_dir).filePath(file_name);
				}

				task_info.set_task_id(task_id);
				task_info.set_task_download_speed(speed);
				task_info.set_task_current_size(completed);
				task_info.set_task_total_size(total_length);
				task_info.set_task_connections(connections);
				task_info.set_task_file_name(file_name);
				task_info.set_task_save_path(save_path);
				task_info.set_task_download_link(download_url);
				task_info.set_task_error_code(parse_string("errorCode").trimmed());
				task_info.set_task_error_message(parse_string("errorMessage").trimmed());

				if (status == "active") {
					task_info.set_task_state(TaskState::kActive);
				} else if (status == "waiting") {
					task_info.set_task_state(TaskState::kWaiting);
				} else if (status == "complete") {
					task_info.set_task_state(TaskState::kComplete);
				} else if (status == "paused") {
					task_info.set_task_state(TaskState::kPause);
				} else if (status == "error") {
					task_info.set_task_state(TaskState::kError);
				} else if (status == "removed") {
					task_info.set_task_state(TaskState::kRemoved);
				}

				return task_info;
			}

			inline gdl::cache::DownloadRecord DownloadRecordFromTaskInfo(const DownloadTaskInfo& info) {
				gdl::cache::DownloadRecord record;
				record.completed_time = std::time(nullptr);
				record.created_time = std::time(nullptr);
				record.connections = static_cast<int32_t>(info.task_connections());
				record.download_speed = static_cast<int32_t>(info.task_download_speed());
				record.download_url = info.task_download_link().toStdString();
				record.downloaded_size = info.task_current_size();
				record.task_id = info.task_id().toStdString();
				record.file_name = info.task_file_name().toStdString();
				record.save_path = info.task_save_path().toStdString();
				record.total_size = info.task_total_size();
				record.state = static_cast<gdl::cache::DownloadState>(info.task_state());
				record.error_message = info.task_error_message().toStdString();
				return record;
			}

			inline DownloadTaskInfo DownloadTaskInfoFromRecord(const gdl::cache::DownloadRecord& record) {
				DownloadTaskInfo info;
				info.set_task_id(QString::fromStdString(record.task_id));
				info.set_task_file_name(QString::fromStdString(record.file_name));
				info.set_task_save_path(QString::fromStdString(record.save_path));
				info.set_task_download_link(QString::fromStdString(record.download_url));
				info.set_task_current_size(record.downloaded_size);
				info.set_task_total_size(record.total_size);
				info.set_task_connections(record.connections);
				info.set_task_download_speed(record.download_speed);
				info.set_task_state(static_cast<TaskState>(record.state));
				info.set_task_error_message(QString::fromStdString(record.error_message));
				return info;
			}

			inline std::optional<RetryTaskRequest> BuildRetryTaskRequest(const DownloadTaskInfo& task) {
				const QString download_link = task.task_download_link().trimmed();
				if (task.task_state() != TaskState::kError || download_link.isEmpty()) {
					return std::nullopt;
				}

				RetryTaskRequest request;
				request.urls.append(download_link);

				const QFileInfo save_file(task.task_save_path());
				const QString directory = save_file.path();
				const QString output_name = save_file.fileName().isEmpty() ? task.task_file_name() : save_file.fileName();
				if (!directory.isEmpty() && directory != QStringLiteral(".")) {
					request.options.insert(QStringLiteral("dir"), directory);
				}
				if (!output_name.isEmpty()) {
					request.options.insert(QStringLiteral("out"), output_name);
				}
				return request;
			}

			inline bool ShouldRestoreHistoryTask(const DownloadTaskInfo& task, bool target_exists) {
				if (target_exists) {
					return true;
				}
				return task.task_state() == TaskState::kError;
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
