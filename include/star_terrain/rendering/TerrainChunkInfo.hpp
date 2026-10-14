#pragma once

#include <glm/glm.hpp>

namespace star::terrain::rendering
{
/// GPU-facing data for a single corner of the terrain.
struct CornerInfo
{
    glm::vec4 ground;
    glm::vec4 geoUpDir;
};

/// GPU-facing record for a single terrain chunk.
struct ChunkRecord
{
    glm::ivec4 corners;
    glm::ivec2 grid;
};
} // namespace star::terrain::rendering
