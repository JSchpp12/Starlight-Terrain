#pragma once

#include "star_terrain/rendering/TerrainRenderingType.hpp"

#include <filesystem>

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
    std::filesystem::path shapeFilePath; 
    std::filesystem::path heightInfoFilePath; 
    rendering::Type renderType;
};

struct TerrainObjectDefinition
{
    TerrainGeometryDefinition geometry; 
    std::filesystem::path vertShaderPath; 
    std::filesystem::path fragShaderPath; 
};
} // namespace star::terrain