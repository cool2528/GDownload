#include "plugin_market_manager.h"

#include <QQmlEngine>
#include <QTimer>
#include <chrono>
#include <thread>

#include "GDLCore/logger.h"
#include "PluginManager/plugin_manager.h"
#include "Settings/settings_manager.h"
#include "language/language_manager.h"

namespace gdl {
	namespace ui {
		namespace market {

			// ---------------- PluginMarketModel ----------------

			PluginMarketModel::PluginMarketModel(QObject* parent) : QAbstractListModel(parent) {}

			int PluginMarketModel::rowCount(const QModelIndex& parent) const {
				if (parent.isValid()) {
					return 0;
				}
				return rows_.size();
			}

			int PluginMarketModel::IndexOf(const QString& name) const {
				for (int i = 0; i < rows_.size(); ++i) {
					if (QString::fromStdString(rows_[i].item.meta.name) == name) {
						return i;
					}
				}
				return -1;
			}

			QVariant PluginMarketModel::data(const QModelIndex& index, int role) const {
				if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size()) {
					return {};
				}
				const auto& row	 = rows_[index.row()];
				const auto& meta = row.item.meta;
				switch (role) {
					case NameRole: return QString::fromStdString(meta.name);
					case DisplayNameRole: return QString::fromStdString(meta.display_name);
					case DescriptionRole: return QString::fromStdString(meta.description);
					case AuthorRole: return QString::fromStdString(meta.author);
					case VerifiedRole: return meta.verified;
					case TypeRole: return QString::fromStdString(meta.type);
					case LatestVersionRole: return QString::fromStdString(meta.latest);
					case InstalledVersionRole: return QString::fromStdString(row.item.installed_version);
					case StateRole:
						if (row.busy) {
							return 3;  // Busy
						}
						return static_cast<int>(row.item.state);
					case EnabledRole: return row.item.enabled;
					case TagsRole: {
						QStringList tags;
						for (const auto& t : meta.url_patterns) {
							tags << QString::fromStdString(t);
						}
						return tags;
					}
					case ProgressRole: return row.progress;
					case StageRole: return row.stage;
					default: return {};
				}
			}

			QHash<int, QByteArray> PluginMarketModel::roleNames() const {
				return {
					{NameRole, "name"},
					{DisplayNameRole, "displayName"},
					{DescriptionRole, "description"},
					{AuthorRole, "author"},
					{VerifiedRole, "verified"},
					{TypeRole, "type"},
					{LatestVersionRole, "latestVersion"},
					{InstalledVersionRole, "installedVersion"},
					{StateRole, "state"},
					{EnabledRole, "enabled"},
					{TagsRole, "tags"},
					{ProgressRole, "progress"},
					{StageRole, "stage"},
				};
			}

			void PluginMarketModel::SetItems(const std::vector<gdl::market::MarketItem>& items) {
				beginResetModel();
				rows_.clear();
				for (const auto& item : items) {
					Row row;
					row.item = item;
					rows_.push_back(std::move(row));
				}
				endResetModel();
			}

			void PluginMarketModel::SetBusy(const QString& name, int progress, const QString& stage) {
				int idx = IndexOf(name);
				if (idx < 0) {
					return;
				}
				rows_[idx].busy		= true;
				rows_[idx].progress = progress;
				rows_[idx].stage	= stage;
				auto model_idx		= index(idx, 0);
				Q_EMIT dataChanged(model_idx, model_idx, {StateRole, ProgressRole, StageRole});
			}

			void PluginMarketModel::ClearBusy(const QString& name) {
				int idx = IndexOf(name);
				if (idx < 0) {
					return;
				}
				rows_[idx].busy		= false;
				rows_[idx].progress = -1;
				rows_[idx].stage.clear();
				auto model_idx = index(idx, 0);
				Q_EMIT dataChanged(model_idx, model_idx, {StateRole, ProgressRole, StageRole});
			}

			// ---------------- PluginMarketManager ----------------

			PluginMarketManager::PluginMarketManager(QObject* parent) : QObject(parent) {
				model_ = new PluginMarketModel(this);
			}

			PluginMarketManager::~PluginMarketManager() = default;

			void PluginMarketManager::Initialize(const QString& plugins_dir, const QString& data_dir) {
				plugins_dir_ = plugins_dir.toStdString();
				data_dir_	 = data_dir.toStdString();
				service_	 = std::make_unique<gdl::market::PluginMarketService>(plugins_dir_, data_dir_);
				SyncLocale();
			}

			void PluginMarketManager::SyncLocale() {
				if (service_) {
					service_->SetLocale(
						language::LanguageManager::Instance().GetCurrentLanguage().toStdString());
				}
			}

			std::vector<std::string> PluginMarketManager::RegistryUrls() const {
				// registry.json 是可变索引：以 raw 为基准，ExpandMirrorUrls 自动补齐中国可达镜像
				// （ghproxy 系前缀 + jsDelivr 各节点）与用户自定义代理前缀
				const std::string repo = "cool2528/gdownload-plugin-registry";
				const std::vector<std::string> base = {
					"https://raw.githubusercontent.com/" + repo + "/main/registry.json",
				};
				const std::string proxy =
					settings::Settings::Instance().GetPluginSourceProxy().toStdString();
				return gdl::market::ExpandMirrorUrls(base, proxy);
			}

