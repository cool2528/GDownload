#include "settings_manager_factory.h"

#include "settings_manager.h"

namespace gdl {
	namespace ui {
		namespace settings {
			ISettings* createSettingsManager(bool isTest) {
				Q_UNUSED(isTest);  // Phase 1 始终返 Impl,Phase 2 加 Fake 分支
				return &SettingsImpl::Instance();
			}
		}  // namespace settings
	}	   // namespace ui
}		   // namespace gdl
