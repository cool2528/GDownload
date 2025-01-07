#pragma once
#include <string>
#include <ctime>
#include "export.h"

namespace gdl {
namespace cache {

// Download task status enumeration
enum class DownloadState {
    kUnknown = 0,
    kWaiting,
    kDownloading,
    kPaused,
    kCompleted,
    kError,
    kCanceled
};

// Pure data structure, not dependent on any UI components
struct DownloadRecord {
    std::string task_id;           // Task ID
    std::string file_name;         // File name
    std::string save_path;         // Save path
    std::string download_url;      // Download URL
    int64_t total_size{0};        // Total size
    int64_t downloaded_size{0};   // Downloaded size
    int32_t download_speed{0};    // Download speed
    int32_t connections{0};       // Number of connections
    DownloadState state{DownloadState::kUnknown}; // Task status
    time_t created_time{0};       // Creation time
    time_t completed_time{0};     // Completion time
    std::string error_message;    // Error message
};

} // namespace cache
} // namespace gdl 