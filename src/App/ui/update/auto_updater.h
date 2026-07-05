#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gdl {
    namespace update {

        // Update progress information
        struct UpdateProgress {
            enum class Stage {
                kChecking,	   // Checking for updates
                kDownloading,  // Downloading update
                kExtracting,   // Extracting update
                kVerifying,	   // Verifying update
                kInstalling,   // Installing update
                kFinished,	   // Update completed
                kFailed		   // Update failed
            };

            Stage stage	   = Stage::kChecking;
            int percentage = 0;
            std::string message;
        };

        // Update configuration
        struct UpdateConfig {
            std::string update_url;			   // 主更新信息地址
            std::string fallback_update_url;   // 主地址不可用时的备用更新信息地址
            std::string current_version;	   // Current version number
            std::string temp_dir;			   // Temporary file directory
            bool allow_beta			 = false;  // Whether to accept beta versions
            int check_interval_hours = 24;	   // Auto check interval (hours)
            bool silent_mode		 = false;  // Silent update mode
            bool enable_auto_check	 = false;  // Enable automatic check
        };

        // Update information
        struct UpdateInfo {
            std::string version;					 // New version number
            std::string download_url;				 // Download URL
            std::string release_notes;				 // Release notes
            std::string release_date;				 // Release date
            bool is_mandatory	 = false;			 // Whether the update is mandatory
            int64_t package_size = 0;				 // Package size (bytes)
            std::string signature;					 // Package signature (for verification)
            std::string sha256;  // SHA-256 校验和（S2，latest.json 提供时启用）
            std::vector<std::string> changed_files;	 // List of changed files
        };

        // Auto updater base class
        class AutoUpdater {
           public:
            using ProgressCallback	  = std::function<void(const UpdateProgress&)>;
            using UpdateCheckCallback = std::function<void(bool has_update, const UpdateInfo& info)>;

            virtual ~AutoUpdater() = default;

            // Initialize the updater
            virtual bool Initialize(const UpdateConfig& config) = 0;

            // Check for updates
            virtual void CheckForUpdates(UpdateCheckCallback callback) = 0;

            // Start downloading and installing updates
            virtual bool StartUpdate(ProgressCallback progress_callback) = 0;

            // Cancel ongoing update
            virtual void CancelUpdate() = 0;

            // Apply downloaded update (may require restart)
            virtual bool ApplyUpdate(bool restart_app = true) = 0;

            // Set custom HTTP request headers
            virtual void SetRequestHeaders(const std::map<std::string, std::string>& headers) {
                request_headers_ = headers;
            }

            // Get the last error
            virtual std::string GetLastError() const { return last_error_; }

            // Create platform-specific updater instance
            static std::unique_ptr<AutoUpdater> Create();

           protected:
            std::string last_error_;
            std::map<std::string, std::string> request_headers_;
        };

    }  // namespace update
}  // namespace gdl
