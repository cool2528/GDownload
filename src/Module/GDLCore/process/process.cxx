#include "process.h"
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <spawn.h>
extern char **environ;  // Add this declaration
#endif
#include <thread>
#include "logger.h"
namespace gdl {
	namespace process {
		namespace detail {
			std::vector<std::int64_t> GetPidsByName(const String_View& name) {
				std::vector<std::int64_t> pids;
#ifdef _WIN32
#else
				String command = "pgrep " + String(name);
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
		std::int64_t Execute(const String_View& command, const std::vector<String>& arguments,
							 const String_View& working_directory) {
			std::int64_t pid{-1};
#ifdef _WIN32

#else
			pid_t native_pid;
			std::vector<char*> argv;
			argv.push_back(const_cast<char*>(command.data()));
			for (const auto& arg : arguments) {
				argv.push_back(const_cast<char*>(arg.data()));
			}
			argv.push_back(nullptr);

			posix_spawn_file_actions_t actions;
			posix_spawn_file_actions_init(&actions);
			
			posix_spawnattr_t attr;
			posix_spawnattr_init(&attr);

			int ret = posix_spawn(&native_pid, command.data(), &actions, &attr, argv.data(), environ);
			
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

		void Kill(std::int64_t process_id) {
#ifdef _WIN32

#else
			auto pid = static_cast<pid_t>(process_id);
			if (::kill(pid, SIGKILL) != 0) {
				LOG_ERR("kill pid {} fail", pid);
			}
#endif
		}

		void KillByName(const String_View& process_name) {
#ifdef _WIN32

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
		bool IsProcessExist(std::int64_t pid) {
#ifdef _WIN32

#else
			return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
		}
	}  // namespace process
}  // namespace gdl
