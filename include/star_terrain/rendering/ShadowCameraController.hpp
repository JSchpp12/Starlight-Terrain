#pragma once

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/ManagerController_RenderResource_Buffer.hpp>

namespace star::terrain::rendering
{
class ShadowCameraController : public star::ManagerController::RenderResource::Buffer
{
  public:
    ShadowCameraController(uint8_t numFramesInFlight, std::shared_ptr<std::vector<Light>> lights,
                           uint8_t mainLightIndex);
    virtual ~ShadowCameraController() = default;
    ShadowCameraController(const ShadowCameraController &) = default;
    ShadowCameraController &operator=(const ShadowCameraController &) = default;
    ShadowCameraController(ShadowCameraController &&) = default;
    ShadowCameraController &operator=(ShadowCameraController &&) = default;

  protected:
    virtual bool doesFrameInFlightDataNeedUpdated(uint8_t frameInFlightIndex) const override;
    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::DeviceContext &device,
                                                                           uint8_t frameInFlightIndex) override;

  private:
    std::shared_ptr<std::vector<Light>> m_lights;
    std::vector<glm::vec3> m_lastLightDirections;
    uint8_t m_mainLightIndex;
};
} // namespace star::terrain::rendering