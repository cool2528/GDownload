#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

// Interface for download plugins that handle URL parsing and metadata
class IDownloadPlugin {
   public:
	using IDownloadPluginPtr	 = std::shared_ptr<IDownloadPlugin>;
	using IDownloadPluginOptions = std::unordered_multimap<std::string, std::string>;
	virtual ~IDownloadPlugin()	 = default;

	// Metadata structure containing plugin information
	struct PluginMetadata {
		std::string name;							 // Name of the plugin
		std::string version;						 // Version of the plugin
		std::string author;							 // Author of the plugin
		std::string description;					 // Description of plugin functionality
		std::vector<std::string> supported_domains;	 // List of domains this plugin can handle
	};

	// Structure containing parsed URL information and download parameters
	struct ParseResult {
		std::string real_url;										// Actual download URL after parsing/redirects
		std::string file_name;										// Suggested filename for the download
		size_t file_size{0};										// Expected file size in bytes (0 if unknown)
		std::unordered_multimap<std::string, std::string> headers;	// Custom HTTP headers for download
		std::unordered_multimap<std::string, std::string> options;	// Additional download options
		std::vector<std::string> mirrors;							// Alternative download URLs
	};

	// Parse the given URL and return download information
	// @param url The URL to parse
	// @return ParseResult if successful, nullopt if parsing fails
	virtual std::optional<ParseResult> ParseUrl(std::string_view url, const IDownloadPluginOptions& options = {}) = 0;

	// Get metadata information about this plugin
	// @return PluginMetadata structure containing plugin information
	virtual PluginMetadata GetPluginMetadata() = 0;

	// Check if this plugin can handle the given URL
	// @param url The URL to check
	// @return true if the plugin can handle this URL, false otherwise
	virtual bool CanHandle(const std::string& url) const = 0;
};

// Function pointer type for creating plugin instances
typedef IDownloadPlugin* (*CreatePluginFunc)();

// Function pointer type for destroying plugin instances
typedef void (*DestroyPluginFunc)(IDownloadPlugin*);
