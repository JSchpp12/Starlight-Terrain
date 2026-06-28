#pragma once

#include "star_terrain/rendering/TerrainRenderingType.hpp"

#include <filesystem>

namespace star::terrain
{
struct TerrainObjectDefinition
{
    std::filesystem::path terrainDir;                   //expected to contain `height_info.json` and `Shape.json`.
    std::filesystem::path vertShaderPath;               
    std::filesystem::path fragShaderPath;
    rendering::Type renderType = rendering::Type::Real;
};
} // namespace star::terrain