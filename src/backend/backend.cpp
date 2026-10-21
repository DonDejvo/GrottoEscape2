#include "backend.h"

const uint16_t indexCache[] = {
    0, 1, 2,
    0, 2, 3
};

Backend::Backend(Window* window)
    : mWindow(window)
{
}

std::vector<uint32_t> Backend::genIndexData() const
{
    std::vector<uint32_t> indexData(LAYER_MAX_TILES * TILE_LAYER_COUNT * 6);
    for (size_t i = 0; i < LAYER_MAX_TILES * TILE_LAYER_COUNT; ++i) {
        for (size_t j = 0; j < 6; ++j) {
            indexData[i * 6 + j] = indexCache[j] + 4 * i;
        }
    }
    return indexData;
}
