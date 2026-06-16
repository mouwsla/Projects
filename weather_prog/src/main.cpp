#include "aviwx/RiskEngine.h"
#include "aviwx/Scenario.h"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>

namespace {

void print_usage(const char* program)
{
    std::cout << "Usage: " << program << " <scenario.json>\n\n"
              << "Example:\n"
              << "  " << program << " examples/demo_scenario.json\n";
}

void print_report(const aviwx::Scenario& scenario, const aviwx::RiskReport& report)
{
    const auto& route = scenario.route;
    const auto& first = route.waypoints.front();
    const auto& last = route.waypoints.back();

    std::cout << "AviWx Risk MVP\n";
    std::cout << "Aircraft: " << scenario.aircraft.type << "\n";
    std::cout << "Weather source: " << scenario.weather_provider.name
              << " (" << scenario.weather_provider.mode << ")\n";
    std::cout << "Base map: " << scenario.base_map.type;
    if (!scenario.base_map.path.empty()) {
        std::cout << " [" << scenario.base_map.path << "]";
    }
    std::cout << "\n";
    std::cout << "Route: " << first.name << " -> " << last.name
              << " (" << route.waypoints.size() << " waypoints, corridor "
              << scenario.route_corridor_nm << " NM)\n";
    if (!route.callsign.empty()) {
        std::cout << "Callsign: " << route.callsign << "\n";
    }
    std::cout << "Overall risk: " << aviwx::to_string(report.level)
              << " (" << std::fixed << std::setprecision(1) << report.score << "/100)\n\n";

    for (std::size_t i = 0; i < report.findings.size(); ++i) {
        const aviwx::RiskFinding& finding = report.findings[i];
        std::cout << i + 1 << ". [" << aviwx::to_string(finding.level) << "] "
                  << finding.title << " - " << std::fixed << std::setprecision(1)
                  << finding.score << "/100\n";
        std::cout << "   " << finding.details << "\n";
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        print_usage(argv[0]);
        return argc == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    try {
        const aviwx::Scenario scenario = aviwx::load_scenario(argv[1]);
        const aviwx::RiskReport report = aviwx::RiskEngine().analyze(scenario);
        print_report(scenario, report);
    } catch (const std::exception& ex) {
        std::cerr << "aviwx: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
