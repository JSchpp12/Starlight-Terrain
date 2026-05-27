#pragma once

#include "star_terrain/file_data/texture_data_info/TextureDataInfo.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <string>
#include <tuple>

namespace star::terrain
{
std::tuple<ReadResult, TextureDataInfo> ReadTerrainTextureInfo(const std::string &path);
}