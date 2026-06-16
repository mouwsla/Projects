#pragma once

#include "aviwx/Geo.h"

#include <filesystem>
#include <string>
#include <vector>

namespace aviwx {

enum class HazardType {
    Thunderstorm,
    Icing,
    Turbulence,
    LowVisibility,
    LowCeiling,
    WindShear,
    VolcanicAsh,
    HeavyPrecipitation,
    Unknown
};

enum class Severity {
    Light,
    Moderate,
    Severe,
    Extreme
};

struct AircraftProfile {
    std::string type{"Generic"};
    bool icing_equipped{};
    double max_crosswind_kt{20.0};
    double max_tailwind_kt{10.0};
    double turbulence_sensitivity{1.0};
};

struct Waypoint {
    std::string name;
    GeoPoint position;
    int altitude_ft{};
    double eta_min{};
};

struct Route {
    std::string callsign;
    double cruise_speed_kt{};
    std::vector<Waypoint> waypoints;
};

struct WeatherHazard {
    std::string id;
    HazardType type{HazardType::Unknown};
    Severity severity{Severity::Light};
    int base_ft{};
    int top_ft{60000};
    double valid_from_min{};
    double valid_to_min{};
    double confidence{1.0};
    std::string description;
    std::vector<GeoPoint> polygon;
};

struct Runway {
    std::string id;
    double heading_deg{};
};

struct AirportWeather {
    std::string icao;
    double visibility_m{};
    int ceiling_ft{};
    double wind_dir_deg{};
    double wind_speed_kt{};
    double gust_kt{};
    std::vector<Runway> runways;
};

struct WeatherProviderInfo {
    std::string name{"Local scenario"};
    std::string mode{"offline"};
    std::string attribution;
    std::string updated_at;
};

struct BaseMapInfo {
    std::string type{"synthetic"};
    std::string path;
    std::string projection{"EPSG:4326"};
};

struct Scenario {
    WeatherProviderInfo weather_provider;
    BaseMapInfo base_map;
    AircraftProfile aircraft;
    Route route;
    double route_corridor_nm{20.0};
    std::vector<WeatherHazard> hazards;
    std::vector<AirportWeather> airports;
};

Scenario load_scenario(const std::filesystem::path& path);

HazardType hazard_type_from_string(const std::string& value);
Severity severity_from_string(const std::string& value);
std::string to_string(HazardType type);
std::string to_string(Severity severity);

} // namespace aviwx
