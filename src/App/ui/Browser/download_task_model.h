#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <deque>
#include <mutex>
#include "cache/cache.h"
namespace gdl {
	namespace ui {
		namespace browser {

            enum class TaskState : int { kComplete = 0, kActive, kPause, kWaiting, kError, kRemoved };

			class DownloadTaskInfo {
			   public:
				DownloadTaskInfo() = default;

				DownloadTaskInfo(const DownloadTaskInfo& other)
					: task_id_(other.task_id_),
					  task_state_(other.task_state_),
					  task_file_name_(other.task_file_name_),
					  task_save_path_(other.task_save_path_),
					  task_download_link_(other.task_download_link_),
					  task_error_code_(other.task_error_code_),
					  task_error_message_(other.task_error_message_),
					  task_total_size_(other.task_total_size_),
					  task_current_size_(other.task_current_size_),
					  task_download_speed_(other.task_download_speed_),
					  task_connections_(other.task_connections_) {}

				DownloadTaskInfo(DownloadTaskInfo&& other) noexcept
					: task_id_(std::move(other.task_id_)),
					  task_state_(other.task_state_),
					  task_file_name_(std::move(other.task_file_name_)),
					  task_save_path_(std::move(other.task_save_path_)),
					  task_download_link_(std::move(other.task_download_link_)),
					  task_error_code_(std::move(other.task_error_code_)),
					  task_error_message_(std::move(other.task_error_message_)),
					  task_total_size_(other.task_total_size_),
					  task_current_size_(other.task_current_size_),
					  task_download_speed_(other.task_download_speed_),
					  task_connections_(other.task_connections_) {}

				DownloadTaskInfo& operator=(const DownloadTaskInfo& other) {
					if (this != &other) {
						task_id_			 = other.task_id_;
						task_state_			 = other.task_state_;
						task_file_name_		 = other.task_file_name_;
						task_save_path_		 = other.task_save_path_;
						task_download_link_	 = other.task_download_link_;
						task_error_code_		 = other.task_error_code_;
						task_error_message_	 = other.task_error_message_;
						task_total_size_	 = other.task_total_size_;
						task_current_size_	 = other.task_current_size_;
						task_download_speed_ = other.task_download_speed_;
						task_connections_	 = other.task_connections_;
					}
					return *this;
				}

				DownloadTaskInfo& operator=(DownloadTaskInfo&& other) noexcept {
					if (this != &other) {
						task_id_			 = std::move(other.task_id_);
						task_state_			 = other.task_state_;
						task_file_name_		 = std::move(other.task_file_name_);
						task_save_path_		 = std::move(other.task_save_path_);
						task_download_link_	 = std::move(other.task_download_link_);
						task_error_code_		 = std::move(other.task_error_code_);
						task_error_message_	 = std::move(other.task_error_message_);
						task_total_size_	 = other.task_total_size_;
						task_current_size_	 = other.task_current_size_;
						task_download_speed_ = other.task_download_speed_;
						task_connections_	 = other.task_connections_;
					}
					return *this;
				}

				QString task_id() const { return task_id_; }
				TaskState task_state() const { return task_state_; }
				QString task_file_name() const { return task_file_name_; }
				QString task_save_path() const { return task_save_path_; }
                QString task_download_link() const { return task_download_link_; }
				QString task_error_code() const { return task_error_code_; }
				QString task_error_message() const { return task_error_message_; }
				std::int64_t task_total_size() const { return task_total_size_; }
				std::int64_t task_current_size() const { return task_current_size_; }
				std::int64_t task_download_speed() const { return task_download_speed_; }
				std::int64_t task_connections() const { return task_connections_; }

				void set_task_id(const QString& task_id) { task_id_ = task_id; }
				void set_task_state(TaskState state) { task_state_ = state; }
				void set_task_file_name(const QString& file_name) { task_file_name_ = file_name; }
                void set_task_save_path(const QString& save_path) {
                    task_save_path_ = save_path;
                    task_save_path_ = task_save_path_.replace("//", "/");
                }
				void set_task_download_link(const QString& link) {
					if (link.isEmpty()) return;
					task_download_link_ = link;
				}
				void set_task_error_code(const QString& error_code) { task_error_code_ = error_code; }
				void set_task_error_message(const QString& error_message) { task_error_message_ = error_message; }
				void set_task_total_size(std::int64_t total_size) { task_total_size_ = total_size; }
				void set_task_current_size(std::int64_t current_size) { task_current_size_ = current_size; }
				void set_task_download_speed(std::int64_t download_speed) { task_download_speed_ = download_speed; }
				void set_task_connections(std::int64_t task_connections) { task_connections_ = task_connections; }

