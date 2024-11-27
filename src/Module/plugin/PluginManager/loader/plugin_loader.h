#pragma once
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace gdl {
	namespace loader {
		class PluginLoader {
		   public:
			bool Load(const std::string& path) {
#if defined(_WIN32) || defined(_WIN64)
				library_handle_ = LoadLibraryA(path.c_str());
#else
				library_handle_ = dlopen(path.c_str(), RTLD_LAZY);
#endif
				return library_handle_ != nullptr;
			}
			void UnLoad() const {
#if defined(_WIN32) || defined(_WIN64)
				FreeLibrary((HMODULE)library_handle_);
#else
				dlclose(library_handle_);
#endif
			}
			void* GetSymbol(const std::string& symbol_name) {
				if (!library_handle_) return nullptr;
#if defined(_WIN32) || defined(_WIN64)
				return (void*)GetProcAddress((HMODULE)library_handle_, symbol_name.c_str());
#else
				return dlsym(library_handle_, symbol_name.c_str());
#endif
			}
			std::string GetError() {
#if defined(_WIN32) || defined(_WIN64)
				DWORD error = GetLastError();
				static char buffer[1024];
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, buffer, sizeof(buffer), nullptr);
				return buffer;
#else
				return dlerror();
#endif
			}

		   private:
			void* library_handle_{nullptr};
		};
	}  // namespace loader
}  // namespace gdl
