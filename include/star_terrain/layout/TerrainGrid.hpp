#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"

#include <glm/glm.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace star::terrain::layout
{
/// Arranges terrain chunks into a 2D grid based on their center coordinates.
/// Chunks are added with their height/texture file paths and bounding rectangle
/// (upper-left / lower-right), then finalized into TerrainChunk objects.
class TerrainGrid
{
  public:
    void add(const std::string &heightFile, const std::string &textureFile, const glm::vec2 &upperLeft,
             const glm::vec2 &lowerRight);

    std::vector<TerrainChunk> getFinalizedChunks();

  private:
    enum class Direction
    {
        north = 0,
        north_east = 1,
        east = 2,
        south_east = 3,
        south = 4,
        south_west = 5,
        west = 6,
        north_west = 7
    };

    struct Space
    {
        Space() = default;
        std::optional<ChunkInfo> chunkInfo = std::nullopt;
    };

    std::vector<ChunkInfo> chunkInfos;

    std::set<float> createXBins(const std::vector<ChunkInfo> &chunkInfos) const;
    std::set<float> createYBins(const std::vector<ChunkInfo> &chunkInfos) const;

    static std::vector<std::vector<Space>> createGrid(const std::set<float> &binsX, const std::set<float> &binsY,
                                                      const std::vector<ChunkInfo> &chunkInfos);

    static float roundFloat(const float &value);

    static size_t getIndexOfValue(const std::set<float> &bin, const float &value);
};
} // namespace star::terrain::layout