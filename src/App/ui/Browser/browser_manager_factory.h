#pragma once

#include "IBrowserManager.h"

namespace gdl {
	namespace ui {
		namespace browser {
			// BrowserManager 工厂:按测试模式返回 Impl 或(Phase 2)Fake 替身
			// Phase 1 始终返回 BrowserManagerImpl 单例;isTest 参数为 Phase 2 预留
			IBrowserManager* createBrowserManager(bool isTest);
		}  // namespace browser
	}	   // namespace ui
}		   // namespace gdl
