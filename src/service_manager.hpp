#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>

#include <ctime>
#include <vector>
#include <memory>

#include "fire_results.hpp"
#include "fire_simulator.hpp"
#include "future.hpp"
#include "reference.hpp"
#include "service.hpp"
#include "worker.hpp"

class ServiceManager
{
public:
    ServiceManager();
    std::unique_ptr<Service>& GetService(ServiceSampleType type);
    std::unique_ptr<Reference>& GetReference();
    void RenderImGui();
    void Download(Worker& worker);
    Future<FireResults> Simulate(Worker& worker, FireSimulatorParams& params);
    Future<FireResults> Fetch(Worker& worker);

private:
    void SetParams(FireSimulatorParams& params) const;

    std::vector<std::unique_ptr<Service>> Services;
    ankerl::unordered_dense::map<ServiceSampleType, int> ServiceIndices;
    ankerl::unordered_dense::map<int, ServiceSampleType> ServiceIndicesToTypes;
    std::vector<std::unique_ptr<Reference>> References;
    int ReferenceIndex = 0;
    std::tm StartDate;
    std::tm EndDate;
    glm::dvec2 MinLatLong;
    glm::dvec2 MaxLatLong;
    double Resolution;
};
