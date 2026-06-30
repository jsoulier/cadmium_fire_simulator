#ifndef CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
#define CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
#if 1 // [wildfire_simulation]

#if USE_TRACY
#include <tracy/Tracy.hpp>
#define CADMIUM_PROFILE_TAG ZoneScoped
#else
#define CADMIUM_PROFILE_TAG
#endif

#endif // [wildfire_simulation]
#endif // CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
