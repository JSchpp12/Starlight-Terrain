#pragma once

#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"
#include "star_terrain/rendering/TerrainTextureLoaderPlan.hpp"

namespace star::terrain
{

struct TerrainObjectDefinition
{
    rendering::TerrainGeometryDefinition geometry;
    TerrainTextureLoaderPlan textures;
};
} // namespace star::terrain
