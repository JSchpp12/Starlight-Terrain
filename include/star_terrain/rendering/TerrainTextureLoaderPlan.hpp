#pragma once

#include "star_terrain/texture_loading/AggressiveLoadingTexturePlan.hpp"
#include "star_terrain/texture_loading/LazyLoadingTexturePlan.hpp"
#include "star_terrain/texture_loading/TerrainTextureMaterials.hpp"

#include <filesystem>
#include <variant>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain
{
using texture_loading::TerrainTextureMaterials;

/// Describes how a TerrainObject should acquire/transcode its per-chunk
/// textures. The contained variant selects the concrete strategy; each strategy
/// owns its own load implementation. New strategies can be added as additional
/// alternatives without changing TerrainObject.
class TerrainTextureLoaderPlan
{
  public:
    using Variant =
        std::variant<texture_loading::LazyLoadingTexturePlan, texture_loading::AggressiveLoadingTexturePlan>;

    Variant plan{texture_loading::LazyLoadingTexturePlan{}};

    static TerrainTextureLoaderPlan Lazy()
    {
        return TerrainTextureLoaderPlan{texture_loading::LazyLoadingTexturePlan{}};
    }

    static TerrainTextureLoaderPlan Aggressive()
    {
        return TerrainTextureLoaderPlan{texture_loading::AggressiveLoadingTexturePlan{}};
    }

    /// Dispatch to the selected strategy's load implementation.
    TerrainTextureMaterials load(core::device::DeviceContext &context,
                                 const std::vector<std::filesystem::path> &texturePaths) const;
};
} // namespace star::terrain
