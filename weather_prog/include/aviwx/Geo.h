#pragma once

#include <vector>

namespace aviwx {

struct GeoPoint {
    double lat_deg{};
    double lon_deg{};
};

double deg_to_rad(double degrees);
double rad_to_deg(double radians);
double normalize_heading(double heading_deg);
double angular_difference_deg(double from_deg, double to_deg);
double haversine_nm(GeoPoint a, GeoPoint b);
double initial_bearing_deg(GeoPoint from, GeoPoint to);
double point_segment_distance_nm(GeoPoint point, GeoPoint a, GeoPoint b);
bool point_in_polygon(GeoPoint point, const std::vector<GeoPoint>& polygon);
bool segment_intersects_polygon(GeoPoint a, GeoPoint b, const std::vector<GeoPoint>& polygon);
double segment_polygon_distance_nm(GeoPoint a, GeoPoint b, const std::vector<GeoPoint>& polygon);

} // namespace aviwx
