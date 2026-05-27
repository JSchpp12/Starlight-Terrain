#pragma once

#include "star_terrain/file_data/terrain_info/TerrainInfo.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <string>
#include <tuple>

namespace star::terrain::io{
std::tuple<ReadResult, TerrainInfo> ReadTerrainInfo(const std::string &path); 
}