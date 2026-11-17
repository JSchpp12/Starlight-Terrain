#pragma once

#include <glm/glm.hpp>

#include <string>

namespace star::terrain
{
/// @brief Terrain-wide data shared by every target chunk in a library cart.
/// @details Stored once per terrain, never per chunk.
struct TerrainTargetInfo
{
    /// @brief The merged elevation raster every chunk is sampled from.
    std::string fullHeightFile;
    /// @brief The terrain's world center, used as the local origin for chunk geometry.
    glm::dvec3 offset;
};
} // namespace star::terrain
