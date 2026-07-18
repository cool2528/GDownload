#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "PluginManager/market/plugin_market_service.h"
#include "singleton.hpp"

class QQmlEngine;

namespace gdl {
	namespace ui {
		namespace market {

			// 市场列表模型：暴露每个插件的元数据 + 本地状态给 QML
			class PluginMarketModel : public QAbstractListModel {
				Q_OBJECT
				enum Roles {
					NameRole = Qt::UserRole + 1,
					DisplayNameRole,
					DescriptionRole,
					AuthorRole,
					VerifiedRole,
					TypeRole,
					LatestVersionRole,
					InstalledVersionRole,
					StateRole,		 // 0=Available 1=Installed 2=UpdateAvailable 3=Busy
					EnabledRole,
					TagsRole,
					ProgressRole,	 // 安装进度 0-100，-1 表示无
					StageRole
				};

			   public:
				explicit PluginMarketModel(QObject* parent = nullptr);
				int rowCount(const QModelIndex& parent = QModelIndex()) const override;
				QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
				QHash<int, QByteArray> roleNames() const override;

				// 用市场项全量刷新
				void SetItems(const std::vector<gdl::market::MarketItem>& items);
				// 更新某插件的忙碌/进度态
				void SetBusy(const QString& name, int progress, const QString& stage);
				void ClearBusy(const QString& name);

			   private:
				struct Row {
					gdl::market::MarketItem item;
					bool busy{false};
					int progress{-1};
					QString stage;
				};
				int IndexOf(const QString& name) const;

			   private:
				QVector<Row> rows_;
			};

			// 插件市场管理器：QML 入口，异步执行注册表拉取与安装/卸载，操作后热重载插件
			class PluginMarketManager : public QObject, public Singleton<PluginMarketManager> {
				Q_OBJECT
				SINGLETON_DECLARE(PluginMarketManager)
				Q_PROPERTY(PluginMarketModel* model READ model CONSTANT)
				Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

			   public:
				~PluginMarketManager() override;

				// 由 MainWindow 传入插件目录与数据目录
				void Initialize(const QString& plugins_dir, const QString& data_dir);

				PluginMarketModel* model() const { return model_; }
				bool busy() const { return busy_.load(); }

				// 拉取注册表并刷新模型
				Q_INVOKABLE void refresh();
				// 安装/更新到最新版
				Q_INVOKABLE void install(const QString& name);
				// 卸载
				Q_INVOKABLE void uninstall(const QString& name);
				// 启用/禁用
				Q_INVOKABLE void setEnabled(const QString& name, bool enabled);

			   Q_SIGNALS:
				void busyChanged();
				void refreshFinished(bool success, const QString& message);
				void operationFinished(const QString& name, bool success, const QString& message);

			   private:
				explicit PluginMarketManager(QObject* parent = nullptr);
				// 官方源 + 国内镜像的注册表地址
				std::vector<std::string> RegistryUrls() const;
				// 将当前界面语言同步给服务，用于本地化插件名称/描述
				void SyncLocale();
				void SetBusy(bool busy);
				// 在 UI 线程重新计算并刷新模型（读取本地状态）
				void RefreshModelFromLocal();
				// 热重载运行时插件
				void ReloadRuntime();

			   private:
				std::unique_ptr<gdl::market::PluginMarketService> service_;
				PluginMarketModel* model_{nullptr};
				std::string plugins_dir_;
				std::string data_dir_;
				std::atomic<bool> busy_{false};
			};

			void RegisterTypes(QQmlEngine* engine);

		}  // namespace market
	}  // namespace ui
}  // namespace gdl
