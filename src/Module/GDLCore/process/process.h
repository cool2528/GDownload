#pragma once
#include "export.h"
#include "globalTypes.h"
#include <cstdint>
namespace gdl {
	namespace process {
		GDLCore_API int64_t Execute(const String_View& command, const std::vector<String>& arguments,
										 const String_View& working_directory = "");
		GDLCore_API void Kill(int64_t pid);
		// 优雅关闭子进程：先给 grace_ms 宽限自行退出，超时则强制终止，并回收（POSIX 防僵尸）（S3）
		GDLCore_API void ShutdownProcess(int64_t pid, int grace_ms);
		GDLCore_API void KillByName(const String_View& process_name);
		GDLCore_API bool IsProcessExist(const String_View& process_name);
		GDLCore_API bool IsProcessExistByPid(int64_t pid);

	}  // namespace process

}  // namespace gdl
