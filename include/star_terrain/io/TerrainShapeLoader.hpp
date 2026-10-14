#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/io/GDALDatasetLoader.hpp"
#include "star_terrain/io/TargetChunkInfo.hpp"

#include <glm/glm.hpp>

#include <concepts>
#include <expected>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace star::terrain
{
/// Error produced when a terrain chunk's shape cannot be loaded.
struct ChunkLoadError
{
    enum class Cause
    {
        dataset_open_failed,
        chunk_load_failed
    };

    Cause cause;
    std::string detail;
};

/// A chunk shape loading policy.
template <typename PolicyT>
concept ChunkLoadPolicy = requires(TargetChunkInfo chunkInfo, GDALDataset *dataset) {
    typename PolicyT::Payload;
    PolicyT{chunkInfo, dataset};
    { std::declval<const PolicyT &>()() } -> std::same_as<typename PolicyT::Payload>;
};

/// Orchestrates loading the shape of every chunk in a terrain.
template <ChunkLoadPolicy PolicyT> class TerrainShapeLoader
{
  public:
    TerrainShapeLoader(std::filesystem::path heightFilePath, std::vector<ChunkInfo> chunks,
                       const glm::dvec3 &worldCenter);

    /// Load every chunk in parallel, preserving input order in the result.
    /// Each entry holds either the chunk's loaded payload or the error that
    /// prevented it from being loaded.
    std::vector<std::expected<typename PolicyT::Payload, ChunkLoadError>> operator()() const;

  private:
    std::filesystem::path m_heightFilePath;
    std::vector<ChunkInfo> m_chunks;
    glm::dvec3 m_worldCenter;
    GDALDatasetLoader m_datasetLoader;
};
} // namespace star::terrain