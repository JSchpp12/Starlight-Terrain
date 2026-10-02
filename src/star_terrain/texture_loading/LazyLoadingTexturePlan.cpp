#include "star_terrain/texture_loading/LazyLoadingTexturePlan.hpp"

#include <starlight/common/materials/TextureMaterial.hpp>

#include <memory>

namespace star::terrain::texture_loading
{
TerrainTextureMaterials LazyLoadingTexturePlan::load(core::device::DeviceContext & /*context*/,
                                                     const std::vector<std::filesystem::path> &texturePaths) const
{
    TerrainTextureMaterials materials{texturePaths.size()};

    for (size_t i{0}; i < texturePaths.size(); i++)
    {
        materials[i] = std::make_shared<star::TextureMaterial>(texturePaths[i].string());
    }

    return materials;
}
} // namespace star::terrain::texture_loading
