#include "star_terrain/generated/terrain_chunk/TerrainDataset.hpp"

#include "star_terrain/generated/terrain_chunk/TerrainTransform.hpp"

#include <algorithm>
#include <cmath>
#include <gdal_priv.h>
#include <starlight/core/Exceptions.hpp>
#include <string>
#include <utility>
#include <vector>

namespace star::terrain
{

TerrainDataset::~TerrainDataset()
{
    try
    {
        if (this->gdalBuffer)
        {
            CPLFree(this->gdalBuffer);
            this->gdalBuffer = nullptr;
        }
    }
    catch (const std::exception &ex)
    {
        STAR_THROW("Memory leak found");
    }
}

TerrainDataset::TerrainDataset(GDALDataset *dataset, glm::dvec2 northEast, glm::dvec2 southEast, glm::dvec2 southWest,
                               glm::dvec2 northWest, glm::dvec2 center, glm::dvec3 offset)
    : m_northEast(std::move(northEast)), m_southEast(std::move(southEast)), m_southWest(std::move(southWest)),
      m_northWest(std::move(northWest)), m_center(std::move(center)), m_offset(std::move(offset))
{
    if (dataset == NULL)
        STAR_THROW("Failed to create dataset");

    initTransforms(dataset);
    initPixelCoords();
    initBandSizes(dataset);
    initGDALBuffer(dataset);
}

glm::ivec2 TerrainDataset::getTexCoordsFromLatLon(const glm::dvec2 &latLon) const
{
    // latLon.x = lat, latLon.y = lon (degrees). Reproject into the raster's
    // geotransform axes (easting/northing for projected rasters)
    const glm::dvec2 rasterXY = this->m_latLonToRaster->toRasterXY(latLon.x, latLon.y);

    if (std::isnan(rasterXY.x) || std::isnan(rasterXY.y))
        STAR_THROW("Failed to reproject chunk corner (lat=" + std::to_string(latLon.x) +
                   ", lon=" + std::to_string(latLon.y) + ") into the height raster's CRS");

    return glm::ivec2{static_cast<int>(std::round((rasterXY.x - geoTransforms[0]) / geoTransforms[1])),
                      static_cast<int>(std::round((rasterXY.y - geoTransforms[3]) / geoTransforms[5]))};
}

glm::ivec2 TerrainDataset::applyOffsetToTexCoords(const glm::ivec2 &texCoords) const
{
    return glm::ivec2{texCoords.x - this->pixOffset.x + this->pixBorderSize,
                      texCoords.y - this->pixOffset.y + this->pixBorderSize};
}

float TerrainDataset::getElevationAtTexCoords(const glm::ivec2 &texCoords) const
{
    const int safeX = std::clamp(texCoords.x, 0, this->m_bufferSize.x - 1);
    const int safeY = std::clamp(texCoords.y, 0, this->m_bufferSize.y - 1);

    return this->gdalBuffer[safeY * this->m_bufferSize.x + safeX];
}

std::vector<float> TerrainDataset::reprojectBatch(const double *lats, const double *lons, size_t numVerts) const
{
    std::vector<float> outElevations(numVerts);
    std::vector<double> rasterLon(lons, lons + numVerts);
    std::vector<double> rasterLat(lats, lats + numVerts);

    // Single batched PROJ call for the entire chunk.
    m_latLonToRaster->toRasterXYBatch(rasterLon.data(), rasterLat.data(), numVerts);

    // Hoist everything the old per-vertex nested calls recomputed each time.
    const double gt0 = geoTransforms[0], gt1 = geoTransforms[1];
    const double gt3 = geoTransforms[3], gt5 = geoTransforms[5];
    // apply offset
    const int offX = pixOffset.x - pixBorderSize;
    const int offY = pixOffset.y - pixBorderSize;
    const int maxBufX = m_bufferSize.x - 1;
    const int maxBufY = m_bufferSize.y - 1;

    for (size_t i = 0; i < numVerts; i++)
    {
        if (std::isnan(rasterLon[i]) || std::isnan(rasterLat[i]))
            STAR_THROW("Failed to reproject chunk vertex into the height raster's CRS");

        // rasterXY -> pixel (was getTexCoordsFromLatLon's tail) then
        // apply offset+border (was applyOffsetToTexCoords), folded into one.
        int px = static_cast<int>(std::round((rasterLon[i] - gt0) / gt1)) - offX;
        int py = static_cast<int>(std::round((rasterLat[i] - gt3) / gt5)) - offY;

        // clamp to buffer bounds
        px = std::clamp(px, 0, maxBufX);
        py = std::clamp(py, 0, maxBufY);

        outElevations[i] = gdalBuffer[static_cast<size_t>(py) * m_bufferSize.x + px];
    }

    return outElevations;
}

void TerrainDataset::initTransforms(GDALDataset *dataset)
{
    if (GDALGetGeoTransform(dataset, this->geoTransforms) != CPLE_None)
        STAR_THROW("Failed to obtain proper geotransform");

    // Build a 4326 -> raster CRS transform for projected height files (e.g.
    // EPSG:3857). A no-op transform is built for geographic/degree rasters;
    // getTexCoordsFromLatLon then falls back to the legacy degree math.
    this->m_latLonToRaster = TerrainTransform::create(dataset);
}

void TerrainDataset::initPixelCoords()
{
    const glm::ivec2 tNorthEast = getTexCoordsFromLatLon(m_northEast);
    const glm::ivec2 tNorthWest = getTexCoordsFromLatLon(m_northWest);
    const glm::ivec2 tSouthEast = getTexCoordsFromLatLon(m_southEast);
    const glm::ivec2 tSouthWest = getTexCoordsFromLatLon(m_southWest);

    const auto crossA = glm::ivec2{std::abs(tSouthEast.x - tNorthWest.x), std::abs(tSouthEast.y - tNorthWest.y)};
    const auto crossB = glm::ivec2{std::abs(tSouthWest.x - tNorthEast.x), std::abs(tSouthWest.y - tNorthEast.y)};
    this->pixSize = glm::ivec2{std::floor((crossA.x + crossB.x) / 2), std::floor((crossA.y + crossB.y) / 2)};

    // Partial edge chunks can be narrower/shorter than a single heightmap
    // pixel. Rounding their corner pixel coordinates then collapses to the
    // same value, yielding a zero-size dimension and a mesh with zero
    // vertices. Clamp each dimension to a minimum of 2 so sub-pixel slivers
    // still generate a valid minimal mesh and the pixSize - 1 divisions
    // in loadLocation() stay safe.
    constexpr int minPixDim = 2;
    this->pixSize.x = std::max(this->pixSize.x, minPixDim);
    this->pixSize.y = std::max(this->pixSize.y, minPixDim);

    this->pixOffset = tNorthWest;
    this->maxPixBounds = this->pixSize + this->pixOffset;
}

void TerrainDataset::initBandSizes(GDALDataset *dataset)
{
    GDALRasterBand *band = dataset->GetRasterBand(1);
    this->fullPixSize = glm::ivec2{band->GetXSize(), band->GetYSize()};
}

void TerrainDataset::initGDALBuffer(GDALDataset *dataset)
{
    GDALRasterBand *band = dataset->GetRasterBand(1);

    int xOff = this->pixOffset.x - this->pixBorderSize;
    int yOff = this->pixOffset.y - this->pixBorderSize;
    int xSize = this->pixSize.x + (2 * this->pixBorderSize);
    int ySize = this->pixSize.y + (2 * this->pixBorderSize);

    xOff = std::max(0, xOff);
    yOff = std::max(0, yOff);
    xSize = std::min(xSize, this->fullPixSize.x - xOff);
    ySize = std::min(ySize, this->fullPixSize.y - yOff);

    // Store the actual dimensions so other methods index correctly
    this->m_bufferSize = glm::ivec2{xSize, ySize};

    this->gdalBuffer = (float *)CPLMalloc(sizeof(float) * xSize * ySize);

    CPLErr error = band->RasterIO(GF_Read, xOff, yOff, xSize, ySize, this->gdalBuffer, xSize, ySize, GDT_Float32, 0, 0);

    if (error != CE_None)
        STAR_THROW("Failed to read raster band");
}
} // namespace star::terrain
