#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/io/TerrainTargetInfo.hpp"
#include "star_terrain/rendering/ChunkMeshDescription.hpp"
#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"

#include <starlight/ShaderResolver.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace star
{
class StarObject;
class StarMaterial;
} // namespace star

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain
{
class TerrainFactory
{
  public:
    struct Output
    {
        std::shared_ptr<star::StarObject> colorTerrain;
        std::shared_ptr<star::StarObject> greyscaleTerrain;
        std::filesystem::path shapeFilePath;
        rendering::ElevationType defaultRenderingType;
    };

    /// @brief Load the chunks' geometry and build the color and shadow objects.
    /// @param terrainInfo Terrain-wide data (height raster + world offset) shared by the chunks.
    /// @param chunks The target chunks selected for this terrain.
    /// @param shapeFilePath The terrain's shape descriptor, used for the image-metric record.
    static Output buildTerrains(core::device::DeviceContext &context, rendering::TerrainGeometryDefinition geometry,
                                const TerrainTargetInfo &terrainInfo, std::span<const ChunkInfo> chunks,
                                const std::filesystem::path &shapeFilePath, const std::filesystem::path &shaderDir,
                                bool aggressiveTextureLoading);

  private:
    static std::shared_ptr<star::StarObject> buildTerrain(
        core::device::DeviceContext &context, const rendering::TerrainGeometryDefinition &geometry,
        std::span<const ChunkInfo> chunks, const std::vector<rendering::ChunkMeshDescription> &meshDescriptions,
        star::ShaderResolver terrainResolver, const std::string &name, bool useGreyscale,
        bool aggressiveTextureLoading);
};
} // namespace star::terrain
