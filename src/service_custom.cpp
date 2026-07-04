#include <SDL3/SDL.h>
#include <ankerl/unordered_dense.h>
#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

#include "service.hpp"

class ServiceCustom : public Service
{
public:
    ServiceCustom()
    {
        for (int i = 0; i < 32; i++)
        {
            const ServiceSampleType type = ServiceSampleType(1 << i);
            if ((ServiceSampleType::All & type) != ServiceSampleType{})
            {
                Values[type] = ServicePixel{};
            }
        }
    }

    const char* GetName() const override
    {
        return "custom";
    }

    const char* GetDisplayName() const override
    {
        return "Custom";
    }

    ServiceSampleType GetSupportedTypes() const override
    {
        return ServiceSampleType::All;
    }

    void RenderImGui() override
    {
        for (auto& [type, value] : Values)
        {
            std::string sliderLabel = std::format("{}##Custom", ServiceSampleTypeToString(type));
            if (ServiceSampleTypeToPixelType(type) == ServicePixelType::U32)
            {
                ImGui::InputScalar(sliderLabel.c_str(), ImGuiDataType_U32, &value.U32);
            }
            else if (ServiceSampleTypeToPixelType(type) == ServicePixelType::F32)
            {
                ImGui::InputFloat(sliderLabel.c_str(), &value.F32);
            }
            else
            {
                SDL_assert(false);
            }
        }
    }

    ServicePixel GetPixel(ServiceSampleType type, const glm::dvec2& latLong) const override
    {
        return Values.at(type);
    }

    ServicePixel GetPixel(ServiceSampleType type, int x, int y) const override
    {
        return GetPixel(type, glm::dvec2{});
    }

    ankerl::unordered_dense::map<ServiceSampleType, ServicePixel> Values;
};

std::unique_ptr<Service> ServiceCreateCustom()
{
    return std::make_unique<ServiceCustom>();
}
