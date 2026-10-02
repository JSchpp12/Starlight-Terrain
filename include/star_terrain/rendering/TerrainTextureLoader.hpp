#pragma once

#include "star_terrain/file_data/TextureDataInfo.hpp"
#include "star_terrain/rendering/TerrainTextureLoaderPlan.hpp"

#include <filesystem>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain
{
/// Resolve the on-disk compressed (.ktx2) path for every chunk described by
/// fileInfo. Throws if a chunk's texture cannot be located.
std::vector<std::filesystem::path> ResolveTerrainTexturePaths(const std::filesystem::path &terrainDir,
                                                              const TextureDataInfo &fileInfo);

/// Build the per-chunk materials for a terrain using the provided loading plan.
/// The returned vector always matches fileInfo.chunks in size and order.
TerrainTextureMaterials LoadTerrainTextures(core::device::DeviceContext &context,
                                            const std::filesystem::path &terrainDir, const TextureDataInfo &fileInfo,
                                            const TerrainTextureLoaderPlan &plan);
} // namespace star::terrain
