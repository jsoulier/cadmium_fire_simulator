// https://open-meteo.com/en/docs

#include <SDL3/SDL_assert.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "math.hpp"
#include "nfdrs4.h"
#include "service.hpp"

static constexpr int kPrefixDays = 56; // extra days required to compute moisture levels

class ServiceOpenMeteo : public Service
{
public:
    const char* GetName() const override
    {
        return "open_meteo";
    }

    const char* GetDisplayName() const override
    {
        return "Open-Meteo";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::WindSpeed |
            ServiceSampleType::WindDirection |
            ServiceSampleType::MoistureOneHour |
            ServiceSampleType::MoistureTenHour |
            ServiceSampleType::MoistureHundredHour |
            ServiceSampleType::MoistureLiveHerbaceous |
            ServiceSampleType::MoistureLiveWoody |
            ServiceSampleType::Temperature |
            ServiceSampleType::RelativeHumidity |
            ServiceSampleType::Precipitation |
            ServiceSampleType::SolarRadiation |
            ServiceSampleType::Snowfall |
            ServiceSampleType::SnowDepth;
    }

    ServiceSampleType GetRequiredSampleTypes(ServiceSampleType types) const override
    {
        static constexpr ServiceSampleType kTypes =
            ServiceSampleType::MoistureOneHour |
            ServiceSampleType::MoistureTenHour |
            ServiceSampleType::MoistureHundredHour |
            ServiceSampleType::MoistureLiveHerbaceous |
            ServiceSampleType::MoistureLiveWoody;
        if ((types & kTypes) != ServiceSampleType{})
        {
            return ServiceSampleType::Temperature |
                ServiceSampleType::RelativeHumidity |
                ServiceSampleType::Precipitation |
                ServiceSampleType::SolarRadiation |
                ServiceSampleType::WindSpeed |
                ServiceSampleType::Snowfall |
                ServiceSampleType::SnowDepth;
        }
        else
        {
            return ServiceSampleType{};
        }
    }

    std::vector<std::string> GetURLs(const glm::dvec2& min, const glm::dvec2& max, const Date& start, const Date& end) const override
    {
        return
        {
            std::format(
                "https://archive-api.open-meteo.com/v1/archive?latitude={:.6f}&longitude={:.6f}&start_date={}&end_date={}&"
                "hourly=wind_speed_10m,wind_direction_10m,temperature_2m,relative_humidity_2m,precipitation,shortwave_radiation,snowfall,snow_depth&"
                "wind_speed_unit=mph&timezone=auto",
                (min.x + max.x) * 0.5,
                (min.y + max.y) * 0.5,
                start.AddDays(-kPrefixDays).ToString(),
                end.ToString()),
        };
    }

    std::vector<ServiceSampleTypeDynamicValue> GetDynamicValues(const std::string& response, ServiceSampleType type) const override
    {
        nlohmann::json json = nlohmann::json::parse(response, nullptr, false);
        const char* name = nullptr;
        switch (type)
        {
        case ServiceSampleType::WindSpeed:
            name = "wind_speed_10m";
            break;
        case ServiceSampleType::WindDirection:
            name = "wind_direction_10m";
            break;
        case ServiceSampleType::Temperature:
            name = "temperature_2m";
            break;
        case ServiceSampleType::RelativeHumidity:
            name = "relative_humidity_2m";
            break;
        case ServiceSampleType::Precipitation:
            name = "precipitation";
            break;
        case ServiceSampleType::SolarRadiation:
            name = "shortwave_radiation";
            break;
        case ServiceSampleType::Snowfall:
            name = "snowfall";
            break;
        case ServiceSampleType::SnowDepth:
            name = "snow_depth";
            break;
        case ServiceSampleType::MoistureOneHour:
        case ServiceSampleType::MoistureTenHour:
        case ServiceSampleType::MoistureHundredHour:
        case ServiceSampleType::MoistureLiveHerbaceous:
        case ServiceSampleType::MoistureLiveWoody:
            return {};
        default:
            SDL_assert(false);
            return {};
        }
        if (json.is_discarded() || !json.is_object() || !json.contains("hourly") || !json["hourly"].is_object())
        {
            spdlog::warn("Failed to load {}: {}", name, GetName());
            return {};
        }
        nlohmann::json& hourly = json["hourly"];
        if (!hourly.contains("time") || !hourly["time"].is_array() || hourly["time"].empty() || !hourly.contains(name) ||
            !hourly[name].is_array() || hourly[name].size() != hourly["time"].size() ||
            !std::all_of(hourly["time"].begin(), hourly["time"].end(), [](const nlohmann::json& json) { return json.is_string(); }) ||
            !std::all_of(hourly[name].begin(), hourly[name].end(), [](const nlohmann::json& json) { return json.is_number(); }))
        {
            spdlog::warn("Failed to load {}: {}", name, GetName());
            return {};
        }
        const nlohmann::json& time = hourly["time"];
        const nlohmann::json& sample = hourly[name];
        std::vector<ServiceSampleTypeDynamicValue> values;
        values.reserve(time.size());
        for (size_t index = 0; index < time.size(); index++)
        {
            ServiceSampleTypeDynamicValue value{};
            value.Time = Date(time[index]).ToEpoch() / 60.0f;
            value.Value.F32 = sample[index];
            values.push_back(value);
        }
        return values;
    }

