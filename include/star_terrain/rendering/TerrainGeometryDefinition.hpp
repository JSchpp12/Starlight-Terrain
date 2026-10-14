#pragma once

#include "star_terrain/rendering/ChunkMeshDescription.hpp"
#include "star_terrain/rendering/TerrainRenderingType.hpp"
#include "star_terrain/rendering/TerrainShapeRepresentationType.hpp"

#include <filesystem>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain::rendering
{

/// @brief Terrain geometry + the GPU vertex/index buffers built from it.
class TerrainGeometryDefinition
{
  public:
    std::vector<ChunkMeshDescription> meshDescriptions;
    std::filesystem::path terrainDir;
    /// Shape representation that produced this definition's GPU resources.
    Representation shapeRepresentation;
    Type defaultRenderType;

    class Builder
    {
      public:
        explicit Builder(core::device::DeviceContext &context) : m_context(context)
        {
        }
        Builder &setTerrainDir(std::filesystem::path terrainDir);
        Builder &setShapeRepresentation(Representation shapeRepresentation);
        Builder &setDefaultRenderType(Type defaultRenderType);
        TerrainGeometryDefinition build();

      private:
        core::device::DeviceContext &m_context;
        std::filesystem::path m_terrainDir;
        Representation m_shapeRepresentation{Representation::Geometry};
        Type m_defaultRenderType{Type::Flat};
    };

  private:
    friend class Builder;

    TerrainGeometryDefinition(std::vector<ChunkMeshDescription> meshDescriptions, std::filesystem::path terrainDir,
                              Representation shapeRepresentation, Type defaultRenderType);
};

} // namespace star::terrain::rendering