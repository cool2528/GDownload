#include "auto_updater.h"

#ifdef _WIN32
#include "win_updater.h"
#elif defined(__APPLE__)
#include "mac_updater.h"
#elif defined(__linux__)
#include "linux_updater.h"
#endif

namespace gdownload {
namespace update {

std::unique_ptr<AutoUpdater> AutoUpdater::Create() {
#ifdef _WIN32
  return std::make_unique<WinUpdater>();
#elif defined(__APPLE__)
  return std::make_unique<MacUpdater>();
#elif defined(__linux__)
  return std::make_unique<LinuxUpdater>();
#else
  return nullptr;  // 不支持的平台
#endif
}

}  // namespace update
}  // namespace gdownload 