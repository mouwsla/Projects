#include "aviwx/Scenario.h"

#include "aviwx/Json.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace aviwx {
namespace {

std::string lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

double optional_number(const Json& object, const std::string& key, double fallback)
{
    return object.contains(key) ? object.at(key).as_number() : fallback;
}

int optional_int(const Json& object, const std::string& key, int fallback)
{
    return static_cast<int>(optional_number(object, key, fallback));
}

bool optional_bool(const Json& object, const std::string& key, bool fallback)
{
    return object.contains(key) ? object.at(key).as_bool() : fallback;
}

std::string optional_string(const Json& object, const std::string& key, std::string fallback)
{
    return object.contains(key) ? object.at(key).as_string() : std::move(fallback);
}

GeoPoint parse_point(const Json& value)
{
    return GeoPoint{
        value.at("lat").as_number(),
        value.at("lon").as_number(),
    };
}

AircraftProfile parse_aircraft(const Json& value)
{
    AircraftProfile aircraft;
    aircraft.type = optional_string(value, "type", aircraft.type);
    aircraft.icing_equipped = optional_bool(value, "icing_equipped", aircraft.icing_equipped);
    aircraft.max_crosswind_kt = optional_number(value, "max_crosswind_kt", aircraft.max_crosswind_kt);
    aircraft.max_tailwind_kt = optional_number(value, "max_tailwind_kt", aircraft.max_tailwind_kt);
    aircraft.turbulence_sensitivity = optional_number(value, "turbulence_sensitivity", aircraft.turbulence_sensitivity);
    return aircraft;
}

Route parse_route(const Json& value)
{
    Route route;
    route.callsign = optional_string(value, "callsign", "");
    route.cruise_speed_kt = optional_number(value, "cruise_speed_kt", 0.0);

    for (const Json& waypoint_json : value.at("waypoints").as_array()) {
        Waypoint waypoint;
        waypoint.name = waypoint_json.at("name").as_string();
        waypoint.position = parse_point(waypoint_json);
        waypoint.altitude_ft = optional_int(waypoint_json, "altitude_ft", 0);
        waypoint.eta_min = optional_number(waypoint_json, "eta_min", 0.0);
        route.waypoints.push_back(std::move(waypoint));
    }

    return route;
}

WeatherHazard parse_hazard(const Json& value)
{
    WeatherHazard hazard;
    hazard.id = optional_string(value, "id", "HAZARD");
    hazard.type = hazard_type_from_string(optional_string(value, "type", "unknown"));
    hazard.severity = severity_from_string(optional_string(value, "severity", "light"));
    hazard.base_ft = optional_int(value, "base_ft", hazard.base_ft);
    hazard.top_ft = optional_int(value, "top_ft", hazard.top_ft);
    hazard.valid_from_min = optional_number(value, "valid_from_min", hazard.valid_from_min);
    hazard.valid_to_min = optional_number(value, "valid_to_min", hazard.valid_to_min);
    hazard.confidence = optional_number(value, "confidence", hazard.confidence);
    hazard.description = optional_string(value, "description", "");

    for (const Json& point_json : value.at("polygon").as_array()) {
        hazard.polygon.push_back(parse_point(point_json));
    }
    return hazard;
}

AirportWeather parse_airport(const Json& value)
{
    AirportWeather airport;
    airport.icao = value.at("icao").as_string();
    airport.visibility_m = optional_number(value, "visibility_m", airport.visibility_m);
    airport.ceiling_ft = optional_int(value, "ceiling_ft", airport.ceiling_ft);
    airport.wind_dir_deg = optional_number(value, "wind_dir_deg", airport.wind_dir_deg);
    airport.wind_speed_kt = optional_number(value, "wind_speed_kt", airport.wind_speed_kt);
    airport.gust_kt = optional_number(value, "gust_kt", airport.gust_kt);

    if (value.contains("runways")) {
        for (const Json& runway_json : value.at("runways").as_array()) {
            Runway runway;
            runway.id = runway_json.at("id").as_string();
            runway.heading_deg = runway_json.at("heading_deg").as_number();
            airport.runways.push_back(std::move(runway));
        }
    }

    return airport;
}

WeatherProviderInfo parse_provider(const Json& value)
{
    WeatherProviderInfo provider;
    provider.name = optional_string(value, "name", provider.name);
    provider.mode = optional_string(value, "mode", provider.mode);
    provider.attribution = optional_string(value, "attribution", provider.attribution);
    provider.updated_at = optional_string(value, "updated_at", provider.updated_at);
    return provider;
}

BaseMapInfo parse_base_map(const Json& value)
{
    BaseMapInfo map;
    map.type = optional_string(value, "type", map.type);
    map.path = optional_string(value, "path", map.path);
    map.projection = optional_string(value, "projection", map.projection);
    return map;
}

Scenario parse_scenario(const Json& root)
{
    Scenario scenario;
    if (root.contains("weather_provider")) {
        scenario.weather_provider = parse_provider(root.at("weather_provider"));
    }
    if (root.contains("base_map")) {
        scenario.base_map = parse_base_map(root.at("base_map"));
    }
    scenario.route_corridor_nm = optional_number(root, "route_corridor_nm", scenario.route_corridor_nm);
    scenario.aircraft = root.contains("aircraft") ? parse_aircraft(root.at("aircraft")) : scenario.aircraft;
    scenario.route = parse_route(root.at("route"));

    if (root.contains("hazards")) {
        for (const Json& hazard_json : root.at("hazards").as_array()) {
            scenario.hazards.push_back(parse_hazard(hazard_json));
        }
    }

    if (root.contains("airports")) {
        for (const Json& airport_json : root.at("airports").as_array()) {
            scenario.airports.push_back(parse_airport(airport_json));
        }
    }

    return scenario;
}

} // namespace

