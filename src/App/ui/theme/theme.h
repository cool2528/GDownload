#pragma once

#include <QtQml/qqml.h>
#include <QColor>
#include <QObject>
#include "Definitions/appDef.h"
#include "Definitions/autoProperty.h"
#include "Definitions/elementPlusColors.h"
#include "GDLCore/singleton.hpp"
class QQmlEngine;
class QJSEngine;
namespace gdl {

	namespace ui {
		namespace theme {
			Q_NAMESPACE
			class GTheme : public QObject, public Singleton<GTheme> {
				Q_OBJECT
				Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)

				// Element Plus 颜色属性
				Q_PROPERTY(QColor primaryColor READ primaryColor NOTIFY darkChanged)
				Q_PROPERTY(QColor successColor READ successColor CONSTANT)
				Q_PROPERTY(QColor warningColor READ warningColor CONSTANT)
				Q_PROPERTY(QColor dangerColor READ dangerColor CONSTANT)
				Q_PROPERTY(QColor infoColor READ infoColor CONSTANT)

				Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY darkChanged)
				Q_PROPERTY(QColor textRegular READ textRegular NOTIFY darkChanged)
				Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY darkChanged)
				Q_PROPERTY(QColor textPlaceholder READ textPlaceholder NOTIFY darkChanged)
				Q_PROPERTY(QColor textDisabled READ textDisabled NOTIFY darkChanged)

				Q_PROPERTY(QColor borderBase READ borderBase NOTIFY darkChanged)
				Q_PROPERTY(QColor borderLight READ borderLight NOTIFY darkChanged)
				Q_PROPERTY(QColor borderLighter READ borderLighter NOTIFY darkChanged)

				Q_PROPERTY(QColor fillBase READ fillBase NOTIFY darkChanged)
				Q_PROPERTY(QColor fillLight READ fillLight NOTIFY darkChanged)
				Q_PROPERTY(QColor fillLighter READ fillLighter NOTIFY darkChanged)

				Q_PROPERTY(QColor bgWhite READ bgWhite NOTIFY darkChanged)
				Q_PROPERTY(QColor bgPage READ bgPage NOTIFY darkChanged)
				Q_PROPERTY(QColor bgBase READ bgBase NOTIFY darkChanged)

				// Element Plus 尺寸属性
				Q_PROPERTY(int sizeLarge READ sizeLarge CONSTANT)
				Q_PROPERTY(int sizeDefault READ sizeDefault CONSTANT)
				Q_PROPERTY(int sizeSmall READ sizeSmall CONSTANT)
				Q_PROPERTY(int radiusBase READ radiusBase CONSTANT)
				Q_PROPERTY(int radiusSmall READ radiusSmall CONSTANT)
				Q_PROPERTY(int spaceLG READ spaceLG CONSTANT)
				Q_PROPERTY(int spaceMD READ spaceMD CONSTANT)
				Q_PROPERTY(int spaceSM READ spaceSM CONSTANT)

				QML_AUTO_PROPERTY(GThemeType::ThemeMode, theme)
				QML_AUTO_PROPERTY(QColor, backgroundColor)
				QML_NAMED_ELEMENT(GTheme)
				QML_SINGLETON
				SINGLETON_DECLARE(GTheme)
			   public:
				static GTheme* create(QQmlEngine*, QJSEngine*);

			   public:
				// 现有方法
				bool dark() const;
				Q_SIGNAL void darkChanged();

				// Element Plus 颜色方法
				QColor primaryColor() const;
				QColor successColor() const;
				QColor warningColor() const;
				QColor dangerColor() const;
				QColor infoColor() const;

				QColor textPrimary() const;
				QColor textRegular() const;
				QColor textSecondary() const;
				QColor textPlaceholder() const;
				QColor textDisabled() const;

				QColor borderBase() const;
				QColor borderLight() const;
				QColor borderLighter() const;

				QColor fillBase() const;
				QColor fillLight() const;
				QColor fillLighter() const;

				QColor bgWhite() const;
				QColor bgPage() const;
				QColor bgBase() const;

				// Element Plus 尺寸方法
				int sizeLarge() const { return ElementPlusColors::Sizes::kLarge; }
				int sizeDefault() const { return ElementPlusColors::Sizes::kDefault; }
				int sizeSmall() const { return ElementPlusColors::Sizes::kSmall; }
				int radiusBase() const { return ElementPlusColors::Sizes::kRadiusBase; }
				int radiusSmall() const { return ElementPlusColors::Sizes::kRadiusSmall; }
				int spaceLG() const { return ElementPlusColors::Sizes::kSpaceLG; }
				int spaceMD() const { return ElementPlusColors::Sizes::kSpaceMD; }
				int spaceSM() const { return ElementPlusColors::Sizes::kSpaceSM; }

				// 工具方法：获取主色的不同层级
				Q_INVOKABLE QColor primaryLight(int level = 1) const;
				Q_INVOKABLE QColor successLight(int level = 3) const;
				Q_INVOKABLE QColor warningLight(int level = 3) const;
				Q_INVOKABLE QColor dangerLight(int level = 3) const;
				Q_INVOKABLE QColor infoLight(int level = 3) const;

			   private:
				explicit GTheme(QObject* parent = nullptr);
				bool SystemIsDarkTheme() const;

			   protected:
				bool eventFilter(QObject* obj, QEvent* event) override;

			   private:
				bool system_is_dark_theme_;
			};

			void RegisterTypes(QQmlEngine* engine);
		}  // namespace theme

	}  // namespace ui

}  // namespace gdl
