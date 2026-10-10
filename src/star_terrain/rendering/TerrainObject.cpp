#include "star_terrain/rendering/TerrainObject.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/rendering/TerrainTextureLoader.hpp"
#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace star::terrain
{

static std::vector<std::shared_ptr<star::StarMaterial>> CreateTerrainMaterials(star::core::device::DeviceContext &context,
                                                                        const TerrainObjectDefinition &def)
{
    // Store by value: the tuple returned by ReadTerrainTextureInfo is a temporary and
    // binding a reference to std::get<1> of it does not extend its lifetime.
    const TextureDataInfo fileInfo =
        std::get<1>(ReadTerrainTextureInfo((def.geometry.terrainDir / "height_info.json").string()));

    if (def.colorMode != ColoringMode::color)
    {
        std::vector<std::shared_ptr<star::StarMaterial>> materials;
        materials.reserve(fileInfo.chunks.size());
        for (size_t i = 0; i < fileInfo.chunks.size(); i++)
        {
            materials.push_back(std::make_shared<star::StarMaterial>());
        }
        return materials;
    }

    return LoadTerrainTextures(context, def.geometry.terrainDir, fileInfo, def.textures);
}

TerrainObject::TerrainObject(star::core::device::DeviceContext &context, TerrainObjectDefinition def,
                             star::ShaderResolver &shaderResolver)
    : star::StarObject(CreateTerrainMaterials(context, def)), m_def(std::move(def))
{
    m_vertexShaderHandle = shaderResolver.resolve(star::Shader_Stage::vertex);
    m_fragmentShaderHandle = shaderResolver.resolve(star::Shader_Stage::fragment);
}

star::PipelineProvider TerrainObject::getPipelineProvider(vk::PipelineLayout pipelineLayout)
{
    return star::PipelineProvider(
        {m_vertexShaderHandle, m_fragmentShaderHandle}, pipelineLayout,
        star::GraphicsOverrides{
            .vertexInput =
                star::VertexInputState{.bindings = star::terrain::rendering::getVertexBindingDescription(),
                                       .attributes = star::terrain::rendering::getVertexInputAttributeDescription()},
            .dynamicStates =
                m_def.colorMode == ColoringMode::greyscale
                    ? std::vector<vk::DynamicState>{vk::DynamicState::eScissor, vk::DynamicState::eViewport,
                                                    vk::DynamicState::eLineWidth, vk::DynamicState::eCullMode}
                    : std::vector<vk::DynamicState>()});
}

std::vector<star::StarMesh> TerrainObject::loadMeshes(star::core::device::DeviceContext &context)
{
    // conditionally pre-load textures
    if (m_def.colorMode == star::terrain::ColoringMode::color)
    {
        for (auto &material : m_meshMaterials)
        {
            static_cast<star::TextureMaterial *>(material.get())->preloadTexture(context);
        }
    }

    const auto &meshDescriptions = m_def.geometry.meshDescriptions;
    assert(meshDescriptions.size() == m_meshMaterials.size() && "Every chunk should have its own material");

    std::vector<star::StarMesh> terrainMeshes;
    terrainMeshes.reserve(meshDescriptions.size());

    for (size_t i = 0; i < meshDescriptions.size(); i++)
    {
        const auto &desc = meshDescriptions[i];
        terrainMeshes.emplace_back(desc.vertBuffer, desc.indBuffer, desc.vertCount, desc.indCount, m_meshMaterials[i],
                                   desc.bbMin, desc.bbMax, false);
    }

    return terrainMeshes;
}
} // namespace star::terrain
