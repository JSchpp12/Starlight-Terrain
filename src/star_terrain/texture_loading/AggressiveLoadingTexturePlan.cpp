#include "star_terrain/texture_loading/AggressiveLoadingTexturePlan.hpp"

#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/common/textures/SharedCompressedTexture.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <tbb/parallel_for.h>

#include <memory>
#include <utility>

namespace star::terrain::texture_loading
{
TerrainTextureMaterials AggressiveLoadingTexturePlan::load(
    core::device::DeviceContext &context, const std::vector<std::filesystem::path> &texturePaths) const
{
    std::vector<std::unique_ptr<star::SharedCompressedTexture>> transcoded(texturePaths.size());

    const auto physicalDevice = context.getDevice().getPhysicalDevice();

    tbb::parallel_for(size_t{0}, texturePaths.size(), [&](size_t i) {
        transcoded[i] = std::make_unique<star::SharedCompressedTexture>(
            star::SharedCompressedTexture::Builder()
                .setPath(texturePaths[i].string())
                .setAttemptGPUCompressionScheme(physicalDevice)
                .build());
        transcoded[i]->triggerTranscode();
    });

    TerrainTextureMaterials materials{texturePaths.size()};
    for (size_t i = 0; i < texturePaths.size(); ++i)
    {
        materials[i] = std::make_shared<star::TextureMaterial>(texturePaths[i].string(), std::move(transcoded[i])); 
    }

    return materials;
}
} // namespace star::terrain::texture_loading
