#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include "star_terrain/rendering/ShadowCasterInfo.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <starlight/virtual/StarCamera.hpp>

namespace star::terrain::rendering
{
ShadowCameraTransfer::ShadowCameraTransfer(glm::vec3 lightDirection, star::StarCamera mainRenderCamera)
    : m_lightDirection(std::move(lightDirection)), m_mainRenderCamera(std::move(mainRenderCamera))
{
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createStagingBuffer(vk::Device &device,
                                                                               VmaAllocator &allocator) const
{
    constexpr vk::DeviceSize size = sizeof(ShadowCameraInfo);

    return StarBuffers::Buffer::Builder(allocator)
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
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    constexpr vk::DeviceSize size = sizeof(ShadowCameraInfo);

    return StarBuffers::Buffer::Builder(allocator)
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

ShadowCameraTransfer::ShadowCameraInfo ShadowCameraTransfer::getCameraInfo() const noexcept
{
    const ShadowCasterInfo calculator{m_mainRenderCamera, m_lightDirection};
    return ShadowCameraInfo{.worldToLightViewProj = calculator.getShadowLightProjection(),
                            .worldToShadowMapProj = glm::mat4()};
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
