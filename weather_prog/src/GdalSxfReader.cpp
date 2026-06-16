#include "aviwx/GdalSxfReader.h"

#include <cpl_error.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace aviwx {
namespace {

struct GdalDatasetDeleter {
    void operator()(GDALDataset* dataset) const
    {
        if (dataset) {
            GDALClose(dataset);
        }
    }
};

struct CoordinateTransformDeleter {
    void operator()(OGRCoordinateTransformation* transform) const
    {
        if (transform) {
            OGRCoordinateTransformation::DestroyCT(transform);
        }
    }
};

using DatasetPtr = std::unique_ptr<GDALDataset, GdalDatasetDeleter>;
using TransformPtr = std::unique_ptr<OGRCoordinateTransformation, CoordinateTransformDeleter>;

struct QuietGdalErrors {
    QuietGdalErrors()
    {
        CPLPushErrorHandler(CPLQuietErrorHandler);
    }

    ~QuietGdalErrors()
    {
        CPLPopErrorHandler();
    }
};

std::string lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool is_sxf_path(const std::filesystem::path& path)
{
    return lower_copy(path.extension().string()) == ".sxf";
}

bool should_load_overview_layer(const std::string& name)
{
    const std::string layer = lower_copy(name);
    return layer.find("system") == std::string::npos
        && layer.find("relief") == std::string::npos
        && layer.find("signature") == std::string::npos
        && layer.find("mainsign") == std::string::npos
        && layer.find("not_classified") == std::string::npos
        && layer.find("not classified") == std::string::npos
        && layer.find("layer16") == std::string::npos
        && layer.find("layer17") == std::string::npos;
}

GeoPoint to_point(const OGRPoint& point)
{
    return GeoPoint{point.getY(), point.getX()};
}

bool valid_geo_point(GeoPoint point)
{
    return std::isfinite(point.lat_deg) && std::isfinite(point.lon_deg)
        && point.lat_deg >= -90.0 && point.lat_deg <= 90.0
        && point.lon_deg >= -180.0 && point.lon_deg <= 180.0;
}

void include_point(SxfMapTile& tile, bool& has_points, GeoPoint point)
{
    if (!valid_geo_point(point)) {
        return;
    }

    if (!has_points) {
        tile.min_lat = point.lat_deg;
        tile.max_lat = point.lat_deg;
        tile.min_lon = point.lon_deg;
        tile.max_lon = point.lon_deg;
        has_points = true;
        return;
    }

    tile.min_lat = std::min(tile.min_lat, point.lat_deg);
    tile.max_lat = std::max(tile.max_lat, point.lat_deg);
    tile.min_lon = std::min(tile.min_lon, point.lon_deg);
    tile.max_lon = std::max(tile.max_lon, point.lon_deg);
}

void include_envelope_corner(SxfMapTile& tile, bool& has_points, double x, double y, OGRCoordinateTransformation* transform)
{
    OGRPoint point(x, y);
    if (transform && point.transform(transform) != OGRERR_NONE) {
        return;
    }
    include_point(tile, has_points, to_point(point));
}

bool intersects(const SxfMapTile& tile, double min_lat, double max_lat, double min_lon, double max_lon)
{
    return tile.max_lat >= min_lat && tile.min_lat <= max_lat
        && tile.max_lon >= min_lon && tile.min_lon <= max_lon;
}

double tile_distance_sq(const SxfMapTile& tile, double lat, double lon)
{
    const double tile_lat = (tile.min_lat + tile.max_lat) * 0.5;
    const double tile_lon = (tile.min_lon + tile.max_lon) * 0.5;
    const double d_lat = tile_lat - lat;
    const double d_lon = tile_lon - lon;
    return d_lat * d_lat + d_lon * d_lon;
}

std::vector<GeoPoint> ring_to_points(const OGRLineString* ring)
{
    std::vector<GeoPoint> points;
    if (!ring) {
        return points;
    }

    points.reserve(static_cast<std::size_t>(ring->getNumPoints()));
    for (int i = 0; i < ring->getNumPoints(); ++i) {
        GeoPoint point{ring->getY(i), ring->getX(i)};
        if (valid_geo_point(point)) {
            points.push_back(point);
        }
    }
    return points;
}

std::string first_text_field(const OGRFeature& feature)
{
    static const std::vector<std::string> preferred_names{
        "name", "nam", "title", "label", "objectname", "localname"
    };

    const OGRFeatureDefn* defn = feature.GetDefnRef();
    if (!defn) {
        return {};
    }

    for (const std::string& preferred : preferred_names) {
        for (int i = 0; i < feature.GetFieldCount(); ++i) {
            const OGRFieldDefn* field = defn->GetFieldDefn(i);
            if (!field || feature.IsFieldNull(i)) {
                continue;
            }
            if (lower_copy(field->GetNameRef()) == preferred && field->GetType() == OFTString) {
                return feature.GetFieldAsString(i);
            }
        }
    }

    for (int i = 0; i < feature.GetFieldCount(); ++i) {
        const OGRFieldDefn* field = defn->GetFieldDefn(i);
        if (field && field->GetType() == OFTString && !feature.IsFieldNull(i)) {
            return feature.GetFieldAsString(i);
        }
    }

    return {};
}

void collect_geometry(const OGRGeometry* geometry, const std::string& label, GisLayer& layer)
{
    if (!geometry) {
        return;
    }

    const OGRwkbGeometryType type = wkbFlatten(geometry->getGeometryType());
    if (type == wkbPoint) {
        const auto* point = geometry->toPoint();
        if (!point) {
            return;
        }
        GisFeature feature;
        feature.type = GisGeometryType::Point;
        feature.label = label;
        GeoPoint geo_point = to_point(*point);
        if (valid_geo_point(geo_point)) {
            feature.parts.push_back({geo_point});
            layer.features.push_back(std::move(feature));
        }
        return;
    }

    if (type == wkbLineString || type == wkbLinearRing) {
        const auto* line = geometry->toLineString();
        if (!line || line->getNumPoints() < 2) {
            return;
        }
        GisFeature feature;
        feature.type = GisGeometryType::Line;
        feature.label = label;
        feature.parts.push_back(ring_to_points(line));
        layer.features.push_back(std::move(feature));
        return;
    }

    if (type == wkbPolygon) {
        const auto* polygon = geometry->toPolygon();
        if (!polygon || !polygon->getExteriorRing()) {
            return;
        }
        GisFeature feature;
        feature.type = GisGeometryType::Polygon;
        feature.label = label;
        feature.parts.push_back(ring_to_points(polygon->getExteriorRing()));
        for (int i = 0; i < polygon->getNumInteriorRings(); ++i) {
            feature.parts.push_back(ring_to_points(polygon->getInteriorRing(i)));
        }
        layer.features.push_back(std::move(feature));
        return;
    }

    if (OGR_GT_IsSubClassOf(type, wkbGeometryCollection)) {
        const auto* collection = geometry->toGeometryCollection();
        if (!collection) {
            return;
        }
        for (int i = 0; i < collection->getNumGeometries(); ++i) {
            collect_geometry(collection->getGeometryRef(i), label, layer);
        }
    }
}

TransformPtr make_transform(const OGRSpatialReference* source)
{
    if (!source) {
        return nullptr;
    }

    std::unique_ptr<OGRSpatialReference> source_clone(source->Clone());
    OGRSpatialReference target;
    target.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    target.SetWellKnownGeogCS("WGS84");
    source_clone->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    return TransformPtr(OGRCreateCoordinateTransformation(source_clone.get(), &target));
}

void load_dataset_layers(const std::filesystem::path& path, GisMap& map, std::size_t max_features)
{
    DatasetPtr dataset(static_cast<GDALDataset*>(
        GDALOpenEx(path.string().c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr)));
    if (!dataset) {
        throw std::runtime_error("GDAL failed to open SXF map: " + path.string());
    }

    for (int layer_index = 0; layer_index < dataset->GetLayerCount(); ++layer_index) {
        if (map.feature_count() >= max_features) {
            return;
        }

        OGRLayer* ogr_layer = dataset->GetLayer(layer_index);
        if (!ogr_layer) {
            continue;
        }
        if (!should_load_overview_layer(ogr_layer->GetName())) {
            continue;
        }

        GisLayer layer;
        layer.name = path.filename().string() + " / " + ogr_layer->GetName();

        TransformPtr transform = make_transform(ogr_layer->GetSpatialRef());
        ogr_layer->ResetReading();

        while (OGRFeature* raw_feature = ogr_layer->GetNextFeature()) {
            std::unique_ptr<OGRFeature, decltype(&OGRFeature::DestroyFeature)> feature(raw_feature, &OGRFeature::DestroyFeature);
            if (map.feature_count() + layer.features.size() >= max_features) {
                break;
            }

            const OGRGeometry* geometry_ref = feature->GetGeometryRef();
            if (!geometry_ref) {
                continue;
            }

            std::unique_ptr<OGRGeometry> geometry(geometry_ref->clone());
            if (transform && geometry->transform(transform.get()) != OGRERR_NONE) {
                continue;
            }

            collect_geometry(geometry.get(), first_text_field(*feature), layer);
        }

        if (!layer.features.empty()) {
            map.layers.push_back(std::move(layer));
        }
    }
}

std::optional<SxfMapTile> index_dataset(const std::filesystem::path& path)
{
    DatasetPtr dataset(static_cast<GDALDataset*>(
        GDALOpenEx(path.string().c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, nullptr, nullptr, nullptr)));
    if (!dataset) {
        return std::nullopt;
    }

    SxfMapTile tile;
    tile.path = path;
    bool has_points = false;

    for (int layer_index = 0; layer_index < dataset->GetLayerCount(); ++layer_index) {
        OGRLayer* layer = dataset->GetLayer(layer_index);
        if (!layer || !should_load_overview_layer(layer->GetName())) {
            continue;
        }

        OGREnvelope envelope;
        if (layer->GetExtent(&envelope, true) != OGRERR_NONE) {
            continue;
        }

        TransformPtr transform = make_transform(layer->GetSpatialRef());
        include_envelope_corner(tile, has_points, envelope.MinX, envelope.MinY, transform.get());
        include_envelope_corner(tile, has_points, envelope.MinX, envelope.MaxY, transform.get());
        include_envelope_corner(tile, has_points, envelope.MaxX, envelope.MinY, transform.get());
        include_envelope_corner(tile, has_points, envelope.MaxX, envelope.MaxY, transform.get());
    }

    if (!has_points) {
        return std::nullopt;
    }
    return tile;
}

} // namespace

