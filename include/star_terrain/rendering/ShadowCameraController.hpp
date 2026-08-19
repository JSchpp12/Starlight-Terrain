#pragma once

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/ManagerController_RenderResource_Buffer.hpp>

namespace star::ManagerController::RenderResource
{
class GlobalInfo;
class InstanceModelInfo;
} // namespace star::ManagerController::RenderResource

namespace star::terrain::rendering
{
class ShadowCameraController : public star::ManagerController::RenderResource::Buffer
{
  public:
    ShadowCameraController(
        uint8_t numFramesInFlight, std::shared_ptr<std::vector<Light>> lights, uint8_t mainLightIndex,
        const star::ManagerController::RenderResource::GlobalInfo *mainRenderCameraController,
        const star::ManagerController::RenderResource::InstanceModelInfo *instanceModelInfoController);
    virtual ~ShadowCameraController() = default;
    ShadowCameraController(const ShadowCameraController &) = default;
    ShadowCameraController &operator=(const ShadowCameraController &) = default;
    ShadowCameraController(ShadowCameraController &&) = default;
    ShadowCameraController &operator=(ShadowCameraController &&) = default;

  protected:
    virtual bool doesFrameInFlightDataNeedUpdated(const common::FrameTracker &frameTracker) const override;
    virtual std::unique_ptr<TransferRequest::Buffer> createTransferRequest(core::device::DeviceContext &device,
                                                                           uint8_t frameInFlightIndex) override;

  private:
    std::vector<glm::vec3> m_lastLightDirections;
    std::shared_ptr<std::vector<Light>> m_lights;
    uint8_t m_mainLightIndex;
    const star::ManagerController::RenderResource::GlobalInfo *m_mainRenderCameraController{nullptr};
    const star::ManagerController::RenderResource::InstanceModelInfo *m_instanceModelInfoController{nullptr};
};
} // namespace star::terrain::rendering