    void DeriveDynamicData(
        ServiceSampleType type,
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        const Date& startDate) override
    {
#if 0
        if (type != ServiceSampleType::Temperature ||
            !DynamicData.contains(ServiceSampleType::RelativeHumidity) ||
            !DynamicData.contains(ServiceSampleType::Precipitation) ||
            !DynamicData.contains(ServiceSampleType::SolarRadiation) ||
            !DynamicData.contains(ServiceSampleType::WindSpeed) ||
            !DynamicData.contains(ServiceSampleType::Snowfall))
        {
            return;
        }
        const DynamicSampleData& temperature = DynamicData.at(ServiceSampleType::Temperature);
        const DynamicSampleData& humidity = DynamicData.at(ServiceSampleType::RelativeHumidity);
        const DynamicSampleData& precipitation = DynamicData.at(ServiceSampleType::Precipitation);
        const DynamicSampleData& solarRadiation = DynamicData.at(ServiceSampleType::SolarRadiation);
        const DynamicSampleData& windSpeed = DynamicData.at(ServiceSampleType::WindSpeed);
        const DynamicSampleData& snowfall = DynamicData.at(ServiceSampleType::Snowfall);
        const DynamicSampleData* snowDepth = DynamicData.contains(ServiceSampleType::SnowDepth) ?  &DynamicData.at(ServiceSampleType::SnowDepth) : nullptr;
        SDL_assert(temperature.Samples.size() == humidity.Samples.size());
        SDL_assert(temperature.Samples.size() == precipitation.Samples.size());
        SDL_assert(temperature.Samples.size() == solarRadiation.Samples.size());
        SDL_assert(temperature.Samples.size() == windSpeed.Samples.size());
        SDL_assert(temperature.Samples.size() == snowfall.Samples.size());
        SDL_assert(!snowDepth || temperature.Samples.size() == snowDepth->Samples.size());
        const auto hoursPerSample = std::lround(temperature.Resolution);
        SDL_assert(hoursPerSample >= 1 && std::abs(temperature.Resolution - hoursPerSample) < 0.001f);

        const double latitude = (minLatLong.x + maxLatLong.x) * 0.5;
        NFDRS4 nfdrs;
        // Fuel model, slope, load transfer and curing only affect NFDRS indices; moisture calculations require latitude and observation hour.
        nfdrs.Init(latitude, 'Y', 1, 0.0, true, true, false, 100, 13);
        // https://research.fs.usda.gov/download/treesearch/68223.pdf
        // Jolly et al. live fuel moisture configuration.
        nfdrs.SetHerbGSIparams(
            1.0,         // Maximum GSI
            0.2,         // Greenup threshold
            -2.0,        // Minimum temperature lower limit (C)
            5.0,         // Minimum temperature upper limit (C)
            900.0,       // VPD lower limit (Pa)
            4100.0,      // VPD upper limit (Pa)
            36000.0,     // Daylight lower limit (seconds)
            39600.0,     // Daylight upper limit (seconds)
            28,
            false,
            28,
            0.0,         // Running total precipitation lower limit (inches)
            10.0 / 25.4, // Running total precipitation upper limit (inches)
            true,
            30.0,        // Minimum herbaceous fuel moisture (%)
            250.0);      // Maximum herbaceous fuel moisture (%)
        nfdrs.SetWoodyGSIparams(
            1.0,         // Maximum GSI
            0.2,         // Greenup threshold
            -2.0,        // Minimum temperature lower limit (C)
            5.0,         // Minimum temperature upper limit (C)
            900.0,       // VPD lower limit (Pa)
            4100.0,      // VPD upper limit (Pa)
            36000.0,     // Daylight lower limit (seconds)
            39600.0,     // Daylight upper limit (seconds)
            28,
            false,
            28,
            0.0,         // Running total precipitation lower limit (inches)
            10.0 / 25.4, // Running total precipitation upper limit (inches)
            true,
            60.0,        // Minimum woody fuel moisture (%)
            200.0);      // Maximum woody fuel moisture (%)

        std::vector<ServiceSampleTypeValue> oneHour;
        std::vector<ServiceSampleTypeValue> tenHour;
        std::vector<ServiceSampleTypeValue> hundredHour;
        std::vector<ServiceSampleTypeValue> liveHerbaceous;
        std::vector<ServiceSampleTypeValue> liveWoody;
        oneHour.reserve(temperature.Samples.size());
        tenHour.reserve(temperature.Samples.size());
        hundredHour.reserve(temperature.Samples.size());
        liveHerbaceous.reserve(temperature.Samples.size());
        liveWoody.reserve(temperature.Samples.size());
        for (size_t index = 0; index < temperature.Samples.size(); index++)
        {
            const Date sampleDate = startDate.AddHours(temperature.Start + index * temperature.Resolution);
            const auto update = [&](size_t weatherIndex, const Date& updateDate)
            {
                const std::tm time = updateDate.ToTm();
                nfdrs.Update(
                    time.tm_year + 1900,
                    time.tm_mon + 1,
                    time.tm_mday,
                    time.tm_hour,
                    MathCelsiusToFahrenheit(temperature.Samples[weatherIndex].F32),
                    humidity.Samples[weatherIndex].F32,
                    precipitation.Samples[weatherIndex].F32 / 25.4,
                    solarRadiation.Samples[weatherIndex].F32,
                    windSpeed.Samples[weatherIndex].F32,
                    (snowDepth && snowDepth->Samples[weatherIndex].F32) > 0.0f || snowfall.Samples[weatherIndex].F32 > 0.0f);
            };
            if (index > 0)
            {
                // Hold the previous coarse weather sample through the intervening hourly NFDRS updates.
                for (auto hour = 1; hour < hoursPerSample; hour++)
                {
                    update(index - 1, sampleDate.AddHours(hour - hoursPerSample));
                }
            }
            update(index, sampleDate);
            oneHour.push_back({.F32 = float(nfdrs.MC1)});
            tenHour.push_back({.F32 = float(nfdrs.MC10)});
            hundredHour.push_back({.F32 = float(nfdrs.MC100)});
            liveHerbaceous.push_back({.F32 = float(nfdrs.MCHERB)});
            liveWoody.push_back({.F32 = float(nfdrs.MCWOOD)});
        }
        SetMoisture(ServiceSampleType::MoistureOneHour, std::move(oneHour));
        SetMoisture(ServiceSampleType::MoistureTenHour, std::move(tenHour));
        SetMoisture(ServiceSampleType::MoistureHundredHour, std::move(hundredHour));
        SetMoisture(ServiceSampleType::MoistureLiveHerbaceous, std::move(liveHerbaceous));
        SetMoisture(ServiceSampleType::MoistureLiveWoody, std::move(liveWoody));
#endif
    }

#if 0
    void PostProcessDynamicData() override
    {
        for (auto& entry : DynamicData)
        {
            DynamicSampleData& data = entry.second;
            if (data.Start >= 0.0f)
            {
                continue;
            }
            // strip kPrefixDays from the data
            int index = std::ceil(-data.Start / data.Resolution);
            SDL_assert(data.Samples.size() >= index);
            data.Samples.erase(data.Samples.begin(), data.Samples.begin() + index);
            data.Start += index * data.Resolution;
        }
    }

    void SetMoisture(ServiceSampleType type, std::vector<ServiceSampleTypeValue> moisture)
    {
        const DynamicSampleData& temperature = DynamicData.at(ServiceSampleType::Temperature);
        DynamicSampleData& data = DynamicData[type];
        data.Start = temperature.Start;
        data.Resolution = temperature.Resolution;
        data.Samples = std::move(moisture);
    }
#endif
};

std::unique_ptr<Service> ServiceCreateOpenMeteo()
{
    return std::make_unique<ServiceOpenMeteo>();
}
