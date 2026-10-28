#pragma once

#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"
#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include <starlight/ShaderResolver.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <filesystem>
#include <memory>
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

    /// @param shaderDir Directory holding the terrain shaders. May be a media-relative candidate.
    /// Both this directory and the geometry's terrainDir are resolved through the IOService
    /// (GetPath command) before anything is built; loaders and the reported shapeFilePath
    /// receive the resolved directories.
    static Output buildTerrains(core::device::DeviceContext &context, rendering::TerrainGeometryDefinition geometry,
                                const std::filesystem::path &shaderDir, bool aggressiveTextureLoading);

  private:
    static std::shared_ptr<star::StarObject> buildTerrain(core::device::DeviceContext &context,
                                                          const rendering::TerrainGeometryDefinition &geometry,
                                                          star::ShaderResolver terrainResolver, const std::string &name,
                                                          bool useGreyscale, bool aggressiveTextureLoading);
};
} // namespace star::terrain
