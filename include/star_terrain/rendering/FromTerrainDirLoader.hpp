#pragma once

#include "star_terrain/rendering/TerrainObjectDefinition.hpp"

#include <starlight/command/detail/create_object/ObjectLoader.hpp>
#include <starlight/core/device/DeviceContext.hpp>

#include <memory>

namespace star::terrain
{
/// Create-time loader for TerrainObject, mirroring star::command::create_object::FromObjFileLoader.
/// Plug into the engine's command pipeline via CreateObject::Builder::setLoader(...).
class FromTerrainDirLoader : public star::command::create_object::ObjectLoader
{
  public:
    FromTerrainDirLoader(star::core::device::DeviceContext &context, TerrainObjectDefinition def);
    std::shared_ptr<star::StarObject> load(star::ShaderResolver &shaderResolver);

  private:
    star::core::device::DeviceContext &m_context;
    TerrainObjectDefinition m_def;
};
} // namespace star::terrain