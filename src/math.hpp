#pragma once

#include <glm/glm.hpp>

static constexpr double kMathMetersPerDegree = 111320.0;

glm::dvec2 MathLatLongToMeters(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong);
