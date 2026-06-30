#include <cstdint>

#include "fire_fuel_model.hpp"

bool FireFuelModelTypeIsBurnable(FireFuelModelType fuelModel)
{
    return fuelModel > 0 && (fuelModel < kFireFuelModelNB1 || fuelModel > kFireFuelModelNB9);
}
