#include "download_task_model.h"
#include <QJSEngine>
namespace gdl {
	namespace ui {
		namespace browser {

			DownloadTaskModel::DownloadTaskModel(QObject* parent) : QAbstractListModel(parent) {
				QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
			}

			DownloadTaskModel::~DownloadTaskModel() {}

			QVariant DownloadTaskModel::data(const QModelIndex& index, int role) const {
				if (!index.isValid() || index.row() >= task_lists_.size()) return QVariant();

				const DownloadTaskInfo& task = task_lists_[index.row()];

				switch (role) {
					case kTaskId:
						return QVariant::fromValue(task.task_id());
					case kTaskState:
						return QVariant::fromValue(static_cast<int>(task.task_state()));
					case kTaskFileName:
						return QVariant::fromValue(task.task_file_name());
					case kTaskSavePath:
						return QVariant::fromValue(task.task_save_path());
					case kTaskTotalSize:
						return QVariant::fromValue(DownloadTaskInfo::FormatFileSize(task.task_total_size()));
					case kTaskCurrentSize:
						return QVariant::fromValue(DownloadTaskInfo::FormatFileSize(task.task_current_size()));
					case kTaskDownloadSpeed:
						return QVariant::fromValue(DownloadTaskInfo::FormatFileSize(task.task_download_speed()));
					case kTaskProgress:
						return QVariant::fromValue(task.progress());
					case kTaskRemainingTime:
						return QVariant::fromValue(task.FormatRemainingTime());
					case kTaskConnections:
						return QVariant::fromValue(task.task_connections());
                    case kTaskDownloadLink:
                        return QVariant::fromValue(task.task_download_link());
					case kTaskErrorCode:
						return QVariant::fromValue(task.task_error_code());
					case kTaskErrorMessage:
						return QVariant::fromValue(task.task_error_message());
					default:
						return QVariant();
				}
			}

			int DownloadTaskModel::rowCount(const QModelIndex& parent) const {
				return GetTaskCount();
			}

			QHash<int, QByteArray> DownloadTaskModel::roleNames() const {
				QHash<int, QByteArray> roles;
				roles[kTaskId]			  = "taskId";
				roles[kTaskState]		  = "taskState";
				roles[kTaskFileName]	  = "fileName";
				roles[kTaskSavePath]	  = "savePath";
				roles[kTaskTotalSize]	  = "totalSize";
				roles[kTaskCurrentSize]	  = "currentSize";
				roles[kTaskDownloadSpeed] = "downloadSpeed";
				roles[kTaskProgress]	  = "progress";
				roles[kTaskRemainingTime] = "remainingTime";
				roles[kTaskConnections]	  = "connections";
                roles[kTaskDownloadLink]  = "downloadLink";
				roles[kTaskErrorCode]      = "errorCode";
				roles[kTaskErrorMessage]   = "errorMessage";
				return roles;
			}

			bool DownloadTaskModel::setData(const QModelIndex& index, const QVariant& value, int role) {
				if (!index.isValid() || index.row() >= task_lists_.size()) return false;

				DownloadTaskInfo& task = task_lists_[index.row()];
				bool changed		   = false;

				switch (role) {
					case kTaskState:
						if (value.canConvert<int>()) {
							task.set_task_state(static_cast<TaskState>(value.toInt()));
							changed = true;
						}
						break;
					case kTaskCurrentSize:
						if (value.canConvert<qint64>()) {
							task.set_task_current_size(value.toLongLong());
							changed = true;
						}
						break;
					case kTaskDownloadSpeed:
						if (value.canConvert<qint64>()) {
							task.set_task_download_speed(value.toLongLong());
							changed = true;
						}
						break;
					case kTaskConnections:
						if (value.canConvert<qint64>()) {
							task.set_task_connections(value.toLongLong());
							changed = true;
						}
						break;
                    case kTaskDownloadLink:
                        if (value.canConvert<QString>()) {
                            task.set_task_download_link(value.toString());
                            changed = true;
                        }
                        break;
					case kTaskErrorCode:
						if (value.canConvert<QString>()) {
							task.set_task_error_code(value.toString());
							changed = true;
						}
						break;
					case kTaskErrorMessage:
						if (value.canConvert<QString>()) {
							task.set_task_error_message(value.toString());
							changed = true;
						}
						break;
				}

				if (changed) {
					emit dataChanged(index, index, {role});
					return true;
				}

				return false;
			}

