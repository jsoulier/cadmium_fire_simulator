#pragma once

#include <cadmium/core/logger/logger.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

class FireModelLogger : public cadmium::Logger
{
public:
    FireModelLogger(const std::string& filepath);
    void start() override;
    void stop() override;
    void logOutput(double time, long modelId, const std::string& modelName, const std::string& port, const std::string& value) override {}
    void logState(double time, long modelId, const std::string& modelName, const std::string& state) override;

private:
    std::shared_ptr<spdlog::logger> Logger;
};
