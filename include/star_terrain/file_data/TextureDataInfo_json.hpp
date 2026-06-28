#pragma once

#include <nlohmann/json.hpp>

namespace star::terrain
{
struct TextureDataInfo;
void to_json(nlohmann::json &j, const TextureDataInfo &data);
void from_json(const nlohmann::json &j, TextureDataInfo &data);
} // namespace star::terrain