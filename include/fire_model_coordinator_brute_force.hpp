#pragma once

#include "fire_model_coordinator.hpp"

class FireModelCoordinatorBruteForce : public FireModelCoordinator
{
public:
    using FireModelCoordinator::FireModelCoordinator;

protected:
    void SimulateInternal() override;
};
