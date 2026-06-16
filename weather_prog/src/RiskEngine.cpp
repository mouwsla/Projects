#include "aviwx/RiskEngine.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace aviwx {
namespace {

double severity_score(Severity severity)
{
    switch (severity) {
    case Severity::Light:
        return 18.0;
    case Severity::Moderate:
        return 40.0;
    case Severity::Severe:
        return 68.0;
    case Severity::Extreme:
        return 90.0;
    }
    return 10.0;
}

double hazard_modifier(HazardType type, const AircraftProfile& aircraft)
{
    switch (type) {
    case HazardType::Thunderstorm:
        return 20.0;
    case HazardType::Icing:
        return aircraft.icing_equipped ? 6.0 : 22.0;
    case HazardType::Turbulence:
        return 12.0 * std::max(0.5, aircraft.turbulence_sensitivity);
    case HazardType::WindShear:
        return 25.0;
    case HazardType::VolcanicAsh:
        return 30.0;
    case HazardType::HeavyPrecipitation:
        return 12.0;
    case HazardType::LowVisibility:
    case HazardType::LowCeiling:
    case HazardType::Unknown:
        return 8.0;
    }
    return 0.0;
}

bool time_overlaps(double a0, double a1, double b0, double b1)
{
    return std::max(a0, b0) <= std::min(a1, b1);
}

bool altitude_overlaps(int a0, int a1, int b0, int b1)
{
    return std::max(a0, b0) <= std::min(a1, b1);
}

std::string fixed(double value, int precision = 1)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

void add_finding(RiskReport& report, double score, std::string title, std::string details)
{
    RiskFinding finding;
    finding.score = std::clamp(score, 0.0, 100.0);
    finding.level = risk_level_for_score(finding.score);
    finding.title = std::move(title);
    finding.details = std::move(details);
    report.score = std::max(report.score, finding.score);
    report.findings.push_back(std::move(finding));
}

void analyze_route_hazards(const Scenario& scenario, RiskReport& report)
{
    const auto& waypoints = scenario.route.waypoints;
    for (std::size_t i = 0; i + 1 < waypoints.size(); ++i) {
        const Waypoint& a = waypoints[i];
        const Waypoint& b = waypoints[i + 1];
        const double segment_start = std::min(a.eta_min, b.eta_min);
        const double segment_end = std::max(a.eta_min, b.eta_min);
        const int altitude_min = std::min(a.altitude_ft, b.altitude_ft);
        const int altitude_max = std::max(a.altitude_ft, b.altitude_ft);

        for (const WeatherHazard& hazard : scenario.hazards) {
            if (!time_overlaps(segment_start, segment_end, hazard.valid_from_min, hazard.valid_to_min)) {
                continue;
            }
            if (!altitude_overlaps(altitude_min, altitude_max, hazard.base_ft, hazard.top_ft)) {
                continue;
            }
            if (hazard.polygon.size() < 3) {
                continue;
            }

            const double distance_nm = segment_polygon_distance_nm(a.position, b.position, hazard.polygon);
            if (distance_nm > scenario.route_corridor_nm) {
                continue;
            }

            const double proximity = distance_nm <= 0.1
                ? 1.0
                : std::clamp(0.65 + 0.35 * (1.0 - distance_nm / scenario.route_corridor_nm), 0.65, 1.0);
            const double raw = (severity_score(hazard.severity) + hazard_modifier(hazard.type, scenario.aircraft))
                * proximity * std::clamp(hazard.confidence, 0.2, 1.0);
            const double score = std::clamp(raw, 0.0, 100.0);

            if (score < 12.0) {
                continue;
            }

            std::ostringstream details;
            details << hazard.id << " intersects or approaches segment " << a.name << "-" << b.name
                    << "; type=" << to_string(hazard.type)
                    << ", severity=" << to_string(hazard.severity)
                    << ", distance=" << fixed(distance_nm) << " NM"
                    << ", layer=" << hazard.base_ft << "-" << hazard.top_ft << " ft"
                    << ", segment altitude=" << altitude_min << "-" << altitude_max << " ft"
                    << ", valid=" << fixed(hazard.valid_from_min, 0) << "-" << fixed(hazard.valid_to_min, 0) << " min.";
            if (!hazard.description.empty()) {
                details << " " << hazard.description;
            }

            add_finding(
                report,
                score,
                to_string(hazard.severity) + " " + to_string(hazard.type) + " near route",
                details.str());
        }
    }
}

void analyze_airports(const Scenario& scenario, RiskReport& report)
{
    if (scenario.route.waypoints.empty()) {
        return;
    }

    const std::string departure = scenario.route.waypoints.front().name;
    const std::string destination = scenario.route.waypoints.back().name;

    for (const AirportWeather& airport : scenario.airports) {
        if (airport.icao != departure && airport.icao != destination) {
            continue;
        }

        const std::string phase = airport.icao == departure ? "departure" : "arrival";

        if (airport.visibility_m > 0.0 && airport.visibility_m < 5000.0) {
            double score = 28.0;
            if (airport.visibility_m < 3000.0) {
                score = 50.0;
            }
            if (airport.visibility_m < 1500.0) {
                score = 78.0;
            }
            add_finding(
                report,
                score,
                "Reduced visibility at " + phase + " airport",
                airport.icao + " visibility is " + fixed(airport.visibility_m, 0) + " m.");
        }

        if (airport.ceiling_ft > 0 && airport.ceiling_ft < 1500) {
            double score = 30.0;
            if (airport.ceiling_ft < 800) {
                score = 55.0;
            }
            if (airport.ceiling_ft < 400) {
                score = 82.0;
            }
            add_finding(
                report,
                score,
                "Low ceiling at " + phase + " airport",
                airport.icao + " ceiling is " + std::to_string(airport.ceiling_ft) + " ft.");
        }

        if (!airport.runways.empty() && airport.wind_speed_kt > 0.0) {
            const double wind_for_limit = airport.gust_kt > airport.wind_speed_kt ? airport.gust_kt : airport.wind_speed_kt;
            double best_crosswind = std::numeric_limits<double>::infinity();
            std::string best_runway;
            for (const Runway& runway : airport.runways) {
                const double crosswind = std::abs(crosswind_component_kt(runway.heading_deg, airport.wind_dir_deg, wind_for_limit));
                if (crosswind < best_crosswind) {
                    best_crosswind = crosswind;
                    best_runway = runway.id;
                }
            }

            const double ratio = best_crosswind / std::max(1.0, scenario.aircraft.max_crosswind_kt);
            if (ratio >= 0.75) {
                double score = 34.0;
                if (ratio >= 1.0) {
                    score = 62.0;
                }
                if (ratio >= 1.25) {
                    score = 82.0;
                }

                std::ostringstream details;
                details << airport.icao << " best runway " << best_runway
                        << " crosswind is " << fixed(best_crosswind) << " kt using "
                        << fixed(wind_for_limit) << " kt wind/gust. Aircraft limit is "
                        << fixed(scenario.aircraft.max_crosswind_kt) << " kt.";
                add_finding(report, score, "Crosswind near or above limit at " + airport.icao, details.str());
            }
        }
    }
}

} // namespace

