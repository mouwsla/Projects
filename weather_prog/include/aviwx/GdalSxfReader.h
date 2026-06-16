#pragma once

#include "aviwx/GisMap.h"

#include <filesystem>
#include <vector>

namespace aviwx {

struct SxfMapTile {
    std::filesystem::path path;
    double min_lat{};
    double max_lat{};
    double min_lon{};
    double max_lon{};
};

std::vector<std::filesystem::path> find_sxf_files(const std::filesystem::path& root);
std::vector<SxfMapTile> index_sxf_files(const std::vector<std::filesystem::path>& paths);
std::vector<std::filesystem::path> select_sxf_files(
    const std::vector<SxfMapTile>& tiles,
    double min_lat,
    double max_lat,
    double min_lon,
    double max_lon,
    std::size_t max_files = 24);
GisMap load_sxf_maps(const std::vector<std::filesystem::path>& paths, std::size_t max_features = 1000000);

} // namespace aviwx
