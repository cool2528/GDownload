#ifndef MAC_UPDATER_H_
#define MAC_UPDATER_H_

#include "auto_updater.h"
#include <memory>

namespace gdownload {
namespace update {

// 前向声明，避免暴露Objective-C++细节
class MacUpdaterImpl;

class MacUpdater : public AutoUpdater {
 public:
  MacUpdater();
  ~MacUpdater() override;

  bool Initialize(const UpdateConfig& config) override;
  void CheckForUpdates(UpdateCheckCallback callback) override;
  bool StartUpdate(ProgressCallback progress_callback) override;
  void CancelUpdate() override;
  bool ApplyUpdate(bool restart_app = true) override;

 private:
  std::unique_ptr<MacUpdaterImpl> impl_;
};

}  // namespace update
}  // namespace gdownload

#endif  // MAC_UPDATER_H_