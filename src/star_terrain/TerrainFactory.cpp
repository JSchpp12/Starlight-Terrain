#include "star_terrain/TerrainFactory.hpp"

#include "star_terrain/Enums.hpp"
#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"
#include "star_terrain/io/TerrainShapeLoader.hpp"
#include "star_terrain/rendering/ChunkUpload.hpp"
#include "star_terrain/rendering/FromTerrainDirLoader.hpp"
#include "star_terrain/rendering/TerrainObjectDefinition.hpp"
#include "star_terrain/rendering/TerrainTextureLoader.hpp"

#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/device/managers/Material.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/material/MaterialProvider.hpp>
#include <starlight/object/StarObject.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <exception>
#include <span>
#include <utility>

namespace star::terrain
{
static std::vector<std::unique_ptr<star::StarMaterial>> CreateTerrainMaterials(core::device::DeviceContext &context,
                                                                               ColoringMode colorMode,
                                                                               const TerrainObjectDefinition &def,
                                                                               std::span<const ChunkInfo> chunks)
{
    if (colorMode != ColoringMode::color)
    {
        std::vector<std::unique_ptr<star::StarMaterial>> materials;
        materials.reserve(chunks.size());
        for (size_t i = 0; i < chunks.size(); i++)
        {
            materials.push_back(std::make_unique<star::StarMaterial>());
        }
        return materials;
    }

    return LoadTerrainTextures(context, def.geometry.terrainDir, chunks, def.textures);
}

/// @brief Load every target chunk as full geometry on worker threads, then upload the payloads to GPU buffers on the
/// main thread.
static std::vector<rendering::ChunkMeshDescription> BuildFullGeometryChunkDescriptions(
    core::device::DeviceContext &context, const TerrainTargetInfo &terrainInfo, std::span<const ChunkInfo> chunks,
    rendering::ElevationType elevationType)
{
    TerrainShapeLoader<LoadFullGeometry> loader(&terrainInfo, chunks, elevationType);
    auto loadedChunks = loader();

    std::vector<rendering::ChunkMeshDescription> meshDescriptions;
    meshDescriptions.reserve(loadedChunks.size());

    for (std::size_t i = 0; i < loadedChunks.size(); i++)
    {
        auto &loadedChunk = loadedChunks[i];
        if (!loadedChunk)
            STAR_THROW("Failed to load terrain chunk: " + loadedChunk.error().detail);

        try
        {
            meshDescriptions.push_back(rendering::uploadChunk(std::move(*loadedChunk), context));
        }
        catch (const std::exception &e)
        {
            STAR_THROW(std::string{"Failed to upload terrain chunk: "} + e.what());
        }
    }

    return meshDescriptions;
}

TerrainFactory::Output TerrainFactory::buildTerrains(
    core::device::DeviceContext &context, rendering::TerrainGeometryDefinition geometry,
    const TerrainTargetInfo &terrainInfo, std::span<const ChunkInfo> chunks, const std::filesystem::path &shapeFilePath,
    const std::filesystem::path &shaderDir, bool aggressiveTextureLoading)
{
    Output output;

    std::vector<rendering::ChunkMeshDescription> meshDescriptions;
    switch (geometry.shapeRepresentation)
    {
    case rendering::Representation::Geometry:
        meshDescriptions = BuildFullGeometryChunkDescriptions(context, terrainInfo, chunks, geometry.defaultRenderType);
        break;
    case rendering::Representation::Height_Map:
        STAR_THROW("The height map terrain shape representation is not implemented yet");
    default:
        STAR_THROW("Unknown terrain shape representation");
    }

    star::ShaderResolver colorResolver =
        star::ShaderResolver::Builder{context.getCmdBus()}
            .setShader(star::Shader_Stage::vertex, (shaderDir / "color.vert").string())
            .setShader(star::Shader_Stage::fragment, (shaderDir / "color.frag").string())
            .build();
    output.colorTerrain = buildTerrain(context, geometry, chunks, meshDescriptions, std::move(colorResolver),
                                       "terrain_color", false, aggressiveTextureLoading);

    star::ShaderResolver shadowResolver =
        star::ShaderResolver::Builder{context.getCmdBus()}
            .setShader(star::Shader_Stage::vertex, (shaderDir / "shadow_cast.vert").string())
            .setShader(star::Shader_Stage::fragment, (shaderDir / "shadow_cast.frag").string())
            .build();
    output.greyscaleTerrain = buildTerrain(context, geometry, chunks, meshDescriptions, std::move(shadowResolver),
                                           "terrain_depth", true, aggressiveTextureLoading);

    output.shapeFilePath = shapeFilePath;
    output.defaultRenderingType = geometry.defaultRenderType;
    return output;
}

std::shared_ptr<star::StarObject> TerrainFactory::buildTerrain(
    core::device::DeviceContext &context, const rendering::TerrainGeometryDefinition &geometry,
    std::span<const ChunkInfo> chunks, const std::vector<rendering::ChunkMeshDescription> &meshDescriptions,
    star::ShaderResolver terrainResolver, const std::string &name, bool useGreyscale, bool aggressiveTextureLoading)
{
    const ColoringMode colorMode = useGreyscale ? ColoringMode::greyscale : ColoringMode::color;
    TerrainObjectDefinition def{.geometry = geometry,
                                .textures = aggressiveTextureLoading ? TerrainTextureLoaderPlan::Aggressive()
                                                                     : TerrainTextureLoaderPlan::Lazy()};

    auto materials = CreateTerrainMaterials(context, colorMode, def, chunks);

    // Preload path-backed textures before the recipes are moved into the manager.
    if (colorMode == ColoringMode::color)
    {
        for (auto &material : materials)
            static_cast<star::TextureMaterial *>(material.get())->preloadTexture(context);
    }

    std::vector<std::unique_ptr<star::IMaterialProvider>> providers;
    providers.reserve(materials.size());
    for (auto &material : materials)
    {
        providers.push_back(std::make_unique<star::MaterialProvider>(std::move(material)));
    }

    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<FromTerrainDirLoader>(colorMode, meshDescriptions, std::move(providers)))
                   .setShaderResolver(std::move(terrainResolver))
                   .setUniqueName(name)
                   .build();
    context.begin().set(cmd).submit();
    cmd.getReply().get()->init(context);
    return cmd.getReply().get();
}

} // namespace star::terrain
