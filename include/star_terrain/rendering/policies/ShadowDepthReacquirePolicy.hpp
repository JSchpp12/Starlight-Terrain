#pragma once

#include <variant>

#include <vulkan/vulkan.hpp>

namespace star::terrain::rendering
{
// Reverse (compute -> shadow): reacquire the depth back for rendering next frame.
// Emitted in the shadow Pre (TerrainShadowRenderPhase::recordPreRenderPassCommands),
// guarded by "compute neighbor ran before".
struct ShadowDepthOwnershipReacquire
{
    uint32_t graphicsQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;

    bool fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept;
};

struct ShadowDepthSameQueueTransitionBack
{
    bool fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept;
};

using ShadowDepthReacquirePolicy = std::variant<ShadowDepthOwnershipReacquire, ShadowDepthSameQueueTransitionBack>;

inline ShadowDepthReacquirePolicy makeShadowDepthReacquirePolicy(uint32_t graphics, uint32_t compute) noexcept
{
    return graphics != compute ? ShadowDepthReacquirePolicy{ShadowDepthOwnershipReacquire{graphics, compute}}
                               : ShadowDepthReacquirePolicy{ShadowDepthSameQueueTransitionBack{}};
}
} // namespace star::terrain::rendering
