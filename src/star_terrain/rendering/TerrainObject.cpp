#include "star_terrain/rendering/TerrainObject.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace star::terrain
{

TerrainObject::TerrainObject(star::core::device::DeviceContext &context, TerrainObjectDefinition def,
                             star::ShaderResolver &shaderResolver)
    : star::StarObject(loadMaterials(
          def.geometry.terrainDir,
          std::get<1>(ReadTerrainTextureInfo((def.geometry.terrainDir / "height_info.json").string())), def.colorMode)),
      m_def(std::move(def))
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
    // conditionally pre-load texturesI
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

std::optional<std::filesystem::path> CheckForCompressedTexture(const std::filesystem::path &terrainDir,
                                                               std::string chunkPath)
{
    chunkPath += ".ktx2";
    std::filesystem::path testPath = terrainDir / std::filesystem::path(chunkPath);
    if (std::filesystem::exists(testPath))
        return std::make_optional(testPath);
    return std::nullopt;
}

std::vector<std::shared_ptr<star::StarMaterial>> TerrainObject::loadMaterials(const std::filesystem::path &terrainDir,
                                                                              const TextureDataInfo &fileInfo,
                                                                              star::terrain::ColoringMode colorMode)
{
    std::vector<std::shared_ptr<star::StarMaterial>> materials;
    materials.reserve(fileInfo.chunks.size());

    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        std::optional<std::filesystem::path> found =
            CheckForCompressedTexture(terrainDir, fileInfo.chunks[i].textureFile);

        if (!found.has_value())
        {
            // manually iterate and search for proper one
            auto files = star::file_helpers::FindFilesInDirectoryWithSameNameIgnoreFileType(
                terrainDir.string(), fileInfo.chunks[i].textureFile);
            for (const auto &file : files)
            {
                if (file.extension() == ".ktx2")
                    found = file;
            }

            if (found.has_value())
                break;
        }

        if (!found.has_value())
        {
            std::ostringstream oss;
            oss << "Failed to find matching texture for file: " << fileInfo.chunks[i].textureFile << std::endl
                << "Ensure terrains are prepared with compressed textures" << std::endl;
            STAR_THROW(oss.str());
        }

        if (colorMode == star::terrain::ColoringMode::color)
        {
            materials.push_back(std::make_shared<star::TextureMaterial>(found.value().string()));
        }
        else
        {
            materials.push_back(std::make_shared<star::StarMaterial>());
        }
    }

    return materials;
}

} // namespace star::terrain