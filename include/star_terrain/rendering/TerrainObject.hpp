#pragma once

#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include "starlight/object/StarObject.hpp"
#include "starlight/virtual/StarMesh.hpp"

#include <memory>
#include <vector>

namespace star::terrain
{
/// @brief Renderable terrain object class to be consumed by a render phase type object.
class TerrainObject : public star::StarObject
{
  public:
    TerrainObject(ColoringMode colorMode, std::vector<std::shared_ptr<star::StarMaterial>> materials,
                  star::Handle vertexShader, star::Handle fragmentShader, std::vector<star::StarMesh> meshes);

    virtual star::PipelineProvider getPipelineProvider(vk::PipelineLayout pipelineLayout) override;

  protected:
    std::vector<star::StarMesh> loadMeshes(star::core::device::DeviceContext &context) override;

  private:
    ColoringMode m_colorDrawMode;
    std::vector<star::StarMesh> m_meshes;
};
} // namespace star::terrain
