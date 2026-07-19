#pragma once
#ifdef _WIN32
    #ifdef Ed2kEngine_EXPORTS
        #define Ed2kEngine_API __declspec(dllexport)
    #else
        #define Ed2kEngine_API __declspec(dllimport)
    #endif
#else
    #ifdef Ed2kEngine_EXPORTS
        #define Ed2kEngine_API __attribute__((visibility("default")))
    #else
        #define Ed2kEngine_API
    #endif
#endif
