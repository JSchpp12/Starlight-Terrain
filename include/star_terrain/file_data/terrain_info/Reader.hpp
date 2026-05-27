#pragma once

#include "star_terrain/file_data/terrain_info/TerrainInfo.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <tuple>
#include <string>

namespace star::terrain::io{
std::tuple<ReadResult, TerrainInfo> ReadTerrainInfo(const std::string &path); 
}