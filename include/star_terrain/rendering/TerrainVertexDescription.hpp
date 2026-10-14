#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace star::terrain::rendering
{
namespace geometry
{
struct FullTerrainVertex;
}

std::vector<vk::VertexInputBindingDescription> getVertexBindingDescription();

std::vector<vk::VertexInputAttributeDescription> getVertexInputAttributeDescription();

} // namespace star::terrain::rendering