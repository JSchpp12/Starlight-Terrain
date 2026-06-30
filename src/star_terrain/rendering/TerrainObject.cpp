#include "star_terrain/rendering/TerrainObject.hpp"

#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"
#include "star_terrain/io/TerrainShapeInfoLoader.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <gdal_priv.h>
#include <tbb/tbb.h>

#include <cassert>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace star::terrain
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

TerrainObject::TerrainObject(star::core::device::DeviceContext &context, TerrainObjectDefinition def,
                             star::ShaderResolver &shaderResolver)
    : star::StarObject(loadMaterials(
          def.terrainDir, std::get<1>(ReadTerrainTextureInfo((def.terrainDir / "height_info.json").string())))),
      m_def(std::move(def))
{
    m_vertexShaderHandle = shaderResolver.resolve(star::Shader_Stage::vertex);
    m_fragmentShaderHandle = shaderResolver.resolve(star::Shader_Stage::fragment);
}

std::vector<star::StarMesh> TerrainObject::loadMeshes(star::core::device::DeviceContext &context)
{
    for (auto &material : m_meshMaterials)
    {
        static_cast<star::TextureMaterial *>(material.get())->preloadTexture(context);
    }

    const auto infoPath = getHeightInfoFilePath();
    auto [readResult, fileInfo] = ReadTerrainTextureInfo(infoPath.string());

    const auto terrainPath = std::filesystem::path(m_def.terrainDir);
    auto loadingShapeInfo = TerrainShapeInfoLoader::SubmitForRead(getShapeFilePath(), context.getCmdBus());

    std::vector<TerrainChunk> chunks;
    GDALAllRegister();

    CoverageInfo shapeInfo = loadingShapeInfo.get();
    glm::dvec3 worldCenter(shapeInfo.center.x, shapeInfo.center.y, 0);

    const auto fullHeightFilePath = terrainPath / std::filesystem::path(fileInfo.fullHeightFilePath);
    bool setWorldCenter = false;
    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        const auto chunkTexturePath = terrainPath / fileInfo.chunks[i].textureFile;
        if (!setWorldCenter)
        {
            setWorldCenter = true;
            worldCenter.z = TerrainChunk::GetHeightAtLocationFromGDAL(fullHeightFilePath.string(), shapeInfo.center.x,
                                                                      shapeInfo.center.y)
                                .value();
        }

        chunks.emplace_back(fullHeightFilePath.string(), chunkTexturePath.string(), fileInfo.chunks[i].cornerNE,
                            fileInfo.chunks[i].cornerSE, fileInfo.chunks[i].cornerSW, fileInfo.chunks[i].cornerNW,
                            worldCenter, fileInfo.chunks[i].center);
    }

    star::core::logging::info("Launching load tasks");

    // GDAL does not allow concurrent access to a single GDALDataset, so each
    // TBB worker opens its own handle to the same height file via
    // enumerable_thread_specific (one dataset per thread, lazily created and
    // destroyed on the thread that owns it).
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

    std::vector<star::StarMesh> terrainMeshes;
    terrainMeshes.reserve(chunks.size());

    assert(chunks.size() == m_meshMaterials.size() && "Every chunk should have its own material");

    for (size_t i = 0; i < chunks.size(); i++)
    {
        terrainMeshes.emplace_back(chunks[i].getMesh(context, m_meshMaterials[i]));
    }

    return terrainMeshes;
}

std::vector<std::shared_ptr<star::StarMaterial>> TerrainObject::loadMaterials(const std::filesystem::path &terrainDir,
                                                                              const TextureDataInfo &fileInfo)
{
    std::vector<std::shared_ptr<star::StarMaterial>> materials(fileInfo.chunks.size());

    for (size_t i = 0; i < fileInfo.chunks.size(); i++)
    {
        const std::filesystem::path *found = nullptr;
        auto files = star::file_helpers::FindFilesInDirectoryWithSameNameIgnoreFileType(terrainDir.string(),
                                                                                        fileInfo.chunks[i].textureFile);
        for (const auto &file : files)
        {
            if (file.extension() == ".ktx2")
                found = &file;
        }

        if (found == nullptr)
        {
            std::ostringstream oss;
            oss << "Failed to find matching texture for file: " << fileInfo.chunks[i].textureFile << std::endl
                << "Ensure terrains are prepared with compressed textures" << std::endl;
            STAR_THROW(oss.str());
        }

        materials[i] = std::make_shared<star::TextureMaterial>(found->string());
    }

    return materials;
}

} // namespace star::terrain