#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>
#include <memory>

#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "reference.hpp"
#include "reference_database.hpp"
#include "service.hpp"
#include "worker.hpp"

class ServiceManager
{
public:
    ServiceManager();
    std::unique_ptr<Service>& GetService(ServiceSampleType type);
    std::unique_ptr<Reference>& GetReference();
    void RenderImGui(Worker& worker);
    void Download(Worker& worker);
    Future<FireResults> Simulate(Worker& worker, ankerl::unordered_dense::set<glm::ivec2> selected);
    Future<FireResults> Fetch(Worker& worker);

private:
    FireSimulator Simulator;
    std::vector<std::unique_ptr<Service>> Services;
    ankerl::unordered_dense::map<ServiceSampleType, int> ServiceIndices;
    ankerl::unordered_dense::map<int, ServiceSampleType> ServiceIndicesToTypes;
    std::vector<std::unique_ptr<Reference>> References;
    int ReferenceIndex;
    ReferenceDatabase Database;
    float TileResolution;
    float TimeResolution;
};
