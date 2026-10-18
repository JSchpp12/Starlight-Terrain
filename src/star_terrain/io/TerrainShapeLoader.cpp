#include "star_terrain/io/TerrainShapeLoader.hpp"

#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <tbb/tbb.h>

#include <exception>
#include <utility>

namespace star::terrain
{
template <ChunkLoadPolicy PolicyT>
TerrainShapeLoader<PolicyT>::TerrainShapeLoader(std::filesystem::path heightFilePath, std::vector<ChunkInfo> chunks,
                                                const glm::dvec3 &worldCenter)
    : m_heightFilePath(std::move(heightFilePath)), m_chunks(std::move(chunks)), m_worldCenter(worldCenter),
      m_datasetLoader(m_heightFilePath)
{
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

            PolicyT policy{TargetChunkInfo{m_heightFilePath.string(), m_chunks[i].cornerNE, m_chunks[i].cornerSE,
                                           m_chunks[i].cornerSW, m_chunks[i].cornerNW, m_worldCenter,
                                           m_chunks[i].center},
                           dataset.value()};

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