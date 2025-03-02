#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gdl {
    namespace update {

        // 更新进度信息
        struct UpdateProgress {
            enum class Stage {
                kChecking,	   // 检查更新
                kDownloading,  // 下载更新
                kExtracting,   // 解压更新
                kVerifying,	   // 验证更新
                kInstalling,   // 安装更新
                kFinished,	   // 更新完成
                kFailed		   // 更新失败
            };

            Stage stage	   = Stage::kChecking;
            int percentage = 0;
            std::string message;
        };

        // 更新配置
        struct UpdateConfig {
            std::string update_url;			   // 更新服务器URL
            std::string current_version;	   // 当前版本号
            std::string temp_dir;			   // 临时文件目录
            bool allow_beta			 = false;  // 是否接受beta版本
            int check_interval_hours = 24;	   // 自动检查间隔(小时)
            bool silent_mode		 = false;  // 静默更新模式
            bool disable_auto_check	 = false;  // 禁用自动检查
        };

        // 更新信息
        struct UpdateInfo {
            std::string version;					 // 新版本号
            std::string download_url;				 // 下载地址
            std::string release_notes;				 // 发布说明
            std::string release_date;				 // 发布日期
            bool is_mandatory	 = false;			 // 是否强制更新
            int64_t package_size = 0;				 // 包大小(字节)
            std::string signature;					 // 包签名(用于验证)
            std::vector<std::string> changed_files;	 // 变更文件列表
        };

        // 自动更新基类
        class AutoUpdater {
           public:
            using ProgressCallback	  = std::function<void(const UpdateProgress&)>;
            using UpdateCheckCallback = std::function<void(bool has_update, const UpdateInfo& info)>;

            virtual ~AutoUpdater() = default;

            // 初始化更新器
            virtual bool Initialize(const UpdateConfig& config) = 0;

            // 检查更新
            virtual void CheckForUpdates(UpdateCheckCallback callback) = 0;

            // 开始下载并安装更新
            virtual bool StartUpdate(ProgressCallback progress_callback) = 0;

            // 取消正在进行的更新
            virtual void CancelUpdate() = 0;

            // 应用已下载的更新(可能需要重启)
            virtual bool ApplyUpdate(bool restart_app = true) = 0;

            // 设置自定义HTTP请求头
            virtual void SetRequestHeaders(const std::map<std::string, std::string>& headers) {
                request_headers_ = headers;
            }

            // 获取最后一次错误
            virtual std::string GetLastError() const { return last_error_; }

            // 创建平台特定的更新器实例
            static std::unique_ptr<AutoUpdater> Create();

           protected:
            std::string last_error_;
            std::map<std::string, std::string> request_headers_;
        };

    }  // namespace update
}  // namespace gdl
