#include "star_terrain/rendering/ChunkUpload.hpp"

#include "ManagerRenderResource.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <cstdint>
#include <memory>
#include <utility>

namespace star::terrain::rendering
{
ChunkMeshDescription uploadChunk(LoadedGeometry loaded, core::device::DeviceContext &context)
{
    const auto &graphicsIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();

    // Capture the counts before handing the payloads off to the transfer requests.
    const uint32_t vertCount = static_cast<uint32_t>(loaded.verts.size());
    const uint32_t indCount = static_cast<uint32_t>(loaded.inds.size());

    star::Handle vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::VertInfo<geometry::FullTerrainVertex>>(
                                   graphicsIndex, std::move(loaded.verts)));

    star::Handle indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(),
        std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, std::move(loaded.inds)));

    return ChunkMeshDescription{loaded.bbMin, loaded.bbMax, vertBuffer, indBuffer, vertCount, indCount};
}
} // namespace star::terrain::rendering