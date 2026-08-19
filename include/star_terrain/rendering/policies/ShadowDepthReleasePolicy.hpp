#pragma once

#include <variant>

#include <vulkan/vulkan.hpp>

namespace star::terrain::rendering
{
struct ShadowDepthOwnershipRelease
{
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;

    // Returns true if a barrier should be recorded.
    bool fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept;
};

struct ShadowDepthSameQueueNoOp
{
    bool fillBarrier(vk::Image, vk::ImageMemoryBarrier2 &) const noexcept;
};

using ShadowDepthReleasePolicy = std::variant<ShadowDepthOwnershipRelease, ShadowDepthSameQueueNoOp>;

inline ShadowDepthReleasePolicy makeShadowDepthReleasePolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute ? ShadowDepthReleasePolicy{ShadowDepthOwnershipRelease{graphics, compute}}
                               : ShadowDepthReleasePolicy{ShadowDepthSameQueueNoOp{}};
}
} // namespace star::terrain::rendering
