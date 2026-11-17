#include "star_terrain/rendering/TerrainTextureLoader.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <optional>
#include <sstream>
#include <variant>
namespace star::terrain
{
static std::optional<std::filesystem::path> CheckForCompressedTexture(const std::filesystem::path &terrainDir,
                                                                      std::string chunkPath)
{
    chunkPath += ".ktx2";
    std::filesystem::path testPath = terrainDir / std::filesystem::path(chunkPath);
    if (std::filesystem::exists(testPath))
        return std::make_optional(testPath);
    return std::nullopt;
}

static std::filesystem::path ResolveTexturePath(const std::filesystem::path &terrainDir, const std::string &textureFile)
{
    std::optional<std::filesystem::path> found = CheckForCompressedTexture(terrainDir, textureFile);

    if (!found.has_value())
    {
        // Fall back to searching the directory for a matching compressed file.
        auto files =
            star::file_helpers::FindFilesInDirectoryWithSameNameIgnoreFileType(terrainDir.string(), textureFile);
        for (const auto &file : files)
        {
            if (file.extension() == ".ktx2")
            {
                found = file;
                break;
            }
        }
    }

    if (!found.has_value())
    {
        std::ostringstream oss;
        oss << "Failed to find matching texture for file: " << textureFile << std::endl
            << "Ensure terrains are prepared with compressed textures" << std::endl;
        STAR_THROW(oss.str());
    }

    return found.value();
}

std::vector<std::filesystem::path> ResolveTerrainTexturePaths(const std::filesystem::path &terrainDir,
                                                              std::span<const ChunkInfo> chunks)
{
    std::vector<std::filesystem::path> paths;
    paths.reserve(chunks.size());
    for (const auto &chunk : chunks)
    {
        paths.push_back(ResolveTexturePath(terrainDir, chunk.textureFile));
    }
    return paths;
}

TerrainTextureMaterials TerrainTextureLoaderPlan::load(core::device::DeviceContext &context,
                                                       const std::vector<std::filesystem::path> &texturePaths) const
{
    return std::visit([&](const auto &concretePlan) { return concretePlan.load(context, texturePaths); }, plan);
}

TerrainTextureMaterials LoadTerrainTextures(core::device::DeviceContext &context,
                                            const std::filesystem::path &terrainDir, std::span<const ChunkInfo> chunks,
                                            const TerrainTextureLoaderPlan &plan)
{
    return plan.load(context, ResolveTerrainTexturePaths(terrainDir, chunks));
}
} // namespace star::terrain
