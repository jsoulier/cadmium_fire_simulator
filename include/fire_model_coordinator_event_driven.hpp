#pragma once

#include "fire_model_coordinator.hpp"

class FireModelCoordinatorEventDriven : public FireModelCoordinator
{
public:
    using FireModelCoordinator::FireModelCoordinator;

protected:
    void SimulateInternal() override;
};
