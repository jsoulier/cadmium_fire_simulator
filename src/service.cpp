#include <SDL3/SDL.h>
#include <cpl_conv.h>
#include <gdal.h>
#include <gdal_utils.h>
#include <gdalwarper.h>
#include <imgui.h>
#include <ogr_srs_api.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "service.hpp"
#include "http.hpp"
#include "service_context.hpp"
#include "timer.hpp"

uint32_t ServiceSampleTypeToIndex(ServiceSampleType type)
{
    return std::countr_zero(uint32_t(type));
}

ServiceSampleType ServiceSampleTypeFromIndex(uint32_t index)
{
    return ServiceSampleType(1 << index);
}

const char* ServiceSampleTypeToString(ServiceSampleType type)
{
    return kServiceSampleTypeStrings[ServiceSampleTypeToIndex(type)];
}

ServiceSampleTypeFormat ServiceSampleTypeToFormat(ServiceSampleType type)
{
    return kServiceSampleTypeFormats[ServiceSampleTypeToIndex(type)];
}

void Service::RenderImGui(ServiceContext& context)
{
    float maxTime = 0.0f;
    for (const auto& [type, value] : context)
    {
        if ((GetSupportedTypes() & type) == ServiceSampleType{} || !std::holds_alternative<ServiceContextDynamicData>(value))
        {
            continue;
        }
        const ServiceContextDynamicData& data = std::get<ServiceContextDynamicData>(value);
        if (!data.Samples.empty())
        {
            maxTime = std::max(maxTime, data.Start + (data.Samples.size() - 1) * data.Resolution);
        }
    }
    if (maxTime > 0.0f)
    {
        Time = std::clamp(Time, 0.0f, maxTime);
        ImGui::SliderFloat("Time (Hours)", &Time, 0.0f, maxTime);
    }
    for (int index = 0; index < kServiceSampleTypeMax; index++)
    {
        const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
        if ((GetSupportedTypes() & type) == ServiceSampleType{})
        {
            continue;
        }
        if (!context.Contains(type))
        {
            continue;
        }
        const ServiceContext::DataValue& data = context.At(type);
        if (!std::holds_alternative<ServiceContextDynamicData>(data) || std::get<ServiceContextDynamicData>(data).Samples.empty())
        {
            continue;
        }
        const ServiceSampleTypeValue value = context.GetValue(type, glm::dvec2{}, Time);
        if (ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32)
        {
            ImGui::Text("%s: %u", ServiceSampleTypeToString(type), value.U32);
        }
        else
        {
            ImGui::Text("%s: %.3f", ServiceSampleTypeToString(type), value.F32);
        }
    }
}

