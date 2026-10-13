#pragma once

#include <glm/glm.hpp>
#include <star_common/Handle.hpp>

#include <cstdint>

namespace star::terrain::rendering
{

struct ChunkMeshDescription
{
    glm::vec3 bbMin;
    glm::vec3 bbMax;
    Handle vertBuffer;
    Handle indBuffer;
    uint32_t vertCount;
    uint32_t indCount;
};

} // namespace star::terrain::rendering
