#include <cadmium/core/simulation/brute_force_root_coordinator.hpp>
#include <cadmium/core/simulation/event_driven_root_coordinator.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <format>
#include <limits>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include "fire_fuel_model.hpp"
#include "fire_grid_coupled.hpp"
#include "fire_model.hpp"
#include "fire_model_logger.hpp"
#include "fire_profile.hpp"
#include "fire_simulator.hpp"

FireSimulator::FireSimulator()
    : Width{0}
    , Height{0}
    , Resolution{30.0}
    , CoordinatorType{FireSimulatorCoordinatorType::EventDriven}
    , Igniting{[](int, int) { return false; }}
    , FuelModel{[](int, int) { return kFireFuelModelFM4; }}
    , Longitude{[](int, int) { return 0.0f; }}
    , Latitude{[](int, int) { return 0.0f; }}
    , Elevation{[](int, int) { return 0.0f; }}
    , Slope{[](int, int) { return 0.0f; }}
    , Aspect{[](int, int) { return 0.0f; }}
    , CanopyCover{[](int, int) { return 0.0f; }}
    , CanopyHeight{[](int, int) { return 0.0f; }}
    , CrownRatio{[](int, int) { return 0.0f; }}
    , WindSpeed{[](int, int, float) { return 5.0f; }}
    , WindDirection{[](int, int, float) { return 0.0f; }}
    , MoistureOneHour{[](int, int, float) { return 8.0f; }}
    , MoistureTenHour{[](int, int, float) { return 9.0f; }}
    , MoistureHundredHour{[](int, int, float) { return 10.0f; }}
    , MoistureLiveHerbaceous{[](int, int, float) { return 60.0f; }}
    , MoistureLiveWoody{[](int, int, float) { return 90.0f; }}
{
}

void FireSimulator::SetSize(int width, int height)
{
    Width = width;
    Height = height;
}

void FireSimulator::SetResolution(float resolution)
{
    Resolution = resolution;
}

void FireSimulator::SetCoordinatorType(FireSimulatorCoordinatorType coordinatorType)
{
    CoordinatorType = coordinatorType;
}

void FireSimulator::SetIgniting(std::function<bool(int x, int y)> igniting)
{
    Igniting = std::move(igniting);
}

void FireSimulator::SetFuelModel(std::function<FireFuelModelType(int x, int y)> fuelModel)
{
    FuelModel = std::move(fuelModel);
}

void FireSimulator::SetLongitude(std::function<double(int x, int y)> longitude)
{
    Longitude = std::move(longitude);
}

void FireSimulator::SetLatitude(std::function<double(int x, int y)> latitude)
{
    Latitude = std::move(latitude);
}

void FireSimulator::SetElevation(std::function<float(int x, int y)> elevation)
{
    Elevation = std::move(elevation);
}

void FireSimulator::SetSlope(std::function<float(int x, int y)> slope)
{
    Slope = std::move(slope);
}

void FireSimulator::SetAspect(std::function<float(int x, int y)> aspect)
{
    Aspect = std::move(aspect);
}

void FireSimulator::SetCanopyCover(std::function<float(int x, int y)> canopyCover)
{
    CanopyCover = std::move(canopyCover);
}

void FireSimulator::SetCanopyHeight(std::function<float(int x, int y)> canopyHeight)
{
    CanopyHeight = std::move(canopyHeight);
}

void FireSimulator::SetCrownRatio(std::function<float(int x, int y)> crownRatio)
{
    CrownRatio = std::move(crownRatio);
}

void FireSimulator::SetWindSpeed(std::function<float(int x, int y, float time)> windSpeed)
{
    WindSpeed = std::move(windSpeed);
}

void FireSimulator::SetWindDirection(std::function<float(int x, int y, float time)> windDirection)
{
    WindDirection = std::move(windDirection);
}

void FireSimulator::SetMoistureOneHour(std::function<float(int x, int y, float time)> moistureOneHour)
{
    MoistureOneHour = std::move(moistureOneHour);
}

void FireSimulator::SetMoistureTenHour(std::function<float(int x, int y, float time)> moistureTenHour)
{
    MoistureTenHour = std::move(moistureTenHour);
}

void FireSimulator::SetMoistureHundredHour(std::function<float(int x, int y, float time)> moistureHundredHour)
{
    MoistureHundredHour = std::move(moistureHundredHour);
}

void FireSimulator::SetMoistureLiveHerbaceous(std::function<float(int x, int y, float time)> moistureLiveHerbaceous)
{
    MoistureLiveHerbaceous = std::move(moistureLiveHerbaceous);
}

