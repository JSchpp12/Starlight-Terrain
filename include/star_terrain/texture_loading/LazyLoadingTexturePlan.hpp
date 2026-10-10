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
/// Load textures lazily: the material only stores the texture path and the actual KTX load + transcode happens on a
/// Starlight transfer worker
struct LazyLoadingTexturePlan
{
    TerrainTextureMaterials load(core::device::DeviceContext &context,
                                 const std::vector<std::filesystem::path> &texturePaths) const;
};
} // namespace star::terrain::texture_loading
