#include "star_terrain/serialization/TerrainInfo_json.hpp"

#include "star_terrain/struct/ChunkInfo.hpp"
#include "star_terrain/struct/TerrainInfo.hpp"
#include "star_terrain/serialization/ChunkInfo_json.hpp"

namespace star::terrain
{
void to_json(nlohmann::json &j, const TerrainInfo &data)
{
}

void from_json(const nlohmann::json &j, TerrainInfo &data)
{
    data.fullHeightFilePath = j["full_terrain_file"]; 

    auto &images = j["images"]; 
    data.chunks.reserve(images.size()); 

    for (size_t i{0}; i < images.size(); i++)
    {
        data.chunks.push_back(images[i]);
    }
}
} // namespace star::terrain