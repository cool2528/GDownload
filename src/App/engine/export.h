#ifdef _WIN32
#ifdef Engine_EXPORTS
#define Engine_API __declspec(dllexport)
#else
#define Engine_API __declspec(dllimport)
#endif
#else
#ifdef Engine_EXPORTS
#define Engine_API __attribute__((visibility("default")))
#else
#define Engine_API __attribute__((visibility("hidden")))
#endif
#endif
