#include "aviwx/RiskEngine.h"

#include <cassert>
#include <cmath>

namespace {

void test_wind_components()
{
    assert(std::abs(aviwx::crosswind_component_kt(0.0, 90.0, 20.0) - 20.0) < 0.001);
    assert(std::abs(aviwx::headwind_component_kt(0.0, 0.0, 20.0) - 20.0) < 0.001);
    assert(std::abs(aviwx::headwind_component_kt(0.0, 180.0, 10.0) + 10.0) < 0.001);
}

void test_route_hazard_risk()
{
    aviwx::Scenario scenario;
    scenario.route_corridor_nm = 20.0;
    scenario.aircraft.type = "C172";
    scenario.aircraft.icing_equipped = false;
    scenario.aircraft.max_crosswind_kt = 15.0;

    scenario.route.callsign = "TEST1";
    scenario.route.waypoints = {
        {"UUEE", {55.97, 37.41}, 0, 0.0},
        {"WP1", {56.20, 38.10}, 6000, 20.0},
        {"UUOB", {56.70, 37.50}, 0, 55.0},
    };

    aviwx::WeatherHazard hazard;
    hazard.id = "SIGMET-TEST";
    hazard.type = aviwx::HazardType::Thunderstorm;
    hazard.severity = aviwx::Severity::Severe;
    hazard.base_ft = 1000;
    hazard.top_ft = 39000;
    hazard.valid_from_min = 0.0;
    hazard.valid_to_min = 60.0;
    hazard.confidence = 0.95;
    hazard.polygon = {
        {56.05, 37.80},
        {56.10, 38.35},
        {56.45, 38.15},
        {56.35, 37.70},
    };
    scenario.hazards.push_back(hazard);

    aviwx::AirportWeather destination;
    destination.icao = "UUOB";
    destination.visibility_m = 4200;
    destination.ceiling_ft = 900;
    destination.wind_dir_deg = 250.0;
    destination.wind_speed_kt = 18.0;
    destination.gust_kt = 26.0;
    destination.runways = {{"07", 70.0}, {"25", 250.0}};
    scenario.airports.push_back(destination);

    const aviwx::RiskReport report = aviwx::RiskEngine().analyze(scenario);
    assert(report.score >= 75.0);
    assert(report.level == aviwx::RiskLevel::Extreme);
    assert(!report.findings.empty());
}

} // namespace

int main()
{
    test_wind_components();
    test_route_hazard_risk();
    return 0;
}
