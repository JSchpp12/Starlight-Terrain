#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include "star_terrain/rendering/ShadowCasterInfo.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <math.h>
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

    return StarBuffers::Buffer::Builder(device.getAllocator().get())
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "ShadowCamera_TransferSRC")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createFinal(
    star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    constexpr vk::DeviceSize size = sizeof(ShadowCameraInfo);

    return StarBuffers::Buffer::Builder(device.getAllocator().get())
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "ShadowCamera")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
}

static void SnapCameraPositionToShadowTexel(star::StarCamera &workingCamera, const ShadowCasterInfo &calculator,
                                            const glm::mat4 &shadowLightProj,
                                            const glm::mat4 &invShadowLightProj) noexcept
{
    ShadowCasterInfo::FrustumCornerInfo frustumInfo = calculator.getMainCameraFrustumInfo();

    auto lightSpaceCenter = shadowLightProj * glm::vec4(frustumInfo.center, 1.0);

    const float texelSize = frustumInfo.viewSphereRadius * 2.0f;
    lightSpaceCenter.x = std::floor(lightSpaceCenter.x / texelSize) * texelSize;
    lightSpaceCenter.y = std::floor(lightSpaceCenter.y / texelSize) * texelSize;

    glm::vec3 snappedCenter = glm::vec3(invShadowLightProj * lightSpaceCenter);
    frustumInfo.center = snappedCenter;
    workingCamera.setPosition(frustumInfo.center);
}

ShadowCameraTransfer::ShadowCameraInfo ShadowCameraTransfer::getCameraInfo() const noexcept
{
    star::StarCamera workingCamera = star::StarCamera(m_calculationInfo.mainRenderCamera);
    ShadowCasterInfo calculator{workingCamera, m_calculationInfo.lightDirection};
    // auto shadowLightProj = calculator.getShadowLightProjection();
    // auto invShadowLightProj = glm::inverse(shadowLightProj);
    // SnapCameraPositionToShadowTexel(workingCamera, calculator, shadowLightProj, invShadowLightProj);

    // recalculate with snapped camera location
    //  shadowLightProj = calculator.getShadowLightProjection();
    const auto shadowLightProj =
        calculator.getShadowLightProjectionWithTexelSnapping(m_calculationInfo.shadowMapResolution);
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
