#include "language/language_manager.h"
#include <QApplication>
#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include "GDLCore/logger.h"
#include "Settings/settings_manager.h"

namespace gdl {
	namespace ui {
		namespace language {

			LanguageManager::~LanguageManager() {
				RemoveCurrentTranslation();
			}

			LanguageManager::LanguageManager(QObject* parent) : QObject(parent) {
#ifdef __APPLE__
				const QString path = QDir::currentPath();
				QString app_path_dir =
					QString::fromStdString(std::filesystem::path(path.toStdString()).parent_path().string());
				translation_path_ = app_path_dir + "/Resources/translations/";
#elif WIN32
				translation_path_ = QDir::currentPath() + "/translations/";
#endif
			}

			void LanguageManager::Initialize(QQmlEngine* engine) {
				engine_ = engine;
				// 从设置中读取上次使用的语言
				const auto saved_language = gdl::ui::settings::Settings::Instance().GetLanguage();
				if (!saved_language.isEmpty()) {
					SwitchLanguage(saved_language);
				}
			}

			bool LanguageManager::SwitchLanguage(const QString& locale) {
				if (current_language_ == locale) {
					return true;
				}

				RemoveCurrentTranslation();

				if (LoadTranslation(locale)) {
					current_language_ = locale;
					// 保存语言设置
					gdl::ui::settings::Settings::Instance().SetLanguage(locale);
					return true;
				}

				return false;
			}

			QString LanguageManager::GetCurrentLanguage() const {
				return current_language_;
			}

			QStringList LanguageManager::GetSupportedLanguages() const {
				return {"en_US", "zh_CN", "zh_TW", "ja_JP", "ko_KR"};
			}

			bool LanguageManager::LoadTranslation(const QString& locale) {
				if (locale == "en_US") {
					qApp->removeTranslator(&translator_);
					current_language_ = locale;
					if (engine_) {
						engine_->retranslate();
					}
					return true;
				}

				const QString qm_file = translation_path_ + translation_prefix_ + locale + ".qm";

				if (translator_.load(qm_file)) {
					if (qApp->installTranslator(&translator_)) {
						current_language_ = locale;
						if (engine_) {
							engine_->retranslate();
						}
						return true;
					}
					LOG_ERR("Failed to install translator for locale: {}", locale.toStdString());
				}
				else {
					LOG_ERR("Failed to load translation file: {}", qm_file.toStdString());
				}

				return false;
			}

			void LanguageManager::RemoveCurrentTranslation() {
				if (!current_language_.isEmpty()) {
					qApp->removeTranslator(&translator_);
				}
			}

			void RegisterTypes(QQmlEngine* engine) {
				if (!engine) {
					LOG_ERR("invalid QQmlEngine");
					return;
				}

				engine->rootContext()->setContextProperty("LanguageManager", &LanguageManager::Instance());
				LanguageManager::Instance().Initialize(engine);
			}

		}  // namespace language
	}  // namespace ui
}  // namespace gdl
