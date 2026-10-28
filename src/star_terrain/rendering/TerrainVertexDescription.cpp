#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include "star_terrain/rendering/geometry/FullTerrainVertex.hpp"

#include <cstddef> // offsetof

namespace star::terrain::rendering
{
std::vector<vk::VertexInputBindingDescription> getVertexBindingDescription()
{
    return std::vector<vk::VertexInputBindingDescription>{
        vk::VertexInputBindingDescription()
            .setBinding(0)
            .setStride(static_cast<uint32_t>(sizeof(geometry::FullTerrainVertex)))
            .setInputRate(vk::VertexInputRate::eVertex)};
}

std::vector<vk::VertexInputAttributeDescription> getVertexInputAttributeDescription()
{
    // Locations match the terrain shaders (terrain.vert/terrain.frag), which
    // declare only three vertex inputs:
    //   location 0 = inPosition (vec3)
    //   location 1 = inNormal    (vec3)
    //   location 2 = inTexCoord  (vec2)
    return std::vector<vk::VertexInputAttributeDescription>{
        vk::VertexInputAttributeDescription()
            .setBinding(0)
            .setLocation(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(static_cast<uint32_t>(offsetof(geometry::FullTerrainVertex, pos))),
        vk::VertexInputAttributeDescription()
            .setBinding(0)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(static_cast<uint32_t>(offsetof(geometry::FullTerrainVertex, normal))),
        vk::VertexInputAttributeDescription()
            .setBinding(0)
            .setLocation(2)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(static_cast<uint32_t>(offsetof(geometry::FullTerrainVertex, texCoord))),
    };
}
} // namespace star::terrain::rendering