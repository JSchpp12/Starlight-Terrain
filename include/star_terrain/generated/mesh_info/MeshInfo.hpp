#pragma once

#include <starlight/struct/Vertex.hpp>

#include <vertex>

namespace star::terrain
{
struct MeshInfo
{
	std::vector<Vertex> vertices; 
	std::vector<uint32_t> indices;
};
}