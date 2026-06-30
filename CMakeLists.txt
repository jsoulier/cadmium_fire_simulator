option(FIRE_SIMULATOR_USE_VENDORED "Use vendored libraries" ON)
option(FIRE_SIMULATOR_USE_TRACY "Enable Tracy profiling zones" OFF)
set(GLM_BUILD_LIBRARY OFF)

if(USE_VENDORED)
    add_subdirectory(lib/json)
    add_subdirectory(lib/unordered_dense)
    add_subdirectory(lib/taskflow)
    add_subdirectory(lib/behave)
    add_subdirectory(lib/glm)
    add_subdirectory(lib/cadmium_v2)
    add_subdirectory(lib/spdlog)
    if(FIRE_SIMULATOR_USE_TRACY)
        add_subdirectory(lib/tracy)
    endif()
endif()

add_library(fire_simulator STATIC
    src/fire_fuel_model.cpp
    src/fire_model.cpp
    src/fire_model_coordinator.cpp
    src/fire_model_coordinator_brute_force.cpp
    src/fire_model_coordinator_event_driven.cpp
    src/fire_model_logger.cpp
    src/fire_simulator.cpp
)
target_include_directories(fire_simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fire_simulator PRIVATE
    Taskflow::Taskflow
    behave
    cadmium
    glm::glm
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    unordered_dense::unordered_dense
)
target_compile_definitions(fire_simulator PRIVATE GLM_ENABLE_EXPERIMENTAL)
set_target_properties(fire_simulator PROPERTIES CXX_STANDARD 23)
if(FIRE_SIMULATOR_USE_TRACY)
    target_link_libraries(fire_simulator PRIVATE Tracy::TracyClient)
    target_compile_definitions(fire_simulator PRIVATE FIRE_SIMULATOR_USE_TRACY=1)
endif()
