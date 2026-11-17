#include "star_terrain/service/TerrainFileService.hpp"

#include <starlight/service/InitParameters.hpp>

#include <utility>

namespace star::terrain::service
{
TerrainFileService::TerrainFileService(std::string configuredTerrainDir)
    : m_configuredTerrainDir(std::move(configuredTerrainDir))
{
}

void TerrainFileService::setInitParameters(star::service::InitParameters &params)
{
    m_cmd = &params.commandBus;
}

void TerrainFileService::init()
{
}

void TerrainFileService::shutdown()
{
}
} // namespace star::terrain::service
