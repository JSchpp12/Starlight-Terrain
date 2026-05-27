#include "star_terrain/file_data/texture_data_info/TextureDataInfo_json.hpp"

#include "star_terrain/file_data/chunk_info/ChunkInfo.hpp"
#include "star_terrain/file_data/texture_data_info/TextureDataInfo.hpp"
#include "star_terrain/file_data/chunk_info/ChunkInfo_json.hpp"

namespace star::terrain
{
void to_json(nlohmann::json &j, const TextureDataInfo &data)
{
}

void from_json(const nlohmann::json &j, TextureDataInfo &data)
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