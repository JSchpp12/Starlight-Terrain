#pragma once

#include "star_terrain/file_data/chunk_info/ChunkInfo.hpp"

#include <string>
#include <vector>


namespace star::terrain
{
struct TerrainInfo
{
    std::string fullHeightFilePath;
    std::vector<ChunkInfo> chunks;
};
} // namespace star::terrain