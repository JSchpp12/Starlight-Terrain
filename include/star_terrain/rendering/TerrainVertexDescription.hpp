#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace star::terrain::rendering
{
struct TerrainVertex;

std::vector<vk::VertexInputBindingDescription> getVertexBindingDescription();

std::vector<vk::VertexInputAttributeDescription> getVertexInputAttributeDescription();

} // namespace star::terrain::rendering