#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "singleton.hpp"

#include "ed2k_download_manager.h"
#include "ed2k_search_result_model.h"
#include "ed2k_server_list_model.h"
#include "ed2k_shared_file_model.h"

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
				// 分享域：上传统计(引擎侧为 u64，QML 侧用 double 承载避免溢出/精度问题)
				Q_PROPERTY(double uploadSpeedBps READ uploadSpeedBps NOTIFY shareStatsChanged)
				Q_PROPERTY(int uploadQueued READ uploadQueued NOTIFY shareStatsChanged)
				Q_PROPERTY(int uploadActive READ uploadActive NOTIFY shareStatsChanged)
				Q_PROPERTY(double totalUploaded READ totalUploaded NOTIFY shareStatsChanged)
			   public:
				~Ed2kManager() override;

				// 生命周期：由 mainwindow.cxx 在 BrowserManagerImpl::Init() 成功之后单独调用
				// (RegisterTypes 早于引擎初始化执行，不能在其中直接触发订阅，见 .cxx 说明)
				void Init();
				void UnInit();

				Q_INVOKABLE Ed2kSearchResultModel* GetSearchResultModel();
				Q_INVOKABLE Ed2kServerListModel* GetServerListModel();
				Q_INVOKABLE Ed2kSharedFileModel* GetSharedFileModel();

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

				// 分享域：目录以 SettingsManager 的 Ed2kSharedDirs('|' 分隔) 为唯一存储；
				// 增删即时写回设置并推送全量目录给引擎(照 SetSharedDirs 全量替换语义)。
				Q_INVOKABLE void AddSharedDir(const QString& dir);
				Q_INVOKABLE void RemoveSharedDir(const QString& dir);
				Q_INVOKABLE void RescanShares();
				Q_INVOKABLE QStringList GetSharedDirs();

				// 内部使用：RegisterTypes() 注册单例后立即调用一次，记录 QQmlEngine 供分享域
				// 读写设置时反射调用 "SettingsManager" QML 单例(原因见 .cxx SettingsSingleton
				// 的说明)。非 Q_INVOKABLE，QML 侧不可见。
				void AttachQmlEngine(QQmlEngine* engine) { qml_engine_ = engine; }

				bool searching() const { return searching_; }
				bool serverConnected() const { return server_connected_; }
				bool highId() const { return high_id_; }
				QString connectedServerName() const { return connected_server_name_; }
				bool kadRunning() const { return kad_running_; }
				int kadContacts() const { return kad_contacts_; }
				bool engineAvailable() const;
				double uploadSpeedBps() const { return upload_speed_bps_; }
				int uploadQueued() const { return upload_queued_; }
				int uploadActive() const { return upload_active_; }
				double totalUploaded() const { return total_uploaded_; }

			   Q_SIGNALS:
				void searchingChanged();
				void serverStateChanged();
				void kadStatusChanged();
				void searchFailed(const QString& error);
				// 连接服务器失败:引擎发布 connected=false 且带非空 error 时上抛,供 UI 弹错误提示
				void serverConnectFailed(const QString& error);
				// 引擎可用性变化:Init() 成功订阅后置真,QML 据此在引擎不可用时显示占位态
				void engineAvailableChanged();
				// 分享域：上传统计/分享文件列表更新
				void shareStatsChanged();
				// 分享操作(SetSharedDirs 等)失败，携带引擎侧错误信息
				void shareOpFailed(const QString& error);
				// server.met 更新完成(成功/失败均触发，供 UI 结束 loading 态)
				void serverMetUpdateFinished(bool ok, const QString& error);
				// 内部跨线程中转：引擎线程 Publish -> 主线程槽(Qt::QueuedConnection)
				void sigSearchPayload(const QString& json);
				void sigServerListPayload(const QString& json);
				void sigServerStatePayload(const QString& json);
				void sigKadStatusPayload(const QString& json);
				void sigShareStatePayload(const QString& json);
				void sigShareOpPayload(const QString& json);
				void sigServerMetPayload(const QString& json);

			   private:
				explicit Ed2kManager(QObject* parent = nullptr);

				void OnSearchPayload(const QString& json);
				void OnServerListPayload(const QString& json);
				void OnServerStatePayload(const QString& json);
				void OnKadStatusPayload(const QString& json);
				void OnShareStatePayload(const QString& json);
				void OnShareOpPayload(const QString& json);
				void OnServerMetPayload(const QString& json);
				// 搜索超时兜底：引擎前台请求串行化，若发起 Search/LoadMore 时恰有其它
				// 前台请求(如 ConnectServer)在途，引擎会静默丢弃本次请求，不会有任何
				// kEd2kSearchResult 到达——没有这个兜底 UI 会永久停在 searching 状态。
				void OnSearchTimeout();
				// 读取 SettingsManager 中的 Ed2kSharedDirs('|' 分隔) 并推送全量目录给引擎；
				// 供 AddSharedDir/RemoveSharedDir/RescanShares/Init() 复用。
				void PushSharedDirsToEngine();
				// 反射读写 "SettingsManager" QML 单例的 Ed2kSharedDirs(见 .cxx 详细说明)。
				QObject* SettingsSingleton() const;
				QString ReadSharedDirsSetting() const;
				void WriteSharedDirsSetting(const QString& value);

				Ed2kSearchResultModel* search_model_ = nullptr;
				Ed2kServerListModel* server_model_ = nullptr;
				Ed2kSharedFileModel* shared_model_ = nullptr;
				engine::Ed2kDownloadManager::Subscription search_sub_;
				engine::Ed2kDownloadManager::Subscription server_list_sub_;
				engine::Ed2kDownloadManager::Subscription server_state_sub_;
				engine::Ed2kDownloadManager::Subscription kad_status_sub_;
				engine::Ed2kDownloadManager::Subscription share_state_sub_;
				engine::Ed2kDownloadManager::Subscription share_op_sub_;
				engine::Ed2kDownloadManager::Subscription server_met_sub_;
				QTimer search_timeout_timer_;
				bool searching_ = false;
				bool server_connected_ = false;
				bool high_id_ = false;
				QString connected_server_name_;
				bool kad_running_ = false;
				int kad_contacts_ = 0;
				double upload_speed_bps_ = 0;
				int upload_queued_ = 0;
				int upload_active_ = 0;
				double total_uploaded_ = 0;
				// 分享状态每秒采样推送去重：负载与上一帧一致时跳过解析与模型重置(见 OnShareStatePayload)
				QString last_share_payload_;
				// 非持有:由 AttachQmlEngine() 记录，生命周期由调用方(mainwindow.cxx/测试用例)保证
				QQmlEngine* qml_engine_ = nullptr;
			};

			void RegisterTypes(QQmlEngine* engine);

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl
