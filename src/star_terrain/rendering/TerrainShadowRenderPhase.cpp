#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/command/command_order/TriggerPass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/renderer/RenderPhaseHelpers.hpp>

#include <cassert>
#include <vector>

namespace star::terrain
{
TerrainShadowRenderPhase::TerrainShadowRenderPhase(const star::core::CommandBus &cmdBus, vk::Device device,
                                                   bool enableShadowCasting)
    : m_shadowCastingEnabled(enableShadowCasting), m_cmdBus(&cmdBus), m_device(device)
{
}

void TerrainShadowRenderPhase::frameUpdate(star::common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);
    const size_t ii = static_cast<size_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

    // Self-trigger: signal this pass's timeline semaphore each frame so the
    // command-order service submits it. The shadow phase currently has no
    // consumers, so nothing else triggers it.
    context.getCmdBus().submit(star::command_order::TriggerPass()
                                   .setTimelineSemaphore(m_timelineSemaphores[ii])
                                   .setSignalValue(context.frameTracker().getCurrent().getNumTimesFrameProcessed() + 1)
                                   .setPass(m_commandBuffer));

    m_renderTargets.frameUpdate(context, m_renderingContext);
    updateDependentData(context);
    RenderPhase::frameUpdate(c);
}

void TerrainShadowRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                                   const star::common::FrameTracker &ft, const uint64_t &frameIndex)
{
    star::core::renderer::waitForTimelineSemaphore(*m_cmdBus, m_device, m_commandBuffer, ft);

    commandBuffer.begin(ft.getCurrent().getFrameInFlightIndex());
    recordCommands(commandBuffer.buffer(ft.getCurrent().getFrameInFlightIndex()), ft, frameIndex);
    commandBuffer.buffer(ft.getCurrent().getFrameInFlightIndex()).end();
}

void TerrainShadowRenderPhase::cleanupRender(star::common::IDeviceContext &context)
{
    RenderPhase::cleanupRender(context);

    auto &c = static_cast<star::core::device::DeviceContext &>(context);
    if (m_globalShaderInfo)
    {
        m_globalShaderInfo->cleanupRender(c.getDevice());
        m_globalShaderInfo.reset();
    }
}

void TerrainShadowRenderPhase::recordPreRenderPassCommands(vk::CommandBuffer &buffer,
                                                            const star::common::FrameTracker &frameTracker)
{
    // Preserve the base per-group pre-render-pass commands (the shadow relied on these before
    // this override was introduced).
    star::core::renderer::RenderPhase::recordPreRenderPassCommands(buffer, frameTracker);

    // The compute (volume) neighbor leaves the shadow depth in eShaderReadOnlyOptimal after
    // sampling it. Once it has run (after the first frames-in-flight grace period), reacquire /
    // transition the depth back to eDepthStencilAttachmentOptimal before beginRendering.
    if (!m_isFirstPass)
    {
        const size_t fi = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
        const vk::Image depthImage =
            m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[fi])->getVulkanImage();

        vk::ImageMemoryBarrier2 reacquireBarrier{};
        std::visit([&](auto &p) { p.fillBarrier(depthImage, reacquireBarrier); }, m_shadowDepthReacquirePolicy);

        buffer.pipelineBarrier2(vk::DependencyInfo()
                                     .setPImageMemoryBarriers(&reacquireBarrier)
                                     .setImageMemoryBarrierCount(1));
    }

    if (m_firstFramePassCounter > 0)
    {
        m_firstFramePassCounter--;
        if (m_firstFramePassCounter == 0)
            m_isFirstPass = false;
    }
}
void TerrainShadowRenderPhase::recordCommands(vk::CommandBuffer &commandBuffer,
                                              const star::common::FrameTracker &frameTracker,
                                              const uint64_t &frameIndex)
{
    vk::Viewport viewport = this->prepareRenderingViewport(m_renderingContext.targetResolution);
    commandBuffer.setViewport(0, viewport);

    {
        const vk::Rect2D scissor = this->prepareRenderingScissor(m_renderingContext.targetResolution);
        commandBuffer.setScissor(0, scissor);
    }

    recordPreRenderPassCommands(commandBuffer, frameTracker);
    recordCommandBufferDependencies(commandBuffer, frameTracker, frameIndex);

    {
        vk::RenderingAttachmentInfo colorAttachments;
        std::optional<vk::RenderingAttachmentInfo> depthAttachment;
        if (m_renderTargets.hasDepth())
            depthAttachment = prepareDynamicRenderingInfoDepthAttachment(frameTracker);

        auto renderArea = vk::Rect2D{vk::Offset2D{}, m_renderingContext.targetResolution};
        const auto renderInfo =
            vk::RenderingInfoKHR().setRenderArea(renderArea).setLayerCount(1).setPDepthAttachment(&*depthAttachment);
        commandBuffer.beginRendering(renderInfo);
    }

    recordRenderingCalls(commandBuffer, frameTracker.getCurrent().getFrameInFlightIndex(), frameIndex);

    commandBuffer.endRendering();

    {
        const size_t fi = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());
        const vk::Image depthImage =
            m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[fi])->getVulkanImage();

        vk::ImageMemoryBarrier2 releaseBarrier{};
        const bool emit = std::visit(
            [&](auto &p) { return p.fillBarrier(depthImage, releaseBarrier); }, m_shadowDepthReleasePolicy);

        if (emit)
        {
            commandBuffer.pipelineBarrier2(vk::DependencyInfo()
                                                .setPImageMemoryBarriers(&releaseBarrier)
                                                .setImageMemoryBarrierCount(1));
        }
    }

    recordPostRenderingCalls(commandBuffer, frameTracker);
}

