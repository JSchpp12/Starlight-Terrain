#pragma once

#include <starlight/core/renderer/DefaultRenderPhase.hpp>
#include <starlight/core/renderer/RenderingContext.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/IDeviceContext.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace star::terrain
{
class TerrainShadowRenderPhaseProvider;

class TerrainShadowRenderPhase : public star::core::renderer::DefaultRenderPhase
{
  public:
    friend class TerrainShadowRenderPhaseProvider;

    explicit TerrainShadowRenderPhase(bool enableShadowCasting);
    virtual ~TerrainShadowRenderPhase() = default;

    TerrainShadowRenderPhase(const TerrainShadowRenderPhase &) = delete;
    TerrainShadowRenderPhase &operator=(const TerrainShadowRenderPhase &) = delete;
    TerrainShadowRenderPhase(TerrainShadowRenderPhase &&) = delete;
    TerrainShadowRenderPhase &operator=(TerrainShadowRenderPhase &&) = delete;

    virtual void frameUpdate(star::common::IDeviceContext &context) override;
    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                     const star::common::FrameTracker &frameTracker,
                                     const uint64_t &frameIndex) override;

    const std::vector<star::Handle> &getTimelineSemaphores() const
    {
        return m_timelineSemaphores;
    }

    void setShadowCastingEnabled(bool value)
    {
        m_shadowCastingEnabled = value;
    }
    bool isShadowCastingEnabled() const
    {
        return m_shadowCastingEnabled;
    }

  protected:
    virtual std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
    getSubmissionOverride() override;

  private:
    std::vector<star::Handle> m_timelineSemaphores;
    bool m_shadowCastingEnabled = false;
    star::core::CommandBus *m_cmdBus{nullptr};
    vk::Device m_device{VK_NULL_HANDLE};

    void waitForSemaphore(const star::common::FrameTracker &ft) const;

    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue);
};
} // namespace star::terrain
