#pragma once
#include "export.h"
#include <string>
#include <memory>
#include <format>
namespace gdl
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };
    class GDLCore_API Logger
    {
    private:
    public:
        Logger(const std::string& name);
        virtual ~Logger();
        private:
        std::string logger_name_;
    };

    GDLCore_API std::shared_ptr<Logger> RegisterLogger(const std::string& name);
    GDLCore_API bool InitializeLoggers();
    GDLCore_API bool ShutdownLoggers();
    GDLCore_API void LogMessage(LogLevel level, const std::string& message);

    template <typename... Args>
    void Debug(const std::string& fmt, Args... args){
        LogMessage(LogLevel::Debug, std::vformat(fmt, std::make_format_args(args...)));
    }
    template <typename... Args>
    void Info(const std::string& fmt, Args... args){
        LogMessage(LogLevel::Info, std::vformat(fmt, std::make_format_args(args...)));
    }
    template <typename... Args>
    void Warning(const std::string& fmt, Args... args){
        LogMessage(LogLevel::Warning, std::vformat(fmt, std::make_format_args(args...)));

    }
    template <typename... Args>
    void Error(const std::string& fmt, Args... args){
        LogMessage(LogLevel::Error, std::vformat(fmt, std::make_format_args(args...)));
    }
    template <typename... Args>
    void Fatal(const std::string& fmt, Args... args){
        LogMessage(LogLevel::Fatal, std::vformat(fmt, std::make_format_args(args...)));
    }
}