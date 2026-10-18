#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"
#include "star_terrain/io/GDALDatasetLoader.hpp"
#include "star_terrain/io/TerrainShapeInfoLoader.hpp"
#include "star_terrain/io/TerrainShapeLoader.hpp"
#include "star_terrain/rendering/ChunkUpload.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <gdal_priv.h>

#include <exception>
#include <filesystem>
#include <sstream>
#include <utility>

namespace star::terrain::rendering
{
/// Load every chunk as full geometry on worker threads, then upload the payloads to GPU buffers on the main thread.
static std::vector<ChunkMeshDescription> buildGeometryMeshDescriptions(core::device::DeviceContext &context,
                                                                       const std::filesystem::path &heightFilePath,
                                                                       std::vector<ChunkInfo> chunks,
                                                                       const glm::dvec3 &worldCenter)
{
    TerrainShapeLoader<LoadFullGeometry> loader(heightFilePath, std::move(chunks), worldCenter);
    auto loadedChunks = loader();

    std::vector<ChunkMeshDescription> meshDescriptions;
    meshDescriptions.reserve(loadedChunks.size());
    for (auto &loadedChunk : loadedChunks)
    {
        if (!loadedChunk)
        {
            star::core::logging::warning("Skipping terrain chunk that failed to load: ", loadedChunk.error().detail);
            continue;
        }

        try
        {
            meshDescriptions.emplace_back(uploadChunk(std::move(*loadedChunk), context));
        }
        catch (const std::exception &e)
        {
            star::core::logging::warning("Skipping terrain chunk that failed to upload: ", e.what());
        }
    }

    return meshDescriptions;
}

TerrainGeometryDefinition::TerrainGeometryDefinition(std::vector<ChunkMeshDescription> meshDescriptions,
                                                     std::filesystem::path terrainDir,
                                                     Representation shapeRepresentation, Type defaultRenderType)
    : meshDescriptions(std::move(meshDescriptions)), terrainDir(std::move(terrainDir)),
      shapeRepresentation(shapeRepresentation), defaultRenderType(std::move(defaultRenderType))
{
}

TerrainGeometryDefinition::Builder &TerrainGeometryDefinition::Builder::setTerrainDir(std::filesystem::path terrainDir)
{
    m_terrainDir = std::move(terrainDir);
    return *this;
}

TerrainGeometryDefinition::Builder &TerrainGeometryDefinition::Builder::setShapeRepresentation(
    Representation shapeRepresentation)
{
    m_shapeRepresentation = shapeRepresentation;
    return *this;
}

TerrainGeometryDefinition::Builder &TerrainGeometryDefinition::Builder::setDefaultRenderType(Type defaultRenderType)
{
    m_defaultRenderType = std::move(defaultRenderType);
    return *this;
}

TerrainGeometryDefinition TerrainGeometryDefinition::Builder::build()
{
    if (m_terrainDir.empty())
        STAR_THROW("TerrainGeometryDefinition::Builder::build() requires a terrain directory");

    const auto infoPath = m_terrainDir / "height_info.json";
    auto [readResult, fileInfo] = ReadTerrainTextureInfo(infoPath.string());

    auto loadingShapeInfo = TerrainShapeInfoLoader::SubmitForRead(m_terrainDir / "Shape.json", m_context.getCmdBus());

    GDALAllRegister();

    CoverageInfo shapeInfo = loadingShapeInfo.get();
    glm::dvec3 worldCenter(shapeInfo.center.x, shapeInfo.center.y, 0);

    const auto fullHeightFilePath = m_terrainDir / std::filesystem::path(fileInfo.fullHeightFilePath);

    if (!std::filesystem::exists(fullHeightFilePath))
    {
        std::ostringstream oss;
        oss << "Elevation file does not exist: " << fullHeightFilePath.string()
            << ". The terrain directory is expected to contain the height raster named in "
            << "height_info.json (fullHeightFilePath = '" << fileInfo.fullHeightFilePath << "').";
        STAR_THROW(oss.str());
    }

    if (!fileInfo.chunks.empty())
    {
        worldCenter.z =
            GetHeightAtLocationFromGDAL(fullHeightFilePath.string(), shapeInfo.center.x, shapeInfo.center.y);
    }

    // The shape representation is a per-terrain choice: dispatch to the
    // concrete loading path once here so everything below it is monomorphic.
    switch (m_shapeRepresentation)
    {
    case Representation::Geometry:
        return TerrainGeometryDefinition{
            buildGeometryMeshDescriptions(m_context, fullHeightFilePath, std::move(fileInfo.chunks), worldCenter),
            std::move(m_terrainDir), m_shapeRepresentation, m_defaultRenderType};
    case Representation::Height_Map:
        STAR_THROW("The height map terrain shape representation is not implemented yet");
    }

    STAR_THROW("Unknown terrain shape representation");
}
} // namespace star::terrain::rendering