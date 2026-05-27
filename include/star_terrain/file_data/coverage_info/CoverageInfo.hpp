#pragma once

#include <glm/glm.hpp>

namespace star::terrain
{
/// Info describing the range of coverage when gathering terrain data from datasources
struct CoverageInfo
{
    glm::dvec2 center;
    int viewDistance;
};
} // namespace star::terrain