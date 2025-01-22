#pragma once
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
			class BrowserManager : public QObject, public Singleton<BrowserManager> {
				Q_OBJECT
				SINGLETON_DECLARE(BrowserManager)
				QML_SINGLETON
			   public:
				static BrowserManager* create(QQmlEngine*, QJSEngine*);

			   public:
				~BrowserManager() override;
				Q_INVOKABLE DownloadTaskModel* GetActiveDownloadModel();

				Q_INVOKABLE DownloadTaskModel* GetStopedDownloadModel();

				Q_INVOKABLE DownloadTaskModel* GetWaitingDownloadModel();

				Q_INVOKABLE bool AddHttpTask(const QVariantList& urls, const QVariantMap& options);

				Q_INVOKABLE bool AddTorrentTask(const QString& tarrent, const QVariantMap& options);

				Q_INVOKABLE bool AddMetalinkTask(const QString& metalink, const QVariantMap& options);

                Q_INVOKABLE bool PauseTask(int page_index, const QString& gid);

                Q_INVOKABLE bool PauseAllTask(int page_index);

                Q_INVOKABLE bool ForcePauseTask(int page_index, const QString& gid);

                Q_INVOKABLE bool ForcePauseAllTask();

                Q_INVOKABLE bool UnpauseTask(int page_index, const QString& gid);

                Q_INVOKABLE bool UnpauseAllTask(int page_index);

                Q_INVOKABLE bool RemoveTask(int page_index, const QString& gid);

                Q_INVOKABLE bool RemoveAllTask(int page_index, bool is_remove_file = true);

				Q_INVOKABLE bool ForceRemoveTask(const QString& gid);

				Q_INVOKABLE bool RemoveDownloadResult(const QString& gid);

				Q_INVOKABLE bool PurgeDownloadResult();

				Q_INVOKABLE bool ChangeOption(const QString& gid, const QVariantMap& options);

				Q_INVOKABLE bool ChangeGlobalOption(const QVariantMap& options);

				Q_INVOKABLE void OpenFileLocation(const QString& file_path);

                Q_INVOKABLE bool RemoveStopTask(const QString& gid, bool is_remove_file = true) const;

                Q_INVOKABLE bool RemoveStopTask(int index, bool is_remove_file = true) const;

                Q_INVOKABLE bool RemoveAllStopTask(bool is_remove_file = true) const;

                Q_INVOKABLE void RefreshTaskList(int page_index);
                Q_INVOKABLE parser::FilePreviewModel* GetFilePreviewModel(const QString& file_path);
				bool Init();
				void UnInit();

			   public:
			   Q_SIGNALS:
				void sigErrorMessage(const QString& error);
				void sigUpdateTasksMessage(const DownloadTaskInfo& info);
                void sigUpdateActiveProgress(const double& progress);

			   private:
				explicit BrowserManager(QObject* parent = nullptr);
				void OnHandleAria2Message(const std::string& msg);
                void OnHandleAria2ActiveProgress(const std::string& msg);
				void InitDownloadHistoryCache() const;
				static gdl::cache::DownloadRecord DownloadTaskInfoToRecord(const DownloadTaskInfo& info);
				static DownloadTaskInfo DownloadRecordToTaskInfo(const gdl::cache::DownloadRecord& record);
				static DownloadTaskInfo Aria2QueryByGidTaskInfo(const std::string& gid);

			   private:
				std::unique_ptr<DownloadTaskModel> active_model_{nullptr};
				std::unique_ptr<DownloadTaskModel> stoped_model_{nullptr};
				std::unique_ptr<DownloadTaskModel> waiting_model_{nullptr};
                engine::Subscription aria2_responce_subcription_{nullptr};
                engine::Subscription aria2_active_progress_subcription_{nullptr};
			};
			void RegisterTypes(QQmlEngine* engine);
		}  // namespace browser
	}  // namespace ui
}  // namespace gdl
