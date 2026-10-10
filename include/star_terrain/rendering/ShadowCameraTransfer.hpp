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
    struct CalculatorInfo
    {
        star::StarCamera mainRenderCamera;
        glm::vec3 lightDirection;
        std::array<uint32_t, 2> shadowMapResolution;
    };

    struct ShadowCameraInfo
    {
        glm::mat4 worldToLightViewProj;
        glm::mat4 invWorldToLightViewProj;
    };

    explicit ShadowCameraTransfer(CalculatorInfo calculationInfo);

    CalculatorInfo m_calculationInfo;

    ShadowCameraInfo getCameraInfo() const noexcept;

  public:
    class Builder
    {
        CalculatorInfo m_calcInfo;

      public:
        Builder() = default;
        Builder &setStarCamera(star::StarCamera camera)
        {
            m_calcInfo.mainRenderCamera = std::move(camera);
            return *this;
        }
        Builder &setLightDir(glm::vec3 lightDir)
        {
            m_calcInfo.lightDirection = std::move(lightDir);
            return *this;
        }
        Builder &setShadowMapResolution(std::array<uint32_t, 2> shadowMapResolution)
        {
            m_calcInfo.shadowMapResolution = std::move(shadowMapResolution);
            return *this;
        }
        ShadowCameraTransfer build()
        {
            return ShadowCameraTransfer(m_calcInfo);
        }
    };

    virtual ~ShadowCameraTransfer() = default;
    ShadowCameraTransfer(const ShadowCameraTransfer &) = default;
    ShadowCameraTransfer &operator=(const ShadowCameraTransfer &) = default;
    ShadowCameraTransfer(ShadowCameraTransfer &&) = default;
    ShadowCameraTransfer &operator=(ShadowCameraTransfer &&) = default;
    friend class Builder;

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(star::core::device::StarDevice &device) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;
    virtual void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;
};
} // namespace star::terrain::rendering