#pragma once
#if defined(__APPLE__)
#include <memory>
#include "auto_updater.h"

namespace gdl {
    namespace update {
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
}  // namespace gdl

#endif
