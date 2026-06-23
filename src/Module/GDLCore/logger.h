#pragma once
#include <fmt/format.h>
#include <memory>
#include <source_location>
#include <string>
#include "export.h"
namespace gdl {
	enum class LogLevel {
		kTrace	  = 0,
		kDebug	  = 1,
		kInfo	  = 2,
		kWarn	  = 3,
		kError	  = 4,
		kCritical = 5,
		kOff	  = 6,
		kNumLevels
	};
	class GDLCore_API Logger {
	   private:
	   public:
		Logger(const std::string& name);
		virtual ~Logger();
		void LogMessage(LogLevel level, const std::string& message,
						const std::source_location& loc = std::source_location::current());

	   private:
		std::string logger_name_;
	};

	GDLCore_API std::shared_ptr<Logger> RegisterLogger(const std::string& name);
	GDLCore_API bool InitializeLoggers(const std::string& file_path);
	GDLCore_API bool ShutdownLoggers();
	GDLCore_API void SetLoggerLevel(LogLevel level);
	GDLCore_API void LogMessage(LogLevel level, const std::source_location& loc, const std::string& message);
	GDLCore_API void LogMessage(LogLevel level, const std::string& message);
    GDLCore_API LogLevel GetLoggerLevel();

	template <typename... Args>
	void LogDebug(const std::string& format, Args... args) {
		LogMessage(LogLevel::kDebug, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogInfo(const std::string& format, Args... args) {
		LogMessage(LogLevel::kInfo, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogWarning(const std::string& format, Args... args) {
		LogMessage(LogLevel::kWarn, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogError(const std::string& format, Args... args) {
		LogMessage(LogLevel::kError, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogCritical(const std::string& format, Args... args) {
		LogMessage(LogLevel::kCritical, fmt::vformat(format, fmt::make_format_args(args...)));
	}

	template <typename... Args>
	void LogDebug(const std::source_location& loc, const std::string& format, Args... args) {
		LogMessage(LogLevel::kDebug, loc, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogInfo(const std::source_location& loc, const std::string& format, Args... args) {
		LogMessage(LogLevel::kInfo, loc, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogWarning(const std::source_location& loc, const std::string& format, Args... args) {
		LogMessage(LogLevel::kWarn, loc, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogError(const std::source_location& loc, const std::string& format, Args... args) {
		LogMessage(LogLevel::kError, loc, fmt::vformat(format, fmt::make_format_args(args...)));
	}
	template <typename... Args>
	void LogCritical(const std::source_location& loc, const std::string& format, Args... args) {
		LogMessage(LogLevel::kCritical, loc, fmt::vformat(format, fmt::make_format_args(args...)));
	}

}  // namespace gdl

#define LOG_DBG(fmt, ...) gdl::LogDebug(std::source_location::current(), fmt, ##__VA_ARGS__);

#define LOG_INFO(fmt, ...) gdl::LogInfo(std::source_location::current(), fmt, ##__VA_ARGS__);

#define LOG_WARN(fmt, ...) gdl::LogWarning(std::source_location::current(), fmt, ##__VA_ARGS__);

#define LOG_ERR(fmt, ...) gdl::LogError(std::source_location::current(), fmt, ##__VA_ARGS__);

#define LOG_CRIT(fmt, ...) gdl::LogCritical(std::source_location::current(), fmt, ##__VA_ARGS__);
