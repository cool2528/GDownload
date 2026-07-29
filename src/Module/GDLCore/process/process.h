#pragma once
#include "export.h"
#include "globalTypes.h"
#include <cstdint>
namespace gdl {
	namespace process {
		// 从 "KEY=VALUE" 形式的环境条目中剔除键名命中 exclude_names 的条目（键名比较大小写不敏感）；
		// 无键名的畸形条目（如 Windows 环境块的 "=C:=..." 隐藏条目）原样保留
		GDLCore_API std::vector<String> FilterEnvEntries(const std::vector<String>& entries,
														 const std::vector<String>& exclude_names);
		// exclude_env 非空时，子进程环境为父进程环境剔除指定变量后的副本；为空则完整继承
		GDLCore_API int64_t Execute(const String_View& command, const std::vector<String>& arguments,
										 const String_View& working_directory = "",
										 const std::vector<String>& exclude_env = {});
		GDLCore_API void Kill(int64_t pid);
		// 优雅关闭子进程：先给 grace_ms 宽限自行退出，超时则强制终止，并回收（POSIX 防僵尸）（S3）
		GDLCore_API void ShutdownProcess(int64_t pid, int grace_ms);
		GDLCore_API void KillByName(const String_View& process_name);
		GDLCore_API bool IsProcessExist(const String_View& process_name);
		GDLCore_API bool IsProcessExistByPid(int64_t pid);

	}  // namespace process

}  // namespace gdl
