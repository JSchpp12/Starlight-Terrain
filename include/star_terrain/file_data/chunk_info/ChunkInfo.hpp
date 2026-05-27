#pragma once

#include <glm/glm.hpp>
#include <string>

namespace star::terrain
{

struct ChunkInfo
{
    glm::dvec2 cornerNE, cornerSE, cornerSW, cornerNW, center;
    std::string textureFile;
};
} // namespace star::terrain