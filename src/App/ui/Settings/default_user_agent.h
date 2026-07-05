#pragma once

#include <QString>

#include "Aria2CManager/default_user_agent.h"

namespace gdl {
	namespace ui {
		namespace settings {

			inline QString DefaultBrowserUserAgent() {
				return QString::fromStdString(gdl::engine::DefaultBrowserUserAgentString());
			}

		}  // namespace settings
	}  // namespace ui
}  // namespace gdl
