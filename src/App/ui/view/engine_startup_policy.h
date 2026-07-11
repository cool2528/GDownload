#pragma once
#include <utility>
namespace gdl::ui {
	template <typename StartEngine, typename InitBrowser, typename StopEngine, typename MarkUnavailable>
	bool RunEngineStartupPolicy(StartEngine&& start_engine, InitBrowser&& init_browser,
		StopEngine&& stop_engine, MarkUnavailable&& mark_unavailable) {
		if (!std::forward<StartEngine>(start_engine)()) {
			std::forward<MarkUnavailable>(mark_unavailable)();
			return false;
		}
		if (!std::forward<InitBrowser>(init_browser)()) {
			std::forward<StopEngine>(stop_engine)();
			std::forward<MarkUnavailable>(mark_unavailable)();
			return false;
		}
		return true;
	}
}
