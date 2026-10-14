#pragma once

#include <glm/glm.hpp>

#include <string>

namespace star::terrain
{
/// Everything needed to identify and locate a single chunk of terrain.
struct TargetChunkInfo
{
    std::string fullHeightFile;
    glm::dvec2 northEast;
    glm::dvec2 southEast;
    glm::dvec2 southWest;
    glm::dvec2 northWest;
    glm::dvec3 offset;
    glm::dvec2 center;
};
} // namespace star::terrain