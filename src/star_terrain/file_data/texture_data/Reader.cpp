#include "star_terrain/file_data/texture_data/Reader.hpp"

#include "star_terrain/file_data/TextureDataInfo_json.hpp"
#include "star_terrain/io/ReadResult.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace star::terrain
{
std::tuple<ReadResult, TextureDataInfo> ReadTerrainTextureInfo(const std::string &path)
{
    std::ifstream i(path);
    nlohmann::json j = nlohmann::json::parse(i);

    TextureDataInfo result;
    result = j.get<TextureDataInfo>();

    return std::make_tuple<ReadResult, TextureDataInfo>(ReadResult{.result = ReadResult::Result::success},
                                                        std::move(result));
}
} // namespace star::terrain