#include "star_terrain/layout/TerrainGrid.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace star::terrain::layout
{

void TerrainGrid::add(const std::string &heightFile, const std::string &textureFile, const glm::vec2 &upperLeft,
                      const glm::vec2 &lowerRight)
{
    chunkInfos.emplace_back(heightFile, textureFile, upperLeft, lowerRight);
}

std::vector<TerrainChunk> TerrainGrid::getFinalizedChunks()
{
    std::set<float> xBins = createXBins(chunkInfos);
    std::set<float> yBins = createYBins(chunkInfos);

    auto grid = createGrid(xBins, yBins, chunkInfos);

    auto &centerOfGrid = grid[std::floor(grid.size() / 2)][std::floor(grid[0].size() / 2)].chunkInfo.value();
    float centerHeight = 0.0f;
    const glm::dvec3 centerOfTerrainGrid =
        glm::dvec3{centerOfGrid.center.x, centerOfGrid.center.y, centerHeight};

    std::vector<TerrainChunk> chunks;
    for (size_t i = 0; i < grid.size(); i++)
    {
        for (size_t j = 0; j < grid[i].size(); j++)
        {
            if (grid[i][j].chunkInfo.has_value())
            {
                auto info = grid[i][j].chunkInfo.value();
            }
        }
    }

    return chunks;
}

std::vector<std::vector<TerrainGrid::Space>> TerrainGrid::createGrid(const std::set<float> &binsX,
                                                                     const std::set<float> &binsY,
                                                                     const std::vector<ChunkInfo> &chunkInfo)
{
    std::vector<std::vector<Space>> grid =
        std::vector<std::vector<Space>>(binsY.size(), std::vector<Space>(binsX.size()));

    for (auto &info : chunkInfo)
    {
        size_t xCenter = getIndexOfValue(binsX, roundFloat(info.center.x));
        size_t yCenter = getIndexOfValue(binsY, roundFloat(info.center.y));

        assert(yCenter < grid.size() && xCenter < grid[0].size());
        assert(!grid[yCenter][xCenter].chunkInfo && "Something is already here");

        grid[yCenter][xCenter].chunkInfo = info;
    }

    return grid;
}

std::set<float> TerrainGrid::createXBins(const std::vector<ChunkInfo> &chunkInfo) const
{
    std::set<float> bins;
    for (auto &info : chunkInfo)
    {
        bins.insert(roundFloat(info.center.x));
    }
    return bins;
}

std::set<float> TerrainGrid::createYBins(const std::vector<ChunkInfo> &chunkInfo) const
{
    std::set<float> bins;
    for (auto &info : chunkInfo)
    {
        bins.insert(roundFloat(info.center.y));
    }
    return bins;
}

float TerrainGrid::roundFloat(const float &value)
{
    float mult = std::pow(10.0f, 2);
    return std::round(value * mult) / mult;
}

size_t TerrainGrid::getIndexOfValue(const std::set<float> &bin, const float &value)
{
    float roundedValue = roundFloat(value);

    auto it = bin.find(roundedValue);

    if (it == bin.end())
    {
        throw std::runtime_error("Value not found in set");
    }

    return std::distance(bin.begin(), it);
}
} // namespace star::terrain::layout