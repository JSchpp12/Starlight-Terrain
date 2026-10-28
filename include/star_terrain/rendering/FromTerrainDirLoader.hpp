#pragma once

#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include <starlight/command/detail/create_object/ObjectLoader.hpp>
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
    FromTerrainDirLoader(TerrainObjectDefinition def, std::vector<std::shared_ptr<star::StarMaterial>> materials,
                         std::vector<star::StarMesh> meshes);
    std::shared_ptr<star::StarObject> load(star::ShaderResolver &shaderResolver);

  private:
    TerrainObjectDefinition m_def;
    std::vector<std::shared_ptr<star::StarMaterial>> m_materials;
    std::vector<star::StarMesh> m_meshes;
};
} // namespace star::terrain
