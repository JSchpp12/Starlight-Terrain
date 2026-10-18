#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include "star_terrain/rendering/ShadowCasterInfo.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <math.h>
#include <starlight/core/Exceptions.hpp>
#include <starlight/maker/buffer/MakeBuffer.hpp>
#include <starlight/virtual/StarCamera.hpp>

namespace star::terrain::rendering
{
ShadowCameraTransfer::ShadowCameraTransfer(ShadowCameraTransfer::CalculatorInfo calculationInfo)
    : m_calculationInfo(std::move(calculationInfo))
{
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createStagingBuffer(
    star::core::device::StarDevice &device) const
{
    constexpr vk::DeviceSize size = sizeof(ShadowCameraInfo);

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                                               "ShadowCamera_TransferSRC")
                                .setInstanceCount(1)
                                .setInstanceSize(size)
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create staging buffer for shadow camera: " + makeResult.error().detail);
    }

    return std::make_unique<StarBuffers::Buffer>(std::move(builtBuffer));
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createFinal(
    star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    constexpr vk::DeviceSize size = sizeof(ShadowCameraInfo);

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferDst |
                                                             vk::BufferUsageFlagBits::eUniformBuffer),
                                               "ShadowCamera")
                                .setInstanceCount(1)
                                .setInstanceSize(size)
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create final buffer for shadow camera: " + makeResult.error().detail);
    }

    return std::make_unique<StarBuffers::Buffer>(std::move(builtBuffer));
}

ShadowCameraTransfer::ShadowCameraInfo ShadowCameraTransfer::getCameraInfo() const noexcept
{
    star::StarCamera workingCamera = star::StarCamera(m_calculationInfo.mainRenderCamera);
    ShadowCasterInfo calculator{workingCamera, m_calculationInfo.lightDirection};
    const std::array<uint32_t, 3> fullResDef{m_calculationInfo.shadowMapResolution[0],
                                             m_calculationInfo.shadowMapResolution[1], 0};

    glm::mat4 shadowLightProj;
    if (m_calculationInfo.windowBounds.has_value())
    {
        // When tracked world bounds are present the window attaches to them: the 512-slice transmittance
        // depth then spans the tracked region instead of the ~38k-unit camera view sphere, keeping the
        // per-slice world distance small enough for the precompute to resolve the medium.
        shadowLightProj = calculator.getShadowLightProjectionForBounds(
            *m_calculationInfo.windowBounds,
            {.resolution = fullResDef, .casterPadding = m_calculationInfo.casterPadding});
    }
    else
    {
        // Otherwise the window follows the main camera's view sphere.
        shadowLightProj = calculator.getShadowLightProjectionForBounds(
            BoundsDefinition::Builder()
                .asCameraFrustumSphere(m_calculationInfo.mainRenderCamera, m_calculationInfo.lightDirection)
                .build(),
            {.resolution = fullResDef});
    }

    return ShadowCameraInfo{.worldToLightViewProj = shadowLightProj,
                            .invWorldToLightViewProj = glm::inverse(shadowLightProj)};
}

void ShadowCameraTransfer::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    auto camInfo = getCameraInfo();
    buffer.writeToBuffer(&camInfo, mapped, sizeof(ShadowCameraInfo));

    buffer.flush();
    buffer.unmap();
}

} // namespace star::terrain::rendering
