#pragma once

#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"

#include <filesystem>

namespace star::terrain
{

enum class ColoringMode
{
    greyscale,
    color
};

struct TerrainObjectDefinition
{
    TerrainGeometryDefinition geometry;
    std::filesystem::path vertShaderPath;
    std::filesystem::path fragShaderPath;
    ColoringMode colorMode;
};
} // namespace star::terrain
