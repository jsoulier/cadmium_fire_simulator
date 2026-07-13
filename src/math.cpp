#include <glm/glm.hpp>

#include <cmath>

#include "math.hpp"

glm::dvec2 MathLatLongToMeters(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong)
{
    double center = (minLatLong.x + maxLatLong.x) * 0.5;
    double width = (maxLatLong.y - minLatLong.y) * kMathMetersPerDegree * std::cos(glm::radians(center));
    double height = (maxLatLong.x - minLatLong.x) * kMathMetersPerDegree;
    return {width, height};
}
