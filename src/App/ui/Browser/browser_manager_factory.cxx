#include "browser_manager_factory.h"

#include "browser_manager.h"

namespace gdl {
	namespace ui {
		namespace browser {
			IBrowserManager* createBrowserManager(bool isTest) {
				Q_UNUSED(isTest);  // Phase 1 始终返 Impl,Phase 2 加 Fake 分支
				return &BrowserManagerImpl::Instance();
			}
		}  // namespace browser
	}	   // namespace ui
}		   // namespace gdl