std::vector<std::filesystem::path> find_sxf_files(const std::filesystem::path& root)
{
    std::vector<std::filesystem::path> files;
    if (!std::filesystem::exists(root)) {
        return files;
    }

    if (std::filesystem::is_regular_file(root) && is_sxf_path(root)) {
        files.push_back(root);
        return files;
    }

    if (!std::filesystem::is_directory(root)) {
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() && is_sxf_path(entry.path())) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::vector<SxfMapTile> index_sxf_files(const std::vector<std::filesystem::path>& paths)
{
    GDALAllRegister();
    QuietGdalErrors quiet_errors;

    std::vector<SxfMapTile> tiles;
    tiles.reserve(paths.size());
    for (const std::filesystem::path& path : paths) {
        if (std::optional<SxfMapTile> tile = index_dataset(path)) {
            tiles.push_back(std::move(*tile));
        }
    }
    return tiles;
}

std::vector<std::filesystem::path> select_sxf_files(
    const std::vector<SxfMapTile>& tiles,
    double min_lat,
    double max_lat,
    double min_lon,
    double max_lon,
    std::size_t max_files)
{
    if (tiles.empty() || max_files == 0) {
        return {};
    }

    if (min_lat > max_lat) {
        std::swap(min_lat, max_lat);
    }
    if (min_lon > max_lon) {
        std::swap(min_lon, max_lon);
    }

    const double center_lat = (min_lat + max_lat) * 0.5;
    const double center_lon = (min_lon + max_lon) * 0.5;
    std::vector<std::tuple<double, std::filesystem::path>> ranked;

    for (const SxfMapTile& tile : tiles) {
        if (intersects(tile, min_lat, max_lat, min_lon, max_lon)) {
            ranked.emplace_back(tile_distance_sq(tile, center_lat, center_lon), tile.path);
        }
    }

    if (ranked.empty()) {
        for (const SxfMapTile& tile : tiles) {
            ranked.emplace_back(tile_distance_sq(tile, center_lat, center_lon), tile.path);
        }
    }

    std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
        if (std::get<0>(lhs) == std::get<0>(rhs)) {
            return std::get<1>(lhs) < std::get<1>(rhs);
        }
        return std::get<0>(lhs) < std::get<0>(rhs);
    });

    std::vector<std::filesystem::path> selected;
    const std::size_t count = std::min(max_files, ranked.size());
    selected.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        selected.push_back(std::get<1>(ranked[i]));
    }
    std::sort(selected.begin(), selected.end());
    return selected;
}

GisMap load_sxf_maps(const std::vector<std::filesystem::path>& paths, std::size_t max_features)
{
    GDALAllRegister();
    QuietGdalErrors quiet_errors;

    GisMap map;
    for (const std::filesystem::path& path : paths) {
        load_dataset_layers(path, map, max_features);
        map.sources.push_back(path);
        if (map.feature_count() >= max_features) {
            break;
        }
    }
    return map;
}

} // namespace aviwx
