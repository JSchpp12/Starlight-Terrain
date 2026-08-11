#include "star_terrain/rendering/TerrainShadowRenderPhaseProvider.hpp"

#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>
#include <functional>
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
    std::shared_ptr<star::core::renderer::FrameData> frameData, star::Handle offscreenPhaseHandle,
    star::StarCamera *camera, bool enableShadowCasting)
    : m_frameData(std::move(frameData)), m_offscreenPhaseHandle(std::move(offscreenPhaseHandle)),
      m_camera(camera), m_enableShadowCasting(enableShadowCasting)
{
}

std::unique_ptr<star::core::renderer::RenderPhase> TerrainShadowRenderPhaseProvider::build(
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry &phases)
{
    const uint8_t numFramesInFlight = c.frameTracker().getSetup().getNumFramesInFlight();

    auto phase = std::make_unique<TerrainShadowRenderPhase>(m_enableShadowCasting);

    phase->m_device = c.getDevice().getVulkanDevice();
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_frameData = m_frameData;

    auto *offscreenPhase = phases.getPhase(m_offscreenPhaseHandle);
    assert(offscreenPhase != nullptr && "offscreen render phase must be built before the terrain shadow render phase");

    phase->computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();

    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    phase->m_commandBuffer = c.getManagerCommandBuffer().submit(
        star::core::device::manager::ManagerCommandBuffer::Request{
            .recordBufferCallback = std::bind(&TerrainShadowRenderPhase::recordCommandBuffer, phase.get(),
                                              std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            .order = star::Command_Buffer_Order::before_render_pass,
            .orderIndex = star::Command_Buffer_Order_Index::second,
            .type = star::Queue_Type::Tcompute,
            .waitStage = vk::PipelineStageFlagBits::eAllCommands,
            .willBeSubmittedEachFrame = true,
            .recordOnce = false,
            .overrideBufferSubmissionCallback = phase->getSubmissionOverride()},
        numFramesInFlight);

    auto cmd = star::command_order::DeclarePass(phase->m_commandBuffer, phase->computeQueueFamilyIndex);
    c.begin().set(cmd).submit();

    return phase;
}
} // namespace star::terrain
