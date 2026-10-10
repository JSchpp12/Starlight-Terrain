#include "star_terrain/rendering/policies/ShadowDepthReacquirePolicy.hpp"

namespace star::terrain::rendering
{
bool ShadowDepthOwnershipReacquire::fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept
{
    out = vk::ImageMemoryBarrier2()
              .setImage(image)
              .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
              .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
              .setSrcStageMask(vk::PipelineStageFlagBits2::eNone) // acquire
              .setSrcAccessMask(vk::AccessFlagBits2::eNone)
              .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                               vk::PipelineStageFlagBits2::eLateFragmentTests)
              .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
              .setSrcQueueFamilyIndex(computeQueueFamilyIndex)
              .setDstQueueFamilyIndex(graphicsQueueFamilyIndex)
              .setSubresourceRange(vk::ImageSubresourceRange()
                                       .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                       .setBaseMipLevel(0)
                                       .setLevelCount(1)
                                       .setBaseArrayLayer(0)
                                       .setLayerCount(1));
    return true;
}

bool ShadowDepthSameQueueTransitionBack::fillBarrier(vk::Image image, vk::ImageMemoryBarrier2 &out) const noexcept
{
    out = vk::ImageMemoryBarrier2()
              .setImage(image)
              .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
              .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
              .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader) // prior compute read
              .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
              .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                               vk::PipelineStageFlagBits2::eLateFragmentTests)
              .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
              .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
              .setSubresourceRange(vk::ImageSubresourceRange()
                                       .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                       .setBaseMipLevel(0)
                                       .setLevelCount(1)
                                       .setBaseArrayLayer(0)
                                       .setLayerCount(1));
    return true;
}
} // namespace star::terrain::rendering