Scenario load_scenario(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open scenario file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse_scenario(Json::parse(buffer.str()));
}

HazardType hazard_type_from_string(const std::string& value)
{
    const std::string type = lower_copy(value);
    if (type == "thunderstorm" || type == "ts" || type == "cb") {
        return HazardType::Thunderstorm;
    }
    if (type == "icing") {
        return HazardType::Icing;
    }
    if (type == "turbulence" || type == "turb") {
        return HazardType::Turbulence;
    }
    if (type == "low_visibility" || type == "visibility") {
        return HazardType::LowVisibility;
    }
    if (type == "low_ceiling" || type == "ceiling") {
        return HazardType::LowCeiling;
    }
    if (type == "wind_shear" || type == "windshear") {
        return HazardType::WindShear;
    }
    if (type == "volcanic_ash" || type == "ash") {
        return HazardType::VolcanicAsh;
    }
    if (type == "heavy_precipitation" || type == "precipitation" || type == "precip") {
        return HazardType::HeavyPrecipitation;
    }
    return HazardType::Unknown;
}

Severity severity_from_string(const std::string& value)
{
    const std::string severity = lower_copy(value);
    if (severity == "moderate" || severity == "mod") {
        return Severity::Moderate;
    }
    if (severity == "severe" || severity == "sev") {
        return Severity::Severe;
    }
    if (severity == "extreme" || severity == "extr") {
        return Severity::Extreme;
    }
    return Severity::Light;
}

std::string to_string(HazardType type)
{
    switch (type) {
    case HazardType::Thunderstorm:
        return "thunderstorm";
    case HazardType::Icing:
        return "icing";
    case HazardType::Turbulence:
        return "turbulence";
    case HazardType::LowVisibility:
        return "low visibility";
    case HazardType::LowCeiling:
        return "low ceiling";
    case HazardType::WindShear:
        return "wind shear";
    case HazardType::VolcanicAsh:
        return "volcanic ash";
    case HazardType::HeavyPrecipitation:
        return "heavy precipitation";
    case HazardType::Unknown:
        return "unknown hazard";
    }
    return "unknown hazard";
}

std::string to_string(Severity severity)
{
    switch (severity) {
    case Severity::Light:
        return "light";
    case Severity::Moderate:
        return "moderate";
    case Severity::Severe:
        return "severe";
    case Severity::Extreme:
        return "extreme";
    }
    return "unknown";
}

} // namespace aviwx
