#include "logger.h"
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
namespace gdl
{
    static const std::string_view default_logger_name = "core";
    static std::vector<spdlog::sink_ptr> global_sinks;
    Logger::Logger(const std::string& name):logger_name_(name){

    }
    Logger::~Logger(){

    }
    std::shared_ptr<Logger> RegisterLogger(const std::string& name){
        auto logger = std::make_shared<spdlog::logger>(name,global_sinks.begin(),global_sinks.end());
        spdlog::register_logger(logger);

    }

    bool InitializeLoggers(){
        if(global_sinks.empty()){
            global_sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            auto file_logger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("gdl.log", 1024 * 1024 * 5, 3);
            global_sinks.push_back(file_logger);
            spdlog::initialize_logger(std::make_shared<spdlog::logger>(default_logger_name, global_sinks.begin(), global_sinks.end()));
        }
        return true;
    }
    bool ShutdownLoggers(){
        spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger){ logger->flush(); });
        return true;
    }

    void LogMessage(LogLevel level, const std::string& message){
        auto logger = spdlog::get(std::string(default_logger_name));
        if(logger){
            switch(level){
                case LogLevel::Info: logger->info(message); break;
                case LogLevel::Warning: logger->warn(message); break;
                case LogLevel::Error: logger->error(message); break;
                case LogLevel::Debug: logger->debug(message); break;
                case LogLevel::Fatal: logger->critical(message); break;
                default: logger->info(message); break;
            }
        }
    }
}