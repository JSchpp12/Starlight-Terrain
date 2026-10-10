#include "star_terrain/rendering/policies/ShadowDepthReleasePolicy.hpp"

namespace star::terrain::rendering
{
bool ShadowDepthOwnershipRelease::fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept
{
    out = vk::ImageMemoryBarrier2()
              .setImage(image)
              .setOldLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
              .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
              .setSrcStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                               vk::PipelineStageFlagBits2::eLateFragmentTests)
              .setSrcAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
              .setDstStageMask(vk::PipelineStageFlagBits2::eNone) // release
              .setDstAccessMask(vk::AccessFlagBits2::eNone)
              .setSrcQueueFamilyIndex(graphicsQueueFamilyIndex)
              .setDstQueueFamilyIndex(computeQueueFamilyIndex)
              .setSubresourceRange(vk::ImageSubresourceRange()
                                       .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                       .setBaseMipLevel(0)
                                       .setLevelCount(1)
                                       .setBaseArrayLayer(0)
                                       .setLayerCount(1));
    return true;
}

bool ShadowDepthSameQueueNoOp::fillBarrier(vk::Image, vk::ImageMemoryBarrier2 &) const noexcept
{
    return false;
}
} // namespace star::terrain::rendering