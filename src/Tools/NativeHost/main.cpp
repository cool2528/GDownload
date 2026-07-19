// GDownload 浏览器扩展 Native Messaging host（com.gdownload.host）
// 职责：读 GDownload 配置文件拿 aria2 RPC 端口/密钥，回应扩展握手；按需唤起主程序。
// 不做数据转发：任务下发仍走扩展直连的 aria2 WebSocket RPC。
//
// 协议（stdio）：4 字节小端长度前缀 + UTF-8 JSON（与扩展 nativeBridge.ts 对齐）。
//   扩展 -> host: {type:"handshake",extVersion} / {type:"launch"} / {type:"ping"}
//   host -> 扩展: {type:"handshakeAck",hostVersion,appRunning,rpcPort,rpcSecret,appVersion}
//                 {type:"launchResult",ok} / {type:"pong"}

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <toml++/toml.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
// clang-format on
#endif

using json = nlohmann::json;

namespace {

constexpr const char* kHostVersion = "1.0.0";
constexpr int kDefaultRpcPort = 16888;
constexpr uint32_t kMaxMessageSize = 64u * 1024u * 1024u;  // 64MB 上限保护

// 读取一条消息：4 字节小端长度 + JSON 负载。EOF/错误返回 nullopt。
std::optional<std::string> ReadMessage() {
	uint32_t length = 0;
	if (std::fread(&length, sizeof(length), 1, stdin) != 1) {
		return std::nullopt;
	}
	if (length == 0 || length > kMaxMessageSize) {
		return std::nullopt;
	}
	std::string buffer(length, '\0');
	if (std::fread(buffer.data(), 1, length, stdin) != length) {
		return std::nullopt;
	}
	return buffer;
}

// 写出一条消息：4 字节小端长度 + JSON 负载。
void WriteMessage(const std::string& payload) {
	const uint32_t length = static_cast<uint32_t>(payload.size());
	std::fwrite(&length, sizeof(length), 1, stdout);
	std::fwrite(payload.data(), 1, payload.size(), stdout);
	std::fflush(stdout);
}

// 解析 GDownload 配置文件路径（与 GDLCore os::GetAppDataDir + "/gdownload/gd.toml" 对齐）
std::string GetConfigPath() {
#ifdef _WIN32
	// Windows: FOLDERID_RoamingAppData 即 %APPDATA%
	const char* appdata = std::getenv("APPDATA");
	if (!appdata) {
		return "";
	}
	return std::string(appdata) + "\\gdownload\\gd.toml";
#elif defined(__APPLE__)
	const char* home = std::getenv("HOME");
	if (!home) {
		return "";
	}
	return std::string(home) + "/Library/Application Support/gdownload/gd.toml";
#else
	// Linux: 与 linux_os::GetAppDataDir 对齐（优先 XDG_DATA_HOME，回退 ~/.local/share）
	if (const char* xdg = std::getenv("XDG_DATA_HOME")) {
		if (xdg[0] != '\0') {
			return std::string(xdg) + "/gdownload/gd.toml";
		}
	}
	const char* home = std::getenv("HOME");
	if (!home) {
		return "";
	}
	return std::string(home) + "/.local/share/gdownload/gd.toml";
#endif
}

struct RpcConfig {
	int port = kDefaultRpcPort;
	std::string secret;
};

// 从 gd.toml 读取 [aria2c] 下的 rpc-listen-port / rpc-secret（值以字符串存储）
RpcConfig ReadRpcConfig() {
	RpcConfig config;
	const std::string path = GetConfigPath();
	if (path.empty()) {
		return config;
	}
	try {
		const toml::table root = toml::parse_file(path);
		const auto* aria2c = root["aria2c"].as_table();
		if (!aria2c) {
			return config;
		}
		if (auto port = (*aria2c)["rpc-listen-port"].value<std::string>()) {
			try {
				config.port = std::stoi(*port);
			} catch (...) {
				// 保留默认端口
			}
		} else if (auto port_int = (*aria2c)["rpc-listen-port"].value<int64_t>()) {
			config.port = static_cast<int>(*port_int);
		}
		if (auto secret = (*aria2c)["rpc-secret"].value<std::string>()) {
			config.secret = *secret;
		}
	} catch (...) {
		// 文件不存在或解析失败：返回默认（端口 16888、空 secret）
	}
	if (config.port <= 0 || config.port > 65535) {
		config.port = kDefaultRpcPort;
	}
	return config;
}

// 探测 GDownload 是否运行：尝试连接本地 aria2 RPC 端口（引擎随主程序启动）
bool IsAppRunning(int port) {
#ifdef _WIN32
	SOCKET sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return false;
	}
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<u_short>(port));
	::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	const bool connected = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
	::closesocket(sock);
	return connected;
#else
	(void)port;
	return false;  // 非 Windows 平台暂用进程检测（后续补），此处保守返回 false
#endif
}

// 唤起 GDownload 主程序（与 host 同目录或上级目录查找 GDownload 可执行文件）
bool LaunchApp() {
#ifdef _WIN32
	wchar_t module_path[MAX_PATH] = {0};
	if (::GetModuleFileNameW(nullptr, module_path, MAX_PATH) == 0) {
		return false;
	}
	std::wstring dir(module_path);
	const size_t slash = dir.find_last_of(L"\\/");
	if (slash != std::wstring::npos) {
		dir = dir.substr(0, slash);
	}
	// 依次尝试同目录、上级目录下的 GDownload.exe
	const std::wstring candidates[] = {dir + L"\\GDownload.exe", dir + L"\\..\\GDownload.exe"};
	for (const auto& exe : candidates) {
		const HINSTANCE result = ::ShellExecuteW(nullptr, L"open", exe.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL);
		if (reinterpret_cast<INT_PTR>(result) > 32) {
			return true;
		}
	}
	return false;
#else
	return false;  // 非 Windows 平台后续补
#endif
}

json HandleMessage(const json& in) {
	const std::string type = in.value("type", "");
	if (type == "handshake") {
		const RpcConfig config = ReadRpcConfig();
		return json{{"type", "handshakeAck"},
					{"hostVersion", kHostVersion},
					{"appRunning", IsAppRunning(config.port)},
					{"rpcPort", config.port},
					{"rpcSecret", config.secret},
					{"appVersion", ""}};
	}
	if (type == "launch") {
		return json{{"type", "launchResult"}, {"ok", LaunchApp()}};
	}
	if (type == "ping") {
		return json{{"type", "pong"}};
	}
	if (type == "parseShare") {
		// T4.1 网盘解析转发：需主程序单实例 IPC 接收，当前尚未实现，返回未支持
		return json{{"type", "parseShareResult"}, {"ok", false}, {"error", "not_implemented"}};
	}
	return json{};  // 未知类型：不回复（返回空对象，由调用方跳过）
}

}  // namespace

int main() {
#ifdef _WIN32
	// Windows 下必须将 stdin/stdout 设为二进制，避免 CRLF 转换破坏长度前缀协议
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	WSADATA wsa_data;
	::WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

	while (true) {
		const std::optional<std::string> message = ReadMessage();
		if (!message) {
			break;  // EOF：扩展断开或浏览器关闭端口
		}
		json in;
		try {
			in = json::parse(*message);
		} catch (...) {
			continue;  // 非法 JSON：跳过
		}
		const json out = HandleMessage(in);
		if (!out.empty()) {
			WriteMessage(out.dump());
		}
	}

#ifdef _WIN32
	::WSACleanup();
#endif
	return 0;
}
