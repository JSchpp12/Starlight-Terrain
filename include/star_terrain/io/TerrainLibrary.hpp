#pragma once

#include <string>

namespace star::terrain
{
/// @brief One terrain library: its folder and the shape descriptor inside it.
/// @details Produced lazily by the terrain file service; the shape file is relative to the root path.
struct TerrainLibrary
{
    /// @brief Resolved absolute path of the terrain folder.
    std::string terrainRootPath;
    /// @brief Shape descriptor filename relative to terrainRootPath ("Shape.json" or "<id>.json").
    std::string terrainShapeFile;
};
} // namespace star::terrain
