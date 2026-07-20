#include <SDL3/SDL.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <variant>

#include "service_context.hpp"
#include "timer.hpp"

ServiceContextStaticData::ServiceContextStaticData()
    : Size(0)
    , GeoTransform{}
    , InverseGeoTransform{}
    , Texture{}
{
}

ServiceContextDynamicData::ServiceContextDynamicData()
    : Start(0.0f)
    , Resolution(0.0f)
{
}

ServiceSampleTypeValue ServiceContext::GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const
{
    if ((ServiceSampleType::Dynamic & type) != ServiceSampleType{})
    {
        return GetDynamicValue(type, time);
    }
    else
    {
        const auto it = Data.find(type);
        SDL_assert(it != Data.end());
        SDL_assert(std::holds_alternative<ServiceContextStaticData>(it->second));
        const ServiceContextStaticData& data = std::get<ServiceContextStaticData>(it->second);
        const double* transform = data.InverseGeoTransform;
        int x = int(transform[0] + latLong.y * transform[1] + latLong.x * transform[2]);
        int y = int(transform[3] + latLong.y * transform[4] + latLong.x * transform[5]);
        return GetStaticValue(data, x, y);
    }
}

ServiceSampleTypeValue ServiceContext::GetValue(ServiceSampleType type, int x, int y, float time) const
{
    if ((ServiceSampleType::Dynamic & type) != ServiceSampleType{})
    {
        return GetDynamicValue(type, time);
    }
    else
    {
        const auto it = Data.find(type);
        SDL_assert(it != Data.end());
        SDL_assert(std::holds_alternative<ServiceContextStaticData>(it->second));
        const ServiceContextStaticData& data = std::get<ServiceContextStaticData>(it->second);
        return GetStaticValue(data, x, y);
    }
}

ServiceSampleTypeValue ServiceContext::GetStaticValue(const ServiceContextStaticData& data, int x, int y) const
{
    // TODO: required for ServiceCustom
    if (data.Pixels.size() == 1)
    {
        return data.Pixels.front();
    }
    // TODO: consider an assert
    if (x < 0 || y < 0 || x >= data.Size.x || y >= data.Size.y)
    {
        return ServiceSampleTypeValue{};
    }
    return data.Pixels[size_t(y) * data.Size.x + x];
}

ServiceSampleTypeValue ServiceContext::GetDynamicValue(ServiceSampleType type, float time) const
{
    SDL_assert((ServiceSampleType::Dynamic & type) != ServiceSampleType{});
    const auto it = Data.find(type);
    SDL_assert(it != Data.end());
    SDL_assert(std::holds_alternative<ServiceContextDynamicData>(it->second));
    const ServiceContextDynamicData& data = std::get<ServiceContextDynamicData>(it->second);
    if (data.Samples.empty() || data.Resolution == 0.0f)
    {
        return ServiceSampleTypeValue{};
    }
    int index = std::round((time - data.Start) / data.Resolution);
    index = std::clamp(index, 0, int(data.Samples.size() - 1));
    return data.Samples[index];
}

glm::ivec2 ServiceContext::GetSize(ServiceSampleType type) const
{
    const auto it = Data.find(type);
    if (it != Data.end() && std::holds_alternative<ServiceContextStaticData>(it->second))
    {
        return std::get<ServiceContextStaticData>(it->second).Size;
    }
    else
    {
        return {};
    }
}

ImTextureRef ServiceContext::GetTextureRef(ServiceSampleType type) const
{
    const auto it = Data.find(type);
    if (it != Data.end() && std::holds_alternative<ServiceContextStaticData>(it->second))
    {
        return std::get<ServiceContextStaticData>(it->second).Texture;
    }
    else
    {
        return ImTextureRef();
    }
}

void ServiceContext::PostDownload()
{
    if (!ImGui::GetCurrentContext())
    {
        return;
    }
    for (auto& [type, value] : Data)
    {
        if (!std::holds_alternative<ServiceContextStaticData>(value))
        {
            continue;
        }
        ServiceContextStaticData& staticData = std::get<ServiceContextStaticData>(value);
        TimerBlock(std::format("{} texture", ServiceSampleTypeToString(type)));
        ImTextureData* texture = IM_NEW(ImTextureData)();
        texture->Create(ImTextureFormat_RGBA32, staticData.Size.x, staticData.Size.y);
        uint32_t* texels = reinterpret_cast<uint32_t*>(texture->GetPixels());
        if (type == ServiceSampleType::FuelModel)
        {
            SDL_assert(ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32);
            for (size_t i = 0; i < staticData.Pixels.size(); i++)
            {
                texels[i] = FireFuelModelTypeGetColor(FireFuelModelType(staticData.Pixels[i].U32));
            }
        }
        else
        {
            SDL_assert(ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::F32);
            static constexpr float kNoData = -9999.0f;
            float minValue = std::numeric_limits<float>::max();
            float maxValue = std::numeric_limits<float>::lowest();
            for (const ServiceSampleTypeValue& pixel : staticData.Pixels)
            {
                if (pixel.F32 == kNoData)
                {
                    continue;
                }
                minValue = std::min(minValue, pixel.F32);
                maxValue = std::max(maxValue, pixel.F32);
            }
            const float range = maxValue - minValue;
            for (size_t i = 0; i < staticData.Pixels.size(); i++)
            {
                uint8_t gray = 0;
                if (staticData.Pixels[i].F32 != kNoData && range > 0.0f)
                {
                    gray = (staticData.Pixels[i].F32 - minValue) / range * 255.0f;
                }
                texels[i] = IM_COL32(gray, gray, gray, 255);
            }
        }
        ImGui::RegisterUserTexture(texture);
        staticData.Texture = texture->GetTexRef();
    }
}

ServiceContext::DataValue& ServiceContext::operator[](ServiceSampleType type)
{
    return Data[type];
}

ServiceContext::DataValue& ServiceContext::At(ServiceSampleType type)
{
    return Data.at(type);
}

const ServiceContext::DataValue& ServiceContext::At(ServiceSampleType type) const
{
    return Data.at(type);
}

bool ServiceContext::Contains(ServiceSampleType type) const
{
    return Data.contains(type);
}

void ServiceContext::Erase(ServiceSampleType type)
{
    Data.erase(type);
}

void ServiceContext::Clear()
{
    Data.clear();
}

ServiceContext::DataMap::iterator ServiceContext::begin()
{
    return Data.begin();
}

ServiceContext::DataMap::iterator ServiceContext::end()
{
    return Data.end();
}

ServiceContext::DataMap::const_iterator ServiceContext::begin() const
{
    return Data.begin();
}

ServiceContext::DataMap::const_iterator ServiceContext::end() const
{
    return Data.end();
}
