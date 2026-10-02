#pragma once

#include "star_terrain/texture_loading/TerrainTextureMaterials.hpp"

#include <filesystem>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain::texture_loading
{
/// Load textures aggressively: every chunk texture is loaded and transcoded up front, in parallel with TBB, before
/// being handed to the material.
struct AggressiveLoadingTexturePlan
{
    TerrainTextureMaterials load(core::device::DeviceContext &context,
                                 const std::vector<std::filesystem::path> &texturePaths) const;
};
} // namespace star::terrain::texture_loading