			void DownloadTaskModel::AddTask(const DownloadTaskInfo& task) {
				{
					std::lock_guard lock(mutex_);
					if (remove_task_id_.contains(task.task_id())) {
						if (task.task_state() == TaskState::kRemoved) {
							return;
						}
						remove_task_id_.remove(task.task_id());
					}
				}
				beginInsertRows(QModelIndex(), 0, 0);
				task_lists_.insert(0, task);
				endInsertRows();
				emit countChanged();
			}

			bool DownloadTaskModel::RemoveTask(int index) {
				if (index < 0 || index >= task_lists_.size()) return false;
				{
					std::unique_lock lock(mutex_);
                    QString task_id = task_lists_[index].task_id();
					if (!remove_task_id_.contains(task_id)) {
						remove_task_id_.insert(task_id, task_id);
						remove_order_.push_back(task_id);
						while (static_cast<int>(remove_order_.size()) > kMaxTombstones) {
							remove_task_id_.remove(remove_order_.front());
							remove_order_.pop_front();
						}
					}
				}
				beginRemoveRows(QModelIndex(), index, index);
				task_lists_.removeAt(index);
				endRemoveRows();
				emit countChanged();
				return true;
			}

			bool DownloadTaskModel::RemoveTaskById(const QString& task_id) {
				for (int i = 0; i < task_lists_.size(); ++i) {
					if (task_lists_[i].task_id() == task_id) {
						return RemoveTask(i);
					}
				}
				return false;
			}

			bool DownloadTaskModel::UpdateTask(int index, const DownloadTaskInfo& task) {
				if (index < 0 || index >= task_lists_.size()) return false;

				task_lists_[index] = task;
				emit dataChanged(createIndex(index, 0), createIndex(index, 0));
				return true;
			}

			bool DownloadTaskModel::UpdateTaskById(const QString& task_id, const DownloadTaskInfo& task) {
				for (int i = 0; i < task_lists_.size(); ++i) {
					if (task_lists_[i].task_id() == task_id) {
						return UpdateTask(i, task);
					}
				}
				return false;
			}
			DownloadTaskInfo* DownloadTaskModel::GetTask(int index) {
				if (index < 0 || index >= task_lists_.size()) return nullptr;
				return &task_lists_[index];
			}

			DownloadTaskInfo* DownloadTaskModel::GetTaskById(const QString& task_id) {
				for (int i = 0; i < task_lists_.size(); ++i) {
					if (task_lists_[i].task_id() == task_id) {
						return &task_lists_[i];
					}
				}
				return nullptr;
			}

			QStringList DownloadTaskModel::GetTaskIds() const {
				QStringList task_ids;
				for (const auto& task : task_lists_) {
					task_ids.append(task.task_id());
				}
				return task_ids;
			}

			void DownloadTaskModel::ClearAllTasks() {
				if (!task_lists_.isEmpty()) {
					beginResetModel();
					task_lists_.clear();
					{
						std::lock_guard lock(mutex_);
						remove_task_id_.clear();
						remove_order_.clear();
					}
					endResetModel();
					emit countChanged();
				}
			}

			int DownloadTaskModel::GetTaskCount() const {
				return task_lists_.size();
			}

			bool DownloadTaskModel::ContainsTask(const QString& task_id) const {
				for (const auto& task : task_lists_) {
					if (task.task_id() == task_id) {
						return true;
					}
				}
				return false;
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