				double progress() const {
					if (task_total_size_ <= 0) return 0.0;
					return (static_cast<double>(task_current_size_) / task_total_size_) * 100.0;
				}

				int remaining_time() const {
					if (task_download_speed_ <= 0) return -1;  // 返回-1表示无法计算
					std::int64_t remaining_size = task_total_size_ - task_current_size_;
					return static_cast<int>(remaining_size / task_download_speed_);
				}

				QString FormatRemainingTime() const {
					int seconds = remaining_time();
					if (seconds < 0) {
						return QObject::tr("Unknown");
					}

					const int hour = seconds / 3600;
					seconds %= 3600;
					const int minute = seconds / 60;
					seconds %= 60;

					QString result;
					if (hour > 0) {
						result += QString::number(hour) + " " + (hour == 1 ? QObject::tr("h") : QObject::tr("h"));
						if (minute > 0) {
							result += " " + QString::number(minute) + " " +
									  (minute == 1 ? QObject::tr("m") : QObject::tr("m"));
						}
					}
					else if (minute > 0) {
						result += QString::number(minute) + " " + (minute == 1 ? QObject::tr("m") : QObject::tr("m"));
						if (seconds > 0) {
							result += " " + QString::number(seconds) + " " +
									  (seconds == 1 ? QObject::tr("s") : QObject::tr("s"));
						}
					}
					else {
						result = QString::number(seconds) + " " + (seconds == 1 ? QObject::tr("s") : QObject::tr("s"));
					}

					return result;
				}

				static QString FormatFileSize(std::int64_t size) {
					const double kb = 1024.0;
					const double mb = kb * 1024.0;
					const double gb = mb * 1024.0;
					const double tb = gb * 1024.0;

					if (size >= tb) {
						return QString::number(size / tb, 'f', 2) + " TB";
					}
					else if (size >= gb) {
						return QString::number(size / gb, 'f', 2) + " GB";
					}
					else if (size >= mb) {
						return QString::number(size / mb, 'f', 2) + " MB";
					}
					else if (size >= kb) {
						return QString::number(size / kb, 'f', 2) + " KB";
					}
					else {
						return QString::number(size) + " B";
					}
				}

			   private:
				QString task_id_;
				TaskState task_state_{TaskState::kError};
				QString task_file_name_;
				QString task_save_path_;
                QString task_download_link_;
				QString task_error_code_;
				QString task_error_message_;
				std::int64_t task_total_size_{0};
				std::int64_t task_current_size_{0};
				std::int64_t task_download_speed_{0};
				std::int64_t task_connections_{0};
			};

			class DownloadTaskModel : public QAbstractListModel {
				Q_OBJECT
				Q_PROPERTY(int count READ GetTaskCount NOTIFY countChanged)  // 供 QML 直接绑定计数（P4）
			   public:
				enum Roles {
					kTaskId = Qt::UserRole + 1,
					kTaskState,
					kTaskFileName,
					kTaskSavePath,
					kTaskTotalSize,
					kTaskCurrentSize,
					kTaskDownloadSpeed,
					kTaskProgress,
					kTaskRemainingTime,
                    kTaskConnections,
                    kTaskDownloadLink,
					kTaskErrorCode,
					kTaskErrorMessage
				};

			   public:
				explicit DownloadTaskModel(QObject* parent = nullptr);
				~DownloadTaskModel() override;
				QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QHash<int, QByteArray> roleNames() const override;
				bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

				void AddTask(const DownloadTaskInfo& task);
				bool RemoveTask(int index);
				bool RemoveTaskById(const QString& task_id);
				bool UpdateTask(int index, const DownloadTaskInfo& task);
				bool UpdateTaskById(const QString& task_id, const DownloadTaskInfo& task);
				DownloadTaskInfo* GetTask(int index);
				DownloadTaskInfo* GetTaskById(const QString& task_id);
				QStringList GetTaskIds() const;
				void ClearAllTasks();
				int GetTaskCount() const;
				bool ContainsTask(const QString& task_id) const;

			   Q_SIGNALS:
				void countChanged();

			   private:
				QVector<DownloadTaskInfo> task_lists_;
				QHash<QString, QString> remove_task_id_;
				std::deque<QString> remove_order_;  // 墓碑插入顺序，配合上限做 FIFO 淘汰（M2）
				static constexpr int kMaxTombstones = 512;
				std::mutex mutex_;
			};
		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
