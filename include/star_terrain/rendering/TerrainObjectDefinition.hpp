#pragma once

#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"

#include <filesystem>

namespace star::terrain
{

struct TerrainObjectDefinition
{
    TerrainGeometryDefinition geometry;
    std::filesystem::path vertShaderPath;
    std::filesystem::path fragShaderPath;
};
} // namespace star::terrain
