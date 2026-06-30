#pragma once

#include <cadmium/core/logger/logger.hpp>
#include <cadmium/core/simulation/coordinator.hpp>
#include <taskflow/taskflow.hpp>

#include <limits>
#include <memory>
#include <utility>
#include <vector>

class FireModelCoordinator
{
public:
    FireModelCoordinator(std::shared_ptr<cadmium::Coupled> model);
    virtual ~FireModelCoordinator() = default;
    void SetLogger(const std::shared_ptr<cadmium::Logger>& log);
    void Start();
    void Stop();
    void Simulate();

protected:
    virtual void SimulateInternal() = 0;

    using ManyToOneCoupling = std::pair<std::shared_ptr<cadmium::PortInterface>, std::vector<std::shared_ptr<cadmium::PortInterface>>>;
    using OneToOneCoupling = std::pair<std::shared_ptr<cadmium::PortInterface>, std::shared_ptr<cadmium::PortInterface>>;

    static constexpr double kMaxTime = std::numeric_limits<double>::infinity();
    std::shared_ptr<cadmium::Coordinator> TopCoordinator;
    std::vector<ManyToOneCoupling> Couplings;
    std::shared_ptr<cadmium::Logger> Logger;
    tf::Executor Executor;
    double LastTime;
};
