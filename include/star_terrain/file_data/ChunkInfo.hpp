#pragma once

#include <glm/glm.hpp>
#include <string>

namespace star::terrain
{

struct ChunkInfo
{
    glm::dvec2 cornerNE, cornerSE, cornerSW, cornerNW, center;
    std::string textureFile;
    std::string heightFile;

    ChunkInfo() = default;

    /// Construct from corner points (NE/SE/SW/NW) with an explicit center.
    ChunkInfo(glm::dvec2 ne, glm::dvec2 se, glm::dvec2 sw, glm::dvec2 nw, glm::dvec2 center,
              std::string textureFile, std::string heightFile = {})
        : cornerNE(ne), cornerSE(se), cornerSW(sw), cornerNW(nw), center(center),
          textureFile(std::move(textureFile)), heightFile(std::move(heightFile))
    {
    }

    /// Construct from an upper-left (NW) and lower-right (SE) bounding rectangle.
    /// Assumes .x = lat, .y = lon (matching the rest of the terrain library).
    ChunkInfo(const std::string &heightFile, const std::string &textureFile,
              const glm::vec2 &upperLeft, const glm::vec2 &lowerRight)
        : cornerNE{upperLeft.x, lowerRight.y},
          cornerSE{lowerRight.x, lowerRight.y},
          cornerSW{lowerRight.x, upperLeft.y},
          cornerNW{upperLeft.x, upperLeft.y},
          center{(upperLeft.x + lowerRight.x) * 0.5, (upperLeft.y + lowerRight.y) * 0.5},
          textureFile(textureFile),
          heightFile(heightFile)
    {
    }
};
} // namespace star::terrain