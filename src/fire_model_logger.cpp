#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <string>

#include "fire_model_logger.hpp"
#include "fire_profile.hpp"

static constexpr int kQueueCapacity = 8192;

FireModelLogger::FireModelLogger(const std::string& filepath)
    : Logger()
{
    spdlog::init_thread_pool(kQueueCapacity, 1);
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
    Logger = std::make_shared<spdlog::async_logger>("fire", sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    Logger->set_pattern("%v");
}

void FireModelLogger::start()
{
    Logger->info("time,x,y,longitude,latitude,elevation,size,status");
}

void FireModelLogger::stop()
{
    Logger->flush();
}

void FireModelLogger::logState(double time, long modelId, const std::string& modelName, const std::string& state)
{
    Logger->info("{},{},{}", time, modelName, state);
}
