// https://portal.opentopography.org/apidocs/#/Public/getGlobalDem

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

static constexpr const char* kURL = "https://portal.opentopography.org/API/globaldem";
static constexpr const char* kDEMType = "SRTMGL1";

class ServiceOpenTopography : public Service
{
public:
    std::string GetName() const override
    {
        return "open_topography";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::Elevation;
    }

    std::vector<std::string> GetSourceURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const override
    {
        const std::string apiKey = GetKey("open_topography.txt");
        if (apiKey.empty())
        {
            return {};
        }
        return
        {
            std::format("/vsicurl_streaming/{}?demtype={}&south={}&north={}&west={}&east={}&outputFormat=GTiff&API_Key={}",
                kURL,
                kDEMType,
                minLatLong.x,
                maxLatLong.x,
                minLatLong.y,
                maxLatLong.y,
                apiKey),
        };
    }

    int GetBand(ServiceSampleType type) const override
    {
        return 1;
    }
};

std::unique_ptr<Service> ServiceCreateOpenTopography()
{
    return std::make_unique<ServiceOpenTopography>();
}
