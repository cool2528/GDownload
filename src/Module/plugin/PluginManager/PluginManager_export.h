#ifdef _WIN32
#ifdef PluginManager_EXPORTS
#define PluginManager_API __declspec(dllexport)
#else
#define PluginManager_API __declspec(dllimport)
#endif
#else
#ifdef PluginManager_EXPORTS
#define PluginManager_API __attribute__((visibility("default")))
#else
#define PluginManager_API __attribute__((visibility("hidden")))
#endif
#endif