void FireSimulator::SetMoistureLiveWoody(std::function<float(int x, int y, float time)> moistureLiveWoody)
{
    MoistureLiveWoody = std::move(moistureLiveWoody);
}

void FireSimulator::SetOutPath(std::string outPath)
{
    OutPath = std::move(outPath);
}

float FireSimulator::GetWindSpeed(int x, int y, float time) const
{
    return WindSpeed(x, y, time);
}

float FireSimulator::GetWindDirection(int x, int y, float time) const
{
    return WindDirection(x, y, time);
}

float FireSimulator::GetMoistureOneHour(int x, int y, float time) const
{
    return MoistureOneHour(x, y, time);
}

float FireSimulator::GetMoistureTenHour(int x, int y, float time) const
{
    return MoistureTenHour(x, y, time);
}

float FireSimulator::GetMoistureHundredHour(int x, int y, float time) const
{
    return MoistureHundredHour(x, y, time);
}

float FireSimulator::GetMoistureLiveHerbaceous(int x, int y, float time) const
{
    return MoistureLiveHerbaceous(x, y, time);
}

float FireSimulator::GetMoistureLiveWoody(int x, int y, float time) const
{
    return MoistureLiveWoody(x, y, time);
}

bool FireSimulator::Simulate()
{
    spdlog::info("Creating fire simulation parameters");
    int columns = Width;
    int rows = Height;
    try
    {
        nlohmann::json scenario;
        scenario["scenario"] = {
            {"shape", {columns, rows}},
            {"origin", {0, 0}},
            {"wrapped", false},
            {"neighborhood", {
                {"type", "moore"},
                {"range", 1},
                {"vicinity", 1}
            }}
        };
        auto model = std::make_shared<FireGridCoupled>(
            "fire", [this](const auto& id, const auto& config)
            {
                return std::make_shared<FireModel>(*this, id, config, Resolution);
            },
            std::move(scenario));
        {
            FireProfileTagBlock("Providers");
            nlohmann::json neighborhood = nlohmann::json::array({{
                {"type", "moore"},
                {"range", 1},
                {"vicinity", 1}
            }});
            for (int y = 0; y < rows; y++)
            {
                for (int x = 0; x < columns; x++)
                {
                    FireFuelModelType fuelModel = FuelModel(x, y);
                    if (!FireFuelModelTypeIsBurnable(fuelModel))
                    {
                        continue;
                    }
                    nlohmann::json config = {
                        {"cell_map", nlohmann::json::array({nlohmann::json::array({x, y})})},
                        {"neighborhood", neighborhood},
                        {"state", {
                            {"Status", Igniting(x, y) ? int(FireCellStatus::Igniting) : 0}
                        }},
                        {"config", {
                            {"FuelModel", fuelModel},
                            {"Slope", Slope(x, y)},
                            {"Aspect", Aspect(x, y)},
                            {"Longitude", Longitude(x, y)},
                            {"Latitude", Latitude(x, y)},
                            {"Height", Elevation(x, y)},
                            {"CanopyCover", CanopyCover(x, y)},
                            {"CanopyHeight", CanopyHeight(x, y)},
                            {"CrownRatio", CrownRatio(x, y)},
                        }}
                    };
                    model->addCells(model->loadCellConfig(std::format("{},{}", x, y), config));
                }
            }
        }
        {
            FireProfileTagBlock("Models");
            spdlog::info("Building fire simulation couplings");
            model->addCouplings();
        }
        const auto simulate = [this](auto coordinator)
        {
            coordinator.setLogger(std::make_shared<FireModelLogger>(OutPath));
            coordinator.start();
            spdlog::info("Started fire simulation");
            std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
            coordinator.simulate();
            std::chrono::duration<double> elapsedTime = std::chrono::steady_clock::now() - startTime;
            spdlog::info("Completed fire simulation in {:.3f}s", elapsedTime.count());
            coordinator.stop();
        };
        switch (CoordinatorType)
        {
        case FireSimulatorCoordinatorType::BruteForce:
            simulate(cadmium::BruteForceRootCoordinator(model));
            break;
        case FireSimulatorCoordinatorType::EventDriven:
            simulate(cadmium::EventDrivenRootCoordinator(model));
            break;
        }
    }
    catch (const std::exception& e)
    {
        spdlog::info("Fire simulation failed: {}", e.what());
        return false;
    }
    return true;
}
