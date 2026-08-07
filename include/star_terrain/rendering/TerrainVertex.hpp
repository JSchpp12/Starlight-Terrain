#pragma once

#include <glm/glm.hpp>

namespace star::terrain::rendering
{

struct TerrainVertex
{
    glm::vec3 pos{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 0.0f};
    glm::vec2 texCoord{0.0f, 0.0f};
};
} // namespace star::terrain::rendering