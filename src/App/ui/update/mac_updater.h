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
            // Mac 后端的错误存于 MacUpdaterImpl,不写基类字段;必须转发,
            // 否则上层 GetLastError 永远为空,检查失败会被误报为"已是最新版"
            std::string GetLastError() const override;
            void ClearLastError() override;

           private:
            std::unique_ptr<MacUpdaterImpl> impl_;
        };

    }  // namespace update
}  // namespace gdl

#endif
