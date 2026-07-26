#include "download_task_model.h"
#include <QJSEngine>
#include <chrono>
namespace gdl {
	namespace ui {
		namespace browser {

			namespace {
				// 进度速率必须用单调钟:系统时间被改(NTP 校正、用户手动调表)会让窗口跨度
				// 变成负数或突然拉长,速率随之算成负值或接近 0,ETA 会莫名其妙地变"未知"。
				std::int64_t SteadyNowMs() {
					using namespace std::chrono;
					return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
				}

				// now_ms 为负表示"生产路径,取当前时刻";非负值来自单元测试的确定性时间轴
				std::int64_t ResolveSampleTime(std::int64_t now_ms) {
					return now_ms < 0 ? SteadyNowMs() : now_ms;
				}
			}  // namespace

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
					case kTaskSources:
						return QVariant::fromValue(task.task_sources());
					case kTaskQueuedSources:
						return QVariant::fromValue(task.task_queued_sources());
                    case kTaskDownloadLink:
                        return QVariant::fromValue(task.task_download_link());
					case kTaskErrorCode:
						return QVariant::fromValue(task.task_error_code());
					case kTaskErrorMessage:
						return QVariant::fromValue(task.task_error_message());
					case kTaskTotalSizeBytes:
						return QVariant::fromValue(static_cast<qint64>(task.task_total_size()));
					case kTaskCurrentSizeBytes:
						return QVariant::fromValue(static_cast<qint64>(task.task_current_size()));
					case kTaskProgressStalled:
						return QVariant::fromValue(task.progress_stalled());
					case kTaskProgressStallKind:
						return QVariant::fromValue(static_cast<int>(task.stall_kind()));
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
				roles[kTaskSources]		  = "sources";
				roles[kTaskQueuedSources] = "queuedSources";
                roles[kTaskDownloadLink]  = "downloadLink";
				roles[kTaskErrorCode]      = "errorCode";
				roles[kTaskErrorMessage]   = "errorMessage";
				roles[kTaskTotalSizeBytes]    = "totalSizeBytes";
				roles[kTaskCurrentSizeBytes]  = "currentSizeBytes";
				roles[kTaskProgressStalled]   = "progressStalled";
				roles[kTaskProgressStallKind] = "progressStallKind";
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
					case kTaskSources:
						if (value.canConvert<qint64>()) {
							task.set_task_sources(value.toLongLong());
							changed = true;
						}
						break;
					case kTaskQueuedSources:
						if (value.canConvert<qint64>()) {
							task.set_task_queued_sources(value.toLongLong());
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

			void DownloadTaskModel::AddTask(const DownloadTaskInfo& task, std::int64_t now_ms) {
				{
					std::lock_guard lock(mutex_);
					if (remove_task_id_.contains(task.task_id())) {
						if (task.task_state() == TaskState::kRemoved) {
							return;
						}
						remove_task_id_.remove(task.task_id());
					}
				}
				DownloadTaskInfo seeded = task;
				// 首次出现即为进度速率窗口的起点。此前没有任何历史,窗口只有一个点,
				// rate_bps() 返回 0,ETA 显示"Unknown",直到跨度够长才给出估计值。
				seeded.SampleProgress(ResolveSampleTime(now_ms));
				beginInsertRows(QModelIndex(), 0, 0);
				task_lists_.insert(0, std::move(seeded));
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

			bool DownloadTaskModel::UpdateTask(int index, const DownloadTaskInfo& task, std::int64_t now_ms) {
				if (index < 0 || index >= task_lists_.size()) return false;

				// 每次采样都是引擎 payload 重建出来的一份全新 TaskInfo,自身没有历史;
				// 进度速率窗口只存在于模型里的旧条目上,必须先接过来再记这一次采样,
				// 否则窗口永远只有一个点,ETA 恒为"Unknown"。
				DownloadTaskInfo merged = task;
				merged.InheritProgressHistoryFrom(task_lists_[index]);
				merged.SampleProgress(ResolveSampleTime(now_ms));
				task_lists_[index] = std::move(merged);
				emit dataChanged(createIndex(index, 0), createIndex(index, 0));
				return true;
			}

			bool DownloadTaskModel::UpdateTaskById(const QString& task_id, const DownloadTaskInfo& task,
												   std::int64_t now_ms) {
				for (int i = 0; i < task_lists_.size(); ++i) {
					if (task_lists_[i].task_id() == task_id) {
						return UpdateTask(i, task, now_ms);
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

			bool DownloadTaskModel::IsTombstoned(const QString& task_id) const {
				std::lock_guard lock(mutex_);
				return remove_task_id_.contains(task_id);
			}

			void DownloadTaskModel::ClearTombstone(const QString& task_id) {
				std::lock_guard lock(mutex_);
				if (remove_task_id_.remove(task_id)) {
					// 同步移除 FIFO 队列中的对应条目,维持与哈希表一一对应;
					// 残留陈旧条目会让后续淘汰误删同 id 重建的新墓碑
					std::erase(remove_order_, task_id);
				}
			}

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
