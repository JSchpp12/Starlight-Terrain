#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/rendering/TerrainTextureLoaderPlan.hpp"

#include <filesystem>
#include <span>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain
{
/// Resolve the on-disk compressed (.ktx2) path for every target chunk.
/// Throws if a chunk's texture cannot be located.
std::vector<std::filesystem::path> ResolveTerrainTexturePaths(const std::filesystem::path &terrainDir,
                                                              std::span<const ChunkInfo> chunks);

/// Build the per-chunk materials for a terrain using the provided loading plan.
/// The returned vector always matches chunks in size and order.
TerrainTextureMaterials LoadTerrainTextures(core::device::DeviceContext &context,
                                            const std::filesystem::path &terrainDir, std::span<const ChunkInfo> chunks,
                                            const TerrainTextureLoaderPlan &plan);
} // namespace star::terrain
