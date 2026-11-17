#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/io/GDALDatasetLoader.hpp"
#include "star_terrain/io/TerrainTargetInfo.hpp"
#include "star_terrain/rendering/TerrainRenderingType.hpp"

#include <concepts>
#include <expected>
#include <span>
#include <string>
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
concept ChunkLoadPolicy = requires(const TerrainTargetInfo *terrainInfo, const ChunkInfo *chunkInfo,
                                   rendering::ElevationType elevationType, GDALDataset *dataset) {
    typename PolicyT::Payload;
    PolicyT{terrainInfo, chunkInfo, elevationType, dataset};
    { std::declval<const PolicyT &>()() } -> std::same_as<typename PolicyT::Payload>;
};

/// Orchestrates loading the shape of every target chunk in a terrain.
/// @details The terrain info and target list are borrowed, not owned: they must outlive the loader.
template <ChunkLoadPolicy PolicyT> class TerrainShapeLoader
{
  public:
    TerrainShapeLoader(const TerrainTargetInfo *terrainInfo, std::span<const ChunkInfo> chunks,
                       rendering::ElevationType elevationType);

    /// Load every chunk in parallel, preserving input order in the result.
    /// Each entry holds either the chunk's loaded payload or the error that
    /// prevented it from being loaded.
    std::vector<std::expected<typename PolicyT::Payload, ChunkLoadError>> operator()() const;

  private:
    const TerrainTargetInfo *m_terrainInfo{nullptr};
    std::span<const ChunkInfo> m_chunks;
    rendering::ElevationType m_elevationType;
    GDALDatasetLoader m_datasetLoader;
};
} // namespace star::terrain