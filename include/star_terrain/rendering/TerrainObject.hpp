#pragma once

#include "star_terrain/file_data/TextureDataInfo.hpp"
#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include "starlight/ShaderResolver.hpp"
#include "starlight/object/StarObject.hpp"

#include <filesystem>

namespace star::terrain
{
/// Renderable terrain object. Subclasses star::StarObject and produces one
/// StarMesh per chunk described by the terrain directory's height_info.json.
class TerrainObject : public star::StarObject
{
  public:
    TerrainObject(star::core::device::DeviceContext &context, TerrainObjectDefinition def,
                  star::ShaderResolver &shaderResolver);

    std::filesystem::path getHeightInfoFilePath() const noexcept
    {
        return m_def.terrainDir / "height_info.json";
    }
    std::filesystem::path getShapeFilePath() const noexcept
    {
        return m_def.terrainDir / "Shape.json";
    }
    rendering::Type getRenderingType() const noexcept
    {
        return m_def.renderType;
    }

  protected:
    std::vector<star::StarMesh> loadMeshes(star::core::device::DeviceContext &context) override;

  private:
    TerrainObjectDefinition m_def;

    static std::vector<std::shared_ptr<star::StarMaterial>> loadMaterials(const std::filesystem::path &terrainDir,
                                                                          const TextureDataInfo &fileInfo);
};
} // namespace star::terrain