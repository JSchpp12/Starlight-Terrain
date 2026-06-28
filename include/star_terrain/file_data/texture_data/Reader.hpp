#pragma once

#include "star_terrain/file_data/TextureDataInfo.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <string>
#include <tuple>

namespace star::terrain
{
std::tuple<ReadResult, TextureDataInfo> ReadTerrainTextureInfo(const std::string &path);
}