#include <glm/glm.hpp>

#include <cmath>

#include "math.hpp"

double MathCelsiusToFahrenheit(double celsius)
{
    return celsius * 9.0 / 5.0 + 32.0;
}

double MathFahrenheitToCelsius(double fahrenheit)
{
    return ((fahrenheit - 32.0) * 5.0 / 9.0);
}

glm::dvec2 MathLatLongToMeters(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong)
{
    double center = (minLatLong.x + maxLatLong.x) * 0.5;
    double width = (maxLatLong.y - minLatLong.y) * kMathMetersPerDegree * std::cos(glm::radians(center));
    double height = (maxLatLong.x - minLatLong.x) * kMathMetersPerDegree;
    return {width, height};
}
