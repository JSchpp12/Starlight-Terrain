#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <starlight/virtual/StarCamera.hpp>
#include <starlight/virtual/TransferRequest_Buffer.hpp>
#include <vector>

namespace star::terrain::rendering
{
class ShadowCameraTransfer : public star::TransferRequest::Buffer
{
  public:
    ShadowCameraTransfer(glm::vec3 lightDirection, star::StarCamera mainRenderCamera);
    virtual ~ShadowCameraTransfer() = default;
    ShadowCameraTransfer(const ShadowCameraTransfer &) = default;
    ShadowCameraTransfer &operator=(const ShadowCameraTransfer &) = default;
    ShadowCameraTransfer(ShadowCameraTransfer &&) = default;
    ShadowCameraTransfer &operator=(ShadowCameraTransfer &&) = default;

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                             VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;
    virtual void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  private:
    struct ShadowCameraInfo
    {
        glm::mat4 worldToLightViewProj;
        glm::mat4 worldToShadowMapProj;
    };

    star::StarCamera m_mainRenderCamera;
    glm::vec3 m_lightDirection;

    ShadowCameraInfo getCameraInfo() const noexcept;
};
} // namespace star::terrain::rendering