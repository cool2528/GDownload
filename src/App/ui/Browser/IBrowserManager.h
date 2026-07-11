#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "Parser/file_preview_model.h"
#include "download_task_model.h"

namespace gdl {
	namespace ui {
		namespace browser {

			// BrowserManager 的纯虚接口
			// 用于在测试代码中注入 FakeBrowserManager 替身,实现 UI 逻辑的隔离测试
			// 实现类 BrowserManagerImpl 通过 Q_OBJECT/Q_INVOKABLE 提供元对象信息,QML 侧不受影响
			class IBrowserManager {
			   public:
				virtual ~IBrowserManager() = default;

				// 同步 Tracker 服务器列表
				virtual void SyncTrackersServerlist() = 0;

				// 获取各分页下载任务模型
				virtual DownloadTaskModel* GetActiveDownloadModel() = 0;
				virtual DownloadTaskModel* GetStopedDownloadModel() = 0;
				virtual DownloadTaskModel* GetWaitingDownloadModel() = 0;

				// 添加下载任务
				virtual bool AddHttpTask(const QVariantList& urls, const QVariantMap& options) = 0;
				virtual bool AddTorrentTask(const QString& tarrent, const QVariantMap& options) = 0;
				virtual bool AddMetalinkTask(const QString& metalink, const QVariantMap& options) = 0;

				// 暂停/恢复任务
				virtual bool PauseTask(int page_index, const QString& gid) = 0;
				virtual bool PauseAllTask(int page_index) = 0;
				virtual bool ForcePauseTask(int page_index, const QString& gid) = 0;
				virtual bool ForcePauseAllTask() = 0;
				virtual bool UnpauseTask(int page_index, const QString& gid) = 0;
				virtual bool UnpauseAllTask(int page_index) = 0;

				// 删除任务
				virtual QVariantMap RemoveTask(int page_index, const QString& gid, bool is_remove_file = false) = 0;
				virtual QVariantMap RemoveAllTask(int page_index, bool is_remove_file = false) = 0;
				virtual bool ForceRemoveTask(const QString& gid) = 0;
				virtual bool RemoveDownloadResult(const QString& gid) = 0;
				virtual bool PurgeDownloadResult() = 0;

				// 修改下载选项
				virtual bool ChangeOption(const QString& gid, const QVariantMap& options) = 0;
				virtual bool ChangeGlobalOption(const QVariantMap& options) = 0;

				// 打开文件所在目录
				virtual void OpenFileLocation(const QString& file_path) = 0;

				// 已停止任务的操作
				virtual bool RetryTask(const QString& gid) = 0;
				virtual QVariantMap RemoveStopTask(const QString& gid, bool is_remove_file = true) = 0;
				virtual QVariantMap RemoveStopTask(int index, bool is_remove_file = true) = 0;
				virtual QVariantMap RemoveAllStopTask(bool is_remove_file = false) = 0;

				// 刷新任务列表与获取文件预览模型
				virtual void RefreshTaskList(int page_index) = 0;
				virtual parser::FilePreviewModel* GetFilePreviewModel(const QString& file_path) = 0;

				// 信号(纯虚声明,由 BrowserManagerImpl 的 Q_SIGNALS 块实现并提供元对象信息)
				virtual void sigErrorMessage(const QString& error) = 0;
				virtual void sigUpdateTasksMessage(const DownloadTaskInfo& info) = 0;
				virtual void sigUpdateActiveProgress(const double& progress) = 0;
				virtual void sigUpdateSyncServerList(const QString& list) = 0;
				virtual void sigTrackerUpdateStatus(const QString& status) = 0;
			};

		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
