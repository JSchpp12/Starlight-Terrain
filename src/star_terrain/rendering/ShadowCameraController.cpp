#include "star_terrain/rendering/ShadowCameraController.hpp"

#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include <cassert>
#include <starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp>

namespace star::terrain::rendering
{
ShadowCameraController::ShadowCameraController(
    uint8_t numFramesInFlight, std::shared_ptr<std::vector<Light>> lights, uint8_t mainLightIndex,
    const star::ManagerController::RenderResource::GlobalInfo *mainRenderCameraController)
    : m_lights(std::move(lights)), m_lastLightDirections(static_cast<size_t>(numFramesInFlight)),
      m_mainLightIndex(mainLightIndex), m_mainRenderCameraController(mainRenderCameraController)
{
}

bool ShadowCameraController::doesFrameInFlightDataNeedUpdated(const common::FrameTracker &frameTracker) const
{
    const size_t frameInFlightIndex = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    assert(frameInFlightIndex < m_lastLightDirections.size() && "Not enough resources were created for this");

    const auto &currentDirection = m_lights->at(m_mainLightIndex).getDirection();
    if (m_lastLightDirections[frameInFlightIndex] != currentDirection ||
        m_mainRenderCameraController->willBeUpdatedThisFrame(frameTracker.getCurrent().getGlobalFrameCounter(),
                                                             frameTracker))
    {
        return true;
    }

    return false;
}

std::unique_ptr<TransferRequest::Buffer> ShadowCameraController::createTransferRequest(
    core::device::DeviceContext &context, uint8_t frameInFlightIndex)
{
    const auto &dir = m_lights->at(m_mainLightIndex).getDirection();

    m_lastLightDirections[frameInFlightIndex] = dir;
    return std::make_unique<ShadowCameraTransfer>(dir, *m_mainRenderCameraController->getCamera());
}
} // namespace star::terrain::rendering
