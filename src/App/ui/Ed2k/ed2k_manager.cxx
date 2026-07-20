#include "ed2k_manager.h"

#include <nlohmann/json.hpp>

#include <QQmlEngine>

#include "Definitions/appDef.h"
#include "ed2k_engine_def.h"
#include "logger.h"

namespace gdl {
	namespace ui {
		namespace ed2k {

			namespace {
				// 搜索/翻页超时兜底：engine 层前台请求串行化，命中守卫时会静默丢弃
				// 请求(无任何 payload 发布)，此处给一个上限，超时则复位 UI 状态。
				constexpr int kSearchTimeoutMs = 60000;
			}  // namespace

			Ed2kManager::Ed2kManager(QObject* parent) : QObject(parent) {
				search_model_ = new Ed2kSearchResultModel(this);
				server_model_ = new Ed2kServerListModel(this);
				// 引擎线程 -> 主线程：全部经 QueuedConnection 中转信号
				connect(this, &Ed2kManager::sigSearchPayload, this, &Ed2kManager::OnSearchPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigServerListPayload, this, &Ed2kManager::OnServerListPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigServerStatePayload, this, &Ed2kManager::OnServerStatePayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigKadStatusPayload, this, &Ed2kManager::OnKadStatusPayload,
						Qt::QueuedConnection);
				search_timeout_timer_.setSingleShot(true);
				search_timeout_timer_.setInterval(kSearchTimeoutMs);
				connect(&search_timeout_timer_, &QTimer::timeout, this, &Ed2kManager::OnSearchTimeout);
			}

			Ed2kManager::~Ed2kManager() { UnInit(); }

			bool Ed2kManager::engineAvailable() const {
				return engine::Ed2kDownloadManager::Instance().EngineIsRunning();
			}

			void Ed2kManager::Init() {
				if (!engineAvailable()) {
					LOG_WARN("Ed2kManager init skipped: ed2k engine is not running");
					return;
				}
				auto& mgr = engine::Ed2kDownloadManager::Instance();
				search_sub_ = mgr.SubscriptionEd2kMessage(kEd2kSearchResult, [this](const std::string& msg) {
					Q_EMIT sigSearchPayload(QString::fromStdString(msg));
				});
				server_list_sub_ = mgr.SubscriptionEd2kMessage(kEd2kServerList, [this](const std::string& msg) {
					Q_EMIT sigServerListPayload(QString::fromStdString(msg));
				});
				server_state_sub_ = mgr.SubscriptionEd2kMessage(kEd2kServerState, [this](const std::string& msg) {
					Q_EMIT sigServerStatePayload(QString::fromStdString(msg));
				});
				kad_status_sub_ = mgr.SubscriptionEd2kMessage(kEd2kKadStatus, [this](const std::string& msg) {
					Q_EMIT sigKadStatusPayload(QString::fromStdString(msg));
				});
				// 订阅全部就绪,引擎已可用:通知 QML 由占位态切回正常内容(engineAvailable 现为 NOTIFY 属性)
				Q_EMIT engineAvailableChanged();
			}

			void Ed2kManager::UnInit() {
				// 未曾 Init(engine 未运行/测试模式)时不触碰 Ed2kDownloadManager 单例，
				// 避免仅为了 UnInit 就意外构造出引擎对象。
				if (!search_sub_ && !server_list_sub_ && !server_state_sub_ && !kad_status_sub_) {
					return;
				}
				auto& mgr = engine::Ed2kDownloadManager::Instance();
				for (auto* sub : {&search_sub_, &server_list_sub_, &server_state_sub_, &kad_status_sub_}) {
					if (*sub) {
						mgr.UnSubscribeEd2kMessage(*sub);
						sub->reset();
					}
				}
			}

			Ed2kSearchResultModel* Ed2kManager::GetSearchResultModel() {
				QQmlEngine::setObjectOwnership(search_model_, QQmlEngine::CppOwnership);
				return search_model_;
			}

			Ed2kServerListModel* Ed2kManager::GetServerListModel() {
				QQmlEngine::setObjectOwnership(server_model_, QQmlEngine::CppOwnership);
				return server_model_;
			}

			void Ed2kManager::StartSearch(const QString& keyword, int fileType, double minSizeBytes,
										  int source) {
				const QString trimmed = keyword.trimmed();
				if (trimmed.isEmpty() || searching_) return;
				searching_ = true;
				Q_EMIT searchingChanged();
				search_timeout_timer_.start();
				engine::Ed2kDownloadManager::Instance().Search(
					trimmed.toStdString(), fileType, static_cast<std::int64_t>(minSizeBytes), source);
			}

			void Ed2kManager::LoadMore() {
				if (searching_) return;
				searching_ = true;
				Q_EMIT searchingChanged();
				search_timeout_timer_.start();
				engine::Ed2kDownloadManager::Instance().SearchMore();
			}

