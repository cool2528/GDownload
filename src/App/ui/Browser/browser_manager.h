#pragma once
#include "IBrowserManager.h"
#include <QtQml/qqml.h>
#include <atomic>
#include <QHash>
#include <QObject>
#include <QVariantMap>
#include "Aria2CManager/aria2c_http_rpc_client.h"
#include "Aria2CManager/aria2c_manager.h"
#include "Parser/file_preview_model.h"
#include "download_task_model.h"
#include "ed2k_download_manager.h"
#include "task_deletion_result.h"
#include "singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {
	namespace ui {
		namespace browser {

			Q_NAMESPACE
			// 下载管理实现类,继承 IBrowserManager 纯虚接口与 Singleton<BrowserManagerImpl>
			// QObject 必须置于基类列表首位,moc 依此链接 staticMetaObject;IBrowserManager 为非 Q_OBJECT 纯虚接口
			// 保留 Q_INVOKABLE 以确保 moc 注册元对象信息,QML 侧方法调用不受影响
			class BrowserManagerImpl : public QObject, public IBrowserManager, public Singleton<BrowserManagerImpl> {
				Q_OBJECT
				Q_PROPERTY(bool engineAvailable READ engineAvailable NOTIFY engineAvailabilityChanged)
				Q_PROPERTY(QString engineUnavailableMessage READ engineUnavailableMessage NOTIFY engineAvailabilityChanged)
				SINGLETON_DECLARE(BrowserManagerImpl)
				QML_SINGLETON
			   public:
				static BrowserManagerImpl* create(QQmlEngine*, QJSEngine*);

			   public:
				~BrowserManagerImpl() override;

				Q_INVOKABLE void SyncTrackersServerlist() override;

				Q_INVOKABLE DownloadTaskModel* GetActiveDownloadModel() override;

				Q_INVOKABLE DownloadTaskModel* GetStopedDownloadModel() override;

				Q_INVOKABLE DownloadTaskModel* GetWaitingDownloadModel() override;

                Q_INVOKABLE bool AddHttpTask(const QVariantList& urls, const QVariantMap& options) override;

                Q_INVOKABLE bool AddTorrentTask(const QString& tarrent, const QVariantMap& options) override;

                Q_INVOKABLE bool AddMetalinkTask(const QString& metalink, const QVariantMap& options) override;

                Q_INVOKABLE bool PauseTask(int page_index, const QString& gid) override;

                Q_INVOKABLE bool PauseAllTask(int page_index) override;

                Q_INVOKABLE bool ForcePauseTask(int page_index, const QString& gid) override;

                Q_INVOKABLE bool ForcePauseAllTask() override;

                Q_INVOKABLE bool UnpauseTask(int page_index, const QString& gid) override;

                Q_INVOKABLE bool UnpauseAllTask(int page_index) override;

                Q_INVOKABLE QVariantMap RemoveTask(int page_index, const QString& gid, bool is_remove_file = false) override;

                Q_INVOKABLE QVariantMap RemoveAllTask(int page_index, bool is_remove_file = false) override;

				Q_INVOKABLE bool ForceRemoveTask(const QString& gid) override;

				Q_INVOKABLE bool RemoveDownloadResult(const QString& gid) override;

				Q_INVOKABLE bool PurgeDownloadResult() override;

				Q_INVOKABLE bool ChangeOption(const QString& gid, const QVariantMap& options) override;

				Q_INVOKABLE bool ChangeGlobalOption(const QVariantMap& options) override;

				Q_INVOKABLE void OpenFileLocation(const QString& file_path) override;

				Q_INVOKABLE bool RetryTask(const QString& gid) override;

                Q_INVOKABLE QVariantMap RemoveStopTask(const QString& gid, bool is_remove_file = true) override;

                Q_INVOKABLE QVariantMap RemoveStopTask(int index, bool is_remove_file = true) override;

                Q_INVOKABLE QVariantMap RemoveAllStopTask(bool is_remove_file = false) override;

                Q_INVOKABLE void RefreshTaskList(int page_index) override;
                Q_INVOKABLE parser::FilePreviewModel* GetFilePreviewModel(const QString& file_path) override;

                // 解析多行 ed2k 链接文本为文件预览模型,供 QML 新建任务对话框展示(Task 6 使用)
                Q_INVOKABLE parser::FilePreviewModel* ParseEd2kLinks(const QString& text);
                // 返回同一文本中的有效 ed2k 链接原文列表,与 ParseEd2kLinks 产出的预览行一一对应。
                // QML 勾选行索引依赖两者数量/顺序完全一致,故必须共用同一 C++ 解析器,
                // 禁止在 QML 侧重新实现过滤规则(规则分歧会导致勾选 A 却下载 B)
                Q_INVOKABLE QStringList GetValidEd2kLinks(const QString& text);
                // 批量提交 ed2k 下载任务,options 目前仅识别 "dir"(保存目录);任一成功即返回 true
                Q_INVOKABLE bool AddEd2kTask(const QVariantList& links, const QVariantMap& options);

				bool Init();
				void UnInit();
				bool engineAvailable() const { return engine_available_; }
				QString engineUnavailableMessage() const { return engine_unavailable_message_; }
				void SetEngineUnavailable(const QString& message);

				// 后续实例启动时请求激活主窗口（单实例防多开）；QML 侧连接 sigActivateWindow 提升窗口
				void TriggerActivateWindow();

			   public:
			   Q_SIGNALS:
				void sigErrorMessage(const QString& error) override;
				void sigUpdateTasksMessage(const DownloadTaskInfo& info) override;
                void sigUpdateActiveProgress(const double& progress) override;
				void sigUpdateSyncServerList(const QString& list) override;
				void sigTrackerUpdateStatus(const QString& status) override;
				void engineAvailabilityChanged();
				// 后续实例启动的窗口激活请求，QML 侧连接以提升主窗口
				void sigActivateWindow();

			   protected:
				// 构造函数为 protected:允许测试替身 FakeBrowserManager 继承并构造基类子对象
				// Singleton<BrowserManagerImpl> 作为 friend 仍可访问(friend 绕过访问控制)
				// 外部代码仍无法直接构造(protected 对非派生类不可见),单例约束保持不变
				explicit BrowserManagerImpl(QObject* parent = nullptr);
			   private:
				// removal_attempts: 本地文件删除的尝试次数上限,批量删除传 1
				// 禁用重试退避,避免多个被占用文件叠加冻结 UI 线程
				TaskDeletionResult RemoveTaskResult(int page_index, const QString& gid, bool is_remove_file,
													int removal_attempts = 3);
				TaskDeletionResult RemoveStopTaskResult(const QString& gid, bool is_remove_file,
														int removal_attempts = 3);
				void OnHandleAria2Message(const std::string& msg);
                void OnHandleAria2ActiveProgress(const std::string& msg);
				void OnHandleTrackerUpdateStatus(const std::string& msg);
				// ed2k 事件转发:两者均在 ed2k 引擎网络线程回调,遵循与 aria2 相同的跨线程 Q_EMIT 模式
				void OnHandleEd2kActiveProgress(const std::string& msg);
				void OnHandleEd2kTaskState(const std::string& msg);
				void InitDownloadHistoryCache() const;
				// 重启续传:对历史记录中状态为失败(kError)的 ed2k 任务重新调用 AddEd2kTask,
				// 依赖引擎 .part.met 实现断点续传;须在 ed2k 引擎初始化成功之后调用
				void RestoreEd2kDownloadHistory() const;
				static gdl::cache::DownloadRecord DownloadTaskInfoToRecord(const DownloadTaskInfo& info);
				static DownloadTaskInfo DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record);
				static DownloadTaskInfo Aria2QueryByGidTaskInfo(const std::string& gid);
				static QString Aria2QueryByGidErrorMessage(const std::string& gid);
				// 下载完成后操作相关方法
				void ExecutePostDownloadAction(const DownloadTaskInfo& task, int actionType, const QString& customCommand);
				void PlayNotificationSound();
				void ExecuteCustomCommand(const QString& command, const QString& filePath, const QString& dir, const QString& gid);

			   private:
				std::unique_ptr<DownloadTaskModel> active_model_{nullptr};
				std::unique_ptr<DownloadTaskModel> stopped_model_{nullptr};
				std::unique_ptr<DownloadTaskModel> waiting_model_{nullptr};
                engine::Subscription aria2_responce_subcription_{nullptr};
                engine::Subscription aria2_active_progress_subcription_{nullptr};
				engine::Subscription aria2_sync_server_list_subcription_{nullptr};
				engine::Subscription aria2_tracker_update_status_subscription_{nullptr};
				engine::Ed2kDownloadManager::Subscription ed2k_active_progress_subscription_{nullptr};
				engine::Ed2kDownloadManager::Subscription ed2k_task_state_subscription_{nullptr};
				// 启动链订阅:server.met 首个更新结果到达后触发自动连接(见 Init 内注释)
				engine::Ed2kDownloadManager::Subscription ed2k_server_met_boot_subscription_{nullptr};
				// 启动链一次性标志:置位后仅首个 server.met 结果触发自动连接;
				// 回调在引擎网络线程执行,与主线程 Init/UnInit 存在跨线程读写,用原子量
				std::atomic_bool ed2k_boot_auto_connect_pending_{false};
				// ed2k 任务最近一次采样信息缓存,用于状态事件(payload 仅含 id/state/error)补全文件名/大小等字段。
				// 仅在 ed2k 引擎网络线程读写:OnHandleEd2kActiveProgress/OnHandleEd2kTaskState 均由该库内部
				// 单线程 io_context 串行派发(见 PubSubSystem::Publish 的 post 语义),两者不会并发访问此表,无需加锁。
				QHash<QString, DownloadTaskInfo> ed2k_task_cache_;
				bool engine_available_{true};
				QString engine_unavailable_message_;
			};
			// 类型别名:保 4 处现有 BrowserManager::Instance() 调用点零改动
			using BrowserManager = BrowserManagerImpl;
			void RegisterTypes(QQmlEngine* engine);
		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
