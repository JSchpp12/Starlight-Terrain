#include "star_terrain/rendering/TerrainShadowRenderPhaseProvider.hpp"

#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <vulkan/vulkan.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <memory>
#include <vector>

namespace star::terrain
{
static std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                                  const star::common::FrameTracker &ft) noexcept
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        evtBus.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

TerrainShadowRenderPhaseProvider::TerrainShadowRenderPhaseProvider(
    star::core::device::DeviceContext &context, std::shared_ptr<std::vector<star::Light>> lights,
    std::shared_ptr<star::StarCamera> camera, std::vector<std::shared_ptr<star::StarObject>> objects,
    bool enableShadowCasting, star::Command_Buffer_Order_Index order)
    : DefaultRenderPhaseProvider(context, std::move(lights), std::move(camera), std::move(objects)),
      m_enableShadowCasting(enableShadowCasting)
{
    m_config.order = star::Command_Buffer_Order::before_render_pass;
    m_config.orderIndex = order;
    m_config.waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
}

star::core::renderer::RenderTargets TerrainShadowRenderPhaseProvider::createRenderTargets(
    core::device::DeviceContext &ctx, star::core::renderer::RenderingContext &renderingContext)
{
    auto targets = star::core::renderer::RenderTargets::forOffscreen(ctx, renderingContext);

    auto *graphicsQueue = core::helper::GetEngineDefaultQueue(ctx.getEventBus(), ctx.getGraphicsManagers().queueManager,
                                                              star::Queue_Type::Tpresent);
    assert(graphicsQueue != nullptr);

    for (const auto &th : targets.depthHandles())
    {
        const auto &tx = ctx.getGraphicsManagers().imageManager.get(th)->texture;

        auto oneTimeSetup = star::core::helper::BeginSingleTimeCommands(
            ctx.getDevice(), ctx.getEventBus(), ctx.getManagerCommandBuffer().m_manager, star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier2 barrier[1]{vk::ImageMemoryBarrier2()
                                               .setOldLayout(vk::ImageLayout::eUndefined)
                                               .setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                               .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                               .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                               .setImage(tx.getVulkanImage())
                                               .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                                               .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                                               .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                                                 vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                                               .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                                               .setSubresourceRange(vk::ImageSubresourceRange()
                                                                        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                                        .setBaseMipLevel(0)
                                                                        .setLevelCount(vk::RemainingMipLevels)
                                                                        .setBaseArrayLayer(0)
                                                                        .setLayerCount(vk::RemainingArrayLayers))};

        oneTimeSetup.buffer().pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barrier));
        star::core::helper::EndSingleTimeCommands(*graphicsQueue, std::move(oneTimeSetup));
    }

    return targets;
}

std::unique_ptr<star::core::renderer::RenderPhase> TerrainShadowRenderPhaseProvider::build(
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry & /*phases*/)
{
    auto phase = std::make_unique<TerrainShadowRenderPhase>(m_enableShadowCasting);

    buildCore(phase.get(), c);

    // Shadow-specific tail: timeline semaphores + cmd-bus/device handles used by
    // the phase's self-trigger (frameUpdate) and submission override.
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_device = c.getDevice().getVulkanDevice();
    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    return phase;
}
} // namespace star::terrain
