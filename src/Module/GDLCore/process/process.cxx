#include "process.h"
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#elif defined(__APPLE__)
#include <signal.h>
#include <spawn.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(__linux__)
#include <spawn.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>  // for waitpid
#endif
#ifndef _WIN32
#include <cerrno>
#include <cstring>     // for strerror
extern char** environ;
#endif
#include <string_view>
#include <thread>
#include "logger.h"
#include "encoding/encoding.h"
namespace gdl {
	namespace process {
		namespace detail {
#ifdef _WIN32
			std::wstring QuoteWindowsArgument(std::wstring_view arg) {
				if (arg.empty()) return L"\"\"";

				bool needs_quotes = false;
				for (wchar_t ch : arg) {
					if (ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\v' || ch == L'"') {
						needs_quotes = true;
						break;
					}
				}
				if (!needs_quotes) return std::wstring(arg);

				std::wstring result = L"\"";
				std::size_t backslashes = 0;
				for (wchar_t ch : arg) {
					if (ch == L'\\') {
						++backslashes;
					}
					else if (ch == L'"') {
						result.append(backslashes * 2 + 1, L'\\');
						result.push_back(ch);
						backslashes = 0;
					}
					else {
						result.append(backslashes, L'\\');
						backslashes = 0;
						result.push_back(ch);
					}
				}
				result.append(backslashes * 2, L'\\');
				result.push_back(L'"');
				return result;
			}
#endif

			std::vector<std::int64_t> GetPidsByName(const String_View& name) {
				std::vector<std::int64_t> pids;
				const String name_string(name);
#ifdef _WIN32
				std::wstring wname = gdl::encoding::Utf8ToWString(name_string);
				HANDLE hSnapshot   = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (hSnapshot == INVALID_HANDLE_VALUE) {
					return pids;
				}
				PROCESSENTRY32 pe32;
				pe32.dwSize = sizeof(PROCESSENTRY32);
				if (!Process32First(hSnapshot, &pe32)) {
					CloseHandle(hSnapshot);
					return pids;
				}
				do {
					if (wcscmp(pe32.szExeFile, wname.c_str()) == 0) {
						pids.push_back(pe32.th32ProcessID);
					}
				} while (Process32Next(hSnapshot, &pe32));
				CloseHandle(hSnapshot);
#else
				String command = "pgrep " + name_string;
				FILE* pipe	   = popen(command.c_str(), "r");
				if (!pipe) {
					return pids;
				}
				char buffer[128];
				while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
					pid_t pid = std::stoi(buffer);
					pids.push_back(pid);
				}
				pclose(pipe);
#endif
				return pids;
			}
		}  // namespace detail
		int64_t Execute(const String_View& command, const std::vector<String>& arguments,
						 const String_View& working_directory) {
			std::int64_t pid{-1};
			const String command_string(command);
			const String working_directory_string(working_directory);
#ifdef _WIN32
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			si.dwFlags				  = STARTF_USESHOWWINDOW;
#if (defined(_WIN32) || defined(_WIN64)) && (defined(DEBUG) || defined(_DEBUG))
			si.wShowWindow = FALSE;
#else
			si.wShowWindow = FALSE;
#endif

			std::wstring application_name = gdl::encoding::Utf8ToWString(command_string);
			std::wstring command_line = detail::QuoteWindowsArgument(application_name);
			for (const auto& args : arguments) {
				command_line += L" " + detail::QuoteWindowsArgument(gdl::encoding::Utf8ToWString(args.data()));
			}
			std::wstring working_directory_w = gdl::encoding::Utf8ToWString(working_directory_string);
			BOOL ret =
				CreateProcess(application_name.data(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
							   working_directory_w.empty() ? nullptr : working_directory_w.data(), &si, &pi);
			if (!ret) {
				LOG_ERR("CreateProcessW failed: {}", GetLastError());
				return -1;
			}
			pid = pi.dwProcessId;
			::CloseHandle(pi.hProcess);
			::CloseHandle(pi.hThread);
			return pid;
#else
			pid_t native_pid;
			std::vector<char*> argv;
			argv.push_back(const_cast<char*>(command_string.c_str()));
			for (const auto& arg : arguments) {
				argv.push_back(const_cast<char*>(arg.data()));
			}
			argv.push_back(nullptr);

			posix_spawn_file_actions_t actions;
			posix_spawn_file_actions_init(&actions);

			posix_spawnattr_t attr;
			posix_spawnattr_init(&attr);
			// 如果有工作目录，设置它
			if (!working_directory_string.empty()) {
				posix_spawn_file_actions_addchdir_np(&actions, working_directory_string.c_str());
			}
			int ret = posix_spawn(&native_pid, command_string.c_str(), &actions, &attr, argv.data(), environ);

			posix_spawn_file_actions_destroy(&actions);
			posix_spawnattr_destroy(&attr);

			if (ret != 0) {
				LOG_ERR("posix_spawn failed: {}", strerror(ret));
				return -1;
			}

			pid = native_pid;
			return pid;
#endif
		}

		void Kill(int64_t pid) {
#ifdef _WIN32
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
			if (hProcess == nullptr) {
				LOG_ERR("OpenProcess failed: {}", GetLastError());
				return;
			}
			if (!TerminateProcess(hProcess, 0)) {
				LOG_ERR("TerminateProcess failed: {}", GetLastError());
			}
			CloseHandle(hProcess);
#else
			if (::kill(static_cast<pid_t>(pid), SIGKILL) != 0) {
				LOG_ERR("Failed to kill process {}: {}", pid, strerror(errno));
			}
#endif
		}

		void KillByName(const String_View& process_name) {
#ifdef _WIN32
			auto pids = detail::GetPidsByName(process_name);
			for (auto& pid : pids) {
				Kill(pid);
			}
#else
			auto pids = detail::GetPidsByName(process_name);
			for (auto& pid : pids) {
				Kill(pid);
			}
#endif
		}

		bool IsProcessExist(const String_View& process_name) {
			return !detail::GetPidsByName(process_name).empty();
		}
		bool IsProcessExistByPid(int64_t pid) {
#ifdef _WIN32
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, static_cast<DWORD>(pid));
			if (hProcess == nullptr) {
				return false;
			}
			CloseHandle(hProcess);
			return true;
#else
			return ::kill(static_cast<pid_t>(pid), 0) == 0;
#endif
		}
	}  // namespace process
}  // namespace gdl
