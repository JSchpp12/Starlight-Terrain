#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/core/Exceptions.hpp>

#include <cassert>
#include <functional>
#include <optional>

namespace star::terrain
{
TerrainShadowRenderPhase::TerrainShadowRenderPhase(bool enableShadowCasting) : m_shadowCastingEnabled(enableShadowCasting)
{
}

void TerrainShadowRenderPhase::frameUpdate(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);

    if (isRenderReady(c))
    {
        // TODO: update terrain shadow-dependent data for the current frame.
    }
}

bool TerrainShadowRenderPhase::isRenderReady(star::core::device::DeviceContext &context)
{
    if (isReady)
    {
        return true;
    }

    // TODO: gate on the terrain shadow pipelines / resources becoming ready.
    isReady = true;

    return isReady;
}

void TerrainShadowRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                                   const star::common::FrameTracker &frameTracker,
                                                   const uint64_t &frameIndex)
{
    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);
}

void TerrainShadowRenderPhase::recordCommands(vk::CommandBuffer &commandBuffer,
                                              const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    // TODO: record the terrain shadow compute commands for this frame.
}

void TerrainShadowRenderPhase::cleanupRender(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);

    // TODO: release terrain shadow resources (buffers, pipelines, descriptor layouts).
    (void)c;
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> TerrainShadowRenderPhase::
    getSubmissionOverride()
{
    return std::nullopt;
}
} // namespace star::terrain
