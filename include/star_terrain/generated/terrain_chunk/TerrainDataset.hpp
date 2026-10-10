#pragma once

#include <glm/glm.hpp>
#include <memory>

class GDALDataset;

namespace star::terrain
{
class TerrainTransform;

class TerrainDataset
{
  public:
    TerrainDataset(GDALDataset *dataset, glm::dvec2 northEast, glm::dvec2 southEast, glm::dvec2 southWest,
                   glm::dvec2 northWest, glm::dvec2 center, glm::dvec3 offset);
    TerrainDataset(const TerrainDataset &) = delete;
    TerrainDataset &operator=(const TerrainDataset &) = delete;
    TerrainDataset(TerrainDataset &&) noexcept = default;
    TerrainDataset &operator=(TerrainDataset &&) noexcept = delete;
    ~TerrainDataset();

    float getElevationAtTexCoords(const glm::ivec2 &texCoords) const;

    glm::ivec2 getTexCoordsFromLatLon(const glm::dvec2 &latLon) const;

    glm::ivec2 applyOffsetToTexCoords(const glm::ivec2 &texCoords) const;

    const glm::dvec2 &getNorthEast() const
    {
        return this->m_northEast;
    }
    const glm::dvec2 &getSouthEast() const
    {
        return this->m_southEast;
    }
    const glm::dvec2 &getSouthWest() const
    {
        return this->m_southWest;
    }
    const glm::dvec2 &getNorthWest() const
    {
        return this->m_northWest;
    }
    const glm::dvec3 &getOffset() const
    {
        return this->m_offset;
    }
    const glm::ivec2 &getPixSize() const
    {
        return this->pixSize;
    }
    const glm::dvec2 &getCenter() const
    {
        return this->m_center;
    }

    std::vector<float> reprojectBatch(const double *lats, const double *lons, size_t numVerts) const;

  private:
    glm::dvec3 m_offset;
    glm::dvec2 m_northEast, m_southEast, m_southWest, m_northWest, m_center;
    glm::ivec2 m_bufferSize, fullPixSize, maxPixBounds, pixOffset, pixSize;
    double geoTransforms[6];
    const int pixBorderSize = 1;
    float *gdalBuffer = nullptr;
    // Optional 4326 -> raster CRS transform. Holds a no-op transform for
    // geographic/degree rasters, in which case getTexCoordsFromLatLon uses
    // legacy math.
    std::unique_ptr<TerrainTransform> m_latLonToRaster;

    void initTransforms(GDALDataset *dataset);

    void initBandSizes(GDALDataset *dataset);

    void initPixelCoords();

    void initGDALBuffer(GDALDataset *dataset);
};

} // namespace star::terrain
