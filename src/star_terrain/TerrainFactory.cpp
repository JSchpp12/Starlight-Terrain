#include "star_terrain/TerrainFactory.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/rendering/FromTerrainDirLoader.hpp"
#include "star_terrain/rendering/TerrainTextureLoader.hpp"

#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/command/FileIO/GetPath.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/object/StarObject.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <cassert>
#include <tuple>
#include <utility>

namespace star::terrain
{
static std::vector<std::shared_ptr<star::StarMaterial>> CreateTerrainMaterials(core::device::DeviceContext &context,
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

static std::vector<star::StarMesh> loadMeshes(core::device::DeviceContext &context, const TerrainObjectDefinition &def,
                                              const std::vector<std::shared_ptr<star::StarMaterial>> &materials)
{
    // conditionally pre-load textures
    if (def.colorMode == star::terrain::ColoringMode::color)
    {
        for (auto &material : materials)
        {
            static_cast<star::TextureMaterial *>(material.get())->preloadTexture(context);
        }
    }

    const auto &meshDescriptions = def.geometry.meshDescriptions;
    assert(meshDescriptions.size() == materials.size() && "Every chunk should have its own material");

    std::vector<star::StarMesh> terrainMeshes;
    terrainMeshes.reserve(meshDescriptions.size());

    for (size_t i = 0; i < meshDescriptions.size(); i++)
    {
        const auto &desc = meshDescriptions[i];
        terrainMeshes.emplace_back(desc.vertBuffer, desc.indBuffer, desc.vertCount, desc.indCount, materials[i],
                                   desc.bbMin, desc.bbMax, false);
    }

    return terrainMeshes;
}

static std::filesystem::path ResolveDirectory(core::CommandBus &cmdBus, const std::filesystem::path &candidate)
{
    star::command::file_io::GetPath cmd;
    cmd.setCandidate(candidate).setKind(star::io::PathKind::directory);
    cmdBus.submit(cmd);
    const auto reply = cmd.getReply().get();
    if (!reply)
    {
        STAR_THROW("TerrainFactory failed to resolve directory: " + reply.error().toString());
    }
    return reply.value();
}

TerrainFactory::Output TerrainFactory::buildTerrains(core::device::DeviceContext &context,
                                                     const rendering::TerrainGeometryDefinition &geometry,
                                                     const std::filesystem::path &shaderDir,
                                                     bool aggressiveTextureLoading)
{
    Output output;

    const std::filesystem::path resolvedShaderDir = ResolveDirectory(context.getCmdBus(), shaderDir);

    rendering::TerrainGeometryDefinition resolvedGeometry = geometry;
    resolvedGeometry.terrainDir = ResolveDirectory(context.getCmdBus(), geometry.terrainDir);

    star::ShaderResolver colorResolver =
        star::ShaderResolver::Builder{context.getCmdBus()}
            .setShader(star::Shader_Stage::vertex, (resolvedShaderDir / "color.vert").string())
            .setShader(star::Shader_Stage::fragment, (resolvedShaderDir / "color.frag").string())
            .build();
    output.colorTerrain = buildTerrain(context, resolvedGeometry, std::move(colorResolver), "terrain_color", false,
                                       aggressiveTextureLoading);

    star::ShaderResolver shadowResolver =
        star::ShaderResolver::Builder{context.getCmdBus()}
            .setShader(star::Shader_Stage::vertex, (resolvedShaderDir / "shadow_cast.vert").string())
            .setShader(star::Shader_Stage::fragment, (resolvedShaderDir / "shadow_cast.frag").string())
            .build();
    output.greyscaleTerrain = buildTerrain(context, resolvedGeometry, std::move(shadowResolver), "terrain_depth", true,
                                           aggressiveTextureLoading);

    output.shapeFilePath = resolvedGeometry.terrainDir / "Shape.json";
    output.defaultRenderingType = resolvedGeometry.defaultRenderType;
    return output;
}

std::shared_ptr<star::StarObject> TerrainFactory::buildTerrain(core::device::DeviceContext &context,
                                                               const rendering::TerrainGeometryDefinition &geometry,
                                                               star::ShaderResolver terrainResolver,
                                                               const std::string &name, bool useGreyscale,
                                                               bool aggressiveTextureLoading)
{
    TerrainObjectDefinition def{.geometry = geometry,
                                .colorMode = useGreyscale ? ColoringMode::greyscale : ColoringMode::color,
                                .textures = aggressiveTextureLoading ? TerrainTextureLoaderPlan::Aggressive()
                                                                     : TerrainTextureLoaderPlan::Lazy()};

    auto materials = CreateTerrainMaterials(context, def);
    auto meshes = loadMeshes(context, def, materials);

    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<FromTerrainDirLoader>(def, std::move(materials), std::move(meshes)))
                   .setShaderResolver(std::move(terrainResolver))
                   .setUniqueName(name)
                   .build();
    context.begin().set(cmd).submit();
    cmd.getReply().get()->init(context);
    return cmd.getReply().get();
}

} // namespace star::terrain
