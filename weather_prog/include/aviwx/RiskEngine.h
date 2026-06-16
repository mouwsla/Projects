#pragma once

#include "aviwx/Scenario.h"

#include <string>
#include <vector>

namespace aviwx {

enum class RiskLevel {
    Low,
    Medium,
    High,
    Extreme
};

struct RiskFinding {
    RiskLevel level{RiskLevel::Low};
    double score{};
    std::string title;
    std::string details;
};

struct RiskReport {
    RiskLevel level{RiskLevel::Low};
    double score{};
    std::vector<RiskFinding> findings;
};

class RiskEngine {
public:
    RiskReport analyze(const Scenario& scenario) const;
};

RiskLevel risk_level_for_score(double score);
std::string to_string(RiskLevel level);
double crosswind_component_kt(double runway_heading_deg, double wind_from_deg, double wind_speed_kt);
double headwind_component_kt(double runway_heading_deg, double wind_from_deg, double wind_speed_kt);

} // namespace aviwx
