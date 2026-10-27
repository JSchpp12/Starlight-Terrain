#include "star_terrain/rendering/TerrainObject.hpp"

#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include <utility>

namespace star::terrain
{

TerrainObject::TerrainObject(ColoringMode colorMode, std::vector<std::shared_ptr<star::StarMaterial>> materials,
                             star::Handle vertexShader, star::Handle fragmentShader, std::vector<star::StarMesh> meshes)
    : star::StarObject(std::move(materials), vertexShader, fragmentShader), m_colorDrawMode(colorMode),
      m_meshes(std::move(meshes))
{
}

star::PipelineProvider TerrainObject::getPipelineProvider(vk::PipelineLayout pipelineLayout)
{
    return star::PipelineProvider::Builder()
        .setShaders({m_vertexShaderHandle, m_fragmentShaderHandle})
        .setPipelineLayout(pipelineLayout)
        .setGraphicsOverrides(
            star::GraphicsOverrides{
                .vertexInput = star::VertexInputState{
                    .bindings = star::terrain::rendering::getVertexBindingDescription(),
                    .attributes = star::terrain::rendering::getVertexInputAttributeDescription()},
                .dynamicStates =
                    m_colorDrawMode == ColoringMode::greyscale
                        ? std::vector<vk::DynamicState>{vk::DynamicState::eScissor, vk::DynamicState::eViewport,
                                                        vk::DynamicState::eLineWidth, vk::DynamicState::eCullMode}
                        : std::vector<vk::DynamicState>()})
        .build();
}

void TerrainObject::recordRenderPassCommands(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout,
                                             uint8_t swapChainIndexNum, uint64_t frameIndex)
{
    switch (m_shapeRenderingMode)
    {
    case rendering::ElevationType::Flat:
        break;
    default:
        this->StarObject::recordRenderPassCommands(commandBuffer, pipelineLayout, swapChainIndexNum, frameIndex);
        break;
    }
}

std::vector<star::StarMesh> TerrainObject::loadMeshes(star::core::device::DeviceContext &context)
{
    return std::move(m_meshes);
}
} // namespace star::terrain
