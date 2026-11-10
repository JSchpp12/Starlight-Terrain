#pragma once

#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include <starlight/command/detail/create_object/ObjectLoader.hpp>
#include <starlight/material/IMaterialProvider.hpp>
#include <starlight/virtual/StarMesh.hpp>

#include <memory>
#include <vector>

namespace star::terrain
{
/// Create-time loader for TerrainObject, mirroring star::command::create_object::FromObjFileLoader.
/// Plug into the engine's command pipeline via CreateObject::Builder::setLoader(...).
class FromTerrainDirLoader : public star::command::create_object::ObjectLoader
{
  public:
    FromTerrainDirLoader(TerrainObjectDefinition def, std::vector<std::unique_ptr<star::IMaterialProvider>> materials);
    std::shared_ptr<star::StarObject> load(star::core::device::DeviceContext &context,
                                           star::ShaderResolver &shaderResolver);

  private:
    TerrainObjectDefinition m_def;
    std::vector<std::unique_ptr<star::IMaterialProvider>> m_materials;
};
} // namespace star::terrain
