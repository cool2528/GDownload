#include "logger.h"
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <mutex>
#include <vector>
namespace gdl {
	// default loooger name
	static const std::string_view kDefaultLoggerName = "core";
	// Global Log Sinks
	static std::vector<spdlog::sink_ptr> kGlobakLoggerSinks;
	static std::atomic_bool kIsShutdown{false};
	static std::mutex kSinksMutex;  // 保护 kGlobakLoggerSinks（B8）
	Logger::Logger(const std::string& name) : logger_name_(name) {}
	Logger::~Logger() {}

	void Logger::LogMessage(LogLevel level, const std::string& message, const std::source_location& loc) {
		if (kIsShutdown.load()) return;
		auto logger = spdlog::get(this->logger_name_);
		spdlog::source_loc local_source(loc.file_name(), loc.line(), loc.function_name());
		if (logger) {
			logger->log(local_source, static_cast<spdlog::level::level_enum>(level), message);
		}
	}
	std::shared_ptr<Logger> RegisterLogger(const std::string& name) {
		if (kIsShutdown.load()) return nullptr;
		// 已注册同名 logger 则复用，避免 register_logger 抛未捕获 spdlog_ex（B8）
		if (!spdlog::get(name)) {
			std::shared_ptr<spdlog::logger> logger;
			{
				std::lock_guard<std::mutex> lock(kSinksMutex);
				logger = std::make_shared<spdlog::logger>(name, kGlobakLoggerSinks.begin(), kGlobakLoggerSinks.end());
			}
			logger->set_level(spdlog::get_level());
			logger->flush_on(spdlog::get_level());
			try {
				spdlog::register_logger(logger);
			} catch (const spdlog::spdlog_ex&) {
				// 并发下可能已被其他线程注册，忽略
			}
		}
		return std::make_shared<gdl::Logger>(name);
	}

	bool InitializeLoggers(const std::string& file_path) {
		// 互斥保护初始化：线程安全，且 Shutdown 清空 sinks 后可重新初始化（B8）
		std::lock_guard<std::mutex> lock(kSinksMutex);
		if (kGlobakLoggerSinks.empty()) {
			kGlobakLoggerSinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			try {
				kGlobakLoggerSinks.push_back(
					std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file_path, 1024 * 1024 * 10, 3));
			} catch (const std::exception& e) {
				// 文件 sink 构造失败（权限/路径）降级为仅控制台，不阻塞启动（B8）
				spdlog::warn("file sink init failed, console only: {}", e.what());
			}
			spdlog::flush_on(spdlog::get_level());
			if (!spdlog::get(std::string(kDefaultLoggerName))) {
				spdlog::initialize_logger(std::make_shared<spdlog::logger>(
					std::string(kDefaultLoggerName), kGlobakLoggerSinks.begin(), kGlobakLoggerSinks.end()));
			}
		}
		kIsShutdown = false;
		return true;
	}
	bool ShutdownLoggers() {
		// 先置标志阻止新日志写入，再 flush + shutdown，
		// 避免 shutdown 期间其他线程仍调用 spdlog 造成数据竞争或访问已析构 registry。
		kIsShutdown = true;
		spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) { logger->flush(); });
		spdlog::shutdown();
		{
			std::lock_guard<std::mutex> lock(kSinksMutex);
			kGlobakLoggerSinks.clear();  // 清空 sinks，支持 Shutdown 后重新 Initialize（B8）
		}
		return true;
	}

	void LogMessage(LogLevel level, const std::source_location& loc, const std::string& message) {
		if (kIsShutdown.load()) return;
		auto logger = spdlog::get(std::string(kDefaultLoggerName));
		spdlog::source_loc local_source(loc.file_name(), loc.line(), loc.function_name());
		if (logger) {
			logger->log(local_source, static_cast<spdlog::level::level_enum>(level), message);
		}
	}

	void LogMessage(LogLevel level, const std::string& message) {
		if (kIsShutdown.load()) return;
		auto logger = spdlog::get(std::string(kDefaultLoggerName));
		if (logger) {
			logger->log(static_cast<spdlog::level::level_enum>(level), message);
		}
	}

	void SetLoggerLevel(LogLevel level) {
		spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
		spdlog::flush_on(spdlog::get_level());
	}

    LogLevel GetLoggerLevel() {
        return static_cast<LogLevel>(spdlog::get_level());
    }

}  // namespace gdl
