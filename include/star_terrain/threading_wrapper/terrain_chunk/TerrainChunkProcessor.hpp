#pragma once

#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"
#include <starlight/core/Exceptions.hpp>
#include <gdal_priv.h>
#include <tbb/tbb.h>

namespace star::terrain::processor::terrain_chunk
{
struct ThreadLocalDataset
{
    GDALDataset *ds{nullptr};
    GDALRasterBand *band{nullptr};

    ThreadLocalDataset(const std::string &path)
    {
        ds = static_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly));
        if (!ds)
            STAR_THROW("Failed to open GDAL dataset");

        band = ds->GetRasterBand(1);
    }

    ~ThreadLocalDataset()
    {
        if (ds)
            GDALClose(ds);
    }
};

struct TerrainChunkProcessor
{
    TerrainChunkProcessor(TerrainChunk chunks[]) : chunks(chunks) {};
    void operator()(const tbb::blocked_range<size_t> &r) const
    {
        thread_local std::unique_ptr<ThreadLocalDataset> dataset;

        TerrainChunk *localChunks = this->chunks;
        if (!dataset)
        {
            dataset = std::make_unique<ThreadLocalDataset>(localChunks[0].getHeightFile());

            if (!dataset)
                STAR_THROW("Failed to load gdal dataset");
        }

        for (size_t i = r.begin(); i != r.end(); ++i)
        {
            localChunks[i].load(dataset->ds);
        }
    }

  private:
    TerrainChunk *const chunks{nullptr};
};
} // namespace star::terrain::processor::terrain_chunk