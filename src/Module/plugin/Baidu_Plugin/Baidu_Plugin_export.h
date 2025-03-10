#pragma once
#ifdef _WIN32
    #ifdef Baidu_Plugin_EXPORTS
        #define Baidu_Plugin_API __declspec(dllexport)
    #else
        #define Baidu_Plugin_API __declspec(dllimport)
    #endif
#else
    #ifdef Baidu_Plugin_EXPORTS
        #define Baidu_Plugin_API __attribute__((visibility("default")))
    #else
        #define Baidu_Plugin_API
    #endif
#endif
