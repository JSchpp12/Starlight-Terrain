#include "star_terrain/rendering/TerrainVertexDescription.hpp"

#include "star_terrain/rendering/TerrainVertex.hpp"

#include <cstddef> // offsetof

namespace star::terrain::rendering
{
std::vector<vk::VertexInputBindingDescription> getVertexBindingDescription()
{
    return std::vector<vk::VertexInputBindingDescription>{vk::VertexInputBindingDescription()
                                                              .setBinding(0)
                                                              .setStride(static_cast<uint32_t>(sizeof(TerrainVertex)))
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
            .setOffset(static_cast<uint32_t>(offsetof(TerrainVertex, pos))),
        vk::VertexInputAttributeDescription()
            .setBinding(0)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(static_cast<uint32_t>(offsetof(TerrainVertex, normal))),
        vk::VertexInputAttributeDescription()
            .setBinding(0)
            .setLocation(2)
            .setFormat(vk::Format::eR32G32Sfloat)
            .setOffset(static_cast<uint32_t>(offsetof(TerrainVertex, texCoord))),
    };
}
} // namespace star::terrain::rendering