#pragma once

#include <nlohmann/json.hpp>

namespace star::terrain
{
struct TerrainInfo; 
void to_json(nlohmann::json &j, const TerrainInfo &data);
void from_json(const nlohmann::json &j, TerrainInfo &data);
}