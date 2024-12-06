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
						return task.task_id();
					case kTaskState:
						return static_cast<int>(task.task_state());
					case kTaskFileName:
						return task.task_file_name();
					case kTaskSavePath:
						return task.task_save_path();
					case kTaskTotalSize:
						return DownloadTaskInfo::FormatFileSize(task.task_total_size());
					case kTaskCurrentSize:
						return DownloadTaskInfo::FormatFileSize(task.task_current_size());
					case kTaskDownloadSpeed:
						return DownloadTaskInfo::FormatFileSize(task.task_download_speed());
					case kTaskProgress:
						return task.progress();
					case kTaskRemainingTime:
						return task.FormatRemainingTime();
					case kTaskConnections:
						return task.task_connections();
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
				}

				if (changed) {
					emit dataChanged(index, index, {role});
					return true;
				}

				return false;
			}

			void DownloadTaskModel::AddTask(const DownloadTaskInfo& task) {
				beginInsertRows(QModelIndex(), task_lists_.size(), task_lists_.size());
				task_lists_.append(task);
				endInsertRows();
			}

			bool DownloadTaskModel::RemoveTask(int index) {
				if (index < 0 || index >= task_lists_.size()) return false;

				beginRemoveRows(QModelIndex(), index, index);
				task_lists_.removeAt(index);
				endRemoveRows();
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

			void DownloadTaskModel::ClearAllTasks() {
				if (!task_lists_.isEmpty()) {
					beginResetModel();
					task_lists_.clear();
					endResetModel();
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