			void PluginMarketManager::SetBusy(bool busy) {
				bool prev = busy_.exchange(busy);
				if (prev != busy) {
					Q_EMIT busyChanged();
				}
			}

			void PluginMarketManager::RefreshModelFromLocal() {
				if (service_) {
					model_->SetItems(service_->ComputeItems());
				}
			}

			void PluginMarketManager::ReloadRuntime() {
				gdl::plugin::DownloadPluginManager::Instance().ReloadJsPlugins(plugins_dir_, data_dir_);
			}

			void PluginMarketManager::refresh() {
				if (!service_ || busy_.load()) {
					return;
				}
				// 离线优先：先用本地已装插件立即填充模型，再后台拉取注册表
				SyncLocale();  // 读取当前界面语言，用于本地化插件名称/描述
				service_->SetUserProxy(settings::Settings::Instance().GetPluginSourceProxy().toStdString());
				RefreshModelFromLocal();
				SetBusy(true);
				auto urls  = RegistryUrls();
				auto start = std::chrono::steady_clock::now();
				// 网络拉取放到线程池，完成后回到 UI 线程更新
				std::thread([this, urls, start]() {
					std::string err;
					bool ok = service_->FetchRegistry(urls, err);
					QString msg = QString::fromStdString(err);
					QMetaObject::invokeMethod(
						this,
						[this, ok, msg, start]() {
							if (ok) {
								RefreshModelFromLocal();
							}
							// 保证加载动画至少显示 kMinBusyMs，避免快速返回时一闪而过看不见
							auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
											   std::chrono::steady_clock::now() - start)
											   .count();
							constexpr int kMinBusyMs = 800;
							int remain				 = kMinBusyMs - static_cast<int>(elapsed);
							auto finish				 = [this, ok, msg]() {
								SetBusy(false);
								Q_EMIT refreshFinished(ok, msg);
							};
							if (remain > 0) {
								QTimer::singleShot(remain, this, finish);
							} else {
								finish();
							}
						},
						Qt::QueuedConnection);
				}).detach();
			}

			void PluginMarketManager::install(const QString& name) {
				if (!service_ || busy_.load()) {
					return;
				}
				// 需要先有 registry；若未拉取则本次不处理（UI 应先 refresh）
				std::string target_name = name.toStdString();
				std::string version;
				for (const auto& p : service_->registry()) {
					if (p.name == target_name) {
						version = p.latest;
					}
				}
				if (version.empty()) {
					Q_EMIT operationFinished(name, false, QStringLiteral("plugin not in registry"));
					return;
				}
				SetBusy(true);
				service_->SetUserProxy(settings::Settings::Instance().GetPluginSourceProxy().toStdString());
				model_->SetBusy(name, 0, QStringLiteral("starting"));
				std::thread([this, name, target_name, version]() {
					std::string err;
					bool ok = service_->InstallPlugin(
						target_name, version,
						[this, name](int pct, const std::string& stage) {
							QString qstage = QString::fromStdString(stage);
							QMetaObject::invokeMethod(
								this, [this, name, pct, qstage]() { model_->SetBusy(name, pct, qstage); },
								Qt::QueuedConnection);
						},
						err);
					QString msg = QString::fromStdString(err);
					QMetaObject::invokeMethod(
						this,
						[this, name, ok, msg]() {
							if (ok) {
								ReloadRuntime();
							}
							model_->ClearBusy(name);
							RefreshModelFromLocal();
							SetBusy(false);
							Q_EMIT operationFinished(name, ok, msg);
						},
						Qt::QueuedConnection);
				}).detach();
			}

			void PluginMarketManager::uninstall(const QString& name) {
				if (!service_ || busy_.load()) {
					return;
				}
				std::string target_name = name.toStdString();
				SetBusy(true);
				std::thread([this, name, target_name]() {
					std::string err;
					bool ok		= service_->UninstallPlugin(target_name, err);
					QString msg = QString::fromStdString(err);
					QMetaObject::invokeMethod(
						this,
						[this, name, ok, msg]() {
							if (ok) {
								ReloadRuntime();
							}
							RefreshModelFromLocal();
							SetBusy(false);
							Q_EMIT operationFinished(name, ok, msg);
						},
						Qt::QueuedConnection);
				}).detach();
			}

			void PluginMarketManager::setEnabled(const QString& name, bool enabled) {
				if (!service_) {
					return;
				}
				std::string err;
				bool ok = service_->SetEnabled(name.toStdString(), enabled, err);
				if (ok) {
					ReloadRuntime();
					RefreshModelFromLocal();
				}
				Q_EMIT operationFinished(name, ok, QString::fromStdString(err));
			}

			void RegisterTypes(QQmlEngine* engine) {
				Q_UNUSED(engine);
				qmlRegisterUncreatableType<PluginMarketModel>("gdl.sdk", 1, 0, "PluginMarketModel",
															  QStringLiteral("model exposed via manager"));
			}

		}  // namespace market
	}  // namespace ui
}  // namespace gdl