			void Ed2kManager::ConnectServer(const QString& ip, int port) {
				engine::Ed2kDownloadManager::Instance().ConnectServer(ip.toStdString(),
																	  static_cast<std::uint16_t>(port));
			}
			void Ed2kManager::DisconnectServer() { engine::Ed2kDownloadManager::Instance().DisconnectServer(); }
			void Ed2kManager::RefreshServers() { engine::Ed2kDownloadManager::Instance().RequestServerList(); }
			void Ed2kManager::AddServer(const QString& ip, int port, const QString& name) {
				engine::Ed2kDownloadManager::Instance().AddServer(ip.toStdString(),
																  static_cast<std::uint16_t>(port),
																  name.toStdString());
			}
			void Ed2kManager::RemoveServer(const QString& ip, int port) {
				engine::Ed2kDownloadManager::Instance().RemoveServer(ip.toStdString(),
																	 static_cast<std::uint16_t>(port));
			}
			void Ed2kManager::UpdateServersFromUrl(const QString& url) {
				engine::Ed2kDownloadManager::Instance().UpdateServerMet(url.toStdString());
			}
			void Ed2kManager::RefreshKadStatus() { engine::Ed2kDownloadManager::Instance().RequestKadStatus(); }

			void Ed2kManager::OnSearchPayload(const QString& json) {
				search_timeout_timer_.stop();
				searching_ = false;
				Q_EMIT searchingChanged();
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					if (!doc.value("ok", false)) {
						Q_EMIT searchFailed(QString::fromStdString(doc.value("error", std::string())));
						return;
					}
					QVector<Ed2kSearchItem> items;
					for (const auto& j : doc.value("items", nlohmann::json::array())) {
						Ed2kSearchItem item;
						item.name = QString::fromStdString(j.value("name", std::string()));
						item.size = static_cast<qint64>(j.value("size", 0ull));
						item.hash_hex = QString::fromStdString(j.value("hash", std::string())).toUpper();
						item.sources = j.value("sources", 0u);
						item.complete_sources = j.value("complete_sources", 0u);
						if (item.name.isEmpty() || item.size <= 0) continue;
						items.append(item);
					}
					if (doc.value("append", false)) {
						search_model_->AppendItems(items);
					} else {
						search_model_->ResetItems(items);
					}
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k search payload: {}", e.what());
				}
			}

			void Ed2kManager::OnServerListPayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					QVector<Ed2kServerItem> items;
					for (const auto& j : doc.value("servers", nlohmann::json::array())) {
						Ed2kServerItem item;
						item.name = QString::fromStdString(j.value("name", std::string()));
						item.ip = QString::fromStdString(j.value("ip", std::string()));
						item.port = static_cast<quint16>(j.value("port", 0));
						item.users = j.value("users", 0u);
						item.files = j.value("files", 0u);
						item.max_users = j.value("max_users", 0u);
						item.connected = j.value("connected", false);
						items.append(item);
					}
					server_model_->ResetItems(items);
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k server list payload: {}", e.what());
				}
			}

			void Ed2kManager::OnServerStatePayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					server_connected_ = doc.value("connected", false);
					high_id_ = doc.value("high_id", false);
					const auto name = QString::fromStdString(doc.value("name", std::string()));
					if (!name.isEmpty() || !server_connected_) {
						connected_server_name_ = server_connected_ ? name : QString();
					}
					Q_EMIT serverStateChanged();
					// 连接失败:引擎在 ConnectServer 失败时发布 connected=false + 非空 error,
					// 主动断开(DisconnectServer)时 error 为空,故仅在 error 非空时上抛失败提示。
					const auto error = QString::fromStdString(doc.value("error", std::string()));
					if (!server_connected_ && !error.isEmpty()) {
						Q_EMIT serverConnectFailed(error);
					}
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k server state payload: {}", e.what());
				}
			}

			void Ed2kManager::OnKadStatusPayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					kad_running_ = doc.value("running", false);
					kad_contacts_ = static_cast<int>(doc.value("contacts", 0ull));
					Q_EMIT kadStatusChanged();
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k kad status payload: {}", e.what());
				}
			}

			void Ed2kManager::OnSearchTimeout() {
				if (!searching_) return;
				searching_ = false;
				Q_EMIT searchingChanged();
				Q_EMIT searchFailed(tr("Search timed out"));
			}

			void RegisterTypes(QQmlEngine* engine) {
				Q_UNUSED(engine)
				// 只注册 QML 单例：Instance() 在此被首次构造，但构造函数很轻(仅 new 两个模型 +
				// 连接信号)，不触碰 ed2k 引擎。真正的 PubSub 订阅(Init())必须等
				// mainwindow.cxx 里 BrowserManagerImpl::Init() 把 ed2k 引擎跑起来之后才能调用，
				// 否则引擎 pubsub 尚未创建，此处提前注册的话 RegisterTypes 里调 Init() 会订阅到
				// 一个还没起来的引擎上——所以 Init() 单独放在 mainwindow.cxx 里调用。
				qmlRegisterSingletonInstance<Ed2kManager>(GEXPORT_MODULE_URL, 1, 0, "Ed2kManager",
														  &Ed2kManager::Instance());
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl
