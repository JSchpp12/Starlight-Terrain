#pragma once

#include "star_terrain/rendering/TerrainRenderingType.hpp"
#include "star_terrain/rendering/TerrainShapeRepresentationType.hpp"

#include <filesystem>

namespace star::terrain::rendering
{

/// @brief Terrain geometry metadata.
/// @details Built by TerrainFactory; the GPU mesh descriptions it was built alongside live in the factory.
struct TerrainGeometryDefinition
{
    std::filesystem::path terrainDir;
    /// Shape representation that produced this definition's GPU resources.
    Representation shapeRepresentation;
    ElevationType defaultRenderType;
};

} // namespace star::terrain::rendering