RiskReport RiskEngine::analyze(const Scenario& scenario) const
{
    if (scenario.route.waypoints.size() < 2) {
        throw std::runtime_error("scenario route must contain at least two waypoints");
    }

    RiskReport report;
    analyze_route_hazards(scenario, report);
    analyze_airports(scenario, report);

    if (report.findings.empty()) {
        add_finding(report, 8.0, "No material weather risk found", "No loaded hazards intersect the route corridor, time window, and altitude band.");
    }

    std::sort(report.findings.begin(), report.findings.end(), [](const RiskFinding& a, const RiskFinding& b) {
        return a.score > b.score;
    });
    report.level = risk_level_for_score(report.score);
    return report;
}

RiskLevel risk_level_for_score(double score)
{
    if (score >= 75.0) {
        return RiskLevel::Extreme;
    }
    if (score >= 50.0) {
        return RiskLevel::High;
    }
    if (score >= 25.0) {
        return RiskLevel::Medium;
    }
    return RiskLevel::Low;
}

std::string to_string(RiskLevel level)
{
    switch (level) {
    case RiskLevel::Low:
        return "LOW";
    case RiskLevel::Medium:
        return "MEDIUM";
    case RiskLevel::High:
        return "HIGH";
    case RiskLevel::Extreme:
        return "EXTREME";
    }
    return "UNKNOWN";
}

double crosswind_component_kt(double runway_heading_deg, double wind_from_deg, double wind_speed_kt)
{
    const double angle = deg_to_rad(angular_difference_deg(runway_heading_deg, wind_from_deg));
    return wind_speed_kt * std::sin(angle);
}

double headwind_component_kt(double runway_heading_deg, double wind_from_deg, double wind_speed_kt)
{
    const double angle = deg_to_rad(angular_difference_deg(runway_heading_deg, wind_from_deg));
    return wind_speed_kt * std::cos(angle);
}

} // namespace aviwx
