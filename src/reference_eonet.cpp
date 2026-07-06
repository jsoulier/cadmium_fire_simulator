// https://eonet.gsfc.nasa.gov/docs/v3

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "date.hpp"
#include "reference.hpp"

static constexpr const char* kURL = "https://eonet.gsfc.nasa.gov/api/v3/events";

class ReferenceEONET : public Reference
{
public:
    const char* GetName() const override
    {
        return "eonet";
    }

    const char* GetDisplayName() const override
    {
        return "NASA EONET";
    }

protected:
    std::vector<std::string> GetURLs(
        const glm::dvec2& minLatLong,
        const glm::dvec2& maxLatLong,
        const Date& startDate,
        const Date& endDate) const override
    {
        return
        {
            std::format("{}?category=wildfires&bbox={},{},{},{}&start={}&end={}",
                kURL,
                minLatLong.y, maxLatLong.x, maxLatLong.y, minLatLong.x,
                startDate.ToString(), endDate.ToString())
        };
    }

    std::vector<ReferencePoint> GetPoints(const std::string& data) const override
    {
        std::vector<ReferencePoint> points;
        nlohmann::json json = nlohmann::json::parse(data, nullptr, false);
        if (json.is_discarded())
        {
            return points;
        }
        for (const nlohmann::json& event : json.value("events", nlohmann::json::array()))
        {
            for (const nlohmann::json& geometry : event.value("geometry", nlohmann::json::array()))
            {
                if (geometry.value("type", "") != "Point")
                {
                    continue;
                }
                const nlohmann::json& coordinates = geometry.value("coordinates", nlohmann::json::array());
                if (!coordinates.is_array() || coordinates.size() < 2)
                {
                    continue;
                }
                double lon = coordinates[0].get<double>();
                double lat = coordinates[1].get<double>();
                std::chrono::sys_seconds time;
                std::istringstream stream(geometry.value("date", ""));
                stream >> std::chrono::parse("%FT%TZ", time);
                if (stream.fail())
                {
                    continue;
                }
                points.push_back({glm::dvec2(lat, lon), double(time.time_since_epoch().count())});
            }
        }
        return points;
    }
};

std::unique_ptr<Reference> ReferenceCreateEONET()
{
    return std::make_unique<ReferenceEONET>();
}
