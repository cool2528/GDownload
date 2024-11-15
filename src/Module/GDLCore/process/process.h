#pragma once
#include <vector>
#include "export.h"
#include "globalTypes.h"
namespace gdl {
	namespace process {
		GDLCore_API std::int64_t Execute(const String_View& command, const std::vector<String_View>& arguments,
										 const String_View& working_directory = "");
		GDLCore_API void Kill(std::int64_t pid);
		GDLCore_API void KillByName(const String_View& process_name);
		GDLCore_API bool IsProcessExist(const String_View& process_name);
		GDLCore_API bool IsProcessExist(std::int64_t pid);

	}  // namespace process

}  // namespace gdl
