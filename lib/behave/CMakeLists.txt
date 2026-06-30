cmake_minimum_required(VERSION 3.24)
project(behave)

add_library(behave STATIC
    src/behave/behaveRun.cpp
    src/behave/behaveUnits.cpp
    src/behave/canopy_coefficient_table.cpp
    src/behave/chaparralFuel.cpp
    src/behave/Contain.cpp
    src/behave/ContainAdapter.cpp
    src/behave/ContainForce.cpp
    src/behave/ContainForceAdapter.cpp
    src/behave/ContainResource.cpp
    src/behave/ContainSim.cpp
    src/behave/crown.cpp
    src/behave/crownInputs.cpp
    src/behave/fineDeadFuelMoistureTool.cpp
    src/behave/fireSize.cpp
    src/behave/fuelModels.cpp
    src/behave/ignite.cpp
    src/behave/igniteInputs.cpp
    src/behave/moistureScenarios.cpp
    src/behave/mortality.cpp
    src/behave/mortality_equation_table.cpp
    src/behave/mortality_inputs.cpp
    src/behave/newext.cpp
    src/behave/palmettoGallberry.cpp
    src/behave/randfuel.cpp
    src/behave/randthread.cpp
    src/behave/relativeHumidity.cpp
    src/behave/safety.cpp
    src/behave/safeSeparationDistanceCalculator.cpp
    src/behave/slopeTool.cpp
    src/behave/species_master_table.cpp
    src/behave/spot.cpp
    src/behave/spotInputs.cpp
    src/behave/surface.cpp
    src/behave/surfaceFireReactionIntensity.cpp
    src/behave/surfaceFuelbedIntermediates.cpp
    src/behave/surfaceInputs.cpp
    src/behave/surfaceFire.cpp
    src/behave/surfaceTwoFuelModels.cpp
    src/behave/westernAspen.cpp
    src/behave/windAdjustmentFactor.cpp
    src/behave/windSpeedUtility.cpp
    src/behave/vaporPressureDeficitCalculator.cpp
    src/behave/CrownFirebrandProcessor.cpp
)
set_target_properties(behave PROPERTIES CXX_STANDARD 14)
target_include_directories(behave PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/behave)
target_compile_definitions(behave PRIVATE _USE_MATH_DEFINES)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/behave)

# For hiding char pointer warnings
if(MSVC)
    target_compile_options(behave PUBLIC /Zc:strictStrings-)
endif()
