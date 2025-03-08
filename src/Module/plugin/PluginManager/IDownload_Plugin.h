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
        std::string image_base64;  // 验证码的图片base64 数据显示
        std::string message;	   // 验证码的提示信息
        std::string input_result;  // 供用户输入的验证码结果
    };
    enum class MsgType : int {
        kSuccess = 0,
        kError	 = 1,
        kWarning = 2,
        kInfo	 = 3,
        kDebug	 = 4,
    };
    using VerificationCallback	= std::function<void(VerificationCallbackParam&)>;
    using IDownloadPluginPtr	= std::shared_ptr<INetDiskDownloadPlugin>;
    using MessageNotifyCallback = std::function<void(std::string_view message, const MsgType&)>;
    virtual ~INetDiskDownloadPlugin()	= default;

	struct PluginMetadata {
        std::string name;							 // 插件名字
        std::string version;						 // 插件版本
        std::string author;							 // 插件作者
        std::string description;					 // 插件功能说明
        std::vector<std::string> supported_domains;	 // 该插件可处理的域名列表
	};

    struct ParseResult {
        std::string real_url;										// 解析/重定向后的实际下载 URL
        std::string file_name;										// 建议的下载文件名
        size_t file_size{0};										// 预期文件大小（以字节为单位）（未知时为 0
        std::unordered_multimap<std::string, std::string> headers;	// 用于下载的自定义 HTTP 标头
        std::unordered_multimap<std::string, std::string> options;	// 其他下载选项
        std::vector<std::string> mirrors;							// 其他下载 URL
	};

    enum class FileType : int {
        // 常规文件
        FILE = 0,
        // 媒体文件
        MEDIA = 1,
        // 压缩文件
        ARCHIVE = 2,
        // 图片文件
        IMAGE = 3,
        // 文档文件
        DOCUMENT = 4,

    };
    // 文件信息结构
    struct FileInfo {
        std::string path;			// 文件相对路径
        std::string name;			// 文件名称
        size_t size;				// 文件大小
        bool is_dir;				// 是否是目录
        FileType type;				// 文件类型
        std::uint64_t file_id;		// 文件 id
        std::uint64_t create_time;	// 创建时间
    };

    /*
     * @breif 根据URL 解析网盘分享链接资产结果
     * @param url 网盘分享链接
     * @param callback 验证码回调函数
     * @return 根目录下子文件信息列表
     */
    virtual std::optional<std::vector<FileInfo>> ParseUrl(std::string_view url, std::string_view password = "") = 0;

    /*
     * @breif 进入目录 根据文件信息获取目录下的文件信息
     * @param info 文件信息
     * @return 目录下子文件信息
     */
    virtual std::optional<std::vector<FileInfo>> EnterDirectory(const FileInfo& info) = 0;

    /*
     * @breif 获取下载信息
     * @param info 文件信息
     * @return 带真实url下载信息 给aria2 使用的
     */

    virtual std::optional<ParseResult> GetDownloadInfo(const FileInfo& info) = 0;

    /*
     * @breif 获取插件元数据
     * @return 插件 元数据
     */

    virtual PluginMetadata GetPluginMetadata() = 0;

    /*
     * @breif 判断是否能处理URL
     * @param url 网盘分享链接 或者 网盘根域名
     */
	virtual bool CanHandle(const std::string& url) const = 0;

    /*
     * @breif 设置验证码回调函数
     */
    void SetVerificationCallback(VerificationCallback callback) { verification_callback_ = callback; }

    /*
     * @breif 设置消息通知回调函数
     */

    void SetMessageNotifyCallback(MessageNotifyCallback callback) { message_notify_callback_ = callback; }

   protected:
    VerificationCallback verification_callback_{nullptr};
    MessageNotifyCallback message_notify_callback_{nullptr};
};

// Function pointer type for creating plugin instances
typedef INetDiskDownloadPlugin* (*CreatePluginFunc)();

// Function pointer type for destroying plugin instances
typedef void (*DestroyPluginFunc)(INetDiskDownloadPlugin*);
