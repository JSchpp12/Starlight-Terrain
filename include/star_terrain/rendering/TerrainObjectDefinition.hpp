#pragma once

#include "star_terrain/rendering/TerrainRenderingType.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <star_common/Handle.hpp>

namespace star::terrain
{

struct ChunkMeshDescription
{
    glm::vec3 bbMin;
    glm::vec3 bbMax;
    Handle vertBuffer;
    Handle indBuffer;
    uint32_t vertCount;
    uint32_t indCount;
};

struct TerrainGeometryDefinition
{
    std::vector<ChunkMeshDescription> meshDescriptions;
    std::filesystem::path terrainDir;
    rendering::Type renderType;

    TerrainGeometryDefinition(std::filesystem::path terrainDir, rendering::Type renderType)
        : terrainDir(std::move(terrainDir)), renderType(std::move(renderType))
    {
    }
};

struct TerrainObjectDefinition
{
    TerrainGeometryDefinition geometry;
    std::filesystem::path vertShaderPath;
    std::filesystem::path fragShaderPath;
};
} // namespace star::terrain