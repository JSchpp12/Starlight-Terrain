#pragma once

#include <glm/glm.hpp>
#include <memory>

class OGRCoordinateTransformation;
class GDALDataset; 

namespace star::terrain
{
class TerrainTransform
{
  public:
    /// Build a transform for `ds`. Always returns a non-null TerrainTransform;
    /// it holds no inner OGRCoordinateTransformation when the raster is already
    /// geographic or has no CRS (no-op transform).
    static std::unique_ptr<TerrainTransform> create(GDALDataset *ds) noexcept;

    /// Convert (lat, lon) [degrees] into the raster's geotransform-axis
    /// coordinates (easting/northing for projected rasters). For a no-op
    /// transform returns (lon, lat) unchanged. Returns NaN on failure.
    glm::dvec2 toRasterXY(double lat, double lon) const noexcept;

    /// Reproject n geographic (lat, lon) pairs into raster-axis coordinates,
    /// in place. After the call, lon[i] / lat[i] hold raster X / Y
    /// (easting / northing). For a no-op transform (geographic raster) the
    /// arrays are left unchanged -- lon/lat already are the raster coordinates.
    /// Returns true if every point succeeded; failed points are set to NaN,
    /// matching toRasterXY's single-point behavior.
    bool toRasterXYBatch(double *lon, double *lat, size_t n) const noexcept;

    /// True when this transform holds no inner OGRCoordinateTransformation
    bool isNoOp() const noexcept
    {
        return m_ct == nullptr;
    }

    ~TerrainTransform();
    TerrainTransform(const TerrainTransform &) = delete;
    TerrainTransform &operator=(const TerrainTransform &) = delete;
    TerrainTransform(TerrainTransform &&) = delete;
    TerrainTransform &operator=(TerrainTransform &&) = delete;

  private:
    explicit TerrainTransform(OGRCoordinateTransformation *ct) noexcept;

    OGRCoordinateTransformation *m_ct = nullptr;
};
} // namespace star::terrain