void TerrainShadowRenderPhase::recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                                    const uint64_t &frameIndex)
{
    for (auto &group : m_renderGroups)
    {
        if (m_globalShaderInfo)
        {
            auto globalSets = m_globalShaderInfo->getDescriptors(frameInFlightIndex);
            if (!globalSets.empty())
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, group.getPipelineLayout(), 0,
                                                 globalSets.size(), globalSets.data(), 0, nullptr);
            }
        }

        group.recordRenderPassCommands(commandBuffer, frameInFlightIndex, frameIndex);
    }
}

void TerrainShadowRenderPhase::recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer,
                                                               const star::common::FrameTracker &frameTracker,
                                                               const uint64_t &frameIndex)
{
    if (m_barrFunction == nullptr)
        return;

    size_t barrCount{0};
    m_barrFunction(frameTracker, frameIndex, m_frameData.get(), &m_dataRoles, &m_renderingContext,
                   m_runtimeBarriers.data(), &barrCount);

    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo().setBufferMemoryBarrierCount(barrCount).setPBufferMemoryBarriers(m_runtimeBarriers.data()));
}

void TerrainShadowRenderPhase::AddOwnsCameraBarrier(const star::common::FrameTracker &frameTracker,
                                                    const uint64_t &frameIndex,
                                                    const star::core::renderer::FrameData *fd, const DataRoles *roles,
                                                    const star::core::renderer::RenderingContext *rc,
                                                    vk::BufferMemoryBarrier2 *data, size_t *dCount) noexcept
{
    const uint8_t frameInFlightIndex = frameTracker.getCurrent().getFrameInFlightIndex();

    const auto *camera = fd->getController(roles->camera);
    if (camera->willBeUpdatedThisFrame(frameIndex, frameTracker))
    {
        auto buffer = rc->bufferTransferRecords.get(camera->getHandle(frameInFlightIndex));

        *(data++) = vk::BufferMemoryBarrier2()
                        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
                        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
                        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader |
                                         vk::PipelineStageFlagBits2::eVertexShader)
                        .setDstAccessMask(vk::AccessFlagBits2::eUniformRead | vk::AccessFlagBits2::eShaderRead)
                        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                        .setBuffer(buffer)
                        .setSize(vk::WholeSize);
        (*dCount)++;
    }
}

TerrainShadowRenderPhase &TerrainShadowRenderPhase::setDataRolesOwned(star::Handle cameraRole)
{
    m_dataRoles = DataRoles{.camera = cameraRole};

    assert(m_frameData && "Frame data needs to be assigned first");
    assert(m_frameData->isResourceDriven(m_dataRoles.camera) && "owned camera role must be a driven buffer");
    m_drivesFrameData = true;
    m_barrFunction = &AddOwnsCameraBarrier;

    return *this;
}

vk::RenderingAttachmentInfo TerrainShadowRenderPhase::prepareDynamicRenderingInfoDepthAttachment(
    const star::common::FrameTracker &frameTracker)
{
    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo{};
    depthAttachmentInfo.imageView =
        m_renderingContext.recordDependentImage.get(m_renderTargets.depthHandles()[index])->getImageView();
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f}};

    return depthAttachmentInfo;
}

vk::Viewport TerrainShadowRenderPhase::prepareRenderingViewport(const vk::Extent2D &resolution)
{
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = resolution.width;
    viewport.height = resolution.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

vk::Rect2D TerrainShadowRenderPhase::prepareRenderingScissor(const vk::Extent2D &resolution)
{
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = resolution;
    return scissor;
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> TerrainShadowRenderPhase::
    getSubmissionOverride()
{
    return star::core::renderer::makeEdgeAwareSubmissionOverride(m_cmdBus, &m_commandBuffer, false);
}
} // namespace star::terrain
