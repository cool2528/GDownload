#pragma once

#ifdef _WIN32
    #ifdef GDLCore_EXPORTS
        #define GDLCore_API __declspec(dllexport)
    #else
        #define GDLCore_API __declspec(dllimport)
    #endif
#else
    #ifdef GDLCore_EXPORTS
        #define GDLCore_API __attribute__((visibility("default")))
    #else
        #define GDLCore_API
    #endif
#endif
