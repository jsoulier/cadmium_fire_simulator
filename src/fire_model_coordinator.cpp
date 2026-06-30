#include <memory>

#include "fire_model_coordinator.hpp"

FireModelCoordinator::FireModelCoordinator(std::shared_ptr<cadmium::Coupled> model)
    : LastTime{0.0}
{
    model->flatten();
    TopCoordinator = std::make_shared<cadmium::Coordinator>(model, 0.0);
    for (const auto& [portTo, portsFrom] : model->getICs())
    {
        Couplings.emplace_back(portTo, portsFrom);
    }
    for (auto& [componentId, component] : model->getComponents())
    {
        component->releasePortLookups();
    }
}

void FireModelCoordinator::SetLogger(const std::shared_ptr<cadmium::Logger>& log)
{
    Logger = log;
    TopCoordinator->setLogger(log);
}

void FireModelCoordinator::Start()
{
    if (Logger)
    {
        Logger->start();
    }
    TopCoordinator->setModelId(0);
    TopCoordinator->start(TopCoordinator->getTimeLast());
}

void FireModelCoordinator::Stop()
{
    TopCoordinator->stop(LastTime);
    if (Logger)
    {
        Logger->stop();
    }
}

void FireModelCoordinator::Simulate()
{
    SimulateInternal();
}
