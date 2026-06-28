#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"

#include <string>
#include <vector>


namespace star::terrain
{
struct TextureDataInfo
{
    std::string fullHeightFilePath;
    std::vector<ChunkInfo> chunks;
};
} // namespace star::terrain