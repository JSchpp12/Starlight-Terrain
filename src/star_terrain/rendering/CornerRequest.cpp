#include "star_terrain/rendering/CornerRequest.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/maker/buffer/MakeBuffer.hpp>

namespace star::terrain::rendering
{
namespace
{
vk::DeviceSize CornerBufferSize(size_t numCorners)
{
    return static_cast<vk::DeviceSize>(numCorners) * sizeof(CornerInfo);
}
} // namespace

std::unique_ptr<star::StarBuffers::Buffer> CornerRequest::createStagingBuffer(
    star::core::device::StarDevice &device) const
{
    const auto size = CornerBufferSize(m_corners.size());

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(star::Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                             VMA_ALLOCATION_CREATE_MAPPED_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                                               "TerrainCorner_SRC")
                                .setInstanceCount(static_cast<uint32_t>(m_corners.size()))
                                .setInstanceSize(sizeof(CornerInfo))
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create staging buffer for terrain corners: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarBuffers::Buffer>(std::move(builtBuffer));
}

std::unique_ptr<star::StarBuffers::Buffer> CornerRequest::createFinal(
    star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    const auto size = CornerBufferSize(m_corners.size());

    uint32_t numIndices = 1;
    std::vector<uint32_t> indices = {m_computeQueueIndex};
    for (const auto &index : transferQueueFamilyIndex)
    {
        indices.emplace_back(index);
        numIndices++;
    }

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(star::Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eConcurrent)
                                                   .setQueueFamilyIndexCount(numIndices)
                                                   .setPQueueFamilyIndices(indices.data())
                                                   .setSize(size)
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferDst |
                                                             vk::BufferUsageFlagBits::eStorageBuffer),
                                               "TerrainCorner")
                                .setInstanceCount(static_cast<uint32_t>(m_corners.size()))
                                .setInstanceSize(sizeof(CornerInfo))
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create final buffer for terrain corners: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarBuffers::Buffer>(std::move(builtBuffer));
}

void CornerRequest::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    // The staging buffer only ever reads this data, but writeToBuffer takes a
    // non-const pointer. Casting away const here is safe.
    buffer.writeToBuffer(const_cast<CornerInfo *>(m_corners.data()), mapped, CornerBufferSize(m_corners.size()));

    buffer.flush();
    buffer.unmap();
}
} // namespace star::terrain::rendering
