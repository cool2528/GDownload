#pragma once
#include <string>
#include <ctime>
#include "export.h"

namespace gdl {
namespace cache {

// Download task status enumeration
enum class DownloadState { kComplete = 0, kActive, kPause, kWaiting, kError, kRemoved };

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
    DownloadState state{DownloadState::kWaiting}; // 任务状态，新建记录默认等待中而非已完成
    time_t created_time{0};       // Creation time
    time_t completed_time{0};     // Completion time
    std::string error_message;    // Error message
};

} // namespace cache
} // namespace gdl 