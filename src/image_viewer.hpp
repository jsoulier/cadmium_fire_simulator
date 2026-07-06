#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <imgui.h>

#include "fire_results.hpp"
#include "service.hpp"
#include "service_manager.hpp"

class ImageViewer
{
public:
    ImageViewer();
    void Draw(ServiceManager& serviceManager, std::optional<FireResults>& results);
    ankerl::unordered_dense::set<glm::ivec2> GetSelected() const;

private:
    ServiceSampleType Type;
    glm::ivec2 Size;
    glm::ivec2 Selected;
    float Zoom;
    ImVec2 Pan;
};
