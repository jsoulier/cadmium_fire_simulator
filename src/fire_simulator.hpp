#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <string>

#include "fire_fuel_model.hpp"

enum class FireSimulatorCoordinatorType
{
    EventDriven, // cells are only computed if they have incoming messages 
    BruteForce,  // cells are computed each frame
};

static constexpr const char* kFireSimulatorCoordinatorTypeStrings[] =
{
    "Event Driven",
    "Brute Force",
};

NLOHMANN_JSON_SERIALIZE_ENUM(FireSimulatorCoordinatorType, {
    {FireSimulatorCoordinatorType::EventDriven, "EventDriven"},
    {FireSimulatorCoordinatorType::BruteForce, "BruteForce"},
})

class FireSimulator
{
public:
    FireSimulator();
    void SetSize(int width, int height);
    void SetResolution(float resolution);
    void SetCoordinatorType(FireSimulatorCoordinatorType coordinatorType);
    void SetIgniting(std::function<bool(int x, int y)> igniting);
    void SetFuelModel(std::function<FireFuelModelType(int x, int y)> fuelModel);
    void SetLongitude(std::function<double(int x, int y)> longitude);
    void SetLatitude(std::function<double(int x, int y)> latitude);
    void SetElevation(std::function<float(int x, int y)> elevation);
    void SetSlope(std::function<float(int x, int y)> slope);
    void SetAspect(std::function<float(int x, int y)> aspect);
    void SetCanopyCover(std::function<float(int x, int y)> canopyCover);
    void SetCanopyHeight(std::function<float(int x, int y)> canopyHeight);
    void SetCrownRatio(std::function<float(int x, int y)> crownRatio);
    void SetWindSpeed(std::function<float(int x, int y, float time)> windSpeed);
    void SetWindDirection(std::function<float(int x, int y, float time)> windDirection);
    void SetMoistureOneHour(std::function<float(int x, int y, float time)> moistureOneHour);
    void SetMoistureTenHour(std::function<float(int x, int y, float time)> moistureTenHour);
    void SetMoistureHundredHour(std::function<float(int x, int y, float time)> moistureHundredHour);
    void SetMoistureLiveHerbaceous(std::function<float(int x, int y, float time)> moistureLiveHerbaceous);
    void SetMoistureLiveWoody(std::function<float(int x, int y, float time)> moistureLiveWoody);
    void SetOutPath(std::string outPath);
    float GetWindSpeed(int x, int y, float time) const;
    float GetWindDirection(int x, int y, float time) const;
    float GetMoistureOneHour(int x, int y, float time) const;
    float GetMoistureTenHour(int x, int y, float time) const;
    float GetMoistureHundredHour(int x, int y, float time) const;
    float GetMoistureLiveHerbaceous(int x, int y, float time) const;
    float GetMoistureLiveWoody(int x, int y, float time) const;
    bool Simulate();

private:
    int Width;
    int Height;
    float Resolution;
    FireSimulatorCoordinatorType CoordinatorType;
    std::function<bool(int x, int y)> Igniting;
    std::function<FireFuelModelType(int x, int y)> FuelModel;
    std::function<double(int x, int y)> Longitude;
    std::function<double(int x, int y)> Latitude;
    std::function<float(int x, int y)> Elevation;
    std::function<float(int x, int y)> Slope;
    std::function<float(int x, int y)> Aspect;
    std::function<float(int x, int y)> CanopyCover;
    std::function<float(int x, int y)> CanopyHeight;
    std::function<float(int x, int y)> CrownRatio;
    std::function<float(int x, int y, float time)> WindSpeed;
    std::function<float(int x, int y, float time)> WindDirection;
    std::function<float(int x, int y, float time)> MoistureOneHour;
    std::function<float(int x, int y, float time)> MoistureTenHour;
    std::function<float(int x, int y, float time)> MoistureHundredHour;
    std::function<float(int x, int y, float time)> MoistureLiveHerbaceous;
    std::function<float(int x, int y, float time)> MoistureLiveWoody;
    std::string OutPath;
};
