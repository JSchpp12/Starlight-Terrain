#include "star_terrain/TerrainFactory.hpp"

#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"
#include "star_terrain/io/LibraryCart.hpp"
#include "star_terrain/io/TerrainShapeLoader.hpp"
#include "star_terrain/rendering/ChunkUpload.hpp"
#include "star_terrain/rendering/FromTerrainDirLoader.hpp"
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
#include <optional>
#include <span>
#include <utility>

namespace star::terrain
{
static std::vector<std::unique_ptr<star::StarMaterial>> CreateTerrainMaterials(core::device::DeviceContext &context,
                                                                               const TerrainObjectDefinition &def,
                                                                               std::span<const ChunkInfo> chunks)
{
    // Chunks come from the cart, which the geometry build narrowed to the chunks that produced a mesh.
    // Materials therefore stay 1:1 with meshDescriptions.
    if (def.colorMode != ColoringMode::color)
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
/// main thread. The result is aligned with the input chunks; failed targets hold std::nullopt.
static std::vector<std::optional<rendering::ChunkMeshDescription>> BuildFullGeometryChunkDescriptions(
    core::device::DeviceContext &context, const TerrainTargetInfo &terrainInfo, std::span<const ChunkInfo> chunks,
    rendering::ElevationType elevationType)
{
    TerrainShapeLoader<LoadFullGeometry> loader(&terrainInfo, chunks, elevationType);
    auto loadedChunks = loader();

    std::vector<std::optional<rendering::ChunkMeshDescription>> meshDescriptions(chunks.size());

    for (std::size_t i = 0; i < loadedChunks.size(); i++)
    {
        auto &loadedChunk = loadedChunks[i];
        if (!loadedChunk)
        {
            star::core::logging::warning("Skipping terrain chunk that failed to load: ", loadedChunk.error().detail);
            continue;
        }

        try
        {
            meshDescriptions[i] = rendering::uploadChunk(std::move(*loadedChunk), context);
        }
        catch (const std::exception &e)
        {
            star::core::logging::warning("Skipping terrain chunk that failed to upload: ", e.what());
        }
    }

    return meshDescriptions;
}

/// @brief Build the full-geometry mesh descriptions for the cart's chunks and narrow the cart to loaded chunks.
/// @details The result is aligned with the cart's chunks; failed targets are dropped from both.
static std::vector<rendering::ChunkMeshDescription> BuildFullGeometryMeshDescriptions(
    core::device::DeviceContext &context, LibraryCart &cart, rendering::ElevationType elevationType)
{
    auto loadedChunks = BuildFullGeometryChunkDescriptions(context, cart.info(), cart.chunks(), elevationType);

    std::vector<rendering::ChunkMeshDescription> meshDescriptions;
    std::vector<std::size_t> survivingIndices;
    meshDescriptions.reserve(loadedChunks.size());
    survivingIndices.reserve(loadedChunks.size());

    for (std::size_t i = 0; i < loadedChunks.size(); i++)
    {
        if (!loadedChunks[i])
            continue;

        meshDescriptions.push_back(std::move(*loadedChunks[i]));
        survivingIndices.push_back(i);
    }

    // Drop targets that produced no mesh so materials built from the cart stay 1:1 with meshDescriptions.
    cart.retainOnly(survivingIndices);

    return meshDescriptions;
}

TerrainFactory::Output TerrainFactory::buildTerrains(core::device::DeviceContext &context,
                                                     rendering::TerrainGeometryDefinition geometry, LibraryCart &cart,
                                                     const std::filesystem::path &shapeFilePath,
                                                     const std::filesystem::path &shaderDir,
                                                     bool aggressiveTextureLoading)
{
    Output output;

    std::vector<rendering::ChunkMeshDescription> meshDescriptions;
    switch (geometry.shapeRepresentation)
    {
    case rendering::Representation::Geometry:
        meshDescriptions = BuildFullGeometryMeshDescriptions(context, cart, geometry.defaultRenderType);
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
    output.colorTerrain = buildTerrain(context, geometry, cart.chunks(), meshDescriptions, std::move(colorResolver),
                                       "terrain_color", false, aggressiveTextureLoading);

    star::ShaderResolver shadowResolver =
        star::ShaderResolver::Builder{context.getCmdBus()}
            .setShader(star::Shader_Stage::vertex, (shaderDir / "shadow_cast.vert").string())
            .setShader(star::Shader_Stage::fragment, (shaderDir / "shadow_cast.frag").string())
            .build();
    output.greyscaleTerrain = buildTerrain(context, geometry, cart.chunks(), meshDescriptions,
                                           std::move(shadowResolver), "terrain_depth", true, aggressiveTextureLoading);

    output.shapeFilePath = shapeFilePath;
    output.defaultRenderingType = geometry.defaultRenderType;
    return output;
}

std::shared_ptr<star::StarObject> TerrainFactory::buildTerrain(
    core::device::DeviceContext &context, const rendering::TerrainGeometryDefinition &geometry,
    std::span<const ChunkInfo> chunks, const std::vector<rendering::ChunkMeshDescription> &meshDescriptions,
    star::ShaderResolver terrainResolver, const std::string &name, bool useGreyscale, bool aggressiveTextureLoading)
{
    TerrainObjectDefinition def{.geometry = geometry,
                                .colorMode = useGreyscale ? ColoringMode::greyscale : ColoringMode::color,
                                .textures = aggressiveTextureLoading ? TerrainTextureLoaderPlan::Aggressive()
                                                                     : TerrainTextureLoaderPlan::Lazy()};

    auto materials = CreateTerrainMaterials(context, def, chunks);

    // Preload path-backed textures before the recipes are moved into the manager.
    if (def.colorMode == star::terrain::ColoringMode::color)
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
                   .setLoader(std::make_unique<FromTerrainDirLoader>(def, meshDescriptions, std::move(providers)))
                   .setShaderResolver(std::move(terrainResolver))
                   .setUniqueName(name)
                   .build();
    context.begin().set(cmd).submit();
    cmd.getReply().get()->init(context);
    return cmd.getReply().get();
}

} // namespace star::terrain
