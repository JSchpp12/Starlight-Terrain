#pragma once

#include "star_terrain/rendering/TerrainRenderingType.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <star_common/Handle.hpp>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

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

/// Terrain geometry + the GPU vertex/index buffers built from it. Built once
/// (by Builder::build, which reads the height raster, generates the chunk
/// meshes, and uploads the vertex/index buffers) so the same buffers can be
/// shared by every TerrainObject that renders this geometry -- e.g. the color
/// terrain and the shadow-cast terrain -- without re-reading the raster or
/// duplicating the buffers.
class TerrainGeometryDefinition
{
  public:
    std::vector<ChunkMeshDescription> meshDescriptions;
    std::filesystem::path terrainDir;
    rendering::Type renderType;

    class Builder
    {
      public:
        explicit Builder(core::device::DeviceContext &context) : m_context(context)
        {
        }
        Builder &setTerrainDir(std::filesystem::path terrainDir);
        Builder &setRenderType(rendering::Type renderType);
        TerrainGeometryDefinition build();

      private:
        core::device::DeviceContext &m_context;
        std::filesystem::path m_terrainDir;
        rendering::Type m_renderType = rendering::Type::Flat;
    };

  private:
    friend class Builder;

    TerrainGeometryDefinition(std::vector<ChunkMeshDescription> meshDescriptions, std::filesystem::path terrainDir,
                              rendering::Type renderType);
};

} // namespace star::terrain