void Service::Download(
    ServiceContext& context,
    ServiceSampleType types,
    const glm::dvec2& minLatLong,
    const glm::dvec2& maxLatLong,
    float tileResolution,
    float timeResolution,
    const Date& startDate,
    const Date& endDate,
    const std::filesystem::path& directory)
{
    SDL_assert(tileResolution > 0.0f);
    SDL_assert(timeResolution > 0.0f);
    TimerBlock(std::format("{} download", GetName()));
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []()
    {
        GDALAllRegister();
        const char* projPath[] = { SDL_GetBasePath(), nullptr };
        OSRSetPROJSearchPaths(projPath);
    });
    const ServiceSampleType requestedTypes = types;
    ServiceSampleType derivedTypes = GetDerivedSampleTypes() & requestedTypes;
    ServiceSampleType requiredTypes = GetRequiredSampleTypes(types);
    types |= requiredTypes; // ensure dynamic dependencies are satisfied
    for (int index = 0; index < kServiceSampleTypeMax; index++)
    {
        const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
        if ((requiredTypes & ServiceSampleType::Dynamic & type) != ServiceSampleType{} && // is a required dynamic type
            (requestedTypes & type) == ServiceSampleType{} && // is not a requested type
            context.Contains(type)) // already contains the type
        {
            types = types & ~type; // dependency already satisfied, remove the type
        }
    }
    SDL_assert((types & ~GetSupportedTypes()) == ServiceSampleType{});
    std::filesystem::create_directories(directory);
    for (int index = 0; index < kServiceSampleTypeMax; index++)
    {
        const ServiceSampleType type = ServiceSampleTypeFromIndex(index);
        SDL_assert((requestedTypes & type) == ServiceSampleType{} || !context.Contains(type)); // didn't request an already satisfied type
    }
    if ((types & ServiceSampleType::Dynamic) != ServiceSampleType{}) // has dynamic types
    {
        TimerBlock(std::format("{} dynamic", GetName()));
        ankerl::unordered_dense::map<ServiceSampleType, std::vector<ServiceSampleTypeDynamicValue>> dynamicValues;
        const std::vector<std::string> urls = GetURLs(minLatLong, maxLatLong, startDate, endDate);
        for (const std::string& url : urls)
        {
            const std::optional<std::string> response = HttpGetAndCache(url, directory);
            if (!response)
            {
                return;
            }
            for (int i = 0; i < kServiceSampleTypeMax; i++)
            {
                const ServiceSampleType type = ServiceSampleTypeFromIndex(i);
                if ((types & ServiceSampleType::Dynamic & type) == ServiceSampleType{}) // is a dynamic type
                {
                    continue;
                }
                std::vector<ServiceSampleTypeDynamicValue> values = GetDynamicValues(*response, type);
                std::vector<ServiceSampleTypeDynamicValue>& allValues = dynamicValues[type];
                if (allValues.empty())
                {
                    allValues = std::move(values);
                }
                else
                {
                    allValues.insert(allValues.end(), values.begin(), values.end());
                }
            }
        }
        // convert to desired resolution (either an upsample or downsample)
        for (auto& [type, values] : dynamicValues)
        {
            if (values.empty())
            {
                continue;
            }
            std::stable_sort(values.begin(), values.end(), [](const ServiceSampleTypeDynamicValue& a, const ServiceSampleTypeDynamicValue& b)
            {
                return a.Time < b.Time;
            });
            ServiceContextDynamicData data;
            data.Start = values.front().Time - startDate.ToEpoch() / 60.0f;
            data.Resolution = timeResolution;
            int index = 0;
            float end = endDate.ToEpoch() / 60.0f;
            for (float time = values.front().Time; time <= end; time += timeResolution)
            {
                // add an index if we're ahead (skip) or noop if we're behind (duplicate)
                while (index + 1 < values.size() && values[index + 1].Time <= time)
                {
                    index++;
                }
                data.Samples.push_back(values[index].Value);
            }
            if (data.Samples.empty())
            {
                spdlog::error("Failed to sample dynamic values for {}: {}", ServiceSampleTypeToString(type), GetName());
                continue;
            }
            context[type] = std::move(data);
        }
        for (int i = 0; i < kServiceSampleTypeMax; i++)
        {
            const ServiceSampleType type = ServiceSampleTypeFromIndex(i);
            if ((types & ServiceSampleType::Dynamic & type) != ServiceSampleType{})
            {
                DeriveDynamicData(context, type, minLatLong, maxLatLong, startDate);
            }
        }
        PostProcessDynamicData(context);
        for (int i = 0; i < kServiceSampleTypeMax; i++)
        {
            const ServiceSampleType type = ServiceSampleTypeFromIndex(i);
            if ((types & ServiceSampleType::Dynamic & type) != ServiceSampleType{} &&
                !context.Contains(type))
            {
                spdlog::error("Failed to get dynamic values for {}: {}", ServiceSampleTypeToString(type), GetName());
            }
        }
    }
    if ((types & ~ServiceSampleType::Dynamic) == ServiceSampleType{})
    {
        SDL_assert((types & ServiceSampleType::Static) == ServiceSampleType{});
        return;
    }
    std::filesystem::path filePath = directory / std::format("{}_{}.{}_{}.{}.tif",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y);
    if (!std::filesystem::exists(filePath))
    {
        TimerBlock(std::format("{} vrt", GetName()));
        std::vector<std::string> sources = GetURLs(minLatLong, maxLatLong, startDate, endDate);
        if (sources.empty())
        {
            return;
        }
        std::vector<const char*> sourceNames;
        sourceNames.reserve(sources.size());
        for (const std::string& source : sources)
        {
            sourceNames.push_back(source.c_str());
        }
        static std::atomic_uint64_t vrtIndex = 0;
        const std::string vrtPath = std::format("/vsimem/service_{}.vrt", vrtIndex++);
        GDALDatasetH vrt = GDALBuildVRT(
            vrtPath.c_str(),
            sourceNames.size(),
            nullptr,
            sourceNames.data(),
            nullptr,
            nullptr);
        if (!vrt)
        {
            spdlog::error("Failed to build VRT for {}: {}", GetName(), CPLGetLastErrorMsg());
            return;
        }
        bool isWgs84 = false;
        const char* sourceWkt = GDALGetProjectionRef(vrt);
        if (sourceWkt && sourceWkt[0])
        {
            OGRSpatialReferenceH wgs84 = OSRNewSpatialReference(nullptr);
            OSRImportFromEPSG(wgs84, 4326);
            OSRSetAxisMappingStrategy(wgs84, OAMS_TRADITIONAL_GIS_ORDER);
            OGRSpatialReferenceH sourceSrs = OSRNewSpatialReference(sourceWkt);
            isWgs84 = OSRIsSame(sourceSrs, wgs84);
            OSRDestroySpatialReference(sourceSrs);
            OSRDestroySpatialReference(wgs84);
        }
        const std::string west = std::format("{}", minLatLong.y);
        const std::string south = std::format("{}", minLatLong.x);
        const std::string east = std::format("{}", maxLatLong.y);
        const std::string north = std::format("{}", maxLatLong.x);
        // clip before downloading (otherwise services like NRCan download all of Canada)
        if (isWgs84)
        {
            TimerBlock(std::format("{} clip", GetName()));
            const char* args[] =
            {
                "-projwin", west.c_str(), north.c_str(), east.c_str(), south.c_str(),
                "-projwin_srs", "EPSG:4326",
                nullptr
            };
            GDALTranslateOptions* options = GDALTranslateOptionsNew(const_cast<char**>(args), nullptr);
            GDALDatasetH dataset = GDALTranslate(filePath.string().c_str(), vrt, options, nullptr);
            if (!dataset)
            {
                spdlog::error("Failed to translate to {} for {}: {}", filePath.string(), GetName(), CPLGetLastErrorMsg());
            }
            else
            {
                GDALClose(dataset);
            }
            GDALTranslateOptionsFree(options);
        }
        else
        {
            TimerBlock(std::format("{} clip and reproject", GetName()));
            const char* args[] =
            {
                "-t_srs", "EPSG:4326", // convert to EPSG:4326 (WGS 84) (uses lat/long) 
                "-te", west.c_str(), south.c_str(), east.c_str(), north.c_str(),
                "-r", "near",
                nullptr
            };
            GDALWarpAppOptions* options = GDALWarpAppOptionsNew(const_cast<char**>(args), nullptr);
            GDALDatasetH dataset = GDALWarp(filePath.string().c_str(), nullptr, 1, &vrt, options, nullptr);
            if (!dataset)
            {
                spdlog::error("Failed to warp {} to {}: {}", GetName(), filePath.string(), CPLGetLastErrorMsg());
            }
            else
            {
                GDALClose(dataset);
            }
            GDALWarpAppOptionsFree(options);
        }
        GDALClose(vrt);
        VSIUnlink(vrtPath.c_str());
    }
    GDALDatasetH highResolution = GDALOpen(filePath.string().c_str(), GA_ReadOnly);
    if (!highResolution)
    {
        spdlog::error("Failed to open {}: {}", filePath.string(), CPLGetLastErrorMsg());
        return;
    }
    const std::string tileResolutionString = std::format("{}", tileResolution);
    const std::string lowResolutionDirectory = (directory / std::format("{}_{}.{}_{}.{}_{}",
        GetName(),
        minLatLong.x,
        minLatLong.y,
        maxLatLong.x,
        maxLatLong.y,
        tileResolution)).string();
    // downsample high resolution GeoTIFF to low resolution array of pixels per band
    for (int i = 0; i < kServiceSampleTypeMax; i++)
    {
        const ServiceSampleType type = ServiceSampleTypeFromIndex(i);
        if ((types & ~ServiceSampleType::Dynamic & type) == ServiceSampleType{})
        {
            continue;
        }
        std::filesystem::path lowResolutionFilePath = std::format("{}_{}.tif", lowResolutionDirectory, int(type));
        if (!std::filesystem::exists(lowResolutionFilePath))
        {
            TimerBlock(std::format("{} {} downsample", GetName(), ServiceSampleTypeToString(type)));
            const std::string bandString = std::format("{}", GetBand(type));
            const std::string algorithmString = ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32 ? "mode" : "average";
            const char* args[] =
            {
                "-b", bandString.c_str(),
                "-tr", tileResolutionString.c_str(), tileResolutionString.c_str(),
                "-r", algorithmString.c_str(),
                nullptr
            };
            GDALTranslateOptions* options = GDALTranslateOptionsNew(const_cast<char**>(args), nullptr);
            GDALDatasetH lowResolution = GDALTranslate(lowResolutionFilePath.string().c_str(), highResolution, options, nullptr);
            if (!lowResolution)
            {
                spdlog::error("Failed to downsample band {} to {}: {}", bandString, lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            }
            else
            {
                GDALClose(lowResolution);
            }
            GDALTranslateOptionsFree(options);
        }
        GDALDatasetH lowResolution = GDALOpen(lowResolutionFilePath.string().c_str(), GA_ReadOnly);
        if (!lowResolution)
        {
            spdlog::error("Failed to open {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
            continue;
        }
        // create e.g. slope/aspect from elevation
        for (int outIndex = 0; outIndex < kServiceSampleTypeMax; outIndex++)
        {
            const ServiceSampleType outType = ServiceSampleTypeFromIndex(outIndex);
            if ((derivedTypes & outType) == ServiceSampleType{})
            {
                continue;
            }
            TimerBlock(std::format("{} {} derive", GetName(), ServiceSampleTypeToString(outType)));
            DeriveStaticData(type, outType, lowResolution, lowResolutionDirectory);
        }
        if ((requestedTypes & type) == ServiceSampleType{})
        {
            GDALClose(lowResolution);
            continue;
        }
        ServiceContextStaticData staticData;
        {
            TimerBlock(std::format("{} {} static", GetName(), ServiceSampleTypeToString(type)));
            staticData.Size.x = GDALGetRasterXSize(lowResolution);
            staticData.Size.y = GDALGetRasterYSize(lowResolution);
            if (GDALGetGeoTransform(lowResolution, staticData.GeoTransform) != CE_None ||
                GDALInvGeoTransform(staticData.GeoTransform, staticData.InverseGeoTransform) == FALSE)
            {
                spdlog::error("Failed to get GeoTransform or InvGeoTransform for {}", lowResolutionFilePath.string());
                GDALClose(lowResolution);
                continue;
            }
            staticData.Wkt = GDALGetProjectionRef(lowResolution);
            staticData.Pixels.resize(staticData.Size.x * staticData.Size.y);
            CPLErr status = GDALRasterIO(
                GDALGetRasterBand(lowResolution, 1), GF_Read,
                0, 0, staticData.Size.x, staticData.Size.y,
                staticData.Pixels.data(), staticData.Size.x, staticData.Size.y,
                ServiceSampleTypeToFormat(type) == ServiceSampleTypeFormat::U32 ? GDT_UInt32 : GDT_Float32, 0, 0);
            GDALClose(lowResolution);
            if (status != CE_None)
            {
                spdlog::error("Failed to read {}: {}", lowResolutionFilePath.string(), CPLGetLastErrorMsg());
                continue;
            }
        }
        {
            TimerBlock(std::format("{} {} post process", GetName(), ServiceSampleTypeToString(type)));
            PostProcessStaticData(type, staticData.Pixels);
        }
        context[type] = std::move(staticData);
    }
    GDALClose(highResolution);
}

void Service::DEMProcessing(GDALDatasetH elevation, const std::string& directory, ServiceSampleType type)
{
    std::string path = std::format("{}_{}.tif", directory, int(type));
    if (std::filesystem::exists(path))
    {
        return;
    }
    const char* processing;
    std::vector<const char*> args = { "-of", "GTiff", "-compute_edges" };
    if (type == ServiceSampleType::Slope)
    {
        static constexpr const char* kDegreesToMetres = "111120";
        processing = "slope";
        args.push_back("-s");
        args.push_back(kDegreesToMetres);
    }
    else if (type == ServiceSampleType::Aspect)
    {
        processing = "aspect";
        args.push_back("-zero_for_flat");
    }
    else
    {
        SDL_assert(false);
    }
    args.push_back(nullptr);
    GDALDEMProcessingOptions* options = GDALDEMProcessingOptionsNew(const_cast<char**>(args.data()), nullptr);
    GDALDatasetH result = GDALDEMProcessing(path.c_str(), elevation, processing, nullptr, options, nullptr);
    if (!result)
    {
        spdlog::error("Failed to derive {} to {}: {}", processing, path, CPLGetLastErrorMsg());
    }
    else
    {
        GDALClose(result);
    }
    GDALDEMProcessingOptionsFree(options);
}
