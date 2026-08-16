#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <starlight/virtual/TransferRequest_Buffer.hpp>
#include <vector>

namespace star::terrain::rendering
{
class ShadowCameraTransfer : public star::TransferRequest::Buffer
{
  public:
    ShadowCameraTransfer(glm::vec3 lightDirection);
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
        glm::mat4 viewProj;
    };
    glm::vec3 m_lightDirection;

    ShadowCameraInfo getCameraInfo() const noexcept;
};
} // namespace star::terrain::rendering