#pragma once

#include <ankerl/unordered_dense.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "fire_fuel_model.hpp"

enum class ServiceSampleType
{
    FuelModel = 1 << 0,
    Elevation = 1 << 1,
    Slope = 1 << 2,
    Aspect = 1 << 3,
    CanopyCover = 1 << 4,
    CanopyHeight = 1 << 5,
    CrownRatio = 1 << 6,
    WindSpeed = 1 << 7,
    WindDirection = 1 << 8,
    MoistureOneHour = 1 << 9,
    MoistureTenHour = 1 << 10,
    MoistureHundredHour = 1 << 11,
    MoistureLiveHerbaceous = 1 << 12,
    MoistureLiveWoody = 1 << 13,
};

struct ServiceSample
{
    FireFuelModelType FuelModel;
    float Elevation;
    float Slope;
    float Aspect;
    float CanopyCover;
    float CanopyHeight;
    float CrownRatio;
    float WindSpeed;
    float WindDirection;
    float MoistureOneHour;
    float MoistureTenHour;
    float MoistureHundredHour;
    float MoistureLiveHerbaceous;
    float MoistureLiveWoody;
};

union ServicePixel
{
    float F32;
    uint32_t U32;
};

enum class ServicePixelType
{
    F32,
    U32,
};

const char* ServiceSampleTypeToString(ServiceSampleType type);
ServicePixelType ServiceSampleTypeToPixelType(ServiceSampleType type);

class Service
{
public:
    virtual ~Service() = default;
    virtual std::string GetName() const = 0;
    virtual ServiceSampleType GetSupportedTypes() const = 0;
    void Download(ServiceSampleType types, const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong, double resolution);
    ServicePixel GetPixel(ServiceSampleType type, const glm::dvec2& latLong) const;
    ServicePixel GetPixel(ServiceSampleType type, int x, int y) const;
    ImTextureRef GetTextureRef(ServiceSampleType type);

private:
    virtual std::vector<std::string> GetSourceURLs(const glm::dvec2& minLatLong, const glm::dvec2& maxLatLong) const = 0;
    virtual int GetBand(ServiceSampleType type) const = 0;
    virtual void PostProcess(ServiceSampleType type, std::vector<ServicePixel>& pixels) {}

    struct Raster
    {
        Raster();

        int Width;
        int Height;
        // [0] upper-left corner X
        // [1] pixel width
        // [2] row rotation (0 for north-up)
        // [3] upper-left corner Y
        // [4] pixel height (usually negative)
        // [5] column rotation (0 for north-up)
        double GeoTransform[6];
        double InverseGeoTransform[6];
        std::string Wkt;
        std::vector<ServicePixel> Pixels;
        ImTextureRef Texture;
    };

    ankerl::unordered_dense::map<ServiceSampleType, Raster> Rasters;
};

std::unique_ptr<Service> ServiceCreateESAWorldCover();
