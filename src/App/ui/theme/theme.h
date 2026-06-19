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
				Q_PROPERTY(QColor bgElevated READ bgElevated NOTIFY darkChanged)

				// Element Plus 尺寸属性
				Q_PROPERTY(int sizeLarge READ sizeLarge CONSTANT)
				Q_PROPERTY(int sizeDefault READ sizeDefault CONSTANT)
				Q_PROPERTY(int sizeSmall READ sizeSmall CONSTANT)
				Q_PROPERTY(int radiusBase READ radiusBase CONSTANT)
				Q_PROPERTY(int radiusSmall READ radiusSmall CONSTANT)
				Q_PROPERTY(int spaceLG READ spaceLG CONSTANT)
				Q_PROPERTY(int spaceMD READ spaceMD CONSTANT)
				Q_PROPERTY(int spaceSM READ spaceSM CONSTANT)

				// 间距令牌
				Q_PROPERTY(int spaceXS READ spaceXS CONSTANT)
				Q_PROPERTY(int spaceXL READ spaceXL CONSTANT)
				Q_PROPERTY(int space2XL READ space2XL CONSTANT)
				Q_PROPERTY(int space3XL READ space3XL CONSTANT)

				// 圆角令牌
				Q_PROPERTY(int radiusRound READ radiusRound CONSTANT)
				Q_PROPERTY(int radiusCircle READ radiusCircle CONSTANT)

				// 动效令牌
				Q_PROPERTY(int durationFast READ durationFast CONSTANT)
				Q_PROPERTY(int durationBase READ durationBase CONSTANT)
				Q_PROPERTY(int durationSlow READ durationSlow CONSTANT)

				// 字号令牌(px)
				Q_PROPERTY(int fontCaption READ fontCaption CONSTANT)
				Q_PROPERTY(int fontBody READ fontBody CONSTANT)
				Q_PROPERTY(int fontSubtitle READ fontSubtitle CONSTANT)
				Q_PROPERTY(int fontTitle READ fontTitle CONSTANT)
				Q_PROPERTY(int fontH1 READ fontH1 CONSTANT)

				// 字重令牌
				Q_PROPERTY(int weightRegular READ weightRegular CONSTANT)
				Q_PROPERTY(int weightMedium READ weightMedium CONSTANT)
				Q_PROPERTY(int weightDemiBold READ weightDemiBold CONSTANT)

				// 布局令牌
				Q_PROPERTY(int navBarWidth READ navBarWidth CONSTANT)
				Q_PROPERTY(int titleBarHeight READ titleBarHeight CONSTANT)
				Q_PROPERTY(int sidebarWidth READ sidebarWidth CONSTANT)
				Q_PROPERTY(int navItemHeight READ navItemHeight CONSTANT)

				// 阴影层级(返回 QVariantMap,含 color/blur/offsetX/offsetY,供 GElevation 消费)
				Q_PROPERTY(QVariantMap elevation1 READ elevation1 NOTIFY darkChanged)
				Q_PROPERTY(QVariantMap elevation2 READ elevation2 NOTIFY darkChanged)
				Q_PROPERTY(QVariantMap elevation3 READ elevation3 NOTIFY darkChanged)
				Q_PROPERTY(QVariantMap elevation4 READ elevation4 NOTIFY darkChanged)

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
				QColor bgElevated() const;

				// Element Plus 尺寸方法
				int sizeLarge() const { return ElementPlusColors::Sizes::kLarge; }
				int sizeDefault() const { return ElementPlusColors::Sizes::kDefault; }
				int sizeSmall() const { return ElementPlusColors::Sizes::kSmall; }
				int radiusBase() const { return ElementPlusColors::Sizes::kRadiusBase; }
				int radiusSmall() const { return ElementPlusColors::Sizes::kRadiusSmall; }
				int spaceLG() const { return ElementPlusColors::Sizes::kSpaceLG; }
				int spaceMD() const { return ElementPlusColors::Sizes::kSpaceMD; }
				int spaceSM() const { return ElementPlusColors::Sizes::kSpaceSM; }

				// 间距令牌方法
				int spaceXS() const { return ElementPlusColors::Sizes::kSpaceXS; }
				int spaceXL() const { return ElementPlusColors::Sizes::kSpaceXL; }
				int space2XL() const { return ElementPlusColors::Sizes::kSpace2XL; }
				int space3XL() const { return ElementPlusColors::Sizes::kSpace3XL; }

				// 圆角令牌方法
				int radiusRound() const { return ElementPlusColors::Sizes::kRadiusRound; }
				int radiusCircle() const { return ElementPlusColors::Sizes::kRadiusCircle; }

				// 动效令牌方法
				int durationFast() const { return ElementPlusColors::Sizes::kDurationFast; }
				int durationBase() const { return ElementPlusColors::Sizes::kDurationBase; }
				int durationSlow() const { return ElementPlusColors::Sizes::kDurationSlow; }

				// 字号令牌方法
				int fontCaption() const { return ElementPlusColors::Sizes::kFontCaption; }
				int fontBody() const { return ElementPlusColors::Sizes::kFontBody; }
				int fontSubtitle() const { return ElementPlusColors::Sizes::kFontSubtitle; }
				int fontTitle() const { return ElementPlusColors::Sizes::kFontTitle; }
				int fontH1() const { return ElementPlusColors::Sizes::kFontH1; }

				// 字重令牌方法
				int weightRegular() const { return ElementPlusColors::Sizes::kWeightRegular; }
				int weightMedium() const { return ElementPlusColors::Sizes::kWeightMedium; }
				int weightDemiBold() const { return ElementPlusColors::Sizes::kWeightDemiBold; }

				// 布局令牌方法
				int navBarWidth() const { return ElementPlusColors::Sizes::kNavBarWidth; }
				int titleBarHeight() const { return ElementPlusColors::Sizes::kTitleBarHeight; }
				int sidebarWidth() const { return ElementPlusColors::Sizes::kSidebarWidth; }
				int navItemHeight() const { return ElementPlusColors::Sizes::kNavItemHeight; }

				// 阴影层级方法(深浅色不同,跟随主题)
				QVariantMap elevation1() const;
				QVariantMap elevation2() const;
				QVariantMap elevation3() const;
				QVariantMap elevation4() const;

				// 工具方法：获取主色色阶
				Q_INVOKABLE QColor primaryLight(int level = 3) const;
				Q_INVOKABLE QColor successLight(int level = 3) const;
				Q_INVOKABLE QColor warningLight(int level = 3) const;
				Q_INVOKABLE QColor dangerLight(int level = 3) const;
				Q_INVOKABLE QColor infoLight(int level = 3) const;

			   private:
				explicit GTheme(QObject* parent = nullptr);
				bool SystemIsDarkTheme() const;
				void applyMenuStyleSheet();

			   protected:
				bool eventFilter(QObject* obj, QEvent* event) override;

			   private:
				bool system_is_dark_theme_;
			};

			void RegisterTypes(QQmlEngine* engine);
		}  // namespace theme

	}  // namespace ui

}  // namespace gdl
