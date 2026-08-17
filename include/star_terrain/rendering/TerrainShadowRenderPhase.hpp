#pragma once

#include <starlight/core/renderer/RenderPhase.hpp>
#include <starlight/core/renderer/RenderingContext.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/IDeviceContext.hpp>

#include <vulkan/vulkan.hpp>

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace star::terrain
{
class TerrainShadowRenderPhaseProvider;

class TerrainShadowRenderPhase : public star::core::renderer::RenderPhase
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
    virtual void cleanupRender(star::common::IDeviceContext &context) override;

    TerrainShadowRenderPhase &setDataRolesOwned(star::Handle cameraRole);

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
    struct DataRoles
    {
        star::Handle camera;
    };
    using OwningBarrierFunction = void (*)(const star::common::FrameTracker &, const uint64_t &,
                                           const star::core::renderer::FrameData *, const DataRoles *,
                                           const star::core::renderer::RenderingContext *, vk::BufferMemoryBarrier2 *,
                                           size_t *) noexcept;

    vk::Viewport prepareRenderingViewport(const vk::Extent2D &resolution);
    vk::Rect2D prepareRenderingScissor(const vk::Extent2D &resolution);
    vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const star::common::FrameTracker &frameTracker);

    virtual void recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                                const uint64_t &frameIndex);
    virtual void recordRenderingCalls(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                      const uint64_t &frameIndex) override;
    void recordCommandBufferDependencies(vk::CommandBuffer &commandBuffer,
                                         const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex);

    virtual std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
    getSubmissionOverride() override;

  private:
    std::array<vk::BufferMemoryBarrier2, 1> m_runtimeBarriers;
    std::unique_ptr<star::StarShaderInfo> m_globalShaderInfo;
    DataRoles m_dataRoles{};
    OwningBarrierFunction m_barrFunction{nullptr};

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

    static void AddOwnsCameraBarrier(const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex,
                                     const star::core::renderer::FrameData *fd, const DataRoles *roles,
                                     const star::core::renderer::RenderingContext *rc, vk::BufferMemoryBarrier2 *data,
                                     size_t *dCount) noexcept;
};
} // namespace star::terrain