#pragma once

#include <starlight/virtual/StarMaterial.hpp>

#include <memory>
#include <vector>

namespace star::terrain::texture_loading
{
/// Materials produced for every chunk of a terrain, in chunk order.
using TerrainTextureMaterials = std::vector<std::shared_ptr<star::StarMaterial>>;
} // namespace star::terrain::texture_loading
