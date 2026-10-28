#include "star_terrain/io/GDALDatasetLoader.hpp"

#include "star_terrain/generated/terrain_chunk/TerrainTransform.hpp"
#include "star_terrain/io/DatasetHandle.hpp"

#include <starlight/core/Exceptions.hpp>

#include <gdal_priv.h>
#include <glm/glm.hpp>
#include <ogr_spatialref.h>

#include <cmath>
#include <memory>
#include <mutex>
#include <utility>

namespace star::terrain
{
void EnsureGDALRegistered()
{
    static std::once_flag once;
    std::call_once(once, [] { GDALAllRegister(); });
}

GDALDatasetLoader::GDALDatasetLoader(std::filesystem::path heightFilePath) : m_heightFilePath(std::move(heightFilePath))
{
}

/// @brief Opens the height raster and hands sole ownership to the caller. Each call gets its own handle, so no two
/// threads ever share a GDALDataset.
/// @return
std::expected<DatasetHandle, GDALLoadError> GDALDatasetLoader::operator()() const
{
    EnsureGDALRegistered();

    const auto path = m_heightFilePath.string();
    DatasetHandle dataset;
    dataset.open(path.c_str());
    if (!dataset)
    {
        return std::unexpected(
            GDALLoadError{GDALLoadError::Cause::dataset_open_failed, "Failed to open GDAL dataset: " + path});
    }

    return dataset;
}

double GetHeightAtLocationFromGDAL(const std::string &path, double latDeg, double lonDeg)
{
    EnsureGDALRegistered();

    // Owns the dataset so every throw/success path closes it exactly once.
    DatasetHandle ds;
    ds.open(path.c_str());

    if (!ds)
    {
        const std::string err = CPLGetLastErrorMsg();
        std::string msg = "Failed to open height raster '" + path + "'";
        if (!err.empty())
            msg += ": " + err;
        STAR_THROW(msg);
    }

    double gt[6];
    if (ds->GetGeoTransform(gt) != CE_None)
        STAR_THROW("Height raster '" + path +
                   "' has no geotransform (unreferenced raster); "
                   "cannot map (lat=" +
                   std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) + ") to pixels.");

    // Assume north-up, no rotation (gt[2] == 0 and gt[4] == 0).
    if (gt[2] != 0.0 || gt[4] != 0.0)
        STAR_THROW("Height raster '" + path + "' is rotated (gt[2]=" + std::to_string(gt[2]) +
                   ", gt[4]=" + std::to_string(gt[4]) + "); rotated rasters are not supported.");

    GDALRasterBand *band = ds->GetRasterBand(1);
    if (!band)
        STAR_THROW("Height raster '" + path + "' has no band 1.");

    const int nx = band->GetXSize();
    const int ny = band->GetYSize();

    std::unique_ptr<TerrainTransform> transform = TerrainTransform::create(ds.get());
    const OGRSpatialReference *rasterSrs = ds->GetSpatialRef();
    if (rasterSrs && !rasterSrs->IsGeographic() && transform->isNoOp())
    {
        const char *projcs = rasterSrs->GetAttrValue("PROJCS");
        STAR_THROW("Height raster '" + path + "' is projected (CRS: '" + std::string(projcs ? projcs : "unknown") +
                   "') but the 4326->raster coordinate transformation could not be created. "
                   "Ensure the PROJ database is available to GDAL.");
    }

    const glm::dvec2 rasterXY = transform->toRasterXY(latDeg, lonDeg);
    const double px = (rasterXY.x - gt[0]) / gt[1];
    const double py = (rasterXY.y - gt[3]) / gt[5];

    if (std::isnan(px) || std::isnan(py))
        STAR_THROW("Failed to reproject (lat=" + std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) +
                   ") into the height raster's CRS ('" + path + "').");

    if (px < 0 || py < 0 || px >= nx || py >= ny)
    {
        STAR_THROW("Requested point (lat=" + std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) +
                   ") maps to pixel (" + std::to_string(px) + ", " + std::to_string(py) +
                   ") which is outside the height raster '" + path + "' (size " + std::to_string(nx) + "x" +
                   std::to_string(ny) +
                   "). Verify the Shape.json center "
                   "lies within the height file's coverage and that the height file's CRS is "
                   "geographic or a supported projection.");
    }

    // Read the nearest pixel. For bilinear, see section 1b below.
    const int ix = static_cast<int>(std::floor(px + 0.5));
    const int iy = static_cast<int>(std::floor(py + 0.5));

    float val = 0.0f;
    if (band->RasterIO(GF_Read, ix, iy, 1, 1, &val, 1, 1, GDT_Float32, 0, 0) != CE_None)
    {
        const std::string err = CPLGetLastErrorMsg();
        std::string msg = "Failed to read elevation pixel from '" + path + "'";
        if (!err.empty())
            msg += ": " + err;
        STAR_THROW(msg);
    }

    int hasNoData = 0;
    const double nodata = band->GetNoDataValue(&hasNoData);
    bool isNoData = (hasNoData && static_cast<double>(val) == nodata);

    double scale = band->GetScale();
    if (scale == 0.0)
        scale = 1.0; // GDAL returns 0 if undefined
    const double offset = band->GetOffset();

    const char *unit = band->GetUnitType(); // typically "m" for meters (can be nullptr)

    return (static_cast<double>(val) * scale + offset);
}
} // namespace star::terrain
