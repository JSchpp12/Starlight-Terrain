#include "star_terrain/file_data/texture_data/Reader.hpp"

#include "star_terrain/file_data/TextureDataInfo_json.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

namespace star::terrain
{
std::tuple<ReadResult, TextureDataInfo> ReadTerrainTextureInfo(const std::string &path)
{
    star::core::logging::debug("ReadTerrainTextureInfo: opening ", path);

    std::ifstream i(path);
    if (!i.is_open())
    {
        STAR_THROW("ReadTerrainTextureInfo: failed to open file: " + path);
    }

    nlohmann::json j = nlohmann::json::parse(i);
    star::core::logging::debug("ReadTerrainTextureInfo: json parsed successfully");

    TextureDataInfo result;
    result = j.get<TextureDataInfo>();

    star::core::logging::debug("ReadTerrainTextureInfo: parsed ", result.chunks.size(), " chunks from ", path);

    return std::make_tuple<ReadResult, TextureDataInfo>(ReadResult{.result = ReadResult::Result::success},
                                                        std::move(result));
}
} // namespace star::terrain