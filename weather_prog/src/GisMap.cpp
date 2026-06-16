#include "aviwx/GisMap.h"

namespace aviwx {

std::size_t GisMap::feature_count() const
{
    std::size_t total = 0;
    for (const GisLayer& layer : layers) {
        total += layer.features.size();
    }
    return total;
}

bool GisMap::empty() const
{
    return feature_count() == 0;
}

} // namespace aviwx
