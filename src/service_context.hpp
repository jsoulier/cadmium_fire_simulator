#pragma once

#include <ankerl/unordered_dense.h>

#include <string>
#include <variant>
#include <vector>

#include "service.hpp"

struct ServiceContextStaticData
{
    ServiceContextStaticData();

    glm::ivec2 Size;
    // [0] upper-left corner X
    // [1] pixel width
    // [2] row rotation (0 for north-up)
    // [3] upper-left corner Y
    // [4] pixel height (usually negative)
    // [5] column rotation (0 for north-up)
    double GeoTransform[6];
    double InverseGeoTransform[6];
    std::string Wkt;
    std::vector<ServiceSampleTypeValue> Pixels;
    ImTextureRef Texture;
};

struct ServiceContextDynamicData
{
    ServiceContextDynamicData();

    float Start;      // hours (TODO: not unique, should be in ServiceContext)
    float Resolution; // hours per sample (TODO: not unique, should be in ServiceContext)
    std::vector<ServiceSampleTypeValue> Samples;
};

class ServiceContext
{
public:
    using DataValue = std::variant<ServiceContextStaticData, ServiceContextDynamicData>;
    using DataMap = ankerl::unordered_dense::map<ServiceSampleType, DataValue>;

    ServiceSampleTypeValue GetValue(ServiceSampleType type, const glm::dvec2& latLong, float time) const;
    ServiceSampleTypeValue GetValue(ServiceSampleType type, int x, int y, float time) const;
    glm::ivec2 GetSize(ServiceSampleType type) const;
    ImTextureRef GetTextureRef(ServiceSampleType type) const;
    void PostDownload();
    DataValue& operator[](ServiceSampleType type);
    DataValue& At(ServiceSampleType type);
    const DataValue& At(ServiceSampleType type) const;
    bool Contains(ServiceSampleType type) const;
    void Erase(ServiceSampleType type);
    void Clear();
    DataMap::iterator begin();
    DataMap::iterator end();
    DataMap::const_iterator begin() const;
    DataMap::const_iterator end() const;

private:
    ServiceSampleTypeValue GetStaticValue(const ServiceContextStaticData& data, int x, int y) const;
    ServiceSampleTypeValue GetDynamicValue(ServiceSampleType type, float time) const;

    DataMap Data;
};
