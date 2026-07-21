#include "ed2k_manager.h"

#include <nlohmann/json.hpp>

#include <QDir>
#include <QQmlEngine>
#include <QtQml/qqml.h>

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
				shared_model_ = new Ed2kSharedFileModel(this);
				// 引擎线程 -> 主线程：全部经 QueuedConnection 中转信号
				connect(this, &Ed2kManager::sigSearchPayload, this, &Ed2kManager::OnSearchPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigServerListPayload, this, &Ed2kManager::OnServerListPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigServerStatePayload, this, &Ed2kManager::OnServerStatePayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigKadStatusPayload, this, &Ed2kManager::OnKadStatusPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigShareStatePayload, this, &Ed2kManager::OnShareStatePayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigShareOpPayload, this, &Ed2kManager::OnShareOpPayload,
						Qt::QueuedConnection);
				connect(this, &Ed2kManager::sigServerMetPayload, this, &Ed2kManager::OnServerMetPayload,
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
				share_state_sub_ = mgr.SubscriptionEd2kMessage(kEd2kShareState, [this](const std::string& msg) {
					Q_EMIT sigShareStatePayload(QString::fromStdString(msg));
				});
				share_op_sub_ = mgr.SubscriptionEd2kMessage(kEd2kShareOpResult, [this](const std::string& msg) {
					Q_EMIT sigShareOpPayload(QString::fromStdString(msg));
				});
				server_met_sub_ = mgr.SubscriptionEd2kMessage(kEd2kServerMetResult, [this](const std::string& msg) {
					Q_EMIT sigServerMetPayload(QString::fromStdString(msg));
				});
				// 应用重启后恢复分享：若设置里已有分享目录，Init() 订阅就绪后立即推给引擎重新分享
				if (!GetSharedDirs().isEmpty()) {
					PushSharedDirsToEngine();
				}
				// 订阅全部就绪,引擎已可用:通知 QML 由占位态切回正常内容(engineAvailable 现为 NOTIFY 属性)
				Q_EMIT engineAvailableChanged();
			}

			void Ed2kManager::UnInit() {
				// 未曾 Init(engine 未运行/测试模式)时不触碰 Ed2kDownloadManager 单例，
				// 避免仅为了 UnInit 就意外构造出引擎对象。
				if (!search_sub_ && !server_list_sub_ && !server_state_sub_ && !kad_status_sub_ && !share_state_sub_ &&
					!share_op_sub_ && !server_met_sub_) {
					return;
				}
				auto& mgr = engine::Ed2kDownloadManager::Instance();
				for (auto* sub : {&search_sub_, &server_list_sub_, &server_state_sub_, &kad_status_sub_,
								  &share_state_sub_, &share_op_sub_, &server_met_sub_}) {
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

			Ed2kSharedFileModel* Ed2kManager::GetSharedFileModel() {
				QQmlEngine::setObjectOwnership(shared_model_, QQmlEngine::CppOwnership);
				return shared_model_;
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

			// 反射访问 "SettingsManager" QML 单例，而非直接调用 settings::Settings::Instance()。
			//
			// 原因：本文件(ed2k_manager.cxx)被 tests/qml_ui/integration/CMakeLists.txt 里
			// qml_ui_tst_ed2k_center 目标以 target_sources 直接编译进轻量测试可执行文件——
			// 该目标只链接 Ed2kEngine/GDLCore，不链接真实 SettingsImpl(SettingsImpl 的
			// staticMetaObject/构造析构/SetValue 模板实例等符号仅存在于 gdownload.exe，
			// 且其 .cxx 还依赖 Aria2CManager，详见 tests/qml_ui/support/FakeSettingsManager.cxx
			// 顶部注释)。若此处直接引用 SettingsImpl 符号，该测试目标会 LNK2019 无法链接。
			// 改为经 QQmlEngine::singletonInstance 取回已注册的 "SettingsManager" 单例
			// (生产环境为真实 SettingsImpl，qml_ui 测试环境为 FakeSettingsManager，二者
			// Get/SetEd2kSharedDirs 签名一致，见 setting.h 的 SETTING_PROPERTY 宏与
			// FakeSettingsManager.h 的 FAKE_SETTING 宏)，用 QMetaObject::invokeMethod 反射调用，
			// 只依赖 QObject 元对象系统，不产生编译期符号依赖。
			QObject* Ed2kManager::SettingsSingleton() const {
				if (!qml_engine_) return nullptr;
				const int type_id = qmlTypeId(GEXPORT_MODULE_URL, 1, 0, "SettingsManager");
				if (type_id < 0) return nullptr;
				return qml_engine_->singletonInstance<QObject*>(type_id);
			}

			QString Ed2kManager::ReadSharedDirsSetting() const {
				auto* settings_obj = SettingsSingleton();
				if (!settings_obj) return QString();
				QString value;
				QMetaObject::invokeMethod(settings_obj, "GetEd2kSharedDirs", Qt::DirectConnection,
										  Q_RETURN_ARG(QString, value));
				return value;
			}

			void Ed2kManager::WriteSharedDirsSetting(const QString& value) {
				auto* settings_obj = SettingsSingleton();
				if (!settings_obj) return;
				QMetaObject::invokeMethod(settings_obj, "SetEd2kSharedDirs", Qt::DirectConnection,
										  Q_ARG(QString, value));
			}

			QStringList Ed2kManager::GetSharedDirs() {
				// 分享目录设置存储约定：单个 QString，以 '|' 分隔多个目录(Windows 路径合法字符中
				// 不含 '|'，跨平台安全)，形如 "D:/a|E:/b"；此处解析，写回时统一 join('|')。
				const QString raw = ReadSharedDirsSetting();
				QStringList dirs;
				for (const auto& part : raw.split(QLatin1Char('|'), Qt::SkipEmptyParts)) {
					const QString trimmed = part.trimmed();
					if (!trimmed.isEmpty()) {
						dirs.append(trimmed);
					}
				}
				return dirs;
			}

			void Ed2kManager::AddSharedDir(const QString& dir) {
				// 路径归一化用 QDir::cleanPath，去重按归一化后的字符串精确比较(不做大小写折叠)。
				const QString cleaned = QDir::cleanPath(dir.trimmed());
				if (cleaned.isEmpty()) return;
				QStringList dirs = GetSharedDirs();
				if (dirs.contains(cleaned)) return;
				dirs.append(cleaned);
				WriteSharedDirsSetting(dirs.join(QLatin1Char('|')));
				PushSharedDirsToEngine();
			}

			void Ed2kManager::RemoveSharedDir(const QString& dir) {
				const QString cleaned = QDir::cleanPath(dir.trimmed());
				QStringList dirs = GetSharedDirs();
				if (!dirs.removeOne(cleaned)) return;
				WriteSharedDirsSetting(dirs.join(QLatin1Char('|')));
				PushSharedDirsToEngine();
			}

			void Ed2kManager::RescanShares() { PushSharedDirsToEngine(); }

			void Ed2kManager::PushSharedDirsToEngine() {
				const QStringList dirs = GetSharedDirs();
				std::vector<std::string> std_dirs;
				std_dirs.reserve(static_cast<std::size_t>(dirs.size()));
				for (const auto& d : dirs) {
					std_dirs.push_back(d.toStdString());
				}
				engine::Ed2kDownloadManager::Instance().SetSharedDirs(std_dirs);
			}

			void Ed2kManager::OnSearchPayload(const QString& json) {
				// 防迟到守卫：搜索超时兜底(OnSearchTimeout)已把 searching_ 复位为 false 并
				// 上抛 searchFailed；此后引擎才送达的迟到结果直接丢弃，不再重置模型/复位状态。
				if (!searching_) {
					return;
				}
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

			void Ed2kManager::OnShareStatePayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					upload_speed_bps_ = static_cast<double>(doc.value("speed_bps", 0ull));
					upload_queued_ = static_cast<int>(doc.value("queued", 0u));
					upload_active_ = static_cast<int>(doc.value("active", 0u));
					total_uploaded_ = static_cast<double>(doc.value("total_uploaded", 0ull));
					Q_EMIT shareStatsChanged();
					QVector<Ed2kSharedItem> items;
					for (const auto& j : doc.value("files", nlohmann::json::array())) {
						Ed2kSharedItem item;
						item.name = QString::fromStdString(j.value("name", std::string()));
						item.path = QString::fromStdString(j.value("path", std::string()));
						item.size = static_cast<qint64>(j.value("size", 0ull));
						item.hash_hex = QString::fromStdString(j.value("hash", std::string())).toUpper();
						item.uploaded = static_cast<qint64>(j.value("uploaded", 0ull));
						item.requests = j.value("requests", 0u);
						if (item.name.isEmpty() || item.size <= 0) continue;
						items.append(item);
					}
					shared_model_->ResetItems(items);
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k share state payload: {}", e.what());
				}
			}

			void Ed2kManager::OnShareOpPayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					if (!doc.value("ok", false)) {
						Q_EMIT shareOpFailed(QString::fromStdString(doc.value("error", std::string())));
					}
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k share op payload: {}", e.what());
				}
			}

			void Ed2kManager::OnServerMetPayload(const QString& json) {
				try {
					const auto doc = nlohmann::json::parse(json.toStdString());
					const bool ok = doc.value("ok", false);
					const auto error = QString::fromStdString(doc.value("error", std::string()));
					Q_EMIT serverMetUpdateFinished(ok, error);
				} catch (const std::exception& e) {
					LOG_ERR("Failed to parse ed2k server.met result payload: {}", e.what());
				}
			}

			void Ed2kManager::OnSearchTimeout() {
				if (!searching_) return;
				searching_ = false;
				Q_EMIT searchingChanged();
				Q_EMIT searchFailed(tr("Search timed out"));
			}

			void RegisterTypes(QQmlEngine* engine) {
				// 只注册 QML 单例：Instance() 在此被首次构造，但构造函数很轻(仅 new 三个模型 +
				// 连接信号)，不触碰 ed2k 引擎。真正的 PubSub 订阅(Init())必须等
				// mainwindow.cxx 里 BrowserManagerImpl::Init() 把 ed2k 引擎跑起来之后才能调用，
				// 否则引擎 pubsub 尚未创建，此处提前注册的话 RegisterTypes 里调 Init() 会订阅到
				// 一个还没起来的引擎上——所以 Init() 单独放在 mainwindow.cxx 里调用。
				qmlRegisterSingletonInstance<Ed2kManager>(GEXPORT_MODULE_URL, 1, 0, "Ed2kManager",
														  &Ed2kManager::Instance());
				// 记录 engine 供分享域反射调用 "SettingsManager" 单例使用(见 SettingsSingleton
				// 说明)。调用时 "SettingsManager" 必须已在同一 engine 上注册——mainwindow.cxx/
				// 集成测试均先注册 SettingsManager 再调用本函数，顺序已保证。
				Ed2kManager::Instance().AttachQmlEngine(engine);
			}

		}  // namespace ed2k
	}  // namespace ui
}  // namespace gdl
