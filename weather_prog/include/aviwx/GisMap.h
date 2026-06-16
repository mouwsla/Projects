#pragma once

#include "aviwx/Geo.h"

#include <filesystem>
#include <string>
#include <vector>

namespace aviwx {

enum class GisGeometryType {
    Point,
    Line,
    Polygon
};

struct GisFeature {
    GisGeometryType type{GisGeometryType::Line};
    std::string label;
    std::vector<std::vector<GeoPoint>> parts;
};

struct GisLayer {
    std::string name;
    std::vector<GisFeature> features;
};

struct GisMap {
    std::vector<std::filesystem::path> sources;
    std::vector<GisLayer> layers;

    std::size_t feature_count() const;
    bool empty() const;
};

} // namespace aviwx
