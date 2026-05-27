#pragma once

#include "star_terrain/struct/TerrainInfo.hpp"
#include "star_terrain/struct/ReadResult.hpp"

#include <tuple>
#include <string>

namespace star::terrain::io{
std::tuple<ReadResult, TerrainInfo> ReadTerrainInfo(const std::string &path); 
}