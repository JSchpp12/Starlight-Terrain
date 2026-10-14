#pragma once

#include "star_terrain/rendering/TerrainChunkInfo.hpp"

#include <starlight/virtual/TransferRequest_Buffer.hpp>

#include <cassert>
#include <memory>
#include <vector>

namespace star::terrain::rendering
{
/// GPU transfer request for per-chunk records.
class ChunkRequest : public star::TransferRequest::Buffer
{
  public:
    ChunkRequest(uint8_t computeQueueIndex, std::vector<ChunkRecord> chunks)
        : m_computeQueueIndex(computeQueueIndex), m_chunks(std::move(chunks))
    {
        assert(!m_chunks.empty() && "ChunkRequest must be given at least one chunk");
    }

    virtual ~ChunkRequest() = default;

    size_t getNumChunks() const noexcept
    {
        return m_chunks.size();
    }

    const std::vector<ChunkRecord> &getChunks() const noexcept
    {
        return m_chunks;
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(
        star::core::device::StarDevice &device) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    uint8_t m_computeQueueIndex;
    std::vector<ChunkRecord> m_chunks;
};
} // namespace star::terrain::rendering
