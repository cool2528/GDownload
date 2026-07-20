#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "singleton.hpp"

#include "ed2k_download_manager.h"
#include "ed2k_search_result_model.h"
#include "ed2k_server_list_model.h"

class QQmlEngine;

namespace gdl {
	namespace ui {
		namespace ed2k {

			// eD2k 专属域(搜索/服务器/Kad 状态)的 QML 单例。
			// 引擎结果经 PubSub 在引擎网络线程回调，本类用 Qt::QueuedConnection 信号转回主线程
			// 后更新模型/属性(照 BrowserManagerImpl 处理 ed2k 下载事件的既有模式)。
			class Ed2kManager : public QObject, public Singleton<Ed2kManager> {
				Q_OBJECT
				SINGLETON_DECLARE(Ed2kManager)
				Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
				Q_PROPERTY(bool serverConnected READ serverConnected NOTIFY serverStateChanged)
				Q_PROPERTY(bool highId READ highId NOTIFY serverStateChanged)
				Q_PROPERTY(QString connectedServerName READ connectedServerName NOTIFY serverStateChanged)
				Q_PROPERTY(bool kadRunning READ kadRunning NOTIFY kadStatusChanged)
				Q_PROPERTY(int kadContacts READ kadContacts NOTIFY kadStatusChanged)
				Q_PROPERTY(bool engineAvailable READ engineAvailable NOTIFY engineAvailableChanged)
			   public:
				~Ed2kManager() override;

				// 生命周期：由 mainwindow.cxx 在 BrowserManagerImpl::Init() 成功之后单独调用
				// (RegisterTypes 早于引擎初始化执行，不能在其中直接触发订阅，见 .cxx 说明)
				void Init();
				void UnInit();

				Q_INVOKABLE Ed2kSearchResultModel* GetSearchResultModel();
				Q_INVOKABLE Ed2kServerListModel* GetServerListModel();

				Q_INVOKABLE void StartSearch(const QString& keyword, int fileType, double minSizeBytes,
											 int source);
				Q_INVOKABLE void LoadMore();
				Q_INVOKABLE void ConnectServer(const QString& ip, int port);
				Q_INVOKABLE void DisconnectServer();
				Q_INVOKABLE void RefreshServers();
				Q_INVOKABLE void AddServer(const QString& ip, int port, const QString& name);
				Q_INVOKABLE void RemoveServer(const QString& ip, int port);
				Q_INVOKABLE void UpdateServersFromUrl(const QString& url);
				Q_INVOKABLE void RefreshKadStatus();

				bool searching() const { return searching_; }
				bool serverConnected() const { return server_connected_; }
				bool highId() const { return high_id_; }
				QString connectedServerName() const { return connected_server_name_; }
				bool kadRunning() const { return kad_running_; }
				int kadContacts() const { return kad_contacts_; }
				bool engineAvailable() const;

			   Q_SIGNALS:
				void searchingChanged();
				void serverStateChanged();
				void kadStatusChanged();
				void searchFailed(const QString& error);
				// 连接服务器失败:引擎发布 connected=false 且带非空 error 时上抛,供 UI 弹错误提示
				void serverConnectFailed(const QString& error);
				// 引擎可用性变化:Init() 成功订阅后置真,QML 据此在引擎不可用时显示占位态
				void engineAvailableChanged();
				// 内部跨线程中转：引擎线程 Publish -> 主线程槽(Qt::QueuedConnection)
				void sigSearchPayload(const QString& json);
				void sigServerListPayload(const QString& json);
				void sigServerStatePayload(const QString& json);
				void sigKadStatusPayload(const QString& json);

			   private:
				explicit Ed2kManager(QObject* parent = nullptr);

				void OnSearchPayload(const QString& json);
				void OnServerListPayload(const QString& json);
				void OnServerStatePayload(const QString& json);
				void OnKadStatusPayload(const QString& json);
				// 搜索超时兜底：引擎前台请求串行化，若发起 Search/LoadMore 时恰有其它
				// 前台请求(如 ConnectServer)在途，引擎会静默丢弃本次请求，不会有任何
				// kEd2kSearchResult 到达——没有这个兜底 UI 会永久停在 searching 状态。
				void OnSearchTimeout();

				Ed2kSearchResultModel* search_model_ = nullptr;
				Ed2kServerListModel* server_model_ = nullptr;
				engine::Ed2kDownloadManager::Subscription search_sub_;
				engine::Ed2kDownloadManager::Subscription server_list_sub_;
				engine::Ed2kDownloadManager::Subscription server_state_sub_;
				engine::Ed2kDownloadManager::Subscription kad_status_sub_;
				QTimer search_timeout_timer_;
				bool searching_ = false;
				bool server_connected_ = false;
				bool high_id_ = false;
				QString connected_server_name_;
				bool kad_running_ = false;
				int kad_contacts_ = 0;
			};

			void RegisterTypes(QQmlEngine* engine);

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl
