#include "aviwx/Geo.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace aviwx {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusNm = 3440.065;
constexpr double kEpsilon = 1.0e-9;

struct ProjectedPoint {
    double x{};
    double y{};
};

ProjectedPoint project_to_nm(GeoPoint origin, GeoPoint point)
{
    const double origin_lat = deg_to_rad(origin.lat_deg);
    const double d_lat = deg_to_rad(point.lat_deg - origin.lat_deg);
    const double d_lon = deg_to_rad(point.lon_deg - origin.lon_deg);
    return {
        d_lon * std::cos(origin_lat) * kEarthRadiusNm,
        d_lat * kEarthRadiusNm,
    };
}

double cross(ProjectedPoint a, ProjectedPoint b, ProjectedPoint c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool on_segment(ProjectedPoint a, ProjectedPoint b, ProjectedPoint p)
{
    return std::min(a.x, b.x) - kEpsilon <= p.x && p.x <= std::max(a.x, b.x) + kEpsilon
        && std::min(a.y, b.y) - kEpsilon <= p.y && p.y <= std::max(a.y, b.y) + kEpsilon
        && std::abs(cross(a, b, p)) <= kEpsilon;
}

bool segments_intersect(ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, ProjectedPoint d)
{
    const double c1 = cross(a, b, c);
    const double c2 = cross(a, b, d);
    const double c3 = cross(c, d, a);
    const double c4 = cross(c, d, b);

    if (((c1 > kEpsilon && c2 < -kEpsilon) || (c1 < -kEpsilon && c2 > kEpsilon))
        && ((c3 > kEpsilon && c4 < -kEpsilon) || (c3 < -kEpsilon && c4 > kEpsilon))) {
        return true;
    }

    return on_segment(a, b, c) || on_segment(a, b, d) || on_segment(c, d, a) || on_segment(c, d, b);
}

double distance(ProjectedPoint a, ProjectedPoint b)
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

double point_segment_distance(ProjectedPoint point, ProjectedPoint a, ProjectedPoint b)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    if (len2 <= kEpsilon) {
        return distance(point, a);
    }

    const double t = std::clamp(((point.x - a.x) * dx + (point.y - a.y) * dy) / len2, 0.0, 1.0);
    const ProjectedPoint projection{a.x + t * dx, a.y + t * dy};
    return distance(point, projection);
}

} // namespace

double deg_to_rad(double degrees)
{
    return degrees * kPi / 180.0;
}

double rad_to_deg(double radians)
{
    return radians * 180.0 / kPi;
}

double normalize_heading(double heading_deg)
{
    double value = std::fmod(heading_deg, 360.0);
    if (value < 0.0) {
        value += 360.0;
    }
    return value;
}

double angular_difference_deg(double from_deg, double to_deg)
{
    double diff = normalize_heading(to_deg) - normalize_heading(from_deg);
    if (diff > 180.0) {
        diff -= 360.0;
    }
    if (diff < -180.0) {
        diff += 360.0;
    }
    return diff;
}

double haversine_nm(GeoPoint a, GeoPoint b)
{
    const double lat1 = deg_to_rad(a.lat_deg);
    const double lat2 = deg_to_rad(b.lat_deg);
    const double d_lat = deg_to_rad(b.lat_deg - a.lat_deg);
    const double d_lon = deg_to_rad(b.lon_deg - a.lon_deg);

    const double h = std::sin(d_lat / 2.0) * std::sin(d_lat / 2.0)
        + std::cos(lat1) * std::cos(lat2) * std::sin(d_lon / 2.0) * std::sin(d_lon / 2.0);
    return 2.0 * kEarthRadiusNm * std::asin(std::sqrt(h));
}

double initial_bearing_deg(GeoPoint from, GeoPoint to)
{
    const double lat1 = deg_to_rad(from.lat_deg);
    const double lat2 = deg_to_rad(to.lat_deg);
    const double d_lon = deg_to_rad(to.lon_deg - from.lon_deg);

    const double y = std::sin(d_lon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2)
        - std::sin(lat1) * std::cos(lat2) * std::cos(d_lon);
    return normalize_heading(rad_to_deg(std::atan2(y, x)));
}

double point_segment_distance_nm(GeoPoint point, GeoPoint a, GeoPoint b)
{
    const ProjectedPoint p = project_to_nm(point, point);
    const ProjectedPoint pa = project_to_nm(point, a);
    const ProjectedPoint pb = project_to_nm(point, b);
    return point_segment_distance(p, pa, pb);
}

bool point_in_polygon(GeoPoint point, const std::vector<GeoPoint>& polygon)
{
    if (polygon.size() < 3) {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const GeoPoint pi = polygon[i];
        const GeoPoint pj = polygon[j];
        const bool intersects = ((pi.lat_deg > point.lat_deg) != (pj.lat_deg > point.lat_deg))
            && (point.lon_deg < (pj.lon_deg - pi.lon_deg) * (point.lat_deg - pi.lat_deg)
                    / ((pj.lat_deg - pi.lat_deg) + kEpsilon)
                + pi.lon_deg);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

bool segment_intersects_polygon(GeoPoint a, GeoPoint b, const std::vector<GeoPoint>& polygon)
{
    if (polygon.size() < 3) {
        return false;
    }
    if (point_in_polygon(a, polygon) || point_in_polygon(b, polygon)) {
        return true;
    }

    const GeoPoint origin{(a.lat_deg + b.lat_deg) / 2.0, (a.lon_deg + b.lon_deg) / 2.0};
    const ProjectedPoint pa = project_to_nm(origin, a);
    const ProjectedPoint pb = project_to_nm(origin, b);

    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const GeoPoint c = polygon[i];
        const GeoPoint d = polygon[(i + 1) % polygon.size()];
        if (segments_intersect(pa, pb, project_to_nm(origin, c), project_to_nm(origin, d))) {
            return true;
        }
    }
    return false;
}

double segment_polygon_distance_nm(GeoPoint a, GeoPoint b, const std::vector<GeoPoint>& polygon)
{
    if (polygon.empty()) {
        return std::numeric_limits<double>::infinity();
    }
    if (segment_intersects_polygon(a, b, polygon)) {
        return 0.0;
    }

    double best = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const GeoPoint c = polygon[i];
        const GeoPoint d = polygon[(i + 1) % polygon.size()];
        best = std::min(best, point_segment_distance_nm(c, a, b));
        best = std::min(best, point_segment_distance_nm(a, c, d));
        best = std::min(best, point_segment_distance_nm(b, c, d));
    }
    return best;
}

} // namespace aviwx
