#include "process.h"
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
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
		std::int64_t Execute(const String_View& command, const std::vector<String_View>& arguments,
							 const String_View& working_directory) {
			std::int64_t pid{-1};
#ifdef _WIN32

#else
			pid = vfork();
			if (pid < 0) {
				LOG_ERR("fork fail {}", command);
				return pid;
			}
			else if (pid == 0) {
				if (!working_directory.empty()) {
					if (chdir(working_directory.data()) != 0) {
						_exit(1);
					}
				}
				std::vector<char*> c_args;
				for (const auto& arg : arguments) {
					c_args.push_back(const_cast<char*>(arg.data()));
				}
				c_args.push_back(nullptr);
				auto ret = execvp(c_args[0], c_args.data());
				_exit(1);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				int status;
				pid_t result = waitpid(pid, &status, WNOHANG);
				if (result == 0) {
					return pid;
				}
				else if (result == pid) {
					if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
						LOG_ERR("Command {} failed to start properly", command);
					}
					return pid;
				}
				else {
					LOG_ERR("Error checking child process status: {}", strerror(errno));
					return pid;
				}
			}
#endif
			return pid;
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
