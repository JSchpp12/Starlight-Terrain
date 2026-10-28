#pragma once

#include "star_terrain/rendering/TerrainChunkInfo.hpp"

#include <starlight/virtual/TransferRequest_Buffer.hpp>

#include <cassert>
#include <memory>
#include <vector>

namespace star::terrain::rendering
{
/// GPU transfer request for terrain corner data.
class CornerRequest : public star::TransferRequest::Buffer
{
  public:
    CornerRequest(uint8_t computeQueueIndex, std::vector<CornerInfo> corners)
        : m_computeQueueIndex(computeQueueIndex), m_corners(std::move(corners))
    {
        assert(!m_corners.empty() && "CornerRequest must be given at least one corner");
    }

    virtual ~CornerRequest() = default;

    size_t getNumCorners() const noexcept
    {
        return m_corners.size();
    }

    const std::vector<CornerInfo> &getCorners() const noexcept
    {
        return m_corners;
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(
        star::core::device::StarDevice &device) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    uint8_t m_computeQueueIndex;
    std::vector<CornerInfo> m_corners;
};
} // namespace star::terrain::rendering
