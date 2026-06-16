#pragma once

#include "aviwx/Geo.h"

namespace aviwx {

// A single point forecast sample taken from the live weather provider
// (Open-Meteo). Every field here is a value the provider actually returns —
// there are no estimated/derived quantities, so the map shows real data only.
struct LiveWeatherSample {
    GeoPoint position;
    double valid_from_min{};
    double valid_to_min{};
    double temperature_c{};
    double precipitation_mm{};
    double cloud_cover_pct{};
    double visibility_m{-1.0};
    double wind_speed_ms{};
    double wind_gust_ms{};
    double wind_direction_deg{};
    double pressure_hpa{};
    double cape_jkg{};
};

} // namespace aviwx
