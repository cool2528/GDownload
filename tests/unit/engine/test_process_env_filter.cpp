#include <gtest/gtest.h>

#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "process/process.h"

namespace {
	using gdl::String;
	using gdl::process::FilterEnvEntries;

	// aria2c 子进程需要剥离代理类环境变量：键名匹配必须大小写不敏感且不误删其他变量
	TEST(ProcessEnvFilterTest, RemovesMatchingKeysCaseInsensitive) {
		const std::vector<String> entries = {
			"all_proxy=socks://127.0.0.1:1080",
			"ALL_PROXY=socks5h://127.0.0.1:1080",
			"http_proxy=http://127.0.0.1:8080",
			"PATH=/usr/bin",
			"HOME=/home/user",
		};
		const auto filtered = FilterEnvEntries(entries, {"all_proxy", "http_proxy"});
		const std::vector<String> expected = {"PATH=/usr/bin", "HOME=/home/user"};
		EXPECT_EQ(filtered, expected);
	}

	TEST(ProcessEnvFilterTest, KeepsAllWhenExcludeListEmpty) {
		const std::vector<String> entries = {"all_proxy=socks://127.0.0.1:1080", "PATH=/usr/bin"};
		EXPECT_EQ(FilterEnvEntries(entries, {}), entries);
	}

	TEST(ProcessEnvFilterTest, KeyMatchIsExactNotPrefix) {
		const std::vector<String> entries = {
			"no_proxy_extra=1",
			"my_no_proxy=1",
			"no_proxy=localhost",
		};
		const auto filtered = FilterEnvEntries(entries, {"no_proxy"});
		const std::vector<String> expected = {"no_proxy_extra=1", "my_no_proxy=1"};
		EXPECT_EQ(filtered, expected);
	}

	// 无 '=' 或以 '=' 开头的畸形条目（如 Windows 环境块的 "=C:=..." 隐藏条目）必须原样保留
	TEST(ProcessEnvFilterTest, MalformedEntriesAreKept) {
		const std::vector<String> entries = {
			"NOEQUALSENTRY",
			"=C:=C:\\Windows",
			"all_proxy=socks://127.0.0.1:1080",
		};
		const auto filtered = FilterEnvEntries(entries, {"all_proxy", "NOEQUALSENTRY"});
		const std::vector<String> expected = {"NOEQUALSENTRY", "=C:=C:\\Windows"};
		EXPECT_EQ(filtered, expected);
	}

	TEST(ProcessEnvFilterTest, KeyExtractionStopsAtFirstEquals) {
		const std::vector<String> entries = {"all_proxy=http://user:pass@host?a=b"};
		EXPECT_TRUE(FilterEnvEntries(entries, {"all_proxy"}).empty());
	}

#ifdef _WIN32
	// 集成验证：真实 spawn 路径上环境块构建与 CREATE_UNICODE_ENVIRONMENT 标志正确。
	// 子进程用 cmd 探测变量是否可见：已定义退出码 7，未定义退出码 0
	DWORD SpawnProbeAndGetExitCode(const std::vector<gdl::String>& exclude_env) {
		const std::vector<gdl::String> args = {
			"/d", "/c", "if defined gdl_test_proxy_probe (exit 7) else (exit 0)"};
		const auto pid = gdl::process::Execute("C:\\Windows\\System32\\cmd.exe", args, "", exclude_env);
		if (pid <= 0) return MAXDWORD;
		HANDLE process =
			OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
		if (process == nullptr) return MAXDWORD;
		DWORD exit_code = MAXDWORD;
		if (WaitForSingleObject(process, 10000) != WAIT_OBJECT_0 ||
			!GetExitCodeProcess(process, &exit_code)) {
			exit_code = MAXDWORD;
		}
		CloseHandle(process);
		return exit_code;
	}

	TEST(ProcessEnvFilterTest, SpawnedProcessInheritsVariableWithoutExclusion) {
		ASSERT_TRUE(SetEnvironmentVariableA("gdl_test_proxy_probe", "socks://127.0.0.1:9"));
		const auto exit_code = SpawnProbeAndGetExitCode({});
		SetEnvironmentVariableA("gdl_test_proxy_probe", nullptr);
		EXPECT_EQ(exit_code, 7u);
	}

	TEST(ProcessEnvFilterTest, SpawnedProcessDoesNotSeeExcludedVariable) {
		ASSERT_TRUE(SetEnvironmentVariableA("gdl_test_proxy_probe", "socks://127.0.0.1:9"));
		const auto exit_code = SpawnProbeAndGetExitCode({"gdl_test_proxy_probe"});
		SetEnvironmentVariableA("gdl_test_proxy_probe", nullptr);
		EXPECT_EQ(exit_code, 0u);
	}
#endif
}  // namespace
