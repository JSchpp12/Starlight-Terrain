#include "star_terrain/file_data/TextureDataInfo_json.hpp"

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/file_data/TextureDataInfo.hpp"
#include "star_terrain/file_data/ChunkInfo_json.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

namespace star::terrain
{
void to_json(nlohmann::json &j, const TextureDataInfo &data)
{
}

void from_json(const nlohmann::json &j, TextureDataInfo &data)
{
    try
    {
        std::vector<std::string> heightFiles = j["elevation_files"];
        data.fullHeightFilePath = heightFiles[0];
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse TextureDataInfo.elevation_files", e);
    }

    try
    {
        auto &images = j["images"];
        data.chunks.reserve(images.size());

        for (size_t i{0}; i < images.size(); i++)
        {
            data.chunks.push_back(images[i]);
        }
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse TextureDataInfo.images", e);
    }
}
} // namespace star::terrain