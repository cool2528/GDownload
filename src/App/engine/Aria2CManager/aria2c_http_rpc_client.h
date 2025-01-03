#pragma once
#include <nlohmann/json.hpp>
#include <variant>
#include "result/result.h"
#include "Engine_export.h"
namespace gdl {
	namespace engine {
		struct SucceedResult {
			std::string body;
		};
		struct ErrorResult {
			std::string err_msg;
			std::int64_t err_code;
		};
		struct Response {
			bool is_succeed{false};
			std::variant<SucceedResult, ErrorResult> result;
		};
		using Aria2Options = std::unordered_multimap<std::string, std::string>;
		class Engine_API Aria2cHttpClient {
		   public:
			explicit Aria2cHttpClient(std::string_view host);
			~Aria2cHttpClient() = default;
			Result<Response> AddUri(const std::vector<std::string>& uris, const Aria2Options& options);
			Result<Response> AddTorrent(const std::string& torrent, const Aria2Options& options);
			Result<Response> AddMetalink(const std::string& metalink, const Aria2Options& options);
			Result<Response> Remove(const std::string& gid);
			Result<Response> ForceRemove(const std::string& gid);
			Result<Response> Pause(const std::string& gid);
			Result<Response> PauseAll();
			Result<Response> ForcePause(const std::string& gid);
			Result<Response> ForcePauseAll();
			Result<Response> Unpause(const std::string& gid);
			Result<Response> UnpauseAll();
			Result<Response> TellStatus(const std::string& gid, const std::vector<std::string>& keys);
			//getUris
			Result<Response> GetUris(const std::string& gid);
			//getFiles
			Result<Response> GetFiles(const std::string& gid);
			//getPeers
			Result<Response> GetPeers(const std::string& gid);
			// getServers
			Result<Response> GetServers(const std::string& gid);
			//tellActive
			Result<Response> TellActive(const std::vector<std::string>& keys);
			//tellWaiting
			Result<Response> TellWaiting(int offset, int num, const std::vector<std::string>& keys);
			//tellStopped
			Result<Response> TellStopped(int offset, int num, const std::vector<std::string>& keys);
			//changePosition
			Result<Response> ChangePosition(const std::string& gid, int pos, int how);
			//getOption
			Result<Response> GetOption(const std::string& gid);
			//changeOption
			Result<Response> changeOption(const std::string& gid, const Aria2Options& options);
			//getGlobalOption
			Result<Response> GetGlobalOption();
			//changeGlobalOption
			Result<Response> ChangeGlobalOption(const Aria2Options& options);
			//getGlobalStat
			Result<Response> GetGlobalStat();
			//purgeDownloadResult
			Result<Response> PurgeDownloadResult();
			//removeDownloadResult
			Result<Response> RemoveDownloadResult(const std::string& gid);
			//getVersion
			Result<Response> GetVersion();
			//shutdown
			Result<Response> Shutdown();
			//forceShutdown
			Result<Response> ForceShutdown();

		   private:
			Result<Response> Send(const std::string_view& method, const nlohmann::json& params);

		   private:
			std::string host_;
			std::int64_t id_{1};
		};
	}  // namespace engine
}  // namespace gdl
