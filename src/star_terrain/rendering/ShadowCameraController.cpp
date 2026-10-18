#include "star_terrain/rendering/ShadowCameraController.hpp"

#include "star_terrain/rendering/ShadowCameraTransfer.hpp"
#include "star_terrain/rendering/ShadowWindowBounds.hpp"

#include <starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp>
#include <starlight/common/controllers/ManagerController_RenderResource_InstanceModelInfo.hpp>

#include <cassert>

namespace star::terrain::rendering
{
ShadowCameraController::ShadowCameraController(
    uint8_t numFramesInFlight, std::shared_ptr<std::vector<Light>> lights, uint8_t mainLightIndex,
    std::array<uint32_t, 2> shadowMapResolution,
    const star::ManagerController::RenderResource::GlobalInfo *mainRenderCameraController,
    const star::ManagerController::RenderResource::InstanceModelInfo *instanceModelInfoController,
    ShadowWindowBoundsProvider *windowBoundsProvider, float shadowCasterPadding)
    : m_lastLightDirections(static_cast<size_t>(numFramesInFlight)), m_lights(std::move(lights)),
      m_shadowMapResolution(std::move(shadowMapResolution)), m_mainRenderCameraController(mainRenderCameraController),
      m_instanceModelInfoController(instanceModelInfoController), m_windowBoundsProvider(windowBoundsProvider),
      m_shadowCasterPadding(shadowCasterPadding), m_mainLightIndex(mainLightIndex)
{
}

bool ShadowCameraController::doesFrameInFlightDataNeedUpdated(const common::FrameTracker &frameTracker) const
{
    const size_t frameInFlightIndex = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    assert(frameInFlightIndex < m_lastLightDirections.size() && "Not enough resources were created for this");

    const auto &currentDirection = m_lights->at(m_mainLightIndex).getDirection();
    if (m_lastLightDirections[frameInFlightIndex] != currentDirection)
    {
        return true;
    }

    // When the window attaches to tracked bounds the camera no longer drives the fit; only the
    // light direction and the tracked bounds do.
    if (m_windowBoundsProvider != nullptr)
    {
        return m_windowBoundsProvider->willShadowWindowBoundsUpdate(frameTracker);
    }

    return m_mainRenderCameraController->willBeUpdatedThisFrame(frameTracker.getCurrent().getGlobalFrameCounter(),
                                                                frameTracker) ||
           m_instanceModelInfoController->willBeUpdatedThisFrame(frameTracker.getCurrent().getGlobalFrameCounter(),
                                                                 frameTracker);
}

std::unique_ptr<TransferRequest::Buffer> ShadowCameraController::createTransferRequest(
    core::device::DeviceContext &context, uint8_t frameInFlightIndex)
{
    const auto &dir = m_lights->at(m_mainLightIndex).getDirection();

    m_lastLightDirections[frameInFlightIndex] = dir;

    auto builder = ShadowCameraTransfer::Builder()
                       .setLightDir(dir)
                       .setStarCamera(*m_mainRenderCameraController->getCamera())
                       .setShadowMapResolution(m_shadowMapResolution)
                       .setCasterPadding(m_shadowCasterPadding);

    if (m_windowBoundsProvider != nullptr)
    {
        BoundsDefinition bounds;
        if (m_windowBoundsProvider->getShadowWindowBounds(bounds))
        {
            builder.setShadowWindowBounds(bounds);
            m_windowBoundsProvider->markShadowWindowBoundsSynced(frameInFlightIndex);
        }
    }

    return std::make_unique<ShadowCameraTransfer>(builder.build());
}
} // namespace star::terrain::rendering
