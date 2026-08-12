#include "star_terrain/rendering/TerrainGeometryDefinition.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"
#include "star_terrain/io/TerrainShapeInfoLoader.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <gdal_priv.h>
#include <tbb/tbb.h>

#include <cassert>
#include <filesystem>
#include <memory>
#include <sstream>
#include <utility>

namespace star::terrain
{

namespace
{
/// Per-thread GDAL dataset holder. GDAL does not allow concurrent use of a
/// single GDALDataset from multiple threads, so each TBB worker opens its own
/// handle to the same height file. Closed in the destructor on the worker
/// thread that created it.
struct ThreadLocalDataset
{
    GDALDataset *ds{nullptr};

    explicit ThreadLocalDataset(const std::string &path)
    {
        ds = static_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly));
        if (!ds)
            STAR_THROW("Failed to open GDAL dataset");
    }

    ThreadLocalDataset(const ThreadLocalDataset &) = delete;
    ThreadLocalDataset &operator=(const ThreadLocalDataset &) = delete;

    ~ThreadLocalDataset()
    {
        if (ds)
            GDALClose(ds);
    }
};
} // namespace

TerrainGeometryDefinition::TerrainGeometryDefinition(std::vector<ChunkMeshDescription> meshDescriptions,
                                                     std::filesystem::path terrainDir, rendering::Type renderType)
    : meshDescriptions(std::move(meshDescriptions)), terrainDir(std::move(terrainDir)),
      renderType(std::move(renderType))
{
}

TerrainGeometryDefinition::Builder &TerrainGeometryDefinition::Builder::setTerrainDir(std::filesystem::path terrainDir)
{
    m_terrainDir = std::move(terrainDir);
    return *this;
}

TerrainGeometryDefinition::Builder &TerrainGeometryDefinition::Builder::setRenderType(rendering::Type renderType)
{
    m_renderType = std::move(renderType);
    return *this;
}

TerrainGeometryDefinition TerrainGeometryDefinition::Builder::build()
{
    if (m_terrainDir.empty())
        STAR_THROW("TerrainGeometryDefinition::Builder::build() requires a terrain directory");

    const auto infoPath = m_terrainDir / "height_info.json";
    auto [readResult, fileInfo] = ReadTerrainTextureInfo(infoPath.string());

    auto loadingShapeInfo = TerrainShapeInfoLoader::SubmitForRead(m_terrainDir / "Shape.json", m_context.getCmdBus());

    std::vector<TerrainChunk> chunks;
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

    bool setWorldCenter = false;
    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        if (!setWorldCenter)
        {
            setWorldCenter = true;
            worldCenter.z = TerrainChunk::GetHeightAtLocationFromGDAL(fullHeightFilePath.string(), shapeInfo.center.x,
                                                                      shapeInfo.center.y);
        }

        chunks.emplace_back(fullHeightFilePath.string(), fileInfo.chunks[i].cornerNE, fileInfo.chunks[i].cornerSE,
                            fileInfo.chunks[i].cornerSW, fileInfo.chunks[i].cornerNW, worldCenter,
                            fileInfo.chunks[i].center);
    }

    star::core::logging::info("Launching load tasks");

    tbb::enumerable_thread_specific<std::unique_ptr<ThreadLocalDataset>> tls;
    tbb::parallel_for(tbb::blocked_range<size_t>(0, chunks.size()), [&](const tbb::blocked_range<size_t> &r) {
        auto &local = tls.local();
        if (!local)
            local = std::make_unique<ThreadLocalDataset>(fullHeightFilePath.string());

        for (size_t i = r.begin(); i != r.end(); ++i)
        {
            chunks[i].load(local->ds);
        }
    });
    tls.clear();
    star::core::logging::info("Done");

    std::vector<ChunkMeshDescription> meshDescriptions;
    meshDescriptions.reserve(chunks.size());
    for (auto &chunk : chunks)
    {
        meshDescriptions.emplace_back(chunk.createMeshDescription(m_context));
    }

    return TerrainGeometryDefinition{std::move(meshDescriptions), std::move(m_terrainDir), m_renderType};
}
} // namespace star::terrain
