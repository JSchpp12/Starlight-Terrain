#pragma once

#include <nlohmann/json.hpp>

namespace star::terrain
{
struct ChunkInfo;
void to_json(nlohmann::json &j, const ChunkInfo &data);
void from_json(const nlohmann::json &j, ChunkInfo &data);
} // namespace star::terrain