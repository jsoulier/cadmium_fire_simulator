#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include "math.hpp"
#include "service_landfire.hpp"

static constexpr double kLandfireMetres = 30.0;

std::vector<std::string> ServiceLandfire::GetURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, const Date& startDate, const Date& endDate) const
{
    glm::dvec2 size = MathLatLongToMeters(minLatLong, maxLatLong);
    int width = std::max(int(size.x / kLandfireMetres), 1);
    int height = std::max(int(size.y / kLandfireMetres), 1);
    return
    {
        std::format(
            "/vsicurl_streaming/https://lfps.usgs.gov/arcgis/rest/services/{}/ImageServer/exportImage"
            "?bbox={},{},{},{}&bboxSR=4326&imageSR=4326&size={},{}"
            "&format=tiff&pixelType=S16&interpolation={}&f=image",
            GetServer(),
            minLatLong.y, minLatLong.x, maxLatLong.y, maxLatLong.x,
            width, height,
            GetInterpolation()),
    };
}

int ServiceLandfire::GetBand(ServiceSampleType type) const
{
    return 1;
}
