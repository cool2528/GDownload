#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class INetDiskDownloadPlugin {
   public:
    struct VerificationCallbackParam {
        std::string image_base64;
        std::string message;
        std::string input_result;
    };
    enum class MsgType : int {
        kSuccess = 0,
        kError	 = 1,
        kWarning = 2,
        kInfo	 = 3,
        kDebug	 = 4,
    };
    using VerificationCallback		  = std::function<void(VerificationCallbackParam&)>;
    using IDownloadPluginPtr		  = std::shared_ptr<INetDiskDownloadPlugin>;
    using MessageNotifyCallback		  = std::function<void(std::string_view message, const MsgType&)>;
    virtual ~INetDiskDownloadPlugin() = default;

	struct PluginMetadata {
        std::string name;
        std::string version;
        std::string author;
        std::string description;
        std::vector<std::string> supported_domains;
	};

    struct ParseResult {
        std::string real_url;
        std::string file_name;
        size_t file_size{0};
        std::unordered_multimap<std::string, std::string> headers;
        std::unordered_multimap<std::string, std::string> options;
        std::vector<std::string> mirrors;
	};

    enum class FileType : int {

        FILE = 0,
        DIR	 = 1
    };

    struct FileInfo {
        std::string path;
        std::string name;
        size_t size{0};
        bool is_dir{false};
        FileType type{FileType::FILE};
        std::string file_id;
        std::uint64_t create_time{0};
		std::string root_path;
    };

    virtual std::optional<std::vector<FileInfo>> ParseUrl(std::string_view url, std::string_view user_token = "") = 0;

    virtual std::optional<std::vector<FileInfo>> EnterDirectory(const FileInfo& info) = 0;

    virtual std::optional<std::vector<ParseResult>> GetDownloadInfo(const FileInfo& info) = 0;

    virtual PluginMetadata GetPluginMetadata() = 0;

	virtual bool CanHandle(const std::string& url) const = 0;

    void SetVerificationCallback(VerificationCallback callback) { verification_callback_ = callback; }

    void SetMessageNotifyCallback(MessageNotifyCallback callback) { message_notify_callback_ = callback; }

   protected:
    VerificationCallback verification_callback_{nullptr};
    MessageNotifyCallback message_notify_callback_{nullptr};
};

// Function pointer type for creating plugin instances
typedef INetDiskDownloadPlugin* (*CreatePluginFunc)();

// Function pointer type for destroying plugin instances
typedef void (*DestroyPluginFunc)(INetDiskDownloadPlugin*);
