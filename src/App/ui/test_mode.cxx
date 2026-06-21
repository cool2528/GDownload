#include "test_mode.h"

#include <QByteArray>

#include <cstdlib>

namespace gdl {
	namespace ui {
		bool isTestMode() {
			const char* v = std::getenv("GDOWNLOAD_TEST");
			return v != nullptr && QByteArray(v) == "1";
		}
	}  // namespace ui
}  // namespace gdl
