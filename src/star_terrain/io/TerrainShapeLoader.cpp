#include "star_terrain/io/TerrainShapeLoader.hpp"

#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <tbb/tbb.h>

#include <cassert>
#include <exception>
#include <filesystem>

namespace star::terrain
{
template <ChunkLoadPolicy PolicyT>
TerrainShapeLoader<PolicyT>::TerrainShapeLoader(const TerrainTargetInfo *terrainInfo, std::span<const ChunkInfo> chunks,
                                                rendering::ElevationType elevationType)
    : m_terrainInfo(terrainInfo), m_chunks(chunks), m_elevationType(elevationType),
      m_datasetLoader(terrainInfo != nullptr ? std::filesystem::path{terrainInfo->fullHeightFile}
                                             : std::filesystem::path{})
{
    assert(m_terrainInfo != nullptr && "TerrainShapeLoader requires terrain target info");
}

template <ChunkLoadPolicy PolicyT>
std::vector<std::expected<typename PolicyT::Payload, ChunkLoadError>> TerrainShapeLoader<PolicyT>::operator()() const
{
    using Payload = typename PolicyT::Payload;

    // Payloads are default constructible, so every slot starts as an empty
    // chunk that gets overwritten by its worker.
    std::vector<std::expected<Payload, ChunkLoadError>> results(m_chunks.size());

    star::core::logging::info("Launching load tasks");
    tbb::parallel_for(tbb::blocked_range<size_t>(0, m_chunks.size()), [&](const tbb::blocked_range<size_t> &r) {
        auto dataset = m_datasetLoader();

        for (size_t i = r.begin(); i != r.end(); ++i)
        {
            if (!dataset)
            {
                results[i] =
                    std::unexpected(ChunkLoadError{ChunkLoadError::Cause::dataset_open_failed, dataset.error().detail});
                continue;
            }

            PolicyT policy{m_terrainInfo, &m_chunks[i], m_elevationType, dataset->get()};

            try
            {
                results[i] = policy();
            }
            catch (const std::exception &e)
            {
                results[i] = std::unexpected(ChunkLoadError{ChunkLoadError::Cause::chunk_load_failed, e.what()});
            }
        }
    });
    star::core::logging::info("Done");

    return results;
}

template class TerrainShapeLoader<LoadFullGeometry>;
} // namespace star::terrain