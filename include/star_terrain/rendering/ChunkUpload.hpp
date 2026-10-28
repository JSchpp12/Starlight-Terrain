#pragma once

#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"
#include "star_terrain/rendering/ChunkMeshDescription.hpp"

namespace star::core::device
{
class DeviceContext;
}

namespace star::terrain::rendering
{
ChunkMeshDescription uploadChunk(LoadedGeometry loaded, core::device::DeviceContext &context);
} // namespace star::terrain::rendering