#include "star_terrain/rendering/FromTerrainDirLoader.hpp"

#include "star_terrain/rendering/TerrainObject.hpp"

#include <starlight/ShaderResolver.hpp>

namespace star::terrain
{

FromTerrainDirLoader::FromTerrainDirLoader(TerrainObjectDefinition def,
                                           std::vector<std::shared_ptr<star::StarMaterial>> materials,
                                           std::vector<star::StarMesh> meshes)
    : m_def(std::move(def)), m_materials(std::move(materials)), m_meshes(std::move(meshes))
{
}

std::shared_ptr<star::StarObject> FromTerrainDirLoader::load(star::ShaderResolver &shaderResolver)
{
    return std::make_shared<TerrainObject>(m_def.colorMode, std::move(m_materials),
                                           shaderResolver.resolve(star::Shader_Stage::vertex),
                                           shaderResolver.resolve(star::Shader_Stage::fragment), std::move(m_meshes));
}

} // namespace star::terrain
