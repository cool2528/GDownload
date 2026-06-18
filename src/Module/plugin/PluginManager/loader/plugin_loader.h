#pragma once
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace gdl {
	namespace loader {
		namespace detail {
#if defined(_WIN32) || defined(_WIN64)
			inline std::wstring ToWidePath(const std::string& path) {
				auto convert = [&path](UINT code_page, DWORD flags) {
					int size = MultiByteToWideChar(code_page, flags, path.c_str(), -1, nullptr, 0);
					if (size <= 0) return std::wstring{};
					std::wstring result(static_cast<std::size_t>(size), L'\0');
					MultiByteToWideChar(code_page, flags, path.c_str(), -1, result.data(), size);
					if (!result.empty() && result.back() == L'\0') result.pop_back();
					return result;
				};

				auto utf8_path = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
				if (!utf8_path.empty()) return utf8_path;
				return convert(CP_ACP, 0);
			}
#endif
		}  // namespace detail

		// RAII wrapper for dynamic libraries. Non-copyable because it owns a native handle.
		class PluginLoader {
		   public:
			PluginLoader() = default;
			~PluginLoader() { UnLoad(); }

			PluginLoader(const PluginLoader&) = delete;
			PluginLoader& operator=(const PluginLoader&) = delete;
			PluginLoader(PluginLoader&& other) noexcept : library_handle_(other.library_handle_) {
				other.library_handle_ = nullptr;
			}
			PluginLoader& operator=(PluginLoader&& other) noexcept {
				if (this != &other) {
					UnLoad();
					library_handle_ = other.library_handle_;
					other.library_handle_ = nullptr;
				}
				return *this;
			}

			bool Load(const std::string& path) {
				UnLoad();
#if defined(_WIN32) || defined(_WIN64)
				// Use the wide API so non-ASCII paths are not interpreted via the process code page.
				std::wstring wpath = detail::ToWidePath(path);
				library_handle_ = LoadLibraryW(wpath.c_str());
#else
				library_handle_ = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
				return library_handle_ != nullptr;
			}
			void UnLoad() {
				if (!library_handle_) return;
#if defined(_WIN32) || defined(_WIN64)
				FreeLibrary(static_cast<HMODULE>(library_handle_));
#else
				dlclose(library_handle_);
#endif
				library_handle_ = nullptr;
			}
			void* GetSymbol(const std::string& symbol_name) {
				if (!library_handle_) return nullptr;
#if defined(_WIN32) || defined(_WIN64)
				return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library_handle_), symbol_name.c_str()));
#else
				return dlsym(library_handle_, symbol_name.c_str());
#endif
			}
			std::string GetError() const {
#if defined(_WIN32) || defined(_WIN64)
				DWORD error = GetLastError();
				char buffer[1024] = {0};
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, buffer, sizeof(buffer), nullptr);
				return buffer;
#else
				const char* err = dlerror();
				return err ? err : std::string();
#endif
			}

		   private:
			void* library_handle_{nullptr};
		};
	}  // namespace loader
}  // namespace gdl
