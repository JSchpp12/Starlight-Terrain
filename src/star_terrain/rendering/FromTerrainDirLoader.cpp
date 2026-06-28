#include "star_terrain/rendering/FromTerrainDirLoader.hpp"

#include "star_terrain/rendering/TerrainObject.hpp"

namespace star::terrain
{

FromTerrainDirLoader::FromTerrainDirLoader(star::core::device::DeviceContext &context, TerrainObjectDefinition def)
    : m_context(context), m_def(std::move(def))
{
}

std::shared_ptr<star::StarObject> FromTerrainDirLoader::load()
{
    return std::make_shared<TerrainObject>(m_context, m_def);
}

} // namespace star::terrain