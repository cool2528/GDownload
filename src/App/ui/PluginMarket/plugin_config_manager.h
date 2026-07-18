#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "PluginManager/plugin_config_store.h"
#include "singleton.hpp"

class QQmlEngine;

namespace gdl {
	namespace ui {
		namespace market {

			// 插件配置管理器：声明式 Schema 的 QML 入口
			// 职责：本地化 schema 提供给表单渲染、配置读写、required 完备性判断、旧百度 Cookie 迁移
			class PluginConfigManager : public QObject, public Singleton<PluginConfigManager> {
				Q_OBJECT
				SINGLETON_DECLARE(PluginConfigManager)
			   public:
				~PluginConfigManager() override;

				// 由 MainWindow 传入数据目录；同时执行旧版 qBaiduPanCookies 迁移
				void Initialize(const QString& data_dir);

				// 本地化后的 settings schema（label/hint 按当前界面语言解析）
				Q_INVOKABLE QVariantList schema(const QString& name) const;
				// 当前配置值（含 schema default 回填）
				Q_INVOKABLE QVariantMap values(const QString& name) const;
				// 保存配置到磁盘；返回是否落盘成功(仅成功时才发出 configChanged)
				Q_INVOKABLE bool save(const QString& name, const QVariantMap& values);
				Q_INVOKABLE void clear(const QString& name);
				// required 字段是否全部有值
				Q_INVOKABLE bool configured(const QString& name) const;
				// schema 是否非空（市场卡片据此显示配置入口）
				Q_INVOKABLE bool hasSchema(const QString& name) const;
				// 路由/状态信息：{name, displayName, needsConfig, configured}
				Q_INVOKABLE QVariantMap pluginInfo(const QString& name) const;

				// C++ 侧：role=token 字段的当前值（NetWorkDiskManager 取 userToken）
				QString TokenFor(const QString& name) const;

			   Q_SIGNALS:
				void configChanged(const QString& name);

			   private:
				explicit PluginConfigManager(QObject* parent = nullptr);
				std::string CurrentLocale() const;

			   private:
				std::unique_ptr<gdl::plugin::PluginConfigStore> store_;
			};

		}  // namespace market
	}  // namespace ui
}  // namespace gdl
