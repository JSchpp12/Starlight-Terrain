#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/renderer/EdgeSubmission.hpp>

#include <cassert>
#include <functional>
#include <vector>

namespace star::terrain
{
TerrainShadowRenderPhase::TerrainShadowRenderPhase(bool enableShadowCasting)
    : m_shadowCastingEnabled(enableShadowCasting)
{
}

void TerrainShadowRenderPhase::frameUpdate(star::common::IDeviceContext &c)
{
    auto &context = static_cast<star::core::device::DeviceContext &>(c);
    const size_t ii = static_cast<size_t>(context.frameTracker().getCurrent().getFrameInFlightIndex());

    // Self-trigger: signal this pass's timeline
    // semaphore each frame so the command-order service submits it. The shadow
    // phase currently has no consumers, so nothing else triggers it.
    context.getCmdBus().submit(star::command_order::TriggerPass()
                                   .setTimelineSemaphore(m_timelineSemaphores[ii])
                                   .setSignalValue(context.frameTracker().getCurrent().getNumTimesFrameProcessed() + 1)
                                   .setPass(m_commandBuffer));

    this->DefaultRenderPhase::frameUpdate(c);
}

void TerrainShadowRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                                   const star::common::FrameTracker &ft, const uint64_t &frameIndex)
{
    waitForSemaphore(ft);
    this->DefaultRenderPhase::recordCommandBuffer(commandBuffer, ft, frameIndex);
}

void TerrainShadowRenderPhase::waitForSemaphore(const star::common::FrameTracker &ft) const
{
    uint64_t signalValue{0};
    vk::Semaphore semaphore{VK_NULL_HANDLE};
    {
        star::command_order::GetPassInfo get{m_commandBuffer};
        m_cmdBus->submit(get);
        signalValue = get.getReply().get().currentSignalValue;
        semaphore = get.getReply().get().signaledSemaphore;
    }

    const uint64_t frameCount = ft.getCurrent().getNumTimesFrameProcessed();
    if (frameCount == signalValue)
    {
        assert(m_device != VK_NULL_HANDLE);
        auto result =
            m_device.waitSemaphores(vk::SemaphoreWaitInfo().setValues(frameCount).setSemaphores(semaphore), UINT64_MAX);

        if (result != vk::Result::eSuccess)
            STAR_THROW("Failed to wait for terrain shadow timeline semaphores");
    }
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> TerrainShadowRenderPhase::
    getSubmissionOverride()
{
    star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride overrideFn =
        std::bind(&TerrainShadowRenderPhase::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
                  std::placeholders::_7);
    return overrideFn;
}

vk::Semaphore TerrainShadowRenderPhase::submitBuffer(star::StarCommandBuffer &buffer,
                                                     const star::common::FrameTracker &frameTracker,
                                                     std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                                     std::vector<vk::Semaphore> dataSemaphores,
                                                     std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                                     std::vector<std::optional<uint64_t>> previousSignaledValues,
                                                     star::StarQueue &queue)
{
    assert(m_cmdBus != nullptr);

    return star::core::renderer::submitEdgeAwarePass(*m_cmdBus, m_commandBuffer, buffer, frameTracker,
                                                     previousCommandBufferSemaphores, dataSemaphores, dataWaitPoints,
                                                     previousSignaledValues, queue,
                                                     /*signalBinaryCompletion=*/false);
}
} // namespace star::terrain
