#include "star_terrain/rendering/TerrainObject.hpp"

#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include <cassert>
#include <utility>

namespace star::terrain
{

TerrainObject::TerrainObject(ColoringMode colorMode, std::vector<std::unique_ptr<star::IMaterialProvider>> materials,
                             star::Handle vertexShader, star::Handle fragmentShader, TerrainObjectDefinition def)
    : star::StarObject(std::move(materials)), m_colorDrawMode(colorMode), m_def(std::move(def))
{
    m_vertexShaderHandle = vertexShader;
    m_fragmentShaderHandle = fragmentShader;
}

star::PipelineProvider TerrainObject::getPipelineProvider(vk::PipelineLayout pipelineLayout)
{
    return star::PipelineProvider::Builder()
        .setShaders({m_vertexShaderHandle, m_fragmentShaderHandle})
        .setPipelineLayout(pipelineLayout)
        .setGraphicsOverrides(star::GraphicsOverrides{
            .vertexInput =
                star::VertexInputState{.bindings = star::terrain::rendering::getVertexBindingDescription(),
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
    (void)context;

    const auto &meshDescriptions = m_def.geometry.meshDescriptions;
    assert(meshDescriptions.size() == m_materialHandles.size() && "Every chunk should have its own material");

    std::vector<star::StarMesh> meshes;
    meshes.reserve(meshDescriptions.size());
    for (size_t i = 0; i < meshDescriptions.size(); i++)
    {
        const auto &desc = meshDescriptions[i];
        meshes.emplace_back(desc.vertBuffer, desc.indBuffer, desc.vertCount, desc.indCount, m_materialHandles[i],
                            desc.bbMin, desc.bbMax, false);
    }

    return meshes;
}
} // namespace star::terrain
