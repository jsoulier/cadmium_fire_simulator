#include <behaveRun.h>
#include <fuelModels.h>
#include <species_master_table.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <numbers>
#include <sstream>
#include <string>

#include "fire_fuel_model.hpp"
#include "fire_model.hpp"
#include "fire_profile.hpp"

static constexpr float kInfinity = std::numeric_limits<float>::infinity();
static constexpr float kDefaultMoisture1Hour = 8.0;
static constexpr float kDefaultMoisture10Hour = 9.0;
static constexpr float kDefaultMoisture100Hour = 10.0;
static constexpr float kDefaultMoistureLiveHerbaceous = 60.0;
static constexpr float kDefaultMoistureLiveWoody = 90.0;

static BehaveRun& GetBehaveRun()
{
    thread_local FuelModels fuelModels;
    thread_local SpeciesMasterTable speciesTable;
    thread_local BehaveRun behave(fuelModels, speciesTable);
    return behave;
}

FireState::FireState()
    : Status(FireCellStatus::Unburnt)
    , BurnTime(kInfinity)
{
}

bool operator!=(const FireState& lhs, const FireState& rhs)
{
    return lhs.Status != rhs.Status || lhs.BurnTime != rhs.BurnTime;
}

std::ostream& operator<<(std::ostream& stream, const FireState& state)
{
    return stream << int(state.Status);
}

FireModel::FireModel(
    const cadmium::celldevs::coordinates& id,
    const std::shared_ptr<const cadmium::celldevs::GridCellConfig<FireState, double>>& config,
    float resolution)
    : cadmium::celldevs::GridCell<FireState, double>(id, config)
    , Resolution{resolution}
    , FuelModel{0}
    , Slope{0.0f}
    , Aspect{0.0f}
    , Longitude{0.0}
    , Latitude{0.0}
    , Height{0.0f}
    , WindSpeed{0.0f}
    , WindDirection{0.0f}
    , CanopyCover{0.0f}
    , CanopyHeight{0.0f}
    , CrownRatio{0.0f}
    , Moisture1Hour{kDefaultMoisture1Hour}
    , Moisture10Hour{kDefaultMoisture10Hour}
    , Moisture100Hour{kDefaultMoisture100Hour}
    , MoistureLiveHerbaceous{kDefaultMoistureLiveHerbaceous}
    , MoistureLiveWoody{kDefaultMoistureLiveWoody}
{
    const nlohmann::json& cellConfig = config->rawCellConfig;
    assert(cellConfig.is_object());
    FuelModel = cellConfig.value("FuelModel", FuelModel);
    Slope = cellConfig.value("Slope", Slope);
    Aspect = cellConfig.value("Aspect", Aspect);
    Longitude = cellConfig.value("Longitude", Longitude);
    Latitude = cellConfig.value("Latitude", Latitude);
    Height = cellConfig.value("Height", Height);
    WindSpeed = cellConfig.value("WindSpeed", WindSpeed);
    WindDirection = cellConfig.value("WindDirection", WindDirection);
    CanopyCover = cellConfig.value("CanopyCover", CanopyCover);
    CanopyHeight = cellConfig.value("CanopyHeight", CanopyHeight);
    CrownRatio = cellConfig.value("CrownRatio", CrownRatio);
    Moisture1Hour = cellConfig.value("Moisture1Hour", Moisture1Hour);
    Moisture10Hour = cellConfig.value("Moisture10Hour", Moisture10Hour);
    Moisture100Hour = cellConfig.value("Moisture100Hour", Moisture100Hour);
    MoistureLiveHerbaceous = cellConfig.value("MoistureLiveHerbaceous", MoistureLiveHerbaceous);
    MoistureLiveWoody = cellConfig.value("MoistureLiveWoody", MoistureLiveWoody);
}

std::ostream& operator<<(std::ostream& stream, const FireModel& model)
{
    return stream << std::format("{:.8f},{:.8f},{:.2f},{:.2f},", model.Longitude, model.Latitude, model.Height, model.Resolution);
}

bool FireModel::isComplete() const
{
    return state.Status == FireCellStatus::Burnt;
}

