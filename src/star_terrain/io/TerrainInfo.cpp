#include "star_terrain/io/TerrainInfo.hpp"

#include "star_terrain/serialization/TerrainInfo_json.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace star::terrain::io
{
std::tuple<ReadResult, TerrainInfo> ReadTerrainInfo(const std::string &path)
{
    std::ifstream i(path); 
    nlohmann::json j = nlohmann::json::parse(i);

    TerrainInfo result; 
    result = j.get<TerrainInfo>(); 

    return std::make_tuple<ReadResult, TerrainInfo>(ReadResult{.result = ReadResult::Result::success},
                                                    std::move(result)); 
}
} // namespace star::terrain::io