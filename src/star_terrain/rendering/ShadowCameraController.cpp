#include "star_terrain/rendering/ShadowCameraController.hpp"

#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include <cassert>

namespace star::terrain::rendering
{
ShadowCameraController::ShadowCameraController(uint8_t numFramesInFlight, std::shared_ptr<std::vector<Light>> lights,
                                               uint8_t mainLightIndex)
    : m_lights(std::move(lights)), m_lastLightDirections(static_cast<size_t>(numFramesInFlight)),
      m_mainLightIndex(mainLightIndex)
{
}

bool ShadowCameraController::doesFrameInFlightDataNeedUpdated(uint8_t frameInFlightIndex) const
{
    assert(frameInFlightIndex < m_lastLightDirections.size() && "Not enough resources were created for this");

    const auto &currentDirection = m_lights->at(m_mainLightIndex).getDirection();
    if (m_lastLightDirections[frameInFlightIndex] != currentDirection)
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
    return std::make_unique<ShadowCameraTransfer>(dir);
}
} // namespace star::terrain::rendering
