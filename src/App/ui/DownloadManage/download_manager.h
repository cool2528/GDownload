#pragma once
#include <QObject>
#include "singleton.hpp"
namespace gdl {
	namespace ui {
		namespace dm {
			class DownloadManager : public QObject, public Singleton<DownloadManager> {
				Q_OBJECT
				SINGLETON_DECLARE(DownloadManager)
			   public:
				~DownloadManager() override;

			   private:
				explicit DownloadManager(QObject* parent = nullptr);
			};
		}  // namespace dm
	}  // namespace ui
}  // namespace gdl
