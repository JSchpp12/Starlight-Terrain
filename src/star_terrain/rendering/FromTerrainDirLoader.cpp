#include "star_terrain/rendering/FromTerrainDirLoader.hpp"

#include "star_terrain/rendering/TerrainObject.hpp"

#include <starlight/ShaderResolver.hpp>

namespace star::terrain
{

FromTerrainDirLoader::FromTerrainDirLoader(TerrainObjectDefinition def,
                                           std::vector<std::unique_ptr<star::IMaterialProvider>> materials)
    : m_def(std::move(def)), m_materials(std::move(materials))
{
}

std::shared_ptr<star::StarObject> FromTerrainDirLoader::load(star::core::device::DeviceContext & /*context*/,
                                                             star::ShaderResolver &shaderResolver)
{
    const ColoringMode colorMode = m_def.colorMode;
    return std::make_shared<TerrainObject>(colorMode, std::move(m_materials),
                                           shaderResolver.resolve(star::Shader_Stage::vertex),
                                           shaderResolver.resolve(star::Shader_Stage::fragment), std::move(m_def));
}

} // namespace star::terrain