FireState FireModel::localComputation(FireState state, const cadmium::celldevs::Neighborhood<cadmium::celldevs::coordinates, FireState, double>& neighborhood) const
{
    FireProfileTag();
    if (state.Status == FireCellStatus::Burnt)
    {
        return state;
    }
    if (state.Status == FireCellStatus::Burning)
    {
        // if the burning cell's state was broadcasted back to the current cell, it's finished burning
        for (const auto& [neighborId, neighbor] : neighborhood)
        {
            if (neighborId == id)
            {
                if (neighbor.state != nullptr && neighbor.state->Status == FireCellStatus::Burning)
                {
                    state.Status = FireCellStatus::Burnt;
                }
                break;
            }
        }
        return state;
    }
    if (!FireFuelModelTypeIsBurnable(FuelModel))
    {
        return state;
    }
    float burnTime = kInfinity;
    if (state.Status == FireCellStatus::Igniting)
    {
        float headRate = GetMaxSpreadRate();
        if (headRate > 0.0f)
        {
            burnTime = Resolution / headRate;
        }
    }
    else
    {
        FireProfileTagBlock("Neighborhood");
        for (const auto& [neighborId, neighbor] : neighborhood)
        {
            if (neighborId == id || neighbor.state == nullptr)
            {
                continue;
            }
            if (neighbor.state->Status != FireCellStatus::Burning)
            {
                continue;
            }
            int deltaE = id[0] - neighborId[0];
            int deltaS = id[1] - neighborId[1];
            double bearing = std::atan2(double(deltaE), double(-deltaS)) * 180.0 / std::numbers::pi;
            if (bearing < 0.0)
            {
                bearing += 360.0;
            }
            float spreadRate = GetDirectionSpreadRate(float(bearing));
            if (spreadRate <= 0.0f)
            {
                continue;
            }
            float distance = Resolution * float(std::hypot(deltaE, deltaS));
            burnTime = std::min(burnTime, distance / spreadRate);
        }
    }
    if (burnTime != kInfinity)
    {
        state.Status = FireCellStatus::Burning;
        state.BurnTime = burnTime;
    }
    return state;
}

double FireModel::outputDelay(const FireState& state) const
{
    if (state.Status == FireCellStatus::Burning)
    {
        return state.BurnTime;
    }
    else
    {
        return kInfinity;
    }
}

std::string FireModel::logState() const
{
    thread_local std::stringstream stream;
    stream.str("");
    stream.clear();
    stream << *this << state;
    return stream.str();
}

float FireModel::GetMaxSpreadRate() const
{
    BehaveRun& behave = GetBehaveRun();
    behave.surface.updateSurfaceInputs(
        FuelModel,
        Moisture1Hour, Moisture10Hour, Moisture100Hour,
        MoistureLiveHerbaceous, MoistureLiveWoody, FractionUnits::Percent,
        WindSpeed, SpeedUnits::MilesPerHour, WindHeightInputMode::TenMeter,
        WindDirection, WindAndSpreadOrientationMode::RelativeToNorth,
        Slope, SlopeUnits::Degrees, Aspect,
        CanopyCover, FractionUnits::Fraction,
        CanopyHeight, LengthUnits::Meters,
        CrownRatio, FractionUnits::Fraction);
    behave.surface.doSurfaceRunInDirectionOfMaxSpread();
    return float(behave.surface.getSpreadRate(SpeedUnits::MetersPerMinute));
}

float FireModel::GetDirectionSpreadRate(float bearing) const
{
    BehaveRun& behave = GetBehaveRun();
    behave.surface.updateSurfaceInputs(
        FuelModel,
        Moisture1Hour, Moisture10Hour, Moisture100Hour,
        MoistureLiveHerbaceous, MoistureLiveWoody, FractionUnits::Percent,
        WindSpeed, SpeedUnits::MilesPerHour, WindHeightInputMode::TenMeter,
        WindDirection, WindAndSpreadOrientationMode::RelativeToNorth,
        Slope, SlopeUnits::Degrees, Aspect,
        CanopyCover, FractionUnits::Fraction,
        CanopyHeight, LengthUnits::Meters,
        CrownRatio, FractionUnits::Fraction);
    behave.surface.doSurfaceRunInDirectionOfInterest(double(bearing), SurfaceFireSpreadDirectionMode::FromIgnitionPoint);
    return float(behave.surface.getSpreadRateInDirectionOfInterest(SpeedUnits::MetersPerMinute));
}
