#pragma once

#include "ISettings.h"

namespace gdl {
	namespace ui {
		namespace settings {
			// Settings 工厂:按测试模式返回 Impl 或(Phase 2)Fake 替身
			// Phase 1 始终返回 SettingsImpl 单例;isTest 参数为 Phase 2 预留
			ISettings* createSettingsManager(bool isTest);
		}  // namespace settings
	}	   // namespace ui
}		   // namespace gdl
