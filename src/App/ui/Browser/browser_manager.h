#pragma once
#include "IBrowserManager.h"
#include <QtQml/qqml.h>
#include <QObject>
#include <QVariantMap>
#include "Aria2CManager/aria2c_http_rpc_client.h"
#include "Aria2CManager/aria2c_manager.h"
#include "Parser/file_preview_model.h"
#include "download_task_model.h"
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

                Q_INVOKABLE bool RemoveTask(int page_index, const QString& gid, bool is_remove_file = false) override;

                Q_INVOKABLE bool RemoveAllTask(int page_index, bool is_remove_file = true) override;

				Q_INVOKABLE bool ForceRemoveTask(const QString& gid) override;

				Q_INVOKABLE bool RemoveDownloadResult(const QString& gid) override;

				Q_INVOKABLE bool PurgeDownloadResult() override;

				Q_INVOKABLE bool ChangeOption(const QString& gid, const QVariantMap& options) override;

				Q_INVOKABLE bool ChangeGlobalOption(const QVariantMap& options) override;

				Q_INVOKABLE void OpenFileLocation(const QString& file_path) override;

                Q_INVOKABLE bool RemoveStopTask(const QString& gid, bool is_remove_file = true) const override;

                Q_INVOKABLE bool RemoveStopTask(int index, bool is_remove_file = true) const override;

                Q_INVOKABLE bool RemoveAllStopTask(bool is_remove_file = true) const override;

                Q_INVOKABLE void RefreshTaskList(int page_index) override;
                Q_INVOKABLE parser::FilePreviewModel* GetFilePreviewModel(const QString& file_path) override;

				bool Init();
				void UnInit();

			   public:
			   Q_SIGNALS:
				void sigErrorMessage(const QString& error) override;
				void sigUpdateTasksMessage(const DownloadTaskInfo& info) override;
                void sigUpdateActiveProgress(const double& progress) override;
				void sigUpdateSyncServerList(const QString& list) override;
				void sigTrackerUpdateStatus(const QString& status) override;

			   protected:
				// 构造函数为 protected:允许测试替身 FakeBrowserManager 继承并构造基类子对象
				// Singleton<BrowserManagerImpl> 作为 friend 仍可访问(friend 绕过访问控制)
				// 外部代码仍无法直接构造(protected 对非派生类不可见),单例约束保持不变
				explicit BrowserManagerImpl(QObject* parent = nullptr);
			   private:
				void OnHandleAria2Message(const std::string& msg);
                void OnHandleAria2ActiveProgress(const std::string& msg);
				void OnHandleTrackerUpdateStatus(const std::string& msg);
				void InitDownloadHistoryCache() const;
				static gdl::cache::DownloadRecord DownloadTaskInfoToRecord(const DownloadTaskInfo& info);
				static DownloadTaskInfo DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record);
				static DownloadTaskInfo Aria2QueryByGidTaskInfo(const std::string& gid);
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
			};
			// 类型别名:保 4 处现有 BrowserManager::Instance() 调用点零改动
			using BrowserManager = BrowserManagerImpl;
			void RegisterTypes(QQmlEngine* engine);
		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
