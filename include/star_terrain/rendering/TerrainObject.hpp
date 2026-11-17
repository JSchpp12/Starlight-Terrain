#pragma once

#include "star_terrain/Enums.hpp"
#include "star_terrain/rendering/ChunkMeshDescription.hpp"
#include "star_terrain/rendering/TerrainRenderingType.hpp"

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
    TerrainObject(ColoringMode colorMode, std::vector<rendering::ChunkMeshDescription> meshDescriptions,
                  std::vector<std::unique_ptr<star::IMaterialProvider>> materials, star::Handle vertexShader,
                  star::Handle fragmentShader);

    virtual star::PipelineProvider getPipelineProvider(vk::PipelineLayout pipelineLayout) override;

    void setRenderShapeType(rendering::ElevationType type)
    {
        m_shapeRenderingMode = type;
    }

    virtual void recordRenderPassCommands(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout,
                                          uint8_t swapChainIndexNum, uint64_t frameIndex) override;

  protected:
    std::vector<star::StarMesh> loadMeshes(star::core::device::DeviceContext &context) override;

  private:
    ColoringMode m_colorDrawMode;
    /// @brief Only needed to build the meshes; released once loadMeshes has run.
    std::vector<rendering::ChunkMeshDescription> m_meshDescriptions;
    rendering::ElevationType m_shapeRenderingMode{rendering::ElevationType::Real};
};
} // namespace star::terrain
