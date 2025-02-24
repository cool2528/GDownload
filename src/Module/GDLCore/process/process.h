#pragma once
#include "export.h"
#include "globalTypes.h"
#include <cstdint>
namespace gdl {
	namespace process {
		GDLCore_API int64_t Execute(const String_View& command, const std::vector<String>& arguments,
										 const String_View& working_directory = "");
		GDLCore_API void Kill(int64_t pid);
		GDLCore_API void KillByName(const String_View& process_name);
		GDLCore_API bool IsProcessExist(const String_View& process_name);
		GDLCore_API bool IsProcessExistByPid(int64_t pid);

	}  // namespace process

}  // namespace